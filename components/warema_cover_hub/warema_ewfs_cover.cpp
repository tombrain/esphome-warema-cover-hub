#include "warema_ewfs_cover.h"

#include "esphome/core/hal.h"
#include "esphome/core/log.h"

namespace esphome {
namespace warema_cover_hub {

static const char *const TAG = "warema_cover_hub.cover";

using namespace esphome::cover;

std::string WaremaCover::build_telegram_(CommandType type) const {
  return build_telegram(type, this->command_, this->devicecode1_, this->devicecode2_);
}

void WaremaCover::dump_config() {
  LOG_COVER("", "Warema EWFS Cover", this);
  ESP_LOGCONFIG(TAG, "  Open Duration: %.1fs", this->open_duration_ / 1e3f);
  ESP_LOGCONFIG(TAG, "  Close Duration: %.1fs", this->close_duration_ / 1e3f);
  ESP_LOGCONFIG(TAG, "  Reverse Delay: %.1fs", this->reverse_delay_ / 1e3f);
  ESP_LOGCONFIG(TAG, "  Follow Remote: %s", YESNO(this->follow_remote_));
  ESP_LOGCONFIG(TAG, "  Command: %s", this->command_.c_str());
  ESP_LOGCONFIG(TAG, "  Devicecode 1: %s", this->devicecode1_.c_str());
  ESP_LOGCONFIG(TAG, "  Devicecode 2: %s", this->devicecode2_.c_str());
  ESP_LOGCONFIG(TAG, "  Telegram OPEN:  %s", this->build_telegram_(CommandType::OPEN).c_str());
  ESP_LOGCONFIG(TAG, "  Telegram CLOSE: %s", this->build_telegram_(CommandType::CLOSE).c_str());
  ESP_LOGCONFIG(TAG, "  Telegram STOP:  %s", this->build_telegram_(CommandType::STOP).c_str());
}

void WaremaCover::setup() {
  auto restore = this->restore_state_();
  if (restore.has_value()) {
    restore->apply(this);
  } else {
    this->position = 0.5f;
  }

  if (this->follow_remote_) {
    this->parent_->add_on_telegram_callback([this](const std::string &telegram, const std::string & /*description*/) {
      CommandType type;
      if (this->is_addressed_by(telegram) && parse_command_type(telegram, &type))
        this->follow_command(type);
    });
  }
}

void WaremaCover::loop() {
  if (this->reverse_direction_ != COVER_OPERATION_IDLE) {
    this->finish_reverse_();
    return;
  }
  if (this->current_operation == COVER_OPERATION_IDLE)
    return;

  const uint32_t now = millis();

  this->recompute_position_();

  if (this->is_at_target_()) {
    if (this->has_built_in_endstop_ &&
        (this->target_position_ == COVER_OPEN || this->target_position_ == COVER_CLOSED)) {
      // Don't send STOP, the cover stops by itself.
      this->current_operation = COVER_OPERATION_IDLE;
    } else {
      this->start_direction_(COVER_OPERATION_IDLE);
    }
    this->publish_state();
  }

  // Publish the estimated position every second while moving
  if (now - this->last_publish_time_ > 1000) {
    this->publish_state(false);
    this->last_publish_time_ = now;
  }
}

CoverTraits WaremaCover::get_traits() {
  auto traits = CoverTraits();
  traits.set_supports_stop(true);
  traits.set_supports_position(true);
  traits.set_supports_toggle(true);
  traits.set_is_assumed_state(true);
  return traits;
}

void WaremaCover::control(const CoverCall &call) {
  if (call.get_stop()) {
    this->start_direction_(COVER_OPERATION_IDLE);
    this->publish_state();
  }
  if (call.get_toggle().has_value()) {
    if (this->current_operation != COVER_OPERATION_IDLE) {
      this->start_direction_(COVER_OPERATION_IDLE);
      this->publish_state();
    } else if (this->position == COVER_CLOSED || this->last_operation_ == COVER_OPERATION_CLOSING) {
      this->target_position_ = COVER_OPEN;
      this->start_direction_(COVER_OPERATION_OPENING);
    } else {
      this->target_position_ = COVER_CLOSED;
      this->start_direction_(COVER_OPERATION_CLOSING);
    }
  }
  if (call.get_position().has_value()) {
    auto pos = *call.get_position();
    if (pos == this->position) {
      // Already there by estimate. At the end positions send anyway, the
      // estimate may be off and the endstop makes this safe.
      if (this->has_built_in_endstop_ && (pos == COVER_OPEN || pos == COVER_CLOSED)) {
        this->target_position_ = pos;
        this->start_direction_(pos == COVER_CLOSED ? COVER_OPERATION_CLOSING : COVER_OPERATION_OPENING);
      }
    } else {
      this->target_position_ = pos;
      this->start_direction_(pos < this->position ? COVER_OPERATION_CLOSING : COVER_OPERATION_OPENING);
    }
  }
}

bool WaremaCover::is_at_target_() const {
  switch (this->current_operation) {
    case COVER_OPERATION_OPENING:
      return this->position >= this->target_position_;
    case COVER_OPERATION_CLOSING:
      return this->position <= this->target_position_;
    case COVER_OPERATION_IDLE:
    default:
      return true;
  }
}

void WaremaCover::start_direction_(CoverOperation dir) {
  if (this->reverse_direction_ != COVER_OPERATION_IDLE) {
    // A reverse is pending: the same direction keeps waiting, anything else cancels it
    if (dir == this->reverse_direction_)
      return;
    this->reverse_direction_ = COVER_OPERATION_IDLE;
  }
  if (dir == this->current_operation && dir != COVER_OPERATION_IDLE)
    return;

  if (dir != COVER_OPERATION_IDLE && this->current_operation != COVER_OPERATION_IDLE) {
    // The receiver would only stop on the opposite command, so stop first
    ESP_LOGD(TAG, "'%s' reversing, stopping first", this->get_name().c_str());
    this->start_direction_(COVER_OPERATION_IDLE);
    this->reverse_direction_ = dir;
    this->reverse_stop_telegram_ = this->build_telegram_(CommandType::STOP);
    this->reverse_stop_sent_ = false;
    this->publish_state();
    return;
  }

  this->recompute_position_();
  switch (dir) {
    case COVER_OPERATION_IDLE:
      ESP_LOGD(TAG, "'%s' stopping", this->get_name().c_str());
      this->parent_->send_command(this->build_telegram_(CommandType::STOP));
      break;
    case COVER_OPERATION_OPENING:
      ESP_LOGD(TAG, "'%s' opening", this->get_name().c_str());
      this->parent_->send_command(this->build_telegram_(CommandType::OPEN));
      break;
    case COVER_OPERATION_CLOSING:
      ESP_LOGD(TAG, "'%s' closing", this->get_name().c_str());
      this->parent_->send_command(this->build_telegram_(CommandType::CLOSE));
      break;
    default:
      return;
  }

  // Toggle reverses the last movement, so a STOP must not overwrite it
  if (dir != COVER_OPERATION_IDLE)
    this->last_operation_ = dir;
  this->current_operation = dir;
  this->last_recompute_time_ = millis();
}

void WaremaCover::follow_command(CommandType type) {
  CoverOperation dir;
  switch (type) {
    case CommandType::OPEN:
      dir = COVER_OPERATION_OPENING;
      break;
    case CommandType::CLOSE:
      dir = COVER_OPERATION_CLOSING;
      break;
    case CommandType::STOP:
      dir = COVER_OPERATION_IDLE;
      break;
    default:
      return;  // function keys do not move the cover
  }

  // The motor got a newer command: our pending repetitions and a pending reverse would override it
  this->parent_->cancel_pending(this->address_telegram());
  this->reverse_direction_ = COVER_OPERATION_IDLE;

  CoverOperation next = dir;
  if (dir != COVER_OPERATION_IDLE && this->current_operation != COVER_OPERATION_IDLE)
    next = dir == this->current_operation ? dir : COVER_OPERATION_IDLE;  // opposite command stops
  if (next == this->current_operation)
    return;

  ESP_LOGD(TAG, "'%s' follows %s from elsewhere", this->get_name().c_str(), command_type_to_str(type));
  this->recompute_position_();
  if (next != COVER_OPERATION_IDLE) {
    this->target_position_ = next == COVER_OPERATION_OPENING ? COVER_OPEN : COVER_CLOSED;
    this->last_operation_ = next;
  }
  this->current_operation = next;
  this->last_recompute_time_ = millis();
  this->publish_state();
}

// Starts the pending direction once the STOP has been sent and the reverse delay has passed.
void WaremaCover::finish_reverse_() {
  if (this->parent_->is_sending(this->reverse_stop_telegram_))
    return;

  const uint32_t now = millis();
  if (!this->reverse_stop_sent_) {
    this->reverse_stop_sent_ = true;
    this->reverse_stop_sent_time_ = now;
  }
  if (now - this->reverse_stop_sent_time_ < this->reverse_delay_)
    return;

  const CoverOperation dir = this->reverse_direction_;
  this->reverse_direction_ = COVER_OPERATION_IDLE;
  this->start_direction_(dir);
  this->publish_state();
}

void WaremaCover::recompute_position_() {
  float dir;
  float action_dur;
  switch (this->current_operation) {
    case COVER_OPERATION_OPENING:
      dir = 1.0f;
      action_dur = this->open_duration_;
      break;
    case COVER_OPERATION_CLOSING:
      dir = -1.0f;
      action_dur = this->close_duration_;
      break;
    default:
      return;
  }

  const uint32_t now = millis();
  this->position += dir * (now - this->last_recompute_time_) / action_dur;
  this->position = clamp(this->position, 0.0f, 1.0f);
  this->last_recompute_time_ = now;
}

}  // namespace warema_cover_hub
}  // namespace esphome
