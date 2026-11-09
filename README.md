# CatLight

A cat-shaped circuit board with RGB eyes and white LED whiskers, built in two forms: a programmable ESP32 night light and a smaller CH32V003 wearable called CatLightPocket.

Both versions have been assembled and used. This is a personal hardware project, not a production-qualified product. Board files and firmware document the builds; they do not establish battery runtime, electrical safety, or manufacturing readiness.

## Choose a version

| | ESP32 CatLight | CatLightPocket |
| --- | --- | --- |
| Purpose | Experiment with lighting scripts | Pin on and wear |
| Firmware | C with ESP-IDF | C with ch32v003fun |
| Interface | Device-hosted browser portal and button | One button; no Wi-Fi or scripting |
| Lighting | Built-in modes and editable Lua scripts | 24 built-in modes |
| Build and use | [ESP32 guide](esp32/README.md) | [Pocket guide](pocket/README.md) |
| Hardware | `pcb/ESPLight-v1.*` | `pcb/CatLightPocket/` |

The [build story and assembled-board photographs](https://mattmadeitwork.com/projects/catlight/) explain the transition from night light to wearable. The original is about five inches tall and has no battery; Pocket uses a low-cost CH32V003 and a small lithium battery soldered directly to the board.

Before assembling either version, read the [hardware and bring-up guide](docs/building.md). The exact battery specification and the correspondence between current PCB files and tested units are not yet established. This is source for experienced builders, not an order-ready hardware release.

## Get the source

```sh
git clone --recurse-submodules https://github.com/rancidone/cat-light.git
cd cat-light
```

For an existing checkout, run `git submodule update --init --recursive`. Pocket uses the pinned `pocket/ch32v003fun` submodule.

## Repository map

- [`esp32/`](esp32/README.md): ESP-IDF firmware, browser portal, and development container.
- [`pocket/`](pocket/README.md): CH32V003 application, board diagnostic, and pinned ch32v003fun dependency.
- `pcb/ESPLight-v1.*`: ESP32 KiCad design source.
- `pcb/CatLightPocket/`: Pocket KiCad design source.
- [`docs/building.md`](docs/building.md): hardware limitations, programming connections, and physical bring-up.
- [`docs/design/`](docs/design/): ESP32 design notes; some older notes predate the folder separation.

Fabrication and assembly exports are generated locally and excluded from Git. Select and check a board revision before generating files for an order.

## Validation and project status

[Firmware builds](.github/workflows/firmware.yml) compile the ESP32 application and both Pocket programs without hardware. Compilation does not verify LED operation, charging, runtime, or board safety. Follow the [physical bring-up checklist](docs/building.md#bring-up-checklist) on the actual assembly.

The source is shared for experienced builders. A tested source-to-board revision, complete parts specifications, and battery/charging details still need confirmation before this becomes a reproducible hardware release.

See [LICENSE](LICENSE). The ch32v003fun submodule retains its own license.
