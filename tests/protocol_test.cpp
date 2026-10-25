// Host test for the protocol code, no ESPHome needed:
//   g++ -std=c++17 -Wall -Wextra -Werror -I components/warema_cover_hub
//       tests/protocol_test.cpp components/warema_cover_hub/warema_protocol.cpp -o protocol_test
//   ./protocol_test

#include <cstdio>
#include <cstdlib>
#include <random>
#include <string>
#include <vector>

#include "warema_protocol.h"

using namespace esphome::warema_cover_hub;

static int failures = 0;

#define CHECK(cond) \
  do { \
    if (!(cond)) { \
      std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
      failures++; \
    } \
  } while (0)

static std::mt19937 rng(42);

static std::string random_bits(size_t n) {
  std::string s;
  for (size_t i = 0; i < n; i++)
    s.push_back(rng() % 2 ? '1' : '0');
  return s;
}

static std::vector<int32_t> jitter(std::vector<int32_t> timings, int32_t max_us) {
  std::uniform_int_distribution<int32_t> dist(-max_us, max_us);
  for (auto &t : timings)
    t += t > 0 ? dist(rng) : -dist(rng);
  return timings;
}

static void append(std::vector<int32_t> &a, const std::vector<int32_t> &b) { a.insert(a.end(), b.begin(), b.end()); }

static void test_known_telegram() {
  // Layout example (fictitious codes)
  const std::string telegram = "S01110100111111S100110001S101010011S";
  CHECK(build_telegram(CommandType::OPEN, "111", "100110001", "101010011") == telegram);
  CommandType type;
  CHECK(parse_command_type(telegram, &type) && type == CommandType::OPEN);

  const auto decoded = decode_telegrams(encode_telegram(telegram));
  CHECK(decoded.size() == 1 && decoded[0] == telegram);
}

static void test_parse() {
  CommandType type;
  CHECK(parse_command_type(build_telegram(CommandType::CLOSE, "011", "000000000", "111111111"), &type) &&
        type == CommandType::CLOSE);
  CHECK(parse_command_type(build_telegram(CommandType::STOP, "011", "000000000", "111111111"), &type) &&
        type == CommandType::STOP);
  CHECK(!parse_command_type("S00000000000000S000000000S000000000S", &type));
  CHECK(!parse_command_type("S0111", &type));
}

// Telegrams of all keys of a real remote (channel 101, fictitious device codes)
static void test_real_keys() {
  const struct {
    const char *telegram;
    CommandType type;
  } keys[] = {
      {"S01110100101111S001101001S110010110S", CommandType::OPEN},
      {"S01110100101110S001101001S110010110S", CommandType::CLOSE},
      {"S00110100101001S001101001S110010110S", CommandType::STOP},
      {"S01110100101001S001101001S110010110S", CommandType::FUNCTION_A},
      {"S01110100101000S001101001S110010110S", CommandType::FUNCTION_B},
      {"S01110100101101S001101001S110010110S", CommandType::FUNCTION_C},
  };
  for (const auto &key : keys) {
    CommandType type;
    CHECK(parse_command_type(key.telegram, &type) && type == key.type);
    CHECK(build_telegram(key.type, "101", "001101001", "110010110") == key.telegram);
  }
  CHECK(describe_telegram(keys[3].telegram) ==
        "FUNCTION_A, command: 101, devicecode1: 001101001, devicecode2: 110010110");
}

static void test_describe() {
  CHECK(describe_telegram(build_telegram(CommandType::STOP, "110", "101000110", "011001010")) ==
        "STOP, command: 110, devicecode1: 101000110, devicecode2: 011001010");
  CHECK(describe_telegram("S00000000000000S000000000S000000000S") ==
        "UNKNOWN, command: 000, devicecode1: 000000000, devicecode2: 000000000");
  CHECK(describe_telegram("S0111") == "INVALID");
}

// All combinations of the bits next to the syncs, where pulses merge
static void test_round_trip() {
  const CommandType types[] = {CommandType::OPEN, CommandType::CLOSE, CommandType::STOP};
  for (int i = 0; i < 2000; i++) {
    const std::string telegram = build_telegram(types[i % 3], random_bits(3), random_bits(9), random_bits(9));
    const auto decoded = decode_telegrams(encode_telegram(telegram));
    CHECK(decoded.size() == 1 && decoded[0] == telegram);
  }
}

