# Warema EWFS Cover Hub for ESPHome

An [ESPHome](https://esphome.io) external component for controlling Warema EWFS roller blinds over 433 MHz.

It consists of a hub that sends the radio telegrams through a
[`remote_transmitter`](https://esphome.io/components/remote_transmitter.html) and any number of covers.
The cover position is estimated from the open and close durations, like ESPHome's
[`time_based`](https://esphome.io/components/cover/time_based.html) cover.

Required hardware: an ESP32 or ESP8266 and a 433 MHz transmitter module (e.g. FS1000A).
Works with both the Arduino and the ESP-IDF framework.

## Installation

```yaml
external_components:
  - source: github://tombrain/esphome-warema-cover-hub@v1.0.0
    components: [warema_cover_hub]
```

Pin a release tag (`@v1.0.0`). Without it, ESPHome follows the `main` branch.

## Configuration

```yaml
remote_transmitter:
  id: rf_transmitter
  pin: GPIO4
  carrier_duty_percent: 100%   # required for 433 MHz OOK modules

warema_cover_hub:
  id: warema_hub
  transmitter_id: rf_transmitter
  number_of_transmissions: 3

cover:
  - platform: warema_cover_hub
    name: "Cover Bathroom"
    command: "111"
    devicecode1: "000011100"
    devicecode2: "000011101"
    open_duration: 24s
    close_duration: 24s
```

A complete example is in [example.yaml](example.yaml).

### Hub: `warema_cover_hub`

| Option | Required | Default | Description |
|---|---|---|---|
| `id` | no | | ID of the hub. Needed only if you configure more than one hub. |
| `transmitter_id` | yes | | ID of the `remote_transmitter` to send with. |
| `number_of_transmissions` | no | `3` | How often each telegram is sent (1–10). |

Telegrams are sent round-robin: if several covers are commanded at once, each one gets its
first transmission before any telegram is repeated.

### Cover: `platform: warema_cover_hub`

| Option | Required | Description |
|---|---|---|
| `warema_cover_hub_id` | no | ID of the hub. Needed only if you configure more than one hub. |
| `command` | yes | 3-bit channel code of the blind, e.g. `"111"`. |
| `devicecode1` | yes | First 9-bit device code, e.g. `"000011100"`. |
| `devicecode2` | yes | Second 9-bit device code. |
| `open_duration` | yes | Time to fully open. |
| `close_duration` | yes | Time to fully close. |

Plus all [base cover options](https://esphome.io/components/cover/#base-cover-configuration).

## Finding the codes

Each blind has its own codes. Read them from the original remote with a 433 MHz receiver:

```yaml
remote_receiver:
  pin: GPIO14
  dump: raw
```

Press a button on the remote and decode the logged timings: a pulse of ~5000 µs is a sync
(`S`), a 1780 µs bit is `0` when it is high first and `1` when it is low first.
The telegram has the layout
`S <8 command bits> <command:3> <suffix:3> S <devicecode1:9> S <devicecode2:9> S`.

## Versioning

This project uses [Semantic Versioning](https://semver.org). Breaking changes to the YAML
configuration increase the major version. See the
[releases](https://github.com/tombrain/esphome-warema-cover-hub/releases) for changes.

### Migrating from 0.x

- The component moved to `components/warema_cover_hub/`.
- `transmitter_pin` is replaced by a `remote_transmitter` and `transmitter_id`.
- The hub's `name` option is gone.
- `warema_cover_hub_id` on covers is optional when there is only one hub.
- The `rc-switch` library is no longer used.

## License

[GPL-3.0](LICENSE). The position tracking of the cover is derived from ESPHome's
[`time_based`](https://github.com/esphome/esphome/tree/dev/esphome/components/time_based) cover,
which is licensed under GPL-3.0 as part of ESPHome's C++ code.
