#pragma once

#include "esphome/core/component.h"
#include "esphome/core/gpio.h"

namespace esphome
{
  namespace warema_cover_hub
  {
    class WaremaCover; // Forward declaration of WaremaCover
    class WaremaCoverHub : public Component
    {
    public:
      void register_cover(WaremaCover *obj);
      void setup() override;
      void dump_config() override;
      void loop() override; // Add this line to declare the loop method

      void set_name(const std::string &name)
      {
        m_name = name;
      }

      auto const &get_name() const
      {
        return m_name;
      }

      void set_transmitter_pin(InternalGPIOPin *pin)
      {
        m_transmitterPin = pin;
      }

      void set_number_of_transmissions(uint8_t const &number_of_transmissions)
      {
        m_numberOfTransmissions = number_of_transmissions;
      }

      void send_command(const std::string &command);

    private:
      std::vector<WaremaCover *> m_covers{};
      unsigned long m_lastTime{0}; // Add this line to keep track of the last time the sensors were displayed
      int m_counter{0};             // Add this line to declare the counter member variable

      std::string m_lastSensorName{}; // Variable to store the last sensor name

      std::string m_name{};

      uint8_t m_numberOfTransmissions{3}; // Default to 3 transmissions

      InternalGPIOPin *m_transmitterPin{nullptr};

    };

  } // namespace warema_cover_hub
} // namespace esphome