// Cheap receivers stretch and shrink pulses
static void test_jitter() {
  for (int i = 0; i < 2000; i++) {
    const std::string telegram = build_telegram(CommandType::OPEN, random_bits(3), random_bits(9), random_bits(9));
    const auto decoded = decode_telegrams(jitter(encode_telegram(telegram), 200));
    CHECK(decoded.size() == 1 && decoded[0] == telegram);
  }
}

// The remote repeats the telegram; noise and a low preamble before it
static void test_repetitions_and_noise() {
  const std::string telegram = build_telegram(CommandType::CLOSE, "110", "101000110", "011001010");
  std::vector<int32_t> timings = {120, -300, 80, -5000};
  for (int i = 0; i < 3; i++) {
    append(timings, encode_telegram(telegram));
    timings.push_back(-10000);
  }
  const auto decoded = decode_telegrams(timings);
  CHECK(decoded.size() == 3);
  for (const auto &d : decoded)
    CHECK(d == telegram);
}

// Captured from a real remote (STOP on channel 101) with a superregenerative receiver.
// The syncs were cut off by a too short idle, they are added here as they are sent:
// 5000 µs, plus 890 µs for each adjacent half bit of the same level. The device codes are
// replaced by fictitious ones, their pulses carry the measured deviations of the capture.
static void test_real_capture() {
  std::vector<int32_t> timings = {5890};
  append(timings,
         {-861, 909, -1752, 922, -854, 1800, -1748, 1799, -866, 898, -1761, 1790, -1759, 1790, -875, 895, -1765});
  timings.push_back(6780);
  append(timings, {-888, 958, -1704, 891, -883, 1776, -1773, 1783, -882, 888, -1770});
  timings.push_back(5890);
  append(timings, {-895, 884, -869, 1788, -870, 904, -1768, 1787, -1770, 884, -885, 1776, -883});
  timings.push_back(5000);

  const auto decoded = decode_telegrams(timings);
  CHECK(decoded.size() == 1);
  if (!decoded.empty())
    CHECK(decoded[0] == build_telegram(CommandType::STOP, "101", "001101001", "110010110"));
}

static void test_incomplete() {
  const std::string telegram = build_telegram(CommandType::STOP, "111", "010001011", "010001100");
  auto timings = encode_telegram(telegram);

  auto truncated = timings;
  truncated.resize(timings.size() - 5);
  CHECK(decode_telegrams(truncated).empty());

  auto corrupted = timings;
  corrupted[10] = corrupted[10] > 0 ? 3000 : -3000;
  CHECK(decode_telegrams(corrupted).empty());

  CHECK(decode_telegrams({}).empty());
}

static void test_same_receiver() {
  const std::string open = build_telegram(CommandType::OPEN, "111", "001101001", "110010110");
  CHECK(same_receiver(open, build_telegram(CommandType::STOP, "111", "001101001", "110010110")));
  // Channel "all" of the same remote is another receiver
  CHECK(!same_receiver(open, build_telegram(CommandType::OPEN, "011", "001101001", "110010110")));
  CHECK(!same_receiver(open, build_telegram(CommandType::OPEN, "111", "001101001", "110010111")));
  CHECK(!same_receiver(open, "S0111"));
}

static void test_queue_round_robin() {
  const std::string a = build_telegram(CommandType::OPEN, "111", "000000001", "000000001");
  const std::string b = build_telegram(CommandType::OPEN, "111", "000000010", "000000010");
  TelegramQueue queue;
  queue.push(a, 3);
  queue.push(b, 2);
  std::vector<std::string> sent;
  while (!queue.empty())
    sent.push_back(queue.pop());
  CHECK((sent == std::vector<std::string>{a, b, a, b, a}));
  CHECK(queue.pop().empty());

  queue.push(a, 0);
  CHECK(queue.empty());
}

