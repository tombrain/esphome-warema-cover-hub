#pragma once

#include "esphome/core/component.h"
#include "esphome/core/helpers.h"
#include "esphome/components/text_sensor/text_sensor.h"

#include "../warema_cover_hub.h"

namespace esphome {
namespace warema_cover_hub {

/// Shows the last telegram received from a remote, with the values of the cover configuration.
class ReceivedCodeTextSensor : public text_sensor::TextSensor, public Component, public Parented<WaremaCoverHub> {
 public:
  void setup() override;
  void dump_config() override;
};

}  // namespace warema_cover_hub
}  // namespace esphome
