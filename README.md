# Warema EWFS Cover Hub for ESPHome

[![CI](https://github.com/tombrain/esphome-warema-cover-hub/actions/workflows/ci.yml/badge.svg)](https://github.com/tombrain/esphome-warema-cover-hub/actions/workflows/ci.yml)

**English** | [Deutsch](README.de.md)

An [ESPHome](https://esphome.io) external component for controlling Warema EWFS roller blinds over 433 MHz.

It consists of a hub that sends the radio telegrams through a
[`remote_transmitter`](https://esphome.io/components/remote_transmitter.html) and any number of covers.
With a [`remote_receiver`](https://esphome.io/components/remote_receiver.html), the hub also
decodes the telegrams of the original remote, so you can read the codes of your blinds.
The cover position is estimated from the open and close durations, like ESPHome's
[`time_based`](https://esphome.io/components/cover/time_based.html) cover.

Works with ESP32 and ESP8266, on both the Arduino and the ESP-IDF framework.

> This project is not affiliated with or endorsed by WAREMA. Use it at your own risk.

## Hardware

You need an ESP32 or ESP8266 and a 433 MHz OOK transmitter module, e.g. FS1000A.

| Transmitter | ESP |
|---|---|
| `VCC` | 5 V (the module accepts 3.3–12 V; a higher voltage gives more range) |
| `GND` | GND |
| `DATA` | any free output GPIO, e.g. GPIO4 (ESP32) or D1 (ESP8266) |

The 3.3 V logic level of the ESP is sufficient for `DATA`. On the ESP32, avoid the strapping
pins GPIO0, 2, 5, 12 and 15.

Solder a 17.3 cm wire (a quarter wavelength at 433.92 MHz) to the `ANT` pad of the module.
Without an antenna the range is only a few meters.

The blinds do not report their position. The component estimates it from the run time, so it
drifts when a blind is moved with the original remote.

## Installation

```yaml
external_components:
  - source: github://tombrain/esphome-warema-cover-hub@v1.4.0
    components: [warema_cover_hub]
```

Pin a release tag (`@v1.4.0`). Without it, ESPHome follows the `main` branch.

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
    devicecode1: "101010010"
    devicecode2: "101010011"
    open_duration: 24s
    close_duration: 24s
```

A complete example is in [example.yaml](example.yaml).

### Hub: `warema_cover_hub`

| Option | Required | Default | Description |
|---|---|---|---|
| `id` | no | | ID of the hub. Needed only if you configure more than one hub. |
| `transmitter_id` | no* | | ID of the `remote_transmitter` to send with. |
| `receiver_id` | no* | | ID of a `remote_receiver`. Received telegrams are logged, see [Finding the codes](#finding-the-codes). Own transmissions are filtered out. |
| `number_of_transmissions` | no | `3` | How often each telegram is sent (1–10). |
| `on_unassigned` | no | | Automation for received telegrams of channels without cover or event. See below. |

\* At least one of `transmitter_id` and `receiver_id` is required.

Telegrams are sent round-robin: if several covers are commanded at once, each one gets its
first transmission before any telegram is repeated. A new command for a cover replaces its
pending repetitions, so a STOP is never followed by a leftover OPEN.

### Cover: `platform: warema_cover_hub`

| Option | Required | Description |
|---|---|---|
| `warema_cover_hub_id` | no | ID of the hub. Needed only if you configure more than one hub. |
| `command` | yes | 3-bit channel code of the blind, e.g. `"111"`. |
| `devicecode1` | yes | First 9-bit device code, e.g. `"101010010"`. |
| `devicecode2` | yes | Second 9-bit device code. |
| `open_duration` | yes | Time to fully open. |
| `close_duration` | yes | Time to fully close. |
| `reverse_delay` | no | Pause before reversing a moving cover, default `1s`. See below. |
| `follow_remote` | no | Track keys pressed on the original remote, default `true`. See below. |
| `members` | no | IDs of the covers this channel moves too, e.g. for an "all" channel. See below. |
| `additional_remotes` | no | Codes (`command`, `devicecode1`, `devicecode2`) of further remotes programmed to this blind. See below. |

Plus all [base cover options](https://esphome.io/components/cover/#base-cover-configuration).

The easiest way to get the durations is a stopwatch: open the blind fully once and close it
fully once. The two times may differ.

A Warema receiver stops when it gets the opposite command while moving. So to reverse a
moving cover, the component sends STOP first and the new direction after all STOP
repetitions are sent and `reverse_delay` has passed.

With a receiver on the hub, a cover follows the keys pressed on the original remote, so its
state in Home Assistant stays right: UP and DOWN start it, the opposite key or STOP stops it,
like the motor does. Nothing is sent for this. Pending repetitions of an own command for the
cover are dropped, so they cannot override the remote. Set `follow_remote: false` to turn
this off for a cover.

A channel that moves several blinds, e.g. "all" on the remote, is a cover with `members`.
When it moves, from Home Assistant or the remote, its members follow without sending
anything. A cover can be a member of several channels; groups cannot be nested.

```yaml
cover:
  - platform: warema_cover_hub
    id: living_1
    name: "Living room 1"
    # ...
  - platform: warema_cover_hub
    id: living_2
    name: "Living room 2"
    # ...
  - platform: warema_cover_hub
    name: "Living room all"
    command: "011"
    # ...
    members: [living_1, living_2]
```

If several remotes are programmed to a blind, the cover sends with its own codes only, but
list the codes of the other remotes under `additional_remotes`. Then their keys count for the
cover as well: it follows them, the log and the text sensor show the cover's name, events with
`cover_id` fire, and `on_unassigned` no longer runs for them. For a group channel on another
remote, add its codes to the group cover.

```yaml
cover:
  - platform: warema_cover_hub
    name: "Living room 1"
    command: "011"
    devicecode1: "101010010"
    devicecode2: "101010011"
    # ...
    additional_remotes:
      - command: "001"
        devicecode1: "110011001"
        devicecode2: "110011010"
```

### Switch: `platform: warema_cover_hub`

Shows or hides received codes in the log and the text sensor, e.g. turned on only while
reading new codes. On by default, the state is restored after a reboot. Requires
`receiver_id` on the hub. Events and covers following the remote work regardless of it.
Named "Show Warema codes" unless you set a `name`.

### Text sensor: `platform: warema_cover_hub`

Shows the last received telegram, e.g.
`OPEN, command: 111, devicecode1: 101010010, devicecode2: 101010011 (cover: Bathroom)`.
Requires `receiver_id` on the hub. Named "Received Warema code" unless you set a `name`.

Both take `warema_cover_hub_id` (needed only with more than one hub) and the base
[switch](https://esphome.io/components/switch/#base-switch-configuration) or
[text sensor](https://esphome.io/components/text_sensor/#base-text-sensor-configuration) options.

### Event: `platform: warema_cover_hub`

Fires an event in Home Assistant for every key pressed on one channel of a remote, so the
remote can trigger automations, e.g. the function keys A, B and C or the keys of a channel
without a blind. Requires `receiver_id` on the hub. Own transmissions do not fire events.

```yaml
event:
  # Channel of a configured cover
  - platform: warema_cover_hub
    name: "Remote Bathroom"
    cover_id: cover_bathroom

  # Channel without a cover
  - platform: warema_cover_hub
    name: "Remote channel 2"
    command: "110"
    devicecode1: "101010010"
    devicecode2: "101010011"
```

| Option | Required | Description |
|---|---|---|
| `cover_id` | one of | ID of a cover whose channel is meant, including its `additional_remotes`. |
| `command`, `devicecode1`, `devicecode2` | one of | The codes of the channel, as for a cover. |
| `warema_cover_hub_id` | no | ID of the hub. Needed only if you configure more than one hub. |

Plus the base [event options](https://esphome.io/components/event/#base-event-configuration).
The event types are `open`, `close`, `stop`, `function_a`, `function_b`, `function_c` and
`learn` (the learn key L).
An automation in Home Assistant reacting to key A:

```yaml
triggers:
  - trigger: state
    entity_id: event.remote_bathroom
    not_from: [unavailable, unknown]
conditions:
  - condition: state
    entity_id: event.remote_bathroom
    attribute: event_type
    state: function_a
actions:
  - action: light.toggle
    target:
      entity_id: light.bathroom
```

## Finding the codes

Each blind is addressed by three codes, which you read from its original remote. The hub can
decode them when you connect a 433 MHz receiver module, e.g. an RXB6 or a cheaper XY-MK-5V.
Connect VCC, GND and DATA like the transmitter. Mind the voltage: the GPIOs of the ESP take
3.3 V only. Run the RXB6 from 3.3 V; the XY-MK-5V needs 5 V and then outputs 5 V on DATA,
so put a voltage divider (e.g. 10 kΩ / 20 kΩ) in front of the GPIO.

![ESP8266 with a 433 MHz receiver on a breadboard](docs/sniffer-setup.jpg)

A receiver-only setup like this is enough for reading the codes:

```yaml
remote_receiver:
  id: rf_receiver
  pin: D5             # ESP8266 (GPIO14); any free input GPIO on the ESP32
  filter: 400us       # suppresses receiver noise, Warema pulses are at least ~800 µs
  idle: 10ms          # at least 7 ms, syncs are up to 6.8 ms long
  buffer_size: 4kb    # ESP8266 only

warema_cover_hub:
  receiver_id: rf_receiver

# Optional: control and show it in Home Assistant
switch:
  - platform: warema_cover_hub    # named "Show Warema codes" unless you set a name

text_sensor:
  - platform: warema_cover_hub    # named "Received Warema code" unless you set a name
```

Press a button on the remote near the receiver and watch the log (or the text sensor):

```
[I][warema_cover_hub]: Received OPEN, command: 111, devicecode1: 101010010, devicecode2: 101010011 (no cover configured)
```

Copy `command`, `devicecode1` and `devicecode2` into the cover configuration. Once a cover with
these codes is configured, the end of the line shows its name instead, e.g. `(cover: Bathroom)`. Repeat this
for every blind or channel of the remote. A channel that moves several blinds at once
(e.g. "all") has its own `command` and becomes a cover of its own.

A regular `Buffer overflow` warning of `remote_receiver`, e.g. about once a minute, is
usually caused by other 433 MHz devices such as weather sensors: their packets are longer than
the buffer. Only their signal is dropped, so you can hide the warning:

```yaml
logger:
  logs:
    remote_receiver: ERROR
```

Besides `OPEN`, `CLOSE` and `STOP`, the function keys A, B and C of the remote are shown as
`FUNCTION_A` to `FUNCTION_C` and the learn key L as `LEARN`. `UNKNOWN` means a key this component does not know yet. Please
open an issue with the telegram from the debug log.

### Reacting to unassigned codes

`on_unassigned` on the hub runs for each key pressed on a channel that has no cover and no
event, e.g. a new remote. It provides the strings `key` (`open`, `close`, `stop`,
`function_a` to `function_c`, `learn` or `unknown`), `command`, `devicecode1`, `devicecode2` and
`description`. For example, to get a notification in Home Assistant:

```yaml
warema_cover_hub:
  receiver_id: rf_receiver
  on_unassigned:
    - homeassistant.event:
        event: esphome.warema_unassigned
        data:
          key: !lambda "return key;"
          command: !lambda "return command;"
          devicecode1: !lambda "return devicecode1;"
          devicecode2: !lambda "return devicecode2;"
```

```yaml
# Home Assistant automation
triggers:
  - trigger: event
    event_type: esphome.warema_unassigned
actions:
  - action: notify.notify
    data:
      message: >-
        New Warema code: {{ trigger.event.data.key }}, command: {{ trigger.event.data.command }},
        devicecode1: {{ trigger.event.data.devicecode1 }}, devicecode2: {{ trigger.event.data.devicecode2 }}
```

`homeassistant.event` requires the option "Allow the device to perform Home Assistant
actions" of the device in the ESPHome integration.

### Protocol

A telegram has the layout
`S <command bits:8> <command:3> <suffix:3> S <devicecode1:9> S <devicecode2:9> S`.
`S` is a 5000 µs high sync, each bit lasts 1780 µs: `0` is high then low, `1` is low then
high (Manchester code). The command bits and the suffix select OPEN, CLOSE or STOP.

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

## Credits

The protocol is taken from [aircontrol](https://github.com/rfkd/aircontrol) by
Ralf Dauberschmidt, which controls Warema EWFS devices from a Raspberry Pi.

## License

[GPL-3.0](LICENSE). The position tracking of the cover is derived from ESPHome's
[`time_based`](https://github.com/esphome/esphome/tree/dev/esphome/components/time_based) cover,
which is licensed under GPL-3.0 as part of ESPHome's C++ code.
