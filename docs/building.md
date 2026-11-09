# Hardware, assembly, and first use

## Status and supported files

| Version | Design source | Firmware | Physical evidence |
| --- | --- | --- | --- |
| ESP32 night light | `pcb/ESPLight-v1.*` | `esp32/` | Assembled, working light; about five inches tall; no battery |
| CatLightPocket | `pcb/CatLightPocket/CatLightPocket.*` | `pocket/` | Several assembled wearables; no bodge wires reported |

The builder has not confirmed that the latest files exactly match a tested physical board. Neither design is a qualified fabrication release. Before ordering, compare the files with your intended revision, review the schematic, run ERC/DRC, and regenerate fabrication and assembly outputs together. Do not infer a known-good revision from the filenames or the existence of a Gerber archive.

## Parts and assembly evidence

Historical Pocket assembly exports are local records and are not distributed in Git. The inspected records disagree: C10, D2, D3, R3, and R4 appear in the designator list but not the BOM or placement list. Several entries lack exact ordering codes. Resolve these gaps against the selected board revision before generating a shopping list or assembly order.

The reported assembly process was mostly hand soldering. One order used hybrid assembly: JLC assembled basic parts, then the builder fitted the connector, LED, and microcontroller. This is not a verified reference-by-reference assembly manifest. No separate ESP32 BOM/order export has been located in this checkout.

For a reproducible order, resolve all missing part identities, ratings, orientation, and assembly exclusions against the selected design first. Check LED pin order and package against the chosen components rather than assuming any RGB LED will fit.

## Power and batteries

The ESP32 build has no battery. Confirm its power input and regulator requirements from the selected board design before applying power; an exact external supply specification has not been recorded here.

Pocket used a small lithium battery with leads soldered directly to the board, not a fitted battery connector. The historical BOM includes a J2 battery-connector footprint, but that is not evidence it was populated. Its exported netlist names J2 pin 1 `/+BATT_CHG` and pin 2 `GND`; check the current design and physical orientation before relying on this historical mapping.

The exact battery model, capacity, chemistry/charge-voltage specification, protection arrangement, and compatible charge current are not documented. **Battery selection and charging are therefore not a verified reproduction procedure.** The historical BOM lists a TP4056 charger, but a part name and a working light do not establish charging compatibility or correct operation. Resolve these details before connecting a cell or attempting charging. Consult the cell and charger manufacturers' specifications; this guide does not certify a safe battery assembly.

One assembled unit lit successfully but would not charge. The cause was not established before it was encased in epoxy. Charging must be checked independently of LED operation; do not pot or seal an unverified assembly.

## ESP32 programming

Use the commands in the [README](../esp32/README.md#build-and-flash) with ESP-IDF v6.0.0 and target `esp32`. The exact serial adapter and board-specific programming wiring have not been recorded. Use the selected design to identify UART, ground, supply, and boot/reset connections before connecting an adapter; do not infer connector pin order from a generic ESP32 board.

After flashing, hold the mode button during boot to clear saved Wi-Fi credentials and enter setup. Connect to `cat-light` with `catlight1` and browse to `http://192.168.4.1/`. Confirm brightness, mode selection, a short trusted Lua script, saving, and cycling saved modes with the button. The portal is unauthenticated HTTP: keep it on an isolated trusted network and never expose it through port forwarding.

## Pocket programming

Initialize the pinned submodule with `git submodule update --init --recursive`. Install the toolchain and programmer dependencies from the [upstream ch32v003fun instructions](https://github.com/cnlohr/ch32v003fun). CI pins xPack RISC-V GCC 15.2.0-1, including its math library; installing only a bare compiler and headers may not provide the `libm` this firmware uses. The makefile auto-detects supported compiler prefixes; `make PREFIX=riscv-none-elf build` is an explicit example when those tools are on `PATH`.

The historical BOM/netlist identifies these programming pads. These are reference designators, not a left-to-right connector order or a confirmed map for every board revision:

| Board pad | Historical function | Programmer signal |
| --- | --- | --- |
| TP1 | +3V3 | Target supply/reference, according to programmer wiring instructions |
| TP2 | SWIO, MCU PD1 | SWIO |
| TP3 | RST, MCU PD7 | Reset if required by the programmer |
| TP4 | GND | Ground |

Locate the pad labels on the selected board and verify them against its design. Do not connect a programmer's higher-voltage output to the +3V3 pad or connect competing power sources. The exact programmer used for the assembled units has not been confirmed.

The repository includes `pocket/wch-link-rv-mode.py` for a WCH-LinkE in ARM mode. Only if using that programmer and needing the mode switch, install PyUSB and libusb as described in the script, then run it explicitly. It changes the programmer's persistent mode; it is not part of compilation and does not establish which programmer was used for the photographed units.

From `pocket/`:

```sh
make
make build-test
make flash-test
make flash
```

The first two commands only compile. `flash-test` replaces the device program with the diagnostic; `flash` restores the lighting application. Use the diagnostic to inspect the LED channels before checking the full application. A tap advances modes; each hold advances one brightness level. Release and hold again for another step. There are 24 modes and five brightness levels, with medium brightness at boot.

## Bring-up checklist

Record results for the actual physical board and the source commit used, not just a similarly named revision:

- Inspect soldering, orientation, power polarity, and shorts before applying power.
- Confirm supply and programming-pad wiring against the selected design.
- Build and flash the correct firmware; check startup and every LED channel.
- Exercise mode selection and brightness. For ESP32, check setup, script editing/saving, and behavior after restart.
- For Pocket, resolve the cell/charging specifications first, then verify battery operation and charging separately under an appropriate manufacturer-based test procedure. A lit LED is not a charging test.
- Record faults, substitutions, bodges, and measurements. Do not claim runtime or electrical/thermal qualification from build success.
- Keep the assembly accessible for diagnosis. Do not encase it in epoxy before testing the complete intended behavior, including charging.

A demonstration video is planned but not yet supplied. Existing photographs document assembled units, not completion of this checklist for the latest design.
