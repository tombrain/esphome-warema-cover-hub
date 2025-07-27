#pragma once

#include "esphome/core/component.h"
#include "esphome/core/automation.h"
#include "esphome/components/cover/cover.h"

#include "warema_cover_hub.h"

namespace esphome
{
    namespace warema_cover_hub
    {
        class WaremaCover : public cover::Cover, public Component
        {
        public:
            explicit WaremaCover(WaremaCoverHub *hub);

            void setup() override;
            void loop() override;
            void dump_config() override;
            float get_setup_priority() const override;

            void set_open_duration(uint32_t open_duration);
            void set_close_duration(uint32_t close_duration);

            void set_devicecode1(const std::string &devicecode1);
            void set_devicecode2(const std::string &devicecode2);
            void set_command(const std::string &command);

            cover::CoverTraits get_traits() override;
            cover::CoverOperation get_last_operation() const;

            std::string get_open_command() const;
            std::string get_close_command() const;
            std::string get_stop_command() const;

        private:
            WaremaCoverHub *hub_{nullptr};

            // Timing and position
            uint32_t open_duration_{};
            uint32_t close_duration_{};
            uint32_t last_recompute_time_{};
            uint32_t start_dir_time_{};
            uint32_t last_publish_time_{};
            float target_position_{};

            // State flags
            bool has_built_in_endstop_{true};
            bool manual_control_{false}; // Remove if unused
            bool assumed_state_{true};  // Remove if unused

            // Operation state
            cover::CoverOperation last_operation_{cover::COVER_OPERATION_OPENING};

            // Device codes
            std::string command_;
            std::string devicecode1_;
            std::string devicecode2_;

            enum class WaremaCommandType
            {
                OPEN,
                CLOSE,
                STOP
            };

            // Methods
            void control(const cover::CoverCall &call) override;
            void on_open();
            void on_close();
            void on_stop();
            bool is_at_target_() const;
            void start_direction_(cover::CoverOperation dir);
            void recompute_position_();
            std::string get_command(WaremaCommandType type) const;
        };
    } // namespace warema_cover_hub
} // namespace esphome
