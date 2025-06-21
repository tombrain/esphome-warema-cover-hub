#pragma once

#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/core/component.h"

namespace esphome
{
    namespace warema_cover_hub
    {
        class WaremaCoverHub; // Forward declaration of WaremaCoverHub

        class TestTextSensor : public text_sensor::TextSensor, public Component
        {
        public:
            // Constructor that accepts a pointer to the WaremaCoverHub
            explicit TestTextSensor(WaremaCoverHub *hub) : hub_(hub) {}

            void setup() override;
            void loop() override;

            void dump_config() override;

            // Method to set the sprungweite value
            void set_sprungweite(int sprungweite) { sprungweite_ = sprungweite; }

        private:
            WaremaCoverHub *hub_{};       // Pointer to the WaremaCoverHub
            int sprungweite_{1};          // Variable to store the sprungweite value
            unsigned long last_update_{}; // Variable to store the last update time
        };

    } // namespace warema_cover_hub
} // namespace esphome