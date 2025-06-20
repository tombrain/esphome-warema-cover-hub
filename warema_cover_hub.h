#pragma once

#include "esphome/core/component.h"
#include "test_text_sensor.h"

namespace esphome
{
  namespace warema_cover_hub
  {

    class WaremaCoverHub : public Component
    {
    public:
      void register_text_sensor(TestTextSensor *obj) { this->text_sensors_.push_back(obj); }
      void setup() override;
      void dump_config() override;
      void loop() override; // Add this line to declare the loop method

      // Method to increase the counter by a given value
      void increase_counter(int value);

    protected:
      std::vector<TestTextSensor *> text_sensors_;
      unsigned long last_time_ = 0; // Add this line to keep track of the last time the sensors were displayed
      int counter_ = 0; // Add this line to declare the counter member variable
    };

  } // namespace warema_cover_hub
} // namespace esphome