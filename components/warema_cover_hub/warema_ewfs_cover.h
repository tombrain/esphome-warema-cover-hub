#pragma once

#include <string>

#include "esphome/core/component.h"
#include "esphome/core/helpers.h"
#include "esphome/components/cover/cover.h"

#include "warema_cover_hub.h"

namespace esphome {
namespace warema_cover_hub {

/// A Warema EWFS cover whose position is estimated from the open/close durations.
class WaremaCover : public cover::Cover, public Component, public Parented<WaremaCoverHub> {
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

  void set_open_duration(uint32_t open_duration) { this->open_duration_ = open_duration; }
  void set_close_duration(uint32_t close_duration) { this->close_duration_ = close_duration; }
  void set_devicecode1(const std::string &devicecode1) { this->devicecode1_ = devicecode1; }
  void set_devicecode2(const std::string &devicecode2) { this->devicecode2_ = devicecode2; }
  void set_command(const std::string &command) { this->command_ = command; }
  void set_reverse_delay(uint32_t reverse_delay) { this->reverse_delay_ = reverse_delay; }

  cover::CoverTraits get_traits() override;

  /// True if the telegram is meant for this cover (same command and device codes).
  bool is_addressed_by(const std::string &telegram) const {
    return same_receiver(telegram, this->build_telegram_(CommandType::STOP));
  }

 protected:
  void control(const cover::CoverCall &call) override;
  bool is_at_target_() const;
  void start_direction_(cover::CoverOperation dir);
  void finish_reverse_();
  void recompute_position_();
  std::string build_telegram_(CommandType type) const;

  uint32_t open_duration_{0};
  uint32_t close_duration_{0};
  uint32_t last_recompute_time_{0};
  uint32_t last_publish_time_{0};
  float target_position_{0};

  // The motor switches off by itself at the end positions, so no STOP is sent there.
  bool has_built_in_endstop_{true};

  cover::CoverOperation last_operation_{cover::COVER_OPERATION_OPENING};

  // The receiver treats the opposite command as STOP, so reversing is STOP, then the new
  // direction once the STOP is sent and reverse_delay_ has passed.
  uint32_t reverse_delay_{1000};
  cover::CoverOperation reverse_direction_{cover::COVER_OPERATION_IDLE};
  std::string reverse_stop_telegram_;
  bool reverse_stop_sent_{false};
  uint32_t reverse_stop_sent_time_{0};

  std::string command_;
  std::string devicecode1_;
  std::string devicecode2_;
};

}  // namespace warema_cover_hub
}  // namespace esphome
