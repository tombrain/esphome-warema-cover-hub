#include "show_codes_switch.h"

#include "esphome/core/log.h"

namespace esphome {
namespace warema_cover_hub {

static const char *const TAG = "warema_cover_hub.switch";

void ShowCodesSwitch::setup() { this->write_state(this->get_initial_state_with_restore_mode().value_or(true)); }

void ShowCodesSwitch::dump_config() { LOG_SWITCH("", "Warema Show Codes Switch", this); }

void ShowCodesSwitch::write_state(bool state) {
  this->parent_->set_show_codes(state);
  this->publish_state(state);
}

}  // namespace warema_cover_hub
}  // namespace esphome
