#pragma once

#include <string>

#include "esphome/core/component.h"
#include "esphome/core/helpers.h"
#include "esphome/components/event/event.h"

#include "../warema_cover_hub.h"
#include "../warema_ewfs_cover.h"

namespace esphome {
namespace warema_cover_hub {

/// Fires an event for each key pressed on a remote channel, e.g. "open" or "function_a".
///
/// The channel is given by a cover (including its additional remotes) or by its command and
/// device codes. Own transmissions
/// are filtered out by the hub, so only keys of a real remote are reported.
class RemoteEvent : public event::Event, public Component, public Parented<WaremaCoverHub> {
 public:
  void setup() override;
  void dump_config() override;

  void set_cover(WaremaCover *cover) { this->cover_ = cover; }
  void set_codes(const std::string &command, const std::string &devicecode1, const std::string &devicecode2) {
    this->address_ = build_telegram(CommandType::STOP, command, devicecode1, devicecode2);
  }

 protected:
  WaremaCover *cover_{nullptr};
  std::string address_;
};

}  // namespace warema_cover_hub
}  // namespace esphome
