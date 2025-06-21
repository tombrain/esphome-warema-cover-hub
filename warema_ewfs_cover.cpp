#include "warema_ewfs_cover.h"
#include "esphome/core/log.h"
#include "esphome/core/hal.h"
#include <queue>
// #include <mutex>

namespace esphome
{
    namespace warema_cover_hub
    {
        static const char *const TAG = "warema_cover_hub.cover";

        using namespace esphome::cover;

        enum class CommandType
        {
            Open,
            Close,
            Stop
        };

        struct CommandEntry
        {
            CommandType command;
            int remaining_transmissions;
        };

        std::queue<CommandEntry> command_queue;
        // std::mutex queue_mutex;

        void WaremaCover::dump_config()
        {
            LOG_COVER("", "Time Based Cover", this);
            ESP_LOGCONFIG(TAG, "  Open Duration: %.1fs", this->open_duration_ / 1e3f);
            ESP_LOGCONFIG(TAG, "  Close Duration: %.1fs", this->close_duration_ / 1e3f);
            // ESP_LOGCONFIG(TAG, "Used GPIO-Pin: %d", m_transmitterPin->get_pin());
            // ESP_LOGCONFIG(TAG, "Number of Transmissions: %d", m_numberOfTransmissions);
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

            // Sende Codes aus der Queue
            // std::lock_guard<std::mutex> lock(queue_mutex);
            // if (!command_queue.empty())
            // {
            //     CommandEntry &entry = command_queue.front();
            //     // Hier den Code senden, abhängig vom CommandType
            //     RCSwitch mySwitch = RCSwitch();
            //     mySwitch.enableTransmit(10); // Set the GPIO pin for transmission

            //     switch (entry.command)
            //     {
            //     case CommandType::Open:
            //         mySwitch.send("00001", 24); // Code für Öffnen
            //         break;
            //     case CommandType::Close:
            //         mySwitch.send("00010", 24); // Code für Schließen
            //         break;
            //     case CommandType::Stop:
            //         mySwitch.send("00011", 24); // Code für Stoppen
            //         break;
            //     }

            //     entry.remaining_transmissions--;
            //     if (entry.remaining_transmissions <= 0)
            //     {
            //         command_queue.pop();
            //     }
            // }
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
                        // for covers with manual control switch, we can't rely on the computed position, so if
                        // the command triggered again, we'll assume it's in the opposite direction anyway.
                        auto op = pos == COVER_CLOSED ? COVER_OPERATION_CLOSING : COVER_OPERATION_OPENING;
                        this->position = pos == COVER_CLOSED ? COVER_OPEN : COVER_CLOSED;
                        this->target_position_ = pos;
                        this->start_direction_(op);
                    }
                    // for covers with built in end stop, we should send the command again
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
                // Funktionsaufruf bei STOP hinzufügen
                this->on_stop();
                break;
            case COVER_OPERATION_OPENING:
                // Funktionsaufruf bei OPEN hinzufügen
                this->on_open();
                break;
            case COVER_OPERATION_CLOSING:
                // Funktionsaufruf bei CLOSE hinzufügen
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

        // Neue Methoden hinzufügen
        void WaremaCover::on_open()
        {
            // Hier den gewünschten Funktionsaufruf hinzufügen
            ESP_LOGD(TAG, "Cover is opening");
            // Beispiel: custom_open_function();
            // std::lock_guard<std::mutex> lock(queue_mutex);
            // command_queue.push({CommandType::Open, 3}); // Code für Öffnen in die Queue eintragen, 3 Übertragungen
        }

        void WaremaCover::on_close()
        {
            // Hier den gewünschten Funktionsaufruf hinzufügen
            ESP_LOGD(TAG, "Cover is closing");
            // Beispiel: custom_close_function();
            // std::lock_guard<std::mutex> lock(queue_mutex);
            // command_queue.push({CommandType::Close, 3}); // Code für Schließen in die Queue eintragen, 3 Übertragungen
        }

        void WaremaCover::on_stop()
        {
            // Hier den gewünschten Funktionsaufruf hinzufügen
            ESP_LOGD(TAG, "Cover is stopping");
            // Beispiel: custom_stop_function();
            // std::lock_guard<std::mutex> lock(queue_mutex);
            // command_queue.push({CommandType::Stop, 3}); // Code für Stoppen in die Queue eintragen, 3 Übertragungen
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
