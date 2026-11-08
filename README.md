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
| Source | `esp32/` | `pocket/` |
| Hardware | `pcb/ESPLight-v1.*` | `pcb/CatLightPocket/` |

The [build story and assembled-board photographs](https://mattmadeitwork.com/projects/catlight/) explain the transition from night light to wearable.

## Get the source

```sh
git clone --recurse-submodules https://github.com/rancidone/cat-light.git
cd cat-light
```

For an existing checkout, run `git submodule update --init --recursive`. Pocket uses the pinned `pocket/ch32v003fun` submodule.

## ESP32: build and flash

Install ESP-IDF v5.5.3 and activate its environment, then:

```sh
cd esp32
idf.py build
idf.py -p /dev/your-serial-port flash monitor
```

The build packages `esp32/www/` into the LittleFS partition and includes it in flashing. The portal contains brightness and mode controls and a Lua editor: try a script in RAM before saving it to flash. The physical button cycles modes.

Hold the button at boot to clear saved Wi-Fi credentials and force setup mode. Connect to the `cat-light` access point using its default setup password `catlight1`, then open `http://192.168.4.1/` to configure Wi-Fi. Once connected to your LAN, open the device's address in a browser. The setup password is public; use this hobby firmware only on a trusted network.

## Pocket: build and flash

Install the RISC-V toolchain and programmer prerequisites described by [ch32v003fun](https://github.com/cnlohr/ch32v003fun). Connect a compatible programmer to the board's programming pads, using the board design to confirm pinout and supply voltage.

```sh
cd pocket
make
```

The default Makefile target builds **and flashes** the connected board. `make flash-test` flashes the separate board-test program instead of the lighting application.

Tap the button to advance through 24 modes, including heartbeat, candle, rainbow, aurora, and lightning. Hold it to advance one of five brightness levels; release and hold again for the next level. Brightness starts at the medium level on boot.

## Evidence and limitations

- `esp32/main/` contains the light engine, Lua interpreter integration, mode manager, persistence, and HTTP server.
- `pocket/modes.c` and `pocket/cat-light-pocket.c` define the wearable's modes and button behavior.
- `pcb/` contains the board designs. Confirm the revision against your physical board before building or ordering anything.
- Treat KiCad source as the design reference. Existing fabrication archives are historical exports, not a verified release of the current design; rerun checks and regenerate outputs before ordering boards.
- `docs/design/` records the original ESP32 firmware design; older documents may use paths from before the `esp32/` reorganization.
- `pocket/board-test.c` is a hardware diagnostic, not an automated qualification suite. Build success is not a substitute for checking a physical board.

See `LICENSE` for the repository license. The ch32v003fun submodule retains its own license.
