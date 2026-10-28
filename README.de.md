# Warema EWFS Cover Hub für ESPHome

[![CI](https://github.com/tombrain/esphome-warema-cover-hub/actions/workflows/ci.yml/badge.svg)](https://github.com/tombrain/esphome-warema-cover-hub/actions/workflows/ci.yml)

[English](README.md) | **Deutsch**

Eine [ESPHome](https://esphome.io) External Component, mit der du Warema-EWFS-Rollläden über 433 MHz steuerst.

Sie besteht aus einem Hub, der die Funktelegramme über einen
[`remote_transmitter`](https://esphome.io/components/remote_transmitter.html) sendet, und beliebig vielen Covers.
Mit einem [`remote_receiver`](https://esphome.io/components/remote_receiver.html) dekodiert der Hub
außerdem die Telegramme der Original-Fernbedienung, damit kannst du die Codes deiner Rollläden auslesen.
Die Position der Rollläden wird aus der Laufzeit berechnet, wie beim
[`time_based`](https://esphome.io/components/cover/time_based.html)-Cover von ESPHome.

Läuft auf ESP32 und ESP8266, mit dem Arduino- und dem ESP-IDF-Framework.

> Dieses Projekt steht in keiner Verbindung zu WAREMA und wird von WAREMA nicht unterstützt.
> Die Nutzung erfolgt auf eigene Gefahr.

## Hardware

Du brauchst einen ESP32 oder ESP8266 und ein 433-MHz-OOK-Sendemodul, z. B. FS1000A.

| Sender | ESP |
|---|---|
| `VCC` | 5 V (das Modul verträgt 3,3–12 V; mehr Spannung bringt mehr Reichweite) |
| `GND` | GND |
| `DATA` | ein freier Ausgangs-GPIO, z. B. GPIO4 (ESP32) oder D1 (ESP8266) |

Der 3,3-V-Pegel des ESP reicht für `DATA`. Beim ESP32 solltest du die Strapping-Pins
GPIO0, 2, 5, 12 und 15 meiden.

Löte einen 17,3 cm langen Draht (eine Viertelwellenlänge bei 433,92 MHz) an das `ANT`-Pad des
Moduls. Ohne Antenne beträgt die Reichweite nur wenige Meter.

Die Rollläden melden ihre Position nicht zurück. Die Komponente schätzt sie anhand der Laufzeit,
deshalb weicht sie ab, wenn ein Rollladen mit der Original-Fernbedienung bewegt wird.

## Installation

```yaml
external_components:
  - source: github://tombrain/esphome-warema-cover-hub@v1.4.0
    components: [warema_cover_hub]
```

Gib einen Release-Tag an (`@v1.4.0`). Ohne Tag folgt ESPHome dem Branch `main`.

## Konfiguration

```yaml
remote_transmitter:
  id: rf_transmitter
  pin: GPIO4
  carrier_duty_percent: 100%   # nötig für 433-MHz-OOK-Module

warema_cover_hub:
  id: warema_hub
  transmitter_id: rf_transmitter
  number_of_transmissions: 3

cover:
  - platform: warema_cover_hub
    name: "Rollladen Bad"
    command: "111"
    devicecode1: "101010010"
    devicecode2: "101010011"
    open_duration: 24s
    close_duration: 24s
```

Ein vollständiges Beispiel findest du in [example.yaml](example.yaml).

### Hub: `warema_cover_hub`

| Option | Pflicht | Standard | Beschreibung |
|---|---|---|---|
| `id` | nein | | ID des Hubs. Nur nötig, wenn du mehr als einen Hub konfigurierst. |
| `transmitter_id` | nein* | | ID des `remote_transmitter`, über den gesendet wird. |
| `receiver_id` | nein* | | ID eines `remote_receiver`. Empfangene Telegramme werden geloggt, siehe [Codes auslesen](#codes-auslesen). Eigene Sendungen werden herausgefiltert. |
| `number_of_transmissions` | nein | `3` | Wie oft jedes Telegramm gesendet wird (1–10). |
| `on_unassigned` | nein | | Automation für empfangene Telegramme von Kanälen ohne Cover oder Event. Siehe unten. |

\* Mindestens eine der Optionen `transmitter_id` und `receiver_id` ist nötig.

Die Telegramme werden reihum gesendet: Bekommen mehrere Covers gleichzeitig einen Befehl, wird
jedes Telegramm einmal gesendet, bevor eines wiederholt wird. Ein neuer Befehl für ein Cover
ersetzt dessen noch ausstehende Wiederholungen, auf ein STOP folgt also nie ein übrig gebliebenes
OPEN.

### Cover: `platform: warema_cover_hub`

| Option | Pflicht | Beschreibung |
|---|---|---|
| `warema_cover_hub_id` | nein | ID des Hubs. Nur nötig, wenn du mehr als einen Hub konfigurierst. |
| `command` | ja | 3-Bit-Kanalcode des Rollladens, z. B. `"111"`. |
| `devicecode1` | ja | Erster 9-Bit-Gerätecode, z. B. `"101010010"`. |
| `devicecode2` | ja | Zweiter 9-Bit-Gerätecode. |
| `open_duration` | ja | Zeit zum vollständigen Öffnen. |
| `close_duration` | ja | Zeit zum vollständigen Schließen. |
| `reverse_delay` | nein | Pause vor dem Umkehren eines fahrenden Covers, Standard `1s`. Siehe unten. |
| `follow_remote` | nein | Tastendrücke der Original-Fernbedienung nachverfolgen, Standard `true`. Siehe unten. |
| `members` | nein | IDs der Covers, die dieser Kanal mitbewegt, z. B. bei einem Kanal „alle“. Siehe unten. |
| `additional_remotes` | nein | Codes (`command`, `devicecode1`, `devicecode2`) weiterer Fernbedienungen, die auf diesen Rollladen eingelernt sind. Siehe unten. |

Dazu kommen alle [Basis-Optionen für Covers](https://esphome.io/components/cover/#base-cover-configuration).

Die Laufzeiten misst du am einfachsten mit der Stoppuhr: Rollladen einmal ganz öffnen und einmal
ganz schließen. Die beiden Zeiten können sich unterscheiden.

Ein Warema-Empfänger bleibt stehen, wenn er während der Fahrt den Gegenbefehl bekommt. Um ein
fahrendes Cover umzukehren, sendet die Komponente deshalb zuerst STOP und die neue Richtung erst,
wenn alle STOP-Wiederholungen gesendet sind und `reverse_delay` vergangen ist.

Hat der Hub einen Empfänger, folgt ein Cover den Tasten der Original-Fernbedienung, damit sein
Zustand in Home Assistant stimmt: HOCH und RUNTER starten es, die Gegentaste oder STOPP hält es an,
genau wie beim Motor. Dabei wird nichts gesendet. Noch ausstehende Wiederholungen eines eigenen
Befehls für das Cover werden verworfen, damit sie die Fernbedienung nicht übersteuern. Mit
`follow_remote: false` lässt sich das pro Cover abschalten.

Ein Kanal, der mehrere Rollläden fährt, z. B. „alle“ auf der Fernbedienung, ist ein Cover mit
`members`. Bewegt er sich, aus Home Assistant oder über die Fernbedienung, folgen seine Mitglieder,
ohne selbst zu senden. Ein Cover kann Mitglied mehrerer Kanäle sein, Gruppen lassen sich aber nicht
verschachteln.

```yaml
cover:
  - platform: warema_cover_hub
    id: wohnzimmer_1
    name: "Wohnzimmer 1"
    # ...
  - platform: warema_cover_hub
    id: wohnzimmer_2
    name: "Wohnzimmer 2"
    # ...
  - platform: warema_cover_hub
    name: "Wohnzimmer alle"
    command: "011"
    # ...
    members: [wohnzimmer_1, wohnzimmer_2]
```

Sind mehrere Fernbedienungen auf einen Rollladen eingelernt, sendet das Cover nur mit seinen
eigenen Codes. Die Codes der anderen Fernbedienungen trägst du unter `additional_remotes` ein.
Dann zählen auch ihre Tasten für das Cover: Es folgt ihnen, Log und Textsensor zeigen den Namen des
Covers, Events mit `cover_id` lösen aus und `on_unassigned` läuft für sie nicht mehr. Für einen
Gruppenkanal auf einer anderen Fernbedienung trägst du dessen Codes beim Gruppen-Cover ein.

```yaml
cover:
  - platform: warema_cover_hub
    name: "Wohnzimmer 1"
    command: "011"
    devicecode1: "101010010"
    devicecode2: "101010011"
    # ...
    additional_remotes:
      - command: "001"
        devicecode1: "110011001"
        devicecode2: "110011010"
```

### Schalter: `platform: warema_cover_hub`

Blendet empfangene Codes in Log und Textsensor ein und aus, z. B. nur eingeschaltet, solange du
neue Codes ausliest. Standardmäßig an, der Zustand bleibt nach einem Neustart erhalten. Braucht
`receiver_id` am Hub. Events und das Mitlaufen der Covers funktionieren unabhängig davon.
Ohne `name` heißt er „Show Warema codes“.

### Textsensor: `platform: warema_cover_hub`

Zeigt das zuletzt empfangene Telegramm an, z. B.
`OPEN, command: 111, devicecode1: 101010010, devicecode2: 101010011 (cover: Bad)`.
Braucht `receiver_id` am Hub. Ohne `name` heißt er „Received Warema code“.

Beide kennen `warema_cover_hub_id` (nur bei mehr als einem Hub nötig) und die Basis-Optionen für
[Schalter](https://esphome.io/components/switch/#base-switch-configuration) bzw.
[Textsensoren](https://esphome.io/components/text_sensor/#base-text-sensor-configuration).

### Event: `platform: warema_cover_hub`

Löst in Home Assistant für jeden Tastendruck auf einem Kanal einer Fernbedienung ein Ereignis aus.
So kann die Fernbedienung Automationen starten, z. B. mit den Funktionstasten A, B und C oder den
Tasten eines Kanals ohne Rollladen. Braucht `receiver_id` am Hub. Eigene Sendungen lösen kein
Ereignis aus.

```yaml
event:
  # Kanal eines konfigurierten Covers
  - platform: warema_cover_hub
    name: "Fernbedienung Bad"
    cover_id: cover_bad

  # Kanal ohne Cover
  - platform: warema_cover_hub
    name: "Fernbedienung Kanal 2"
    command: "110"
    devicecode1: "101010010"
    devicecode2: "101010011"
```

| Option | Pflicht | Beschreibung |
|---|---|---|
| `cover_id` | eins von beiden | ID eines Covers, dessen Kanal gemeint ist, einschließlich seiner `additional_remotes`. |
| `command`, `devicecode1`, `devicecode2` | eins von beiden | Die Codes des Kanals, wie beim Cover. |
| `warema_cover_hub_id` | nein | ID des Hubs. Nur nötig, wenn du mehr als einen Hub konfigurierst. |

Dazu kommen die [Basis-Optionen für Events](https://esphome.io/components/event/#base-event-configuration).
Die Ereignistypen sind `open`, `close`, `stop`, `function_a`, `function_b`, `function_c` und
`learn` (die Lerntaste L).
Eine Automation in Home Assistant, die auf Taste A reagiert:

```yaml
triggers:
  - trigger: state
    entity_id: event.fernbedienung_bad
    not_from: [unavailable, unknown]
conditions:
  - condition: state
    entity_id: event.fernbedienung_bad
    attribute: event_type
    state: function_a
actions:
  - action: light.toggle
    target:
      entity_id: light.bad
```

## Codes auslesen

Jeder Rollladen wird über drei Codes angesprochen, die du von seiner Original-Fernbedienung
ausliest. Das erledigt der Hub, wenn du ein 433-MHz-Empfangsmodul anschließt, z. B. einen RXB6
oder einen günstigeren XY-MK-5V. VCC, GND und DATA schließt du an wie beim Sender. Achte auf
die Spannung: Die GPIOs des ESP vertragen nur 3,3 V. Den RXB6 betreibst du an 3,3 V. Der
XY-MK-5V braucht 5 V und gibt dann auch 5 V auf DATA aus, deshalb gehört ein Spannungsteiler
(z. B. 10 kΩ / 20 kΩ) vor den GPIO.

![ESP8266 mit 433-MHz-Empfänger auf einem Steckbrett](docs/sniffer-setup.jpg)

Zum Auslesen der Codes reicht ein Aufbau nur mit Empfänger wie dieser:

```yaml
remote_receiver:
  id: rf_receiver
  pin: D5             # ESP8266 (GPIO14); beim ESP32 ein beliebiger freier Eingangs-GPIO
  filter: 400us       # unterdrückt Empfängerrauschen, Warema-Pulse sind mindestens ~800 µs lang
  idle: 10ms          # mindestens 7 ms, ein Sync ist bis zu 6,8 ms lang
  buffer_size: 4kb    # nur ESP8266

warema_cover_hub:
  receiver_id: rf_receiver

# Optional: in Home Assistant schalten und anzeigen
switch:
  - platform: warema_cover_hub
    name: "Warema Codes anzeigen"     # optional, Standard: "Show Warema codes"

text_sensor:
  - platform: warema_cover_hub
    name: "Empfangener Warema Code"   # optional, Standard: "Received Warema code"
```

Drücke in der Nähe des Empfängers eine Taste der Fernbedienung und beobachte das Log (oder den
Textsensor):

```
[I][warema_cover_hub]: Received OPEN, command: 111, devicecode1: 101010010, devicecode2: 101010011 (no cover configured)
```

Übernimm `command`, `devicecode1` und `devicecode2` in die Cover-Konfiguration. Sobald ein Cover
mit diesen Codes konfiguriert ist, steht am Zeilenende stattdessen sein Name, z. B. `(cover: Bad)`. Wiederhole das
für jeden Rollladen bzw. jeden Kanal der Fernbedienung. Ein Kanal, der mehrere Rollläden
gleichzeitig fährt (z. B. „alle“), hat einen eigenen `command` und wird ein eigenes Cover.

Eine regelmäßige Warnung `Buffer overflow` von `remote_receiver`, z. B. etwa einmal pro Minute,
stammt meist von anderen 433-MHz-Geräten wie Funk-Temperatursensoren: Ihre Pakete sind länger
als der Puffer. Verworfen wird nur ihr Signal, du kannst die Warnung also ausblenden:

```yaml
logger:
  logs:
    remote_receiver: ERROR
```

Neben `OPEN`, `CLOSE` und `STOP` werden die Funktionstasten A, B und C der Fernbedienung als
`FUNCTION_A` bis `FUNCTION_C` angezeigt, die Lerntaste L als `LEARN`. `UNKNOWN` steht für eine Taste, die diese Komponente
noch nicht kennt. Eröffne dann bitte ein Issue mit dem Telegramm aus dem Debug-Log.

### Auf nicht zugeordnete Codes reagieren

`on_unassigned` am Hub läuft bei jedem Tastendruck auf einem Kanal, der weder ein Cover noch ein
Event hat, z. B. bei einer neuen Fernbedienung. Zur Verfügung stehen die Texte `key` (`open`,
`close`, `stop`, `function_a` bis `function_c`, `learn` oder `unknown`), `command`, `devicecode1`,
`devicecode2` und `description`. Zum Beispiel für eine Benachrichtigung in Home Assistant:

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
# Automation in Home Assistant
triggers:
  - trigger: event
    event_type: esphome.warema_unassigned
actions:
  - action: notify.notify
    data:
      message: >-
        Neuer Warema-Code: {{ trigger.event.data.key }}, command: {{ trigger.event.data.command }},
        devicecode1: {{ trigger.event.data.devicecode1 }}, devicecode2: {{ trigger.event.data.devicecode2 }}
```

Für `homeassistant.event` muss in der ESPHome-Integration beim Gerät die Option „Dem Gerät erlauben,
Home-Assistant-Aktionen auszuführen“ aktiviert sein.

### Protokoll

Ein Telegramm ist so aufgebaut:
`S <Befehlsbits:8> <command:3> <Suffix:3> S <devicecode1:9> S <devicecode2:9> S`.
`S` ist ein Sync-Puls mit 5000 µs High, jedes Bit dauert 1780 µs: `0` ist erst High, dann Low,
`1` ist erst Low, dann High (Manchester-Code). Befehlsbits und Suffix legen fest, ob es OPEN,
CLOSE oder STOP ist.

## Versionierung

Dieses Projekt folgt [Semantic Versioning](https://semver.org). Inkompatible Änderungen an der
YAML-Konfiguration erhöhen die Hauptversion. Die Änderungen stehen bei den
[Releases](https://github.com/tombrain/esphome-warema-cover-hub/releases).

### Umstieg von 0.x

- Die Komponente liegt jetzt unter `components/warema_cover_hub/`.
- `transmitter_pin` wird durch einen `remote_transmitter` und `transmitter_id` ersetzt.
- Die Option `name` des Hubs entfällt.
- `warema_cover_hub_id` ist bei den Covers optional, wenn es nur einen Hub gibt.
- Die Bibliothek `rc-switch` wird nicht mehr verwendet.

## Credits

Das Protokoll stammt aus [aircontrol](https://github.com/rfkd/aircontrol) von Ralf Dauberschmidt.
Damit lassen sich Warema-EWFS-Geräte von einem Raspberry Pi aus steuern.

## Lizenz

[GPL-3.0](LICENSE). Die Positionsberechnung des Covers ist vom
[`time_based`](https://github.com/esphome/esphome/tree/dev/esphome/components/time_based)-Cover
von ESPHome abgeleitet, das als Teil des C++-Codes von ESPHome unter GPL-3.0 steht.
