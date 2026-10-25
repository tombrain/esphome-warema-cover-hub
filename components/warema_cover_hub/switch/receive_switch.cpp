#include "receive_switch.h"

#include "esphome/core/log.h"

namespace esphome {
namespace warema_cover_hub {

static const char *const TAG = "warema_cover_hub.switch";

void ReceiveSwitch::setup() { this->write_state(this->get_initial_state_with_restore_mode().value_or(true)); }

void ReceiveSwitch::dump_config() { LOG_SWITCH("", "Warema Receive Switch", this); }

void ReceiveSwitch::write_state(bool state) {
  this->parent_->set_receiving(state);
  this->publish_state(state);
}

}  // namespace warema_cover_hub
}  // namespace esphome
