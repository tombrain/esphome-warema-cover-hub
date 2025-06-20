#include "warema_cover_hub.h"
#include "esphome/core/log.h"
#include "esphome/core/helpers.h" // Für ESPHome-Mutex

namespace esphome
{
    namespace warema_cover_hub
    {

        static const char *TAG = "warema_cover_hub";
        Mutex counter_mutex; // Define a mutex for protecting the counter

        void WaremaCoverHub::setup()
        {
            // This will be called by App.setup()
        }

        void WaremaCoverHub::dump_config()
        {
            ESP_LOGCONFIG(TAG, "Warema Cover Hub:");
            ESP_LOGCONFIG(TAG, "  Number of text sensors: %d", this->text_sensors_.size());
        }

        void WaremaCoverHub::loop()
        {
            unsigned long current_time = millis();
            if (current_time - last_time_ >= 2000)
            { // Check if 3 seconds have passed
                ESP_LOGI(TAG, "Number of text sensors: %d", this->text_sensors_.size());
                ESP_LOGI(TAG, "Counter value: %d", this->counter_); // Log the counter value
                last_time_ = current_time;                          // Update the last time
            }
        }

        void WaremaCoverHub::increase_counter(int value)
        {
            LockGuard lock(counter_mutex); // Lock the mutex
            this->counter_ += value;
        }
    } // namespace warema_cover_hub
} // namespace esphome