static void test_queue_limit() {
  TelegramQueue queue;
  for (size_t i = 0; i < TelegramQueue::MAX_ENTRIES; i++) {
    std::string code;  // distinct device codes, i in binary
    for (int bit = 8; bit >= 0; bit--)
      code.push_back((i >> bit) & 1 ? '1' : '0');
    CHECK(queue.push(build_telegram(CommandType::OPEN, "111", code, "000000000"), 3));
  }
  const std::string extra = build_telegram(CommandType::OPEN, "111", "111111111", "111111111");
  CHECK(!queue.push(extra, 3));
  CHECK(queue.size() == TelegramQueue::MAX_ENTRIES);

  // Replacing a pending command still works when full
  const std::string first = queue.pop();
  CHECK(queue.push(build_telegram(CommandType::STOP, "111", first.substr(16, 9), "000000000"), 3));
  CHECK(queue.size() == TelegramQueue::MAX_ENTRIES);
}

// A STOP must not be followed by leftover repetitions of the OPEN before it
static void test_queue_replaces_pending_command() {
  const std::string open = build_telegram(CommandType::OPEN, "111", "010001011", "010001100");
  const std::string stop = build_telegram(CommandType::STOP, "111", "010001011", "010001100");
  const std::string other = build_telegram(CommandType::OPEN, "110", "101000110", "011001010");

  TelegramQueue queue;
  queue.push(open, 10);
  queue.push(other, 10);
  CHECK(queue.pop() == open);
  CHECK(queue.pop() == other);
  queue.push(stop, 10);
  CHECK(queue.size() == 2);

  int stops = 0, others = 0;
  while (!queue.empty()) {
    const std::string t = queue.pop();
    CHECK(t != open);
    stops += t == stop;
    others += t == other;
  }
  CHECK(stops == 10);
  CHECK(others == 9);  // the other cover keeps its remaining repetitions
}

static void test_queue_contains() {
  const std::string stop = build_telegram(CommandType::STOP, "111", "010001011", "010001100");
  const std::string close = build_telegram(CommandType::CLOSE, "111", "010001011", "010001100");
  TelegramQueue queue;
  CHECK(!queue.contains(stop));
  queue.push(stop, 2);
  CHECK(queue.contains(stop));
  CHECK(!queue.contains(close));
  queue.pop();
  CHECK(queue.contains(stop));  // one transmission left
  queue.pop();
  CHECK(!queue.contains(stop));
}

static void test_echo_filter() {
  const std::string open = build_telegram(CommandType::OPEN, "111", "010001011", "010001100");
  const std::string stop = build_telegram(CommandType::STOP, "111", "010001011", "010001100");

  EchoFilter filter;
  CHECK(!filter.is_echo(open, 1000));
  filter.add_sent(open, 1000);
  CHECK(filter.is_echo(open, 1000));
  CHECK(filter.is_echo(open, 2999));
  CHECK(!filter.is_echo(open, 3000));
  CHECK(!filter.is_echo(stop, 1500));  // another button on the remote is no echo

  // Every repetition restarts the window
  filter.add_sent(open, 2500);
  CHECK(filter.is_echo(open, 4000));

  // millis() wraps around after ~49 days
  EchoFilter wrapped;
  wrapped.add_sent(open, 0xFFFFFF00u);
  CHECK(wrapped.is_echo(open, 0x00000100u));
  CHECK(!wrapped.is_echo(open, 0x00000800u));

  // The oldest entry is dropped when full
  EchoFilter full;
  for (uint32_t i = 0; i <= EchoFilter::MAX_ENTRIES; i++)
    full.add_sent(build_telegram(CommandType::OPEN, "111", random_bits(9), "000000000"), 100 + i);
  full.add_sent(stop, 200);
  CHECK(full.is_echo(stop, 300));
}

int main() {
  test_known_telegram();
  test_parse();
  test_describe();
  test_real_keys();
  test_round_trip();
  test_jitter();
  test_repetitions_and_noise();
  test_incomplete();
  test_real_capture();
  test_echo_filter();
  test_same_receiver();
  test_queue_round_robin();
  test_queue_replaces_pending_command();
  test_queue_limit();
  test_queue_contains();
  if (failures == 0)
    std::printf("All tests passed\n");
  return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
