#pragma once

#include "esphome/core/component.h"


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
        this->name_ = name;
      }

      auto const &get_name() const
      {
        return this->name_;
      }

    private:
      std::vector<WaremaCover *> covers_{};
      unsigned long last_time_{0}; // Add this line to keep track of the last time the sensors were displayed
      int counter_{0};             // Add this line to declare the counter member variable

      std::string lastSensorName_{}; // Variable to store the last sensor name

      std::string name_{};
    };

  } // namespace warema_cover_hub
} // namespace esphome