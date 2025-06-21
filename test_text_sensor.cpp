#include "test_text_sensor.h"

#include "warema_cover_hub.h" // Include the header file for WaremaCoverHub

#include "esphome/core/log.h"

namespace esphome
{
    namespace warema_cover_hub
    {
        static const char *TAG = "warema_cover_hub.test_text_sensor";

        void TestTextSensor::dump_config()
        {
            ESP_LOGCONFIG(TAG, "TestTextSensor:");
            ESP_LOGCONFIG(TAG, "  Name: %s", this->name_.c_str());
            ESP_LOGCONFIG(TAG, "  Sprungweite: %d", this->sprungweite_);
        }

        void TestTextSensor::setup()
        {
            // Setup code here
            this->last_update_ = millis();
        }

        void TestTextSensor::loop()
        {
            // Check if sprungweite seconds have passed
            if (millis() - this->last_update_ >= this->sprungweite_ * 10000)
            {
                // Increase the counter of the hub by 1
                this->hub_->increase_counter(1, *this);
                ESP_LOGD(TAG, "Hub '%s' counter increased by '%s'", this->hub_->get_name().c_str(), this->get_name().c_str());

                // Update the last update time
                this->last_update_ = millis();
            }
        }

    } // namespace warema_cover_hub
} // namespace esphome