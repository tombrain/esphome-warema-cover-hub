#pragma once

#include "esphome/core/component.h"
#include "esphome/core/helpers.h"
#include "esphome/components/switch/switch.h"

#include "../warema_cover_hub.h"

namespace esphome {
namespace warema_cover_hub {

/// Shows or hides received codes in the log and the text sensor, for reading new codes.
class ShowCodesSwitch : public switch_::Switch, public Component, public Parented<WaremaCoverHub> {
 public:
  void setup() override;
  void dump_config() override;

 protected:
  void write_state(bool state) override;
};

}  // namespace warema_cover_hub
}  // namespace esphome
