#include "warema_cover_hub.h"
#include "esphome/core/log.h"
#include "esphome/core/helpers.h" // Für ESPHome-Mutex

#include "warema_ewfs_cover.h"

#include "RCSwitchWarema.h"

#include <list>

namespace esphome
{
    namespace warema_cover_hub
    {
        static const char *TAG = "warema_cover_hub";

        class RollingCommandQueue
        {
            struct CommandEntry
            {
                std::string m_command;
                int m_remaining_transmissions;
            };

        public:
            RollingCommandQueue(int transmissions)
                : m_transmissions(transmissions)
            {
            }

            void enqueue(const std::string &command)
            {
                m_queue.push_back({command, m_transmissions});
            }

            std::string dequeue()
            {
                auto entry{m_queue.front()};
                m_queue.pop_front();

                entry.m_remaining_transmissions -= 1;

                if (entry.m_remaining_transmissions > 0)
                {
                    m_queue.push_back(entry);
                }

                return entry.m_command;
            }

            bool is_empty() const
            {
                return m_queue.empty();
            }

        private:
            std::list<CommandEntry> m_queue;
            int m_transmissions;
        };

        std::unique_ptr<RollingCommandQueue> m_command_queue{};
        Mutex m_queue_mutex{};
        RCSwitchWarema m_mySwitch{};
        unsigned int m_dataLength{1780};
        unsigned int m_syncLength{5000};
        unsigned int m_sendDelay{10000};

        void WaremaCoverHub::register_cover(WaremaCover *obj)
        {
            this->m_covers.push_back(obj);
        }

        void WaremaCoverHub::setup()
        {
            m_command_queue = std::make_unique<RollingCommandQueue>(m_numberOfTransmissions);
        }

        void WaremaCoverHub::dump_config()
        {
            ESP_LOGCONFIG(TAG, "Warema Cover Hub:");
            ESP_LOGCONFIG(TAG, "  Name: %s", this->get_name().c_str());
            ESP_LOGCONFIG(TAG, "  Used GPIO-Pin: %d", m_transmitterPin->get_pin());
            ESP_LOGCONFIG(TAG, "  Number of Transmissions: %d", m_numberOfTransmissions);
            ESP_LOGCONFIG(TAG, "  Number of registered covers: %d", this->m_covers.size());
        }

        void WaremaCoverHub::loop()
        {
            unsigned long current_time = millis();
            // if (current_time - m_last_time >= 2000)
            // { // Check if 3 seconds have passed
            //     ESP_LOGI(TAG, "Number of text sensors: %d", this->m_text_sensors.size());
            //     ESP_LOGI(TAG, "Counter value: %d", this->m_counter); // Log the counter value
            //     m_last_time = current_time;                          // Update the last time
            // }

            // alle 30 sekunden code ausfuehren
            // unsigned long current_time_30s = millis();
            // if (current_time_30s - m_last_time >= 30000)
            // {
            //     ESP_LOGI(TAG, "Number of text sensors: %d", this->m_text_sensors.size());
            //     for (auto *sensor : this->m_text_sensors)
            //     {
            //         // id des sensors loggen
            //         ESP_LOGI(TAG, "Text Sensor ID: %s", sensor->get_name().c_str());
            //     }
            //     m_last_time = current_time_30s; // Update the last time
            // }
            LockGuard lock{m_queue_mutex};

            if (!m_command_queue->is_empty())
            {
                auto const command{m_command_queue->dequeue()};

                ESP_LOGD(TAG, "Processing command from queue: '%s'", command.c_str());

                m_mySwitch.enableTransmit(m_transmitterPin->get_pin());
                m_mySwitch.sendMC(command, m_dataLength, m_syncLength, 1, m_sendDelay);
                m_mySwitch.disableTransmit();
            }
        }

        void WaremaCoverHub::send_command(const std::string &command)
        {
            ESP_LOGD(TAG, "send_command: '%s'", command.c_str());

            if (command.length() == 36)
            {
                LockGuard lock{m_queue_mutex};
                m_command_queue->enqueue(command);
                ESP_LOGD(TAG, "Command '%s' added to queue", command.c_str());
            }
        }

    } // namespace warema_cover_hub
} // namespace esphome