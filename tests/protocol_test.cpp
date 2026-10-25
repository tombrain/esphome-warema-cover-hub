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
  // Example from the original remote
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

int main() {
  test_known_telegram();
  test_parse();
  test_round_trip();
  test_jitter();
  test_repetitions_and_noise();
  test_incomplete();
  if (failures == 0)
    std::printf("All tests passed\n");
  return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
