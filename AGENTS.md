## Project

Cat-shaped LED boards: the current CH32V003 CatLightPocket wearable and the original ESP32 night light. Both have RGB eyes, white LED whiskers, and a button.

## Status and stack

- Pocket: C with separately cloned ch32v003fun; current development focus.
- ESP32: C with ESP-IDF v6.0.0 (matches `esp32/dependencies.lock`); retired with known bugs. Fixes considered on demand. Do not treat historical design drafts as an active implementation plan.

## Where things are

- Pocket firmware: `./pocket/`; build entry point `./pocket/Makefile`.
- Retired ESP32 firmware: `./esp32/main/`; build entry point `./esp32/CMakeLists.txt`; assets `./esp32/www/`.
- Historical ESP32 design and discovery: `./docs/design/`, `./docs/discovery/`.
- Testing: read `./docs/testing-brief.md` before writing tests.
- Hardware and bring-up: `./docs/building.md`.
- KiCad boards: `./pcb/`, `./pcb/CatLightPocket/`.
