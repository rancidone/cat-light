## Project

Cat-shaped LED night light with custom PCB (KiCad, ESP32) and ESP-IDF C firmware. The hardware has RGB eye LEDs, 4 white whisker leds, and a button.

## Stack

- C
- ESP-IDF v5.5.3

## Where things are

- Firmware Design:
  - `./docs/design` — see `./docs/design/TEMPLATE.md` for frontmatter contract and status lifecycle
  - `./docs/discovery`
- Testing: `./docs/testing-brief.md` — read before writing any tests
- Makefile: `./CMakeLists.txt`
- Main Module: `./main/`
- Kicad PCB: `./pcb/`

## At the start of every session

Unless the user directly says otherwise you should:

1. Read the latest `handoff_*.md` file in `./docs/summaries/` for current state.
2. List all files in `./docs/summaries/` to understand what's been processed.
3. Report:
   - **Project:** name and type
   - **Last session:** what was accomplished (from the latest handoff)
   - **Next steps:** what the next session should do (from the latest handoff)
   - **Open questions:** anything unresolved