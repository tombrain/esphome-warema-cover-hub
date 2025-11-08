The plugin enables control of Warema EWFS roller blinds.

It consists of a hub that transmits the radio signals and individual roller blinds that can be controlled. The control system is based on the logic of ‘TimeBasedCover’, which calculates the status over time.

The only hardware required is an ESP and a transmitter for 433 MHz signals.

Each roller shutter must be configured with its own codes. The codes can be sniffed using the following setup.


How ot use: https://github.com/jesserockz/esphome-external-component-examples

Structure of files:

```
esphome
├── components
|   ├── warema_cover_hub
|   │   ├── __init__.py
|   │   ├── cover.py
|   │   ├── RCSwitchWarema.cpp
|   │   ├── RCSwitchWarema.h
|   │   ├── warema_cover_hub.cpp
|   │   ├── warema_cover_hub.h
|   │   ├── warema_ewfs_cover.cpp
|   │   ├── warema_ewfs_cover.h
│  ...
```

```yaml
# example configuration:

warema_cover_hub:
  id: warema_hub
  name: "warema_hub_name"
  transmitter_pin: D1
  number_of_transmissions: 10

cover:
  - platform: warema_cover_hub
    warema_cover_hub_id: warema_hub
    name: "Cover Bathroom"
    command: "111"
    devicecode1: "000011100"
    devicecode2: "000011101"
    open_duration: 24sec
    close_duration: 24sec
```