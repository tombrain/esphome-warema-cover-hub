#include "warema_cover_hub.h"

#include "esphome/core/hal.h"
#include "esphome/core/log.h"

namespace esphome {
namespace warema_cover_hub {

static const char *const TAG = "warema_cover_hub";

static const size_t TELEGRAM_LENGTH = 36;
static const int32_t BIT_LENGTH_US = 1780;
static const int32_t SYNC_LENGTH_US = 5000;
static const uint32_t TRANSMIT_GAP_MS = 10;

void WaremaCoverHub::dump_config() {
  ESP_LOGCONFIG(TAG, "Warema Cover Hub:");
  ESP_LOGCONFIG(TAG, "  Number of transmissions: %u", this->number_of_transmissions_);
  ESP_LOGCONFIG(TAG, "  Registered covers: %u", static_cast<unsigned>(this->covers_.size()));
}

void WaremaCoverHub::loop() {
  if (this->queue_.empty())
    return;

  // The transmitter may return before the telegram is on air (non_blocking on ESP32),
  // so wait for the telegram duration plus the gap, counted from the start.
  const uint32_t now = millis();
  if (now - this->last_transmit_time_ < this->busy_time_)
    return;

  QueueEntry entry = std::move(this->queue_.front());
  this->queue_.pop_front();

  ESP_LOGV(TAG, "Transmitting '%s' (%u left)", entry.telegram.c_str(), entry.remaining_transmissions - 1);
  const uint32_t duration_us = this->transmit_(entry.telegram);
  this->last_transmit_time_ = now;
  this->busy_time_ = (duration_us + 999) / 1000 + TRANSMIT_GAP_MS;

  if (--entry.remaining_transmissions > 0)
    this->queue_.push_back(std::move(entry));
}

void WaremaCoverHub::send_command(const std::string &telegram) {
  if (telegram.length() != TELEGRAM_LENGTH) {
    ESP_LOGW(TAG, "Ignoring telegram '%s': expected %u characters, got %u", telegram.c_str(),
             static_cast<unsigned>(TELEGRAM_LENGTH), static_cast<unsigned>(telegram.length()));
    return;
  }
  ESP_LOGD(TAG, "Queueing telegram '%s'", telegram.c_str());
  this->queue_.push_back({telegram, this->number_of_transmissions_});
}

// Encoding: 'S' = high for SYNC_LENGTH_US, 's' = low for SYNC_LENGTH_US,
// '0' = high then low, '1' = low then high (each half of BIT_LENGTH_US).
uint32_t WaremaCoverHub::transmit_(const std::string &telegram) {
  remote_base::RawTimings timings;
  timings.reserve(telegram.length() * 2);

  // Appends a pulse, merging it with the previous one if it has the same level.
  auto append = [&timings](bool high, int32_t length) {
    if (!timings.empty() && (timings.back() > 0) == high) {
      timings.back() += high ? length : -length;
    } else {
      timings.push_back(high ? length : -length);
    }
  };

  const int32_t half_bit = BIT_LENGTH_US / 2;
  for (char c : telegram) {
    switch (c) {
      case 'S':
        append(true, SYNC_LENGTH_US);
        break;
      case 's':
        append(false, SYNC_LENGTH_US);
        break;
      case '0':
        append(true, half_bit);
        append(false, half_bit);
        break;
      case '1':
        append(false, half_bit);
        append(true, half_bit);
        break;
      default:
        break;
    }
  }

  uint32_t duration_us = 0;
  for (int32_t t : timings)
    duration_us += t > 0 ? t : -t;

  auto call = this->transmitter_->transmit();
  call.get_data()->set_data(timings);
  call.perform();
  return duration_us;
}

}  // namespace warema_cover_hub
}  // namespace esphome
