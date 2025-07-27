#include "warema_ewfs_cover.h"
#include "esphome/core/log.h"
#include "esphome/core/hal.h"
#include <queue>

namespace esphome
{
    namespace warema_cover_hub
    {
        static const char *const TAG = "warema_cover_hub.cover";
        using namespace esphome::cover;

        // Konstruktor
        WaremaCover::WaremaCover(WaremaCoverHub *hub) : hub_(hub) {}

        // Setter
        void WaremaCover::set_open_duration(uint32_t open_duration) { this->open_duration_ = open_duration; }
        void WaremaCover::set_close_duration(uint32_t close_duration) { this->close_duration_ = close_duration; }
        void WaremaCover::set_devicecode1(const std::string &devicecode1) { this->devicecode1_ = devicecode1; }
        void WaremaCover::set_devicecode2(const std::string &devicecode2) { this->devicecode2_ = devicecode2; }
        void WaremaCover::set_command(const std::string &command) { this->command_ = command; }

        // Getter
        cover::CoverOperation WaremaCover::get_last_operation() const { return this->last_operation_; }

        std::string WaremaCover::get_command(WaremaCommandType type) const
        {
            // Start bits for the command
            std::string command{"S"};
            command.reserve(36); // Reserve space for the command

            switch (type)
            {
            case WaremaCommandType::OPEN:
                command.append("01110100");
                command.append(command_);
                command.append("111");
                break;

            case WaremaCommandType::CLOSE:
                command.append("01110100");
                command.append(command_);
                command.append("110");
                break;

            case WaremaCommandType::STOP:
                command.append("00110100");
                command.append(command_);
                command.append("001");
                break;
            }

            command.push_back('S');
            command.append(devicecode1_)
                .push_back('S');
            command.append(devicecode2_)
                .push_back('S');
            return command;
        }

        void WaremaCover::dump_config()
        {
            LOG_COVER("", "WaremaCover", this);
            ESP_LOGCONFIG(TAG, "  Open Duration: %.1fs", this->open_duration_ / 1e3f);
            ESP_LOGCONFIG(TAG, "  Close Duration: %.1fs", this->close_duration_ / 1e3f);
            ESP_LOGCONFIG(TAG, "  Command:    %s", this->command_.c_str());
            ESP_LOGCONFIG(TAG, "  Devicecode 1: %s", this->devicecode1_.c_str());
            ESP_LOGCONFIG(TAG, "  Devicecode 2: %s", this->devicecode2_.c_str());
            ESP_LOGCONFIG(TAG, "  Command OPEN:  %s", this->get_command(WaremaCommandType::OPEN).c_str());
            ESP_LOGCONFIG(TAG, "  Command CLOSE: %s", this->get_command(WaremaCommandType::CLOSE).c_str());
            ESP_LOGCONFIG(TAG, "  Command STOP:  %s", this->get_command(WaremaCommandType::STOP).c_str());
        }

        void WaremaCover::setup()
        {
            auto restore = this->restore_state_();
            if (restore.has_value())
            {
                restore->apply(this);
            }
            else
            {
                this->position = 0.5f;
            }
        }

        void WaremaCover::loop()
        {
            if (this->current_operation == COVER_OPERATION_IDLE)
                return;

            const uint32_t now = millis();

            // Recompute position every loop cycle
            this->recompute_position_();

            if (this->is_at_target_())
            {
                if (this->has_built_in_endstop_ &&
                    (this->target_position_ == COVER_OPEN || this->target_position_ == COVER_CLOSED))
                {
                    // Don't trigger stop, let the cover stop by itself.
                    this->current_operation = COVER_OPERATION_IDLE;
                }
                else
                {
                    this->start_direction_(COVER_OPERATION_IDLE);
                }
                this->publish_state();
            }

            // Send current position every second
            if (now - this->last_publish_time_ > 1000)
            {
                this->publish_state(false);
                this->last_publish_time_ = now;
            }
        }

        float WaremaCover::get_setup_priority() const
        {
            return setup_priority::DATA;
        }

        CoverTraits WaremaCover::get_traits()
        {
            auto traits = CoverTraits();
            traits.set_supports_stop(true);
            traits.set_supports_position(true);
            traits.set_supports_toggle(true);
            traits.set_is_assumed_state(this->assumed_state_);
            return traits;
        }

