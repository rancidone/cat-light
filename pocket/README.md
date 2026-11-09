# CatLightPocket

A CH32V003 wearable with RGB eyes, white whiskers, and one button. It has no Wi-Fi or scripting.

[All projects](../README.md) · [Hardware and bring-up](../docs/building.md)

## Build and flash

Install the RISC-V toolchain and programmer prerequisites described by [ch32v003fun](https://github.com/cnlohr/ch32v003fun). Connect a compatible programmer to the board's programming pads, using the board design to confirm pinout and supply voltage.

```sh
cd pocket
make
make build-test
```

`make` (or `make build`) only builds the lighting application; `make build-test` only builds the board diagnostic. Neither requires a connected programmer. After checking the [programming-pad map](../docs/building.md#pocket-programming), use `make flash` to build and flash the lighting application, or `make flash-test` to build and flash the diagnostic. Flashing replaces the program on the connected board.

Tap the button to advance through 24 modes, including heartbeat, candle, rainbow, aurora, and lightning. Hold it to advance one of five brightness levels; release and hold again for the next level. Brightness starts at the medium level on boot.

## Timer regression check

Run `make test-host` from `pocket/` with a host C compiler. It checks elapsed-time behavior across timer rollovers without a board or RISC-V toolchain. The firmware samples the hardware counter continuously and accumulates tick differences before converting to milliseconds.

## Using Pocket

- At startup, the application selects heartbeat mode and medium brightness.
- Tap the button to advance to the next mode.
- Hold for about 400 ms to advance one brightness level. Release before holding again. Levels wrap around after the brightest setting.
- Mode and brightness selections reset when the device restarts.

`cat-light-pocket.c` handles the button and main loop, `modes.c` defines the patterns, and `light_engine.c` drives the LEDs. `board-test.c` is a separate diagnostic program; flash the main application again after diagnostics.

The exact battery and charge specifications remain unresolved. Read the hardware guide before connecting a cell or attempting charging.
