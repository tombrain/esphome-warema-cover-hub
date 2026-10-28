#include "learn_button.h"

#include "esphome/core/log.h"

namespace esphome {
namespace warema_cover_hub {

static const char *const TAG = "warema_cover_hub.button";

void LearnButton::dump_config() {
  LOG_BUTTON("", "Warema Learn Button", this);
  ESP_LOGCONFIG(TAG, "  Cover: '%s'", this->cover_->get_name().c_str());
  ESP_LOGCONFIG(TAG, "  Duration: %.1fs", this->duration_ / 1e3f);
}

void LearnButton::press_action() {
  if (this->cover_->is_learning()) {
    ESP_LOGI(TAG, "'%s' stops sending the learn key", this->cover_->get_name().c_str());
    this->cover_->stop_learning();
    return;
  }
  ESP_LOGI(TAG, "'%s' sends the learn key for %.1fs", this->cover_->get_name().c_str(), this->duration_ / 1e3f);
  this->cover_->start_learning(this->duration_);
}

}  // namespace warema_cover_hub
}  // namespace esphome
