# Building CatLight

| Version | Board files | Firmware instructions |
| --- | --- | --- |
| CatLightPocket wearable | `pcb/CatLightPocket/` | [Pocket](../pocket/README.md) |
| Original ESP32 night light | `pcb/ESPLight-v1.*` | [ESP32](../esp32/README.md) |

[Photos, video, and the build story](https://mattmadeitwork.com/projects/catlight/)

## Assembly and power

Most boards were hand-soldered. For one Pocket batch, JLC assembled the basic parts; I added the connector, LED, and microcontroller.

Pocket uses a small lithium battery with leads soldered directly to the board. The exact cell and charging specifications still need documenting. Check the cell, charger settings, polarity, and LED pinout against the schematic and component datasheets before assembling your own. The original ESP32 version uses external power and has no battery.

## Pocket programming

Clone [ch32v003fun](https://github.com/cnlohr/ch32v003fun) into `pocket/ch32v003fun` and follow its toolchain and programmer setup instructions. The firmware needs a RISC-V toolchain with `libm`; CI uses xPack RISC-V GCC 15.2.0-1.

The programming pads are along the tail. Check the labels against your board revision; this table is not their physical order.

| Pad | Signal |
| --- | --- |
| TP1 | +3V3 |
| TP2 | SWIO (PD1) |
| TP3 | Reset (PD7) |
| TP4 | Ground |

Use a compatible programmer at 3.3 V and avoid connecting competing power sources. If your WCH-LinkE is in ARM mode, `pocket/wch-link-rv-mode.py` switches it to RISC-V mode; see the script for dependencies.

From `pocket/`:

```sh
make             # Build the lighting application
make build-test  # Build the LED diagnostic
make flash-test  # Flash the diagnostic
make flash       # Flash the lighting application
```

Tap the button to change patterns. Hold to advance one brightness level, then release before holding again.

## ESP32 programming

Follow the [ESP32 build and flash instructions](../esp32/README.md#build-and-flash). This firmware is retired and has known bugs.

Hold the mode button during boot to reset Wi-Fi setup. Connect to `cat-light` with password `catlight1`, then open `http://192.168.4.1/`. Once connected to your network, open `http://<device-ip>/app.html`; the root redirect and `cat-light.local` link are broken. The web interface has no authentication, so use it only on a trusted network.

## Bring-up checklist

- Check soldering, component orientation, polarity, and shorts.
- Flash the diagnostic and check each LED channel, then flash the application.
- Check the button, patterns, and brightness. On ESP32, also try editing and saving a Lua pattern.
- Test battery operation and charging separately before sealing the board.

One Pocket lit up but would not charge—and I only found out after encasing it in epoxy. Test first, epoxy later.
