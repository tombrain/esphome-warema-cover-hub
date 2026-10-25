#include "received_code_text_sensor.h"

#include "esphome/core/log.h"

namespace esphome {
namespace warema_cover_hub {

static const char *const TAG = "warema_cover_hub.text_sensor";

void ReceivedCodeTextSensor::setup() {
  this->parent_->add_on_telegram_callback(
      [this](const std::string &telegram) { this->publish_state(describe_telegram(telegram)); });
}

void ReceivedCodeTextSensor::dump_config() { LOG_TEXT_SENSOR("", "Warema Received Code", this); }

}  // namespace warema_cover_hub
}  // namespace esphome