        void WaremaCover::control(const CoverCall &call)
        {
            if (call.get_stop())
            {
                this->start_direction_(COVER_OPERATION_IDLE);
                this->publish_state();
            }
            if (call.get_toggle().has_value())
            {
                if (this->current_operation != COVER_OPERATION_IDLE)
                {
                    this->start_direction_(COVER_OPERATION_IDLE);
                    this->publish_state();
                }
                else
                {
                    if (this->position == COVER_CLOSED || this->last_operation_ == COVER_OPERATION_CLOSING)
                    {
                        this->target_position_ = COVER_OPEN;
                        this->start_direction_(COVER_OPERATION_OPENING);
                    }
                    else
                    {
                        this->target_position_ = COVER_CLOSED;
                        this->start_direction_(COVER_OPERATION_CLOSING);
                    }
                }
            }
            if (call.get_position().has_value())
            {
                auto pos = *call.get_position();
                if (pos == this->position)
                {
                    // already at target
                    if (this->manual_control_ && (pos == COVER_OPEN || pos == COVER_CLOSED))
                    {
                        auto op = pos == COVER_CLOSED ? COVER_OPERATION_CLOSING : COVER_OPERATION_OPENING;
                        this->position = pos == COVER_CLOSED ? COVER_OPEN : COVER_CLOSED;
                        this->target_position_ = pos;
                        this->start_direction_(op);
                    }
                    if (this->has_built_in_endstop_ && (pos == COVER_OPEN || pos == COVER_CLOSED))
                    {
                        auto op = pos == COVER_CLOSED ? COVER_OPERATION_CLOSING : COVER_OPERATION_OPENING;
                        this->target_position_ = pos;
                        this->start_direction_(op);
                    }
                }
                else
                {
                    auto op = pos < this->position ? COVER_OPERATION_CLOSING : COVER_OPERATION_OPENING;
                    if (this->manual_control_ && (pos == COVER_OPEN || pos == COVER_CLOSED))
                    {
                        this->position = pos == COVER_CLOSED ? COVER_OPEN : COVER_CLOSED;
                    }
                    this->target_position_ = pos;
                    this->start_direction_(op);
                }
            }
        }

        bool WaremaCover::is_at_target_() const
        {
            switch (this->current_operation)
            {
            case COVER_OPERATION_OPENING:
                return this->position >= this->target_position_;
            case COVER_OPERATION_CLOSING:
                return this->position <= this->target_position_;
            case COVER_OPERATION_IDLE:
            default:
                return true;
            }
        }

        void WaremaCover::start_direction_(CoverOperation dir)
        {
            if (dir == this->current_operation && dir != COVER_OPERATION_IDLE)
                return;

            this->recompute_position_();
            switch (dir)
            {
            case COVER_OPERATION_IDLE:
                this->on_stop();
                break;
            case COVER_OPERATION_OPENING:
                this->on_open();
                break;
            case COVER_OPERATION_CLOSING:
                this->on_close();
                break;
            default:
                return;
            }

            this->current_operation = dir;

            const uint32_t now = millis();
            this->start_dir_time_ = now;
            this->last_recompute_time_ = now;
        }

        void WaremaCover::on_open()
        {
            ESP_LOGD(TAG, "Cover is opening");
             hub_->send_command(this->get_command(WaremaCommandType::OPEN));
            this->last_operation_ = COVER_OPERATION_OPENING;
        }

        void WaremaCover::on_close()
        {
            ESP_LOGD(TAG, "Cover is closing");
            hub_->send_command(this->get_command(WaremaCommandType::CLOSE));
            this->last_operation_ = COVER_OPERATION_CLOSING;
        }

        void WaremaCover::on_stop()
        {
            ESP_LOGD(TAG, "Cover is stopping");
            hub_->send_command(this->get_command(WaremaCommandType::STOP));
            this->last_operation_ = COVER_OPERATION_IDLE;
        }

        void WaremaCover::recompute_position_()
        {
            if (this->current_operation == COVER_OPERATION_IDLE)
                return;

            float dir;
            float action_dur;
            switch (this->current_operation)
            {
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
    } // namespace warema_cover_hub
} // namespace esphome
