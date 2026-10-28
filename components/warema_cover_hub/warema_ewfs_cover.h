#pragma once

#include <string>
#include <vector>

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
  void set_follow_remote(bool follow_remote) { this->follow_remote_ = follow_remote; }
  /// Makes this cover a group channel that also moves the member.
  void add_member(WaremaCover *member) { this->members_.push_back(member); }
  /// Another remote programmed to this receiver. Only used for receiving, sending uses the own codes.
  void add_additional_remote(const std::string &command, const std::string &devicecode1,
                             const std::string &devicecode2) {
    this->additional_addresses_.push_back(build_telegram(CommandType::STOP, command, devicecode1, devicecode2));
  }

  cover::CoverTraits get_traits() override;

  /// True if the telegram is meant for this cover (codes of this cover or of an additional remote).
  bool is_addressed_by(const std::string &telegram) const;
  /// Any telegram with this cover's own command and device codes, for comparing receivers.
  std::string address_telegram() const { return this->build_telegram_(CommandType::STOP); }

  /// Sends the learn key L with this cover's codes for the given time, like holding L on a
  /// remote. A receiver in learn mode stores these codes as a further remote.
  void start_learning(uint32_t duration) {
    this->parent_->send_command_for(this->build_telegram_(CommandType::LEARN), duration);
  }
  void stop_learning() { this->parent_->cancel_pending(this->address_telegram()); }
  bool is_learning() const { return this->parent_->is_sending(this->build_telegram_(CommandType::LEARN)); }

  /// Tracks a command the motor received from elsewhere (e.g. the original remote) without
  /// sending anything. Mirrors the receiver: the opposite command stops a moving cover.
  void follow_command(CommandType type);

 protected:
  void control(const cover::CoverCall &call) override;
  bool is_at_target_() const;
  void start_direction_(cover::CoverOperation dir);
  void finish_reverse_();
  void recompute_position_();
  void notify_members_(CommandType type);
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

  bool follow_remote_{true};
  std::vector<WaremaCover *> members_{};
  std::vector<std::string> additional_addresses_{};

  std::string command_;
  std::string devicecode1_;
  std::string devicecode2_;
};

}  // namespace warema_cover_hub
}  // namespace esphome
