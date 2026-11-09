## Project

Cat-shaped LED night light with custom PCB (KiCad, ESP32) and ESP-IDF C firmware. The hardware has RGB eye LEDs, 4 white whisker leds, and a button.

## Stack

- C
- ESP-IDF v6.0.0 (matches `esp32/dependencies.lock`)

## Where things are

- Firmware Design:
  - `./docs/design` — see `./docs/design/TEMPLATE.md` for frontmatter contract and status lifecycle
  - `./docs/discovery`
- Testing: `./docs/testing-brief.md` — read before writing any tests
- Build entry points: `./esp32/CMakeLists.txt`, `./pocket/Makefile`
- Main Module: `./esp32/main/`
- Kicad PCB: `./pcb/`
