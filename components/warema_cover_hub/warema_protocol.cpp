#include "warema_protocol.h"

#include <cstdlib>

namespace esphome {
namespace warema_cover_hub {

static const int32_t BIT_LENGTH_US = 1780;
static const int32_t HALF_BIT_US = BIT_LENGTH_US / 2;
static const int32_t SYNC_LENGTH_US = 5000;
// High pulses at least this long are a sync (possibly merged with an adjacent half bit)
static const int32_t SYNC_MIN_US = 3500;

const char *command_type_to_str(CommandType type) {
  switch (type) {
    case CommandType::OPEN:
      return "OPEN";
    case CommandType::CLOSE:
      return "CLOSE";
    case CommandType::STOP:
      return "STOP";
    default:
      return "UNKNOWN";
  }
}

std::string build_telegram(CommandType type, const std::string &command, const std::string &devicecode1,
                           const std::string &devicecode2) {
  std::string telegram;
  telegram.reserve(TELEGRAM_LENGTH);
  telegram.push_back(SYNC);

  switch (type) {
    case CommandType::OPEN:
      telegram.append(OPEN_COMMAND_BITS).append(command).append(OPEN_SUFFIX);
      break;
    case CommandType::CLOSE:
      telegram.append(CLOSE_COMMAND_BITS).append(command).append(CLOSE_SUFFIX);
      break;
    case CommandType::STOP:
      telegram.append(STOP_COMMAND_BITS).append(command).append(STOP_SUFFIX);
      break;
  }

  telegram.push_back(SYNC);
  telegram.append(devicecode1).push_back(SYNC);
  telegram.append(devicecode2).push_back(SYNC);
  return telegram;
}

bool parse_command_type(const std::string &telegram, CommandType *type) {
  if (telegram.length() != TELEGRAM_LENGTH)
    return false;
  const std::string bits = telegram.substr(1, 8);
  const std::string suffix = telegram.substr(12, 3);
  if (bits == OPEN_COMMAND_BITS && suffix == OPEN_SUFFIX) {
    *type = CommandType::OPEN;
  } else if (bits == CLOSE_COMMAND_BITS && suffix == CLOSE_SUFFIX) {
    *type = CommandType::CLOSE;
  } else if (bits == STOP_COMMAND_BITS && suffix == STOP_SUFFIX) {
    *type = CommandType::STOP;
  } else {
    return false;
  }
  return true;
}

std::string describe_telegram(const std::string &telegram) {
  if (telegram.length() != TELEGRAM_LENGTH)
    return "INVALID";
  CommandType type;
  std::string description = parse_command_type(telegram, &type) ? command_type_to_str(type) : "UNKNOWN";
  description.append(", command: ").append(telegram, 9, 3);
  description.append(", devicecode1: ").append(telegram, 16, 9);
  description.append(", devicecode2: ").append(telegram, 26, 9);
  return description;
}

// Encoding: 'S' = high for SYNC_LENGTH_US, 's' = low for SYNC_LENGTH_US,
// '0' = high then low, '1' = low then high (each half of BIT_LENGTH_US).
std::vector<int32_t> encode_telegram(const std::string &telegram) {
  std::vector<int32_t> timings;
  timings.reserve(telegram.length() * 2);

  // Appends a pulse, merging it with the previous one if it has the same level.
  auto append = [&timings](bool high, int32_t length) {
    if (!timings.empty() && (timings.back() > 0) == high) {
      timings.back() += high ? length : -length;
    } else {
      timings.push_back(high ? length : -length);
    }
  };

  for (char c : telegram) {
    switch (c) {
      case 'S':
        append(true, SYNC_LENGTH_US);
        break;
      case 's':
        append(false, SYNC_LENGTH_US);
        break;
      case '0':
        append(true, HALF_BIT_US);
        append(false, HALF_BIT_US);
        break;
      case '1':
        append(false, HALF_BIT_US);
        append(true, HALF_BIT_US);
        break;
      default:
        break;
    }
  }
  return timings;
}

// Inverse of encode_telegram(): splits the pulses into half bits and decodes them between
// the syncs. A sync merges with an adjacent half bit of the same level: the end of a '1'
// before it and the start of a '0' after it.
std::vector<std::string> decode_telegrams(const std::vector<int32_t> &timings) {
  std::vector<std::string> telegrams;
  std::vector<std::string> segments;  // bits between two syncs
  std::string halves;                 // half bits of the current segment, 'H' or 'L'
  bool synced = false;

  for (int32_t t : timings) {
    const bool high = t > 0;
    int32_t length = std::abs(t);

    if (high && length >= SYNC_MIN_US) {
      if (synced) {
        if (halves.size() % 2 == 1) {
          halves.push_back('H');
          length -= HALF_BIT_US;
        }
        std::string bits;
        bool valid = true;
        for (size_t i = 0; i < halves.size(); i += 2) {
          if (halves[i] == 'H' && halves[i + 1] == 'L') {
            bits.push_back('0');
          } else if (halves[i] == 'L' && halves[i + 1] == 'H') {
            bits.push_back('1');
          } else {
            valid = false;
            break;
          }
        }
        if (valid) {
          segments.push_back(bits);
          if (segments.size() > 3)
            segments.erase(segments.begin());
        } else {
          segments.clear();
        }

        if (segments.size() == 3 && segments[0].size() == 14 && segments[1].size() == 9 && segments[2].size() == 9) {
          std::string telegram;
          telegram.reserve(TELEGRAM_LENGTH);
          for (const auto &segment : segments)
            telegram.append(1, SYNC).append(segment);
          telegram.push_back(SYNC);
          telegrams.push_back(telegram);
          segments.clear();
        }
      }
      synced = true;
      halves.clear();
      if (length - SYNC_LENGTH_US >= HALF_BIT_US / 2)
        halves.push_back('H');
      continue;
    }

    if (!synced)
      continue;
    const int32_t count = (length + HALF_BIT_US / 2) / HALF_BIT_US;
    if (count < 1 || count > 2) {
      // Gap or noise, wait for the next sync
      synced = false;
      segments.clear();
      continue;
    }
    halves.append(count, high ? 'H' : 'L');
  }
  return telegrams;
}

}  // namespace warema_cover_hub
}  // namespace esphome
