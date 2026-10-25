#pragma once

#include <cstddef>
#include <cstdint>
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
static const char *const OPEN_SUFFIX = "111";
static const char *const CLOSE_SUFFIX = "110";
static const char *const STOP_SUFFIX = "001";

enum class CommandType { OPEN, CLOSE, STOP };

const char *command_type_to_str(CommandType type);

/// Builds a telegram, e.g. "S01110100111111S000011100S000011101S".
std::string build_telegram(CommandType type, const std::string &command, const std::string &devicecode1,
                           const std::string &devicecode2);

/// Returns false if the telegram is no known command.
bool parse_command_type(const std::string &telegram, CommandType *type);

/// Converts a telegram into raw timings in microseconds (positive = high, negative = low).
std::vector<int32_t> encode_telegram(const std::string &telegram);

/// Finds all complete telegrams in raw timings.
std::vector<std::string> decode_telegrams(const std::vector<int32_t> &timings);

}  // namespace warema_cover_hub
}  // namespace esphome
