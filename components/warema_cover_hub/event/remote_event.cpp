#include "remote_event.h"

#include "esphome/core/log.h"

namespace esphome {
namespace warema_cover_hub {

static const char *const TAG = "warema_cover_hub.event";

void RemoteEvent::setup() {
  if (this->cover_ != nullptr)
    this->address_ = this->cover_->address_telegram();
  this->parent_->register_channel(this->address_);

  this->parent_->add_on_telegram_callback([this](const std::string &telegram, const std::string & /*description*/) {
    if (!same_receiver(telegram, this->address_))
      return;
    CommandType type;
    if (parse_command_type(telegram, &type))
      this->trigger(command_type_to_event_type(type));
  });
}

void RemoteEvent::dump_config() {
  LOG_EVENT("", "Warema Remote Event", this);
  if (this->address_.length() == TELEGRAM_LENGTH) {
    ESP_LOGCONFIG(TAG, "  Command: %s", this->address_.substr(9, 3).c_str());
    ESP_LOGCONFIG(TAG, "  Devicecode 1: %s", this->address_.substr(16, 9).c_str());
    ESP_LOGCONFIG(TAG, "  Devicecode 2: %s", this->address_.substr(26, 9).c_str());
  }
}

}  // namespace warema_cover_hub
}  // namespace esphome
