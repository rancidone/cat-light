## Project

Cat-shaped LED night light with custom PCB (KiCad, ESP32 main board + CH32V003 pocket edition). The hardware has RGB eye LEDs, 4 white whisker LEDs, and a button.

## Stack

- C
- ESP-IDF v5.5.3 (ESP32 firmware)
- ch32v003fun, bare-metal (CH32V003 pocket firmware)

## Where things are

- ESP32 firmware: `./esp32/` — ESP-IDF project, run `idf.py` from this directory
  - Main module: `./esp32/main/`
  - Web UI assets: `./esp32/www/`
  - Makefile: `./esp32/CMakeLists.txt`
- CH32V003 pocket firmware: `./pocket/` — ch32v003fun project, run `make` from this directory
- Firmware Design:
  - `./docs/design` — see `./docs/design/TEMPLATE.md` for frontmatter contract and status lifecycle
  - `./docs/discovery`
- Testing: `./docs/testing-brief.md` — read before writing any tests
- KiCad PCB: `./pcb/` (main board), `./pcb/CatLightPocket/` (pocket edition)
