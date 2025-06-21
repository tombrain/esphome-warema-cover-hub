#pragma once

#include "esphome/core/component.h"
#include "esphome/core/automation.h"
#include "esphome/components/cover/cover.h"

// #include "RCSwitchWarema.h"

namespace esphome
{
    namespace warema_cover_hub
    {
        class WaremaCoverHub;
        class WaremaCover : public cover::Cover, public Component
        {
        public:
            // Constructor that accepts a pointer to the WaremaCoverHub
            explicit WaremaCover(WaremaCoverHub *hub) : hub_(hub) {}

            void setup() override;
            void loop() override;
            void dump_config() override;
            float get_setup_priority() const override;

            void set_open_duration(uint32_t open_duration) { this->open_duration_ = open_duration; }
            void set_close_duration(uint32_t close_duration) { this->close_duration_ = close_duration; }
            cover::CoverTraits get_traits() override;
            cover::CoverOperation get_last_operation() const { return this->last_operation_; }

        private:
            WaremaCoverHub *hub_{};       // Pointer to the WaremaCoverHub

            void on_open(); // Deklaration der neuen Methode
            void on_close(); // Deklaration der neuen Methode
            void on_stop(); // Deklaration der neuen Methode

        protected:
            void control(const cover::CoverCall &call) override;
            bool is_at_target_() const;

            void start_direction_(cover::CoverOperation dir);

            void recompute_position_();

            uint32_t open_duration_;
            uint32_t close_duration_;

            uint32_t last_recompute_time_{0};
            uint32_t start_dir_time_{0};
            uint32_t last_publish_time_{0};
            float target_position_{0};
            bool has_built_in_endstop_{false};
            bool manual_control_{false};
            bool assumed_state_{false};
            cover::CoverOperation last_operation_{cover::COVER_OPERATION_OPENING};

            // unsigned int m_dataLength{1780};
            // unsigned int m_syncLength{5000};
            // uint8_t m_numberOfTransmissions{3}; // Default to 3 transmissions
            // unsigned int m_sendDelay{10000};

            // InternalGPIOPin *m_transmitterPin{nullptr};

            // RCSwitchWarema m_mySwitch{};
        };
    } // namespace warema_cover_hub
} // namespace esphome
