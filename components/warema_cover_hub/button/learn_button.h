#pragma once

#include <cstdint>

#include "esphome/core/component.h"
#include "esphome/components/button/button.h"

#include "../warema_ewfs_cover.h"

namespace esphome {
namespace warema_cover_hub {

/// Sends the learn key L with the codes of a cover, to teach them to its receiver as a
/// further remote. Pressing again while sending stops it.
class LearnButton : public button::Button, public Component {
 public:
  void dump_config() override;

  void set_cover(WaremaCover *cover) { this->cover_ = cover; }
  void set_duration(uint32_t duration) { this->duration_ = duration; }

 protected:
  void press_action() override;

  WaremaCover *cover_{nullptr};
  uint32_t duration_{5000};
};

}  // namespace warema_cover_hub
}  // namespace esphome
