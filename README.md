# SmartLights ESP8266 — starter project

Platform: Wemos D1 mini / ESP8266, Arduino framework, PlatformIO.

## What's included

- Shared configuration structure and SaverFile persistence in LittleFS.
- Configurable active relay count (`relayCount`, default 8; maximum `MAX_RELAY = 24`).
- Per-relay name, comment and power-up state.
- Radio command and schedule data structures.
- Two Wi-Fi profiles, two NTP server slots and POSIX timezone setting.
- Starter module interfaces for relay, Wi-Fi, NTP, radio, MQTT, web and rules.
- Three 74HC595 shift-register output driver scaffold.

## Build

Open the folder in VS Code + PlatformIO and run **Build**.

Upload filesystem contents separately using PlatformIO's LittleFS upload task after adding files to `data/`.

## Important hardware assumptions to confirm

1. 74HC595 wiring in `src/Relay.cpp`: D5=clock, D6=data, D7=latch.
2. Relay polarity: `RELAY_ACTIVE_LOW` in `src/Relay.cpp`.
3. R1A-M5N UART: assumed 9600 baud, RX on D1 and newline-delimited codes. Confirm against the module documentation and actual output.
4. Relay bit order may need reversing after hardware testing.

## Security

- Replace the default admin password (`change-me-now`) before enabling admin routes.
- Setup AP currently uses SSID `SmartLights-Setup`, password `setup1234`. Change this before deployment.
- The web server currently exposes only a diagnostic page; admin authentication is not implemented yet.
- MQTT and non-blocking scenario execution are interface stubs, not finished features.

## LittleFS layout

- `/config.bin` is managed by SaverFile.
- HTML/CSS/JS assets will live in the local `data/` folder and be uploaded to LittleFS separately.
- Do not format LittleFS automatically after a mount failure; that could erase settings and web assets.

## Configuration format

The stored struct has a `schemaVersion`. If the structure changes incompatibly, increment `CONFIG_SCHEMA_VERSION` in `src/Config.cpp` and intentionally handle migration/reset. Saver's file format version (`CONFIG_FORMAT_VERSION`) is a separate value.

## Current stage

This is a consistent project scaffold, not a finished smart-home controller. It includes working configuration persistence and initial module skeletons. Web administration, MQTT, radio frame parsing details, rules scheduler, AP captive/configuration flow and end-to-end hardware tests remain to be implemented.
