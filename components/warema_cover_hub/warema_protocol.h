#pragma once

#include <cstddef>
#include <cstdint>
#include <deque>
#include <string>
#include <vector>

namespace esphome {
namespace warema_cover_hub {

// Warema EWFS protocol, as implemented by aircontrol (https://github.com/rfkd/aircontrol).
//
// Telegram layout (36 characters):
// S <command bits:8> <command:3> <suffix:3> S <devicecode1:9> S <devicecode2:9> S
static const size_t TELEGRAM_LENGTH = 36;
static const char SYNC = 'S';
static const char *const OPEN_COMMAND_BITS = "01110100";
static const char *const CLOSE_COMMAND_BITS = "01110100";  // identical to OPEN, direction is in the suffix
static const char *const STOP_COMMAND_BITS = "00110100";
// The function keys A, B and C of the remote use the command bits of OPEN/CLOSE
static const char *const FUNCTION_COMMAND_BITS = "01110100";
static const char *const OPEN_SUFFIX = "111";
static const char *const CLOSE_SUFFIX = "110";
static const char *const STOP_SUFFIX = "001";
static const char *const FUNCTION_A_SUFFIX = "001";
static const char *const FUNCTION_B_SUFFIX = "000";
static const char *const FUNCTION_C_SUFFIX = "101";

enum class CommandType { OPEN, CLOSE, STOP, FUNCTION_A, FUNCTION_B, FUNCTION_C };

const char *command_type_to_str(CommandType type);
/// Event type for Home Assistant, e.g. "open" or "function_a".
const char *command_type_to_event_type(CommandType type);

/// Builds a telegram, e.g. "S01110100111111S000011100S000011101S".
std::string build_telegram(CommandType type, const std::string &command, const std::string &devicecode1,
                           const std::string &devicecode2);

/// Returns false if the telegram is no known command.
bool parse_command_type(const std::string &telegram, CommandType *type);

/// Describes a telegram with the values of the cover configuration, e.g.
/// "OPEN, command: 111, devicecode1: 000011100, devicecode2: 000011101".
std::string describe_telegram(const std::string &telegram);

/// Converts a telegram into raw timings in microseconds (positive = high, negative = low).
std::vector<int32_t> encode_telegram(const std::string &telegram);

/// Finds all complete telegrams in raw timings.
std::vector<std::string> decode_telegrams(const std::vector<int32_t> &timings);

/// True if both telegrams address the same receiver (same command and device codes).
bool same_receiver(const std::string &a, const std::string &b);

/// Telegrams waiting to be sent, each a number of times, round-robin.
///
/// A new telegram replaces pending ones for the same receiver, so a STOP is not
/// followed by leftover repetitions of the OPEN before it.
class TelegramQueue {
 public:
  /// Bounds the memory use; one entry per receiver is the normal case.
  static constexpr size_t MAX_ENTRIES = 32;

  /// Returns false if the queue is full and the telegram was dropped.
  bool push(const std::string &telegram, uint8_t transmissions);
  bool empty() const { return this->entries_.empty(); }
  size_t size() const { return this->entries_.size(); }
  /// True while the telegram has transmissions left.
  bool contains(const std::string &telegram) const;
  /// Drops pending telegrams for the receiver of the given telegram.
  void remove_receiver(const std::string &telegram);
  /// Returns the next telegram to send (empty if none) and requeues it if it has
  /// transmissions left.
  std::string pop();

 protected:
  struct Entry {
    std::string telegram;
    uint8_t remaining;
  };
  std::deque<Entry> entries_;
};

/// Recognizes received telegrams that are the echo of our own transmissions.
///
/// A received telegram is an echo if it equals a telegram sent within the last
/// ECHO_WINDOW_MS. Times are millis() values, wraparound is handled.
class EchoFilter {
 public:
  static constexpr uint32_t ECHO_WINDOW_MS = 2000;
  static constexpr size_t MAX_ENTRIES = 8;

  void add_sent(const std::string &telegram, uint32_t now);
  bool is_echo(const std::string &telegram, uint32_t now) const;

 protected:
  struct Entry {
    std::string telegram;
    uint32_t time;
  };
  std::vector<Entry> sent_;
};

}  // namespace warema_cover_hub
}  // namespace esphome
