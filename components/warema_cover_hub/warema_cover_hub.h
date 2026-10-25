#pragma once

#include <cstdint>
#include <deque>
#include <string>
#include <vector>

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
/// for several covers issued at the same time reach all receivers quickly.
///
/// Received telegrams (e.g. from the original remote) are logged with their codes.
class WaremaCoverHub : public Component, public remote_base::RemoteReceiverListener {
 public:
  void loop() override;
  void dump_config() override;
  bool on_receive(remote_base::RemoteReceiveData data) override;

  void set_transmitter(remote_base::RemoteTransmitterBase *transmitter) { this->transmitter_ = transmitter; }
  void set_has_receiver(bool has_receiver) { this->has_receiver_ = has_receiver; }
  /// Turns decoding of received telegrams on and off, e.g. from a switch.
  void set_receiving(bool receiving) { this->receiving_ = receiving; }
  void set_number_of_transmissions(uint8_t number_of_transmissions) {
    this->number_of_transmissions_ = number_of_transmissions;
  }
  void register_cover(WaremaCover *cover) { this->covers_.push_back(cover); }
  /// Called with each received telegram, repetitions of the remote only once.
  template<typename F> void add_on_telegram_callback(F &&callback) {
    this->telegram_callback_.add(std::forward<F>(callback));
  }

  /// Queues a telegram, e.g. "S01110100111111S000011100S000011101S".
  void send_command(const std::string &telegram);

 protected:
  struct QueueEntry {
    std::string telegram;
    uint8_t remaining_transmissions;
  };

  /// Sends one telegram and returns its duration in microseconds.
  uint32_t transmit_(const std::string &telegram);
  void handle_telegram_(const std::string &telegram);

  remote_base::RemoteTransmitterBase *transmitter_{nullptr};
  bool has_receiver_{false};
  bool receiving_{true};
  uint8_t number_of_transmissions_{3};
  std::vector<WaremaCover *> covers_{};
  std::deque<QueueEntry> queue_{};
  uint32_t last_transmit_time_{0};
  uint32_t busy_time_{0};
  std::string last_received_telegram_;
  uint32_t last_received_time_{0};
  CallbackManager<void(const std::string &)> telegram_callback_;
};

}  // namespace warema_cover_hub
}  // namespace esphome
