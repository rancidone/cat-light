# Remaining release work

CatLightPocket is the current focus. Before calling the hardware a reproducible release:

- Identify a tested source commit and the corresponding assembled board revision.
- Complete the parts list, exact ordering codes, ratings, and assembly exclusions.
- Record Pocket battery, protection, and charging specifications and verify charging independently of LED operation.
- Confirm programming equipment, pad orientation, and supply wiring on the selected revision.
- Record physical bring-up results using [the checklist](docs/building.md#bring-up-checklist).

ESP32 firmware is retired with known bugs. Fixes will be considered on demand; its historical design drafts are not an active backlog. See [known issues](esp32/README.md).
