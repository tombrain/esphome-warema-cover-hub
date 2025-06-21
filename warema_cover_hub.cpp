#include "warema_cover_hub.h"
#include "esphome/core/log.h"
#include "esphome/core/helpers.h" // Für ESPHome-Mutex

#include "warema_ewfs_cover.h"

namespace esphome
{
    namespace warema_cover_hub
    {

        static const char *TAG = "warema_cover_hub";
        Mutex counter_mutex; // Define a mutex for protecting the counter

        void WaremaCoverHub::register_text_sensorX(TestTextSensor *obj)
        {
            // Log the registration of the text sensor
            this->text_sensors_.push_back(obj);
        }

        void WaremaCoverHub::register_cover(WaremaCover *obj)
        {
            // Log the registration of the cover
            this->covers_.push_back(obj);
        }

        void WaremaCoverHub::setup()
        {
            // This will be called by App.setup()
        }

        void WaremaCoverHub::dump_config()
        {
            ESP_LOGCONFIG(TAG, "Warema Cover Hub:");
            ESP_LOGCONFIG(TAG, "  Name: %s", this->get_name().c_str());
            ESP_LOGCONFIG(TAG, "  Number of registered text sensors: %d", this->text_sensors_.size());
            ESP_LOGCONFIG(TAG, "  Number of registered covers: %d", this->covers_.size());
        }

        void WaremaCoverHub::loop()
        {
            unsigned long current_time = millis();
            // if (current_time - last_time_ >= 2000)
            // { // Check if 3 seconds have passed
            //     ESP_LOGI(TAG, "Number of text sensors: %d", this->text_sensors_.size());
            //     ESP_LOGI(TAG, "Counter value: %d", this->counter_); // Log the counter value
            //     last_time_ = current_time;                          // Update the last time
            // }

            // alle 30 sekunden code ausfuehren
            // unsigned long current_time_30s = millis();
            // if (current_time_30s - last_time_ >= 30000)
            // {
            //     ESP_LOGI(TAG, "Number of text sensors: %d", this->text_sensors_.size());
            //     for (auto *sensor : this->text_sensors_)
            //     {
            //         // id des sensors loggen
            //         ESP_LOGI(TAG, "Text Sensor ID: %s", sensor->get_name().c_str());
            //     }
            //     last_time_ = current_time_30s; // Update the last time
            // }
        }

        void WaremaCoverHub::increase_counter(int value, TestTextSensor const &obj)
        {
            // Log the sensor name and value
            ESP_LOGI(TAG, "Increasing counter by %d for sensor: %s", value, obj.get_name().c_str());

            LockGuard lock(counter_mutex); // Lock the mutex
            this->counter_ += value;
        }
    } // namespace warema_cover_hub
} // namespace esphome