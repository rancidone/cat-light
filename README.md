# CatLight

CatLight is a cat-shaped circuit board with RGB eyes and white LED whiskers. It started as a programmable night light and became **CatLightPocket**, a small wearable light with 24 animated patterns and one-button control.

Pocket is the current focus: tap to change the pattern, or hold to step through five brightness levels. It uses a CH32V003 microcontroller, runs without Wi-Fi, and starts in heartbeat mode at medium brightness. Both versions have been assembled and used.

Read the [build story and see assembled-board photographs](https://mattmadeitwork.com/projects/catlight/), or start with the [Pocket guide](pocket/README.md).

## Two versions

| | CatLightPocket | Original ESP32 CatLight |
| --- | --- | --- |
| Status | Current project | Retired; known bugs |
| Form | Small battery-powered wearable | Externally powered night light, about five inches tall |
| Lighting | 24 built-in patterns | Built-in modes and editable Lua scripts |
| Controls | One button; five brightness levels | Button and device-hosted browser portal |
| Firmware | C with ch32v003fun | C with ESP-IDF |
| Guide | [Build and use Pocket](pocket/README.md) | [Retired ESP32 firmware](esp32/README.md) |
| Board source | `pcb/CatLightPocket/` | `pcb/ESPLight-v1.*` |

**The ESP32 firmware is retired with known bugs. Fixes will be considered on demand; there is no ongoing development or scheduled maintenance.** Its source, browser assets, and design notes remain available as a record of the original build.

## Build your own

This is a personal hardware project shared for experienced builders. Before assembling either version, read the [hardware and bring-up guide](docs/building.md). It covers programming connections, assembly evidence, and the remaining gaps in parts and power specifications.

Pocket's exact battery and charging specifications, and the correspondence between the current board files and tested units, still need confirmation. The repository is not yet an order-ready hardware release. Build success and photographs do not establish battery runtime, electrical safety, or manufacturing readiness.

Clone with the pinned Pocket dependency:

```sh
git clone --recurse-submodules https://github.com/rancidone/cat-light.git
cd cat-light
```

For an existing checkout, run `git submodule update --init --recursive`. Follow the [Pocket guide](pocket/README.md) for the toolchain, building, and flashing.

## What's in the repository

- [`pocket/`](pocket/README.md): wearable firmware, board diagnostic, and pinned ch32v003fun dependency.
- `pcb/CatLightPocket/`: Pocket KiCad design source.
- [`esp32/`](esp32/README.md): retired ESP-IDF firmware, browser portal, and development container.
- `pcb/ESPLight-v1.*`: original ESP32 KiCad design source.
- [`docs/building.md`](docs/building.md): hardware status, assembly, programming, and physical bring-up.
- [`docs/design/`](docs/design/README.md) and [`docs/discovery/`](docs/discovery/v2-firmware-discovery.md): historical ESP32 design material.
- [`TODO.md`](TODO.md): remaining release documentation work.

Fabrication and assembly exports are generated locally and excluded from Git. Select and check a board revision before generating files for an order.

## Validation and license

[Firmware builds](.github/workflows/firmware.yml) compile both Pocket programs and retain a compilation check for the retired ESP32 application. Pocket also has a [host timer regression check](docs/testing-brief.md). These checks do not replace testing on the actual assembly; follow the [bring-up checklist](docs/building.md#bring-up-checklist).

See [LICENSE](LICENSE). The ch32v003fun submodule retains its own license.
