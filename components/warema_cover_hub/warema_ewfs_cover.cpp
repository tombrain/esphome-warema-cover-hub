#include "warema_ewfs_cover.h"

#include "esphome/core/hal.h"
#include "esphome/core/log.h"

namespace esphome {
namespace warema_cover_hub {

static const char *const TAG = "warema_cover_hub.cover";

using namespace esphome::cover;

// Telegram layout (36 characters):
// S <command bits:8> <command:3> <suffix:3> S <devicecode1:9> S <devicecode2:9> S
static const char SYNC = 'S';
static const char *const OPEN_COMMAND_BITS = "01110100";
static const char *const CLOSE_COMMAND_BITS = "01110100";  // identical to OPEN, direction is in the suffix
static const char *const STOP_COMMAND_BITS = "00110100";
static const char *const OPEN_SUFFIX = "111";
static const char *const CLOSE_SUFFIX = "110";
static const char *const STOP_SUFFIX = "001";

std::string WaremaCover::build_telegram_(CommandType type) const {
  std::string telegram;
  telegram.reserve(36);
  telegram.push_back(SYNC);

  switch (type) {
    case CommandType::OPEN:
      telegram.append(OPEN_COMMAND_BITS).append(this->command_).append(OPEN_SUFFIX);
      break;
    case CommandType::CLOSE:
      telegram.append(CLOSE_COMMAND_BITS).append(this->command_).append(CLOSE_SUFFIX);
      break;
    case CommandType::STOP:
      telegram.append(STOP_COMMAND_BITS).append(this->command_).append(STOP_SUFFIX);
      break;
  }

  telegram.push_back(SYNC);
  telegram.append(this->devicecode1_).push_back(SYNC);
  telegram.append(this->devicecode2_).push_back(SYNC);
  return telegram;
}

void WaremaCover::dump_config() {
  LOG_COVER("", "Warema EWFS Cover", this);
  ESP_LOGCONFIG(TAG, "  Open Duration: %.1fs", this->open_duration_ / 1e3f);
  ESP_LOGCONFIG(TAG, "  Close Duration: %.1fs", this->close_duration_ / 1e3f);
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
}

void WaremaCover::loop() {
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
  if (dir == this->current_operation && dir != COVER_OPERATION_IDLE)
    return;

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

  this->last_operation_ = dir;
  this->current_operation = dir;
  this->last_recompute_time_ = millis();
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
