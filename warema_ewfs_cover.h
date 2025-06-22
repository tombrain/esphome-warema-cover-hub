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
            void set_channelcode(const std::string &channelcode);

            cover::CoverTraits get_traits() override;
            cover::CoverOperation get_last_operation() const;

            std::string get_open_command() const;
            std::string get_close_command() const;
            std::string get_stop_command() const;

        private:
            WaremaCoverHub *hub_{};

            void on_open();
            void on_close();
            void on_stop();

            void control(const cover::CoverCall &call) override;
            bool is_at_target_() const;

            void start_direction_(cover::CoverOperation dir);

            void recompute_position_();

            uint32_t open_duration_{0};
            uint32_t close_duration_{0};

            uint32_t last_recompute_time_{0};
            uint32_t start_dir_time_{0};
            uint32_t last_publish_time_{0};
            float target_position_{0};
            bool has_built_in_endstop_{false};
            bool manual_control_{false};
            bool assumed_state_{false};
            cover::CoverOperation last_operation_{cover::COVER_OPERATION_OPENING};

            std::string channelcode_{};
            std::string devicecode1_{};
            std::string devicecode2_{};

            static constexpr std::string_view fix_command_part{"011101"};
            static constexpr std::string_view stop_command_prefix{"001101"};

            // Enum für die Art des Kommandos
            enum class WaremaCommandType
            {
                OPEN,
                CLOSE,
                STOP
            };

            std::string get_command(WaremaCommandType type) const;
        };
    } // namespace warema_cover_hub
} // namespace esphome
