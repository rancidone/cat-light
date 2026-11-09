# CatLight

A cat-shaped circuit board with RGB eyes and white LED whiskers. It started as an ESP32 night light, then became **CatLightPocket**, a small battery-powered light I can pin on and wear.

[Photos, video, and the build story](https://mattmadeitwork.com/projects/catlight/)

## CatLightPocket

Pocket uses a CH32V003 with C firmware built on ch32v003fun. It has 24 lighting patterns, including heartbeat, candle, rainbow, and lightning. Tap the button to change patterns; hold it to step through five brightness levels.

- [Firmware, build, and flashing instructions](pocket/README.md)
- [Board designs](pcb/CatLightPocket/)

## Original CatLight

The original is an ESP32 night light, about five inches tall. It serves a browser interface for brightness, lighting modes, and editing Lua patterns. Scripts can run in RAM while experimenting, then be saved to the light.

The ESP32 firmware is retired and has known bugs. Its code and design notes are here for anyone interested in the original build.

- [ESP32 firmware and setup](esp32/README.md)
- Board designs: `pcb/ESPLight-v1.*`

## Build your own

```sh
git clone https://github.com/rancidone/cat-light.git
cd cat-light
```

Follow the guide for your version above. The [hardware guide](docs/building.md) covers assembly, programming connections, and details still needing confirmation before reproducing the boards.

## License

See [LICENSE](LICENSE). ch32v003fun has its own license.
