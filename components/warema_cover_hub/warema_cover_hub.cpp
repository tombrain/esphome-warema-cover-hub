#include "warema_cover_hub.h"
#include "warema_ewfs_cover.h"

#include <algorithm>
#include <cstdlib>

#include "esphome/core/hal.h"
#include "esphome/core/log.h"

namespace esphome {
namespace warema_cover_hub {

static const char *const TAG = "warema_cover_hub";

static const uint32_t TRANSMIT_GAP_MS = 10;
// 100 ms, in centiseconds like Component::warn_if_blocking_over_
static const uint8_t BLOCKING_WARN_THRESHOLD_CS = 10;
// Receptions this long after our own transmission are its echo and ignored
static const uint32_t RECEIVE_HOLDOFF_MS = 100;
// The remote repeats each telegram, report repetitions within this window only once
static const uint32_t DUPLICATE_WINDOW_MS = 1000;

void WaremaCoverHub::setup() {
  // On the ESP8266 the transmitter blocks for the whole telegram (~77 ms). That is expected,
  // so only warn about blocking beyond it.
  this->warn_if_blocking_over_ = BLOCKING_WARN_THRESHOLD_CS;

  // All device codes of real remotes seen so far start with 0
  const uint32_t random = random_uint32();
  this->suggested_code_ = "command: 111, devicecode1: 0";
  for (int i = 0; i < 8; i++)
    this->suggested_code_.push_back((random >> i) & 1 ? '1' : '0');
  this->suggested_code_.append(", devicecode2: 0");
  for (int i = 8; i < 16; i++)
    this->suggested_code_.push_back((random >> i) & 1 ? '1' : '0');
}

void WaremaCoverHub::dump_config() {
  ESP_LOGCONFIG(TAG, "Warema Cover Hub:");
  ESP_LOGCONFIG(TAG, "  Transmitter: %s", YESNO(this->transmitter_ != nullptr));
  ESP_LOGCONFIG(TAG, "  Receiver: %s", YESNO(this->has_receiver_));
  ESP_LOGCONFIG(TAG, "  Number of transmissions: %u", this->number_of_transmissions_);
  ESP_LOGCONFIG(TAG, "  Registered covers: %u", static_cast<unsigned>(this->covers_.size()));
  if (this->transmitter_ != nullptr)
    ESP_LOGCONFIG(TAG, "  Random code for a new remote: %s", this->suggested_code_.c_str());
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

void WaremaCoverHub::send_command_for(const std::string &telegram, uint32_t duration_ms) {
  uint32_t telegram_us = 0;
  for (int32_t t : encode_telegram(telegram))
    telegram_us += std::abs(t);
  // Each transmission takes the telegram plus the gap, like in loop()
  const uint32_t period_ms = (telegram_us + 999) / 1000 + TRANSMIT_GAP_MS;
  const uint32_t transmissions = std::min<uint32_t>((duration_ms + period_ms - 1) / period_ms, UINT8_MAX);
  this->queue_telegram_(telegram, std::max<uint32_t>(transmissions, 1));
}

void WaremaCoverHub::queue_telegram_(const std::string &telegram, uint8_t transmissions) {
  if (this->transmitter_ == nullptr) {
    ESP_LOGW(TAG, "Ignoring telegram '%s': no transmitter configured", telegram.c_str());
    return;
  }
  if (telegram.length() != TELEGRAM_LENGTH) {
    ESP_LOGW(TAG, "Ignoring telegram '%s': expected %u characters, got %u", telegram.c_str(),
             static_cast<unsigned>(TELEGRAM_LENGTH), static_cast<unsigned>(telegram.length()));
    return;
  }
  if (!this->queue_.push(telegram, transmissions)) {
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

  const std::string description = this->describe_received_(telegram);
  if (this->show_codes_) {
    ESP_LOGI(TAG, "Received %s", description.c_str());
  } else {
    ESP_LOGV(TAG, "Received %s", description.c_str());
  }
  ESP_LOGD(TAG, "Received telegram '%s'", telegram.c_str());
  this->telegram_callback_.call(telegram, description);
  if (!this->is_assigned_(telegram))
    this->unassigned_callback_.call(telegram, description);
}

bool WaremaCoverHub::is_assigned_(const std::string &telegram) const {
  for (const auto *cover : this->covers_) {
    if (cover->is_addressed_by(telegram))
      return true;
  }
  for (const auto &channel : this->channels_) {
    if (same_receiver(telegram, channel))
      return true;
  }
  return false;
}

std::string WaremaCoverHub::describe_received_(const std::string &telegram) const {
  std::string names;
  for (const auto *cover : this->covers_) {
    if (!cover->is_addressed_by(telegram))
      continue;
    if (!names.empty())
      names.append(", ");
    names.append(cover->get_name().c_str());
  }

  std::string description = describe_telegram(telegram);
  if (names.empty()) {
    description.append(" (no cover configured)");
  } else {
    description.append(" (cover: ").append(names).append(")");
  }
  return description;
}

}  // namespace warema_cover_hub
}  // namespace esphome
