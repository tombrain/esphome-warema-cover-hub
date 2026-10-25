#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "esphome/core/automation.h"
#include "esphome/core/component.h"
#include "esphome/core/helpers.h"
#include "esphome/components/remote_base/remote_base.h"

#include "warema_protocol.h"

namespace esphome {
namespace warema_cover_hub {

class WaremaCover;

/// Sends Warema EWFS telegrams via a remote_transmitter and decodes received ones.
///
/// Telegrams are queued and sent round-robin: every queued telegram is sent once per
/// round until it has been transmitted `number_of_transmissions` times. This way commands
/// for several covers issued at the same time reach all receivers quickly. A new command
/// replaces the pending one of the same cover.
///
/// Received telegrams (e.g. from the original remote) are logged with their codes.
class WaremaCoverHub : public Component, public remote_base::RemoteReceiverListener {
 public:
  void loop() override;
  void dump_config() override;
  bool on_receive(remote_base::RemoteReceiveData data) override;

  void set_transmitter(remote_base::RemoteTransmitterBase *transmitter) { this->transmitter_ = transmitter; }
  void set_has_receiver(bool has_receiver) { this->has_receiver_ = has_receiver; }
  /// Shows received codes in the log and the text sensor, e.g. switched for reading new codes.
  /// Events and covers following the remote work regardless.
  void set_show_codes(bool show_codes) { this->show_codes_ = show_codes; }
  bool is_showing_codes() const { return this->show_codes_; }
  void set_number_of_transmissions(uint8_t number_of_transmissions) {
    this->number_of_transmissions_ = number_of_transmissions;
  }
  void register_cover(WaremaCover *cover) { this->covers_.push_back(cover); }
  /// Marks a channel as assigned (e.g. by an event), given by any telegram of it.
  void register_channel(const std::string &telegram) { this->channels_.push_back(telegram); }
  /// Called with each received telegram and its description (with the matching covers),
  /// repetitions of the remote only once.
  template<typename F> void add_on_telegram_callback(F &&callback) {
    this->telegram_callback_.add(std::forward<F>(callback));
  }
  /// Like add_on_telegram_callback, but only for telegrams of channels without cover or event.
  template<typename F> void add_on_unassigned_callback(F &&callback) {
    this->unassigned_callback_.add(std::forward<F>(callback));
  }

  /// Queues a telegram, e.g. "S01110100111111S000011100S000011101S".
  void send_command(const std::string &telegram);
  /// True while the telegram has transmissions left.
  bool is_sending(const std::string &telegram) const { return this->queue_.contains(telegram); }
  /// Drops pending telegrams for the receiver of the given telegram.
  void cancel_pending(const std::string &telegram) { this->queue_.remove_receiver(telegram); }

 protected:
  /// Sends one telegram and returns its duration in microseconds.
  uint32_t transmit_(const std::string &telegram);
  void handle_telegram_(const std::string &telegram);
  /// Describes a received telegram including the names of the covers it addresses.
  std::string describe_received_(const std::string &telegram) const;
  bool is_assigned_(const std::string &telegram) const;

  remote_base::RemoteTransmitterBase *transmitter_{nullptr};
  bool has_receiver_{false};
  bool show_codes_{true};
  uint8_t number_of_transmissions_{3};
  std::vector<WaremaCover *> covers_{};
  std::vector<std::string> channels_{};
  TelegramQueue queue_;
  uint32_t last_transmit_time_{0};
  uint32_t busy_time_{0};
  std::string last_received_telegram_;
  uint32_t last_received_time_{0};
  EchoFilter echo_filter_;
  CallbackManager<void(const std::string &, const std::string &)> telegram_callback_;
  CallbackManager<void(const std::string &, const std::string &)> unassigned_callback_;
};

/// Fires for received telegrams of channels that have no cover or event, e.g. a new remote.
class UnassignedTrigger : public Trigger<std::string, std::string, std::string, std::string, std::string> {
 public:
  explicit UnassignedTrigger(WaremaCoverHub *parent) {
    parent->add_on_unassigned_callback([this](const std::string &telegram, const std::string &description) {
      CommandType type;
      const char *key = parse_command_type(telegram, &type) ? command_type_to_event_type(type) : "unknown";
      this->trigger(key, telegram.substr(9, 3), telegram.substr(16, 9), telegram.substr(26, 9), description);
    });
  }
};

}  // namespace warema_cover_hub
}  // namespace esphome
