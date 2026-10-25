#include "warema_cover_hub.h"

#include <cstdlib>

#include "esphome/core/hal.h"
#include "esphome/core/log.h"

namespace esphome {
namespace warema_cover_hub {

static const char *const TAG = "warema_cover_hub";

static const uint32_t TRANSMIT_GAP_MS = 10;
// Receptions this long after our own transmission are its echo and ignored
static const uint32_t RECEIVE_HOLDOFF_MS = 100;
// The remote repeats each telegram, report repetitions within this window only once
static const uint32_t DUPLICATE_WINDOW_MS = 1000;

void WaremaCoverHub::dump_config() {
  ESP_LOGCONFIG(TAG, "Warema Cover Hub:");
  ESP_LOGCONFIG(TAG, "  Transmitter: %s", YESNO(this->transmitter_ != nullptr));
  ESP_LOGCONFIG(TAG, "  Receiver: %s", YESNO(this->has_receiver_));
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

  const std::string telegram = this->queue_.pop();
  ESP_LOGV(TAG, "Transmitting '%s' (%u queued)", telegram.c_str(), static_cast<unsigned>(this->queue_.size()));
  const uint32_t duration_us = this->transmit_(telegram);
  this->last_transmit_time_ = now;
  this->echo_filter_.add_sent(telegram, now);
  this->busy_time_ = (duration_us + 999) / 1000 + TRANSMIT_GAP_MS;
}

void WaremaCoverHub::send_command(const std::string &telegram) {
  if (this->transmitter_ == nullptr) {
    ESP_LOGW(TAG, "Ignoring telegram '%s': no transmitter configured", telegram.c_str());
    return;
  }
  if (telegram.length() != TELEGRAM_LENGTH) {
    ESP_LOGW(TAG, "Ignoring telegram '%s': expected %u characters, got %u", telegram.c_str(),
             static_cast<unsigned>(TELEGRAM_LENGTH), static_cast<unsigned>(telegram.length()));
    return;
  }
  if (!this->queue_.push(telegram, this->number_of_transmissions_)) {
    ESP_LOGW(TAG, "Ignoring telegram '%s': queue is full", telegram.c_str());
    return;
  }
  ESP_LOGD(TAG, "Queued telegram '%s'", telegram.c_str());
}

uint32_t WaremaCoverHub::transmit_(const std::string &telegram) {
  const remote_base::RawTimings timings = encode_telegram(telegram);

  uint32_t duration_us = 0;
  for (int32_t t : timings)
    duration_us += std::abs(t);

  auto call = this->transmitter_->transmit();
  call.get_data()->set_data(timings);
  call.perform();
  return duration_us;
}

bool WaremaCoverHub::on_receive(remote_base::RemoteReceiveData data) {
  if (!this->receiving_)
    return false;
  if (!this->queue_.empty() || millis() - this->last_transmit_time_ < this->busy_time_ + RECEIVE_HOLDOFF_MS)
    return false;

  const auto telegrams = decode_telegrams(data.get_raw_data());
  for (const auto &telegram : telegrams)
    this->handle_telegram_(telegram);
  return !telegrams.empty();
}

void WaremaCoverHub::handle_telegram_(const std::string &telegram) {
  const uint32_t now = millis();
  // Our own transmission, received late (e.g. the loop was blocked after sending)
  if (this->echo_filter_.is_echo(telegram, now)) {
    ESP_LOGV(TAG, "Ignoring echo of own telegram '%s'", telegram.c_str());
    return;
  }

  const bool repeated =
      telegram == this->last_received_telegram_ && now - this->last_received_time_ < DUPLICATE_WINDOW_MS;
  this->last_received_telegram_ = telegram;
  this->last_received_time_ = now;
  if (repeated)
    return;

  ESP_LOGI(TAG, "Received %s", describe_telegram(telegram).c_str());
  ESP_LOGD(TAG, "Received telegram '%s'", telegram.c_str());
  this->telegram_callback_.call(telegram);
}

}  // namespace warema_cover_hub
}  // namespace esphome
