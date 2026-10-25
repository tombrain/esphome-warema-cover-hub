# Changelog

All notable changes to this project are documented in this file.
The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added

- Event platform: fires `open`, `close`, `stop` or `function_a` to `function_c` in Home
  Assistant for each key pressed on a remote channel, given by `cover_id` or its codes.
- Received telegrams name the configured covers they address, in the log and the text sensor,
  e.g. `(cover: Bathroom)` or `(no cover configured)`.

## [1.1.0] - 2026-10-25

### Added

- Decoding of telegrams from the original remote with an optional `remote_receiver`
  (`receiver_id` on the hub). Received codes are logged, ready to copy into a cover.
- A hub without transmitter works as a receiver-only sniffer; `transmitter_id` is optional now.
- Switch platform to turn decoding of received telegrams on and off from Home Assistant.
- Text sensor platform showing the last received code in Home Assistant.
- The function keys A, B and C of the remote are recognized (`FUNCTION_A` to `FUNCTION_C`).
- Config check: a receiver with `idle` below 7 ms is rejected, it would cut the telegrams at
  their syncs.
- Cover option `reverse_delay` (default 1 s), the pause before reversing a moving cover.
- Own transmissions are recognized by content, not only by timing, so their echo is not
  reported as received even when the loop is delayed after sending.
- README: reading the codes, recommended receiver settings, setup photo, credits,
  German translation.

### Fixed

- A new command for a cover replaces its pending repetitions. Before, e.g. a STOP shortly
  after an OPEN was sent alternately with the remaining OPEN repetitions, so the blind could
  start moving again.
- Reversing a moving cover sends STOP first and the new direction after it. Warema receivers
  stop on the opposite command, so the blind stopped while Home Assistant assumed it reversed.
- Toggle after a STOP reverses the last movement. Before, it always closed the cover.
- The send queue is limited to 32 telegrams, so a flood of commands cannot exhaust the memory.
- `open_duration` and `close_duration` of 0 are rejected; they made the computed position invalid.

## [1.0.1] - 2026-10-25

No functional changes.

### Added

- GPL-3.0 license.
- CI: lint and compile for ESP32 (ESP-IDF) and ESP8266 (Arduino).
- README: hardware and wiring, disclaimer.

### Changed

- Code formatted with ESPHome's clang-format and ruff settings.

## [1.0.0] - 2026-10-25

First release as an ESPHome external component.

### Changed

- **Breaking:** The component moved to `components/warema_cover_hub/`.
- **Breaking:** `transmitter_pin` is replaced by a `remote_transmitter` and `transmitter_id`.
  Set `carrier_duty_percent: 100%` on the transmitter.
- Telegrams are sent through ESPHome's `remote_transmitter`; works with Arduino and ESP-IDF.
- `warema_cover_hub_id` on covers is optional when there is only one hub.
- Fixed pause between telegrams (telegram duration + 10 ms), safe with the non-blocking
  transmitter on the ESP32.

### Removed

- **Breaking:** The hub's `name` option.
- The `rc-switch` library.

## [0.1.0] - 2025-11-08

First tagged version, before the restructuring.

[Unreleased]: https://github.com/tombrain/esphome-warema-cover-hub/compare/v1.1.0...HEAD
[1.1.0]: https://github.com/tombrain/esphome-warema-cover-hub/compare/v1.0.1...v1.1.0
[1.0.1]: https://github.com/tombrain/esphome-warema-cover-hub/compare/v1.0.0...v1.0.1
[1.0.0]: https://github.com/tombrain/esphome-warema-cover-hub/compare/v0.1.0...v1.0.0
[0.1.0]: https://github.com/tombrain/esphome-warema-cover-hub/releases/tag/v0.1.0
