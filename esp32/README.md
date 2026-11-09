# ESP32 CatLight

A programmable, externally powered night light with a browser portal and Lua lighting patterns.

[All projects](../README.md) · [Hardware and bring-up](../docs/building.md)

## Build and flash

Install [ESP-IDF v6.0.0](https://docs.espressif.com/projects/esp-idf/en/v6.0/esp32/get-started/index.html) and activate its environment, then:

```sh
cd esp32  # from the repository root
idf.py set-target esp32
idf.py build
idf.py -p /dev/your-serial-port flash monitor
```

The build packages `esp32/www/` into the LittleFS partition and includes it in flashing. The portal contains brightness and mode controls and a Lua editor: try a script in RAM before saving it to flash. The physical button cycles modes.

The SDK version matches the committed dependency lockfile and development container.

Hold the button at boot to clear saved Wi-Fi credentials and force setup mode. Connect to the `cat-light` access point using its default setup password `catlight1`, then open `http://192.168.4.1/` to configure Wi-Fi. Once connected to your LAN, open the device's address in a browser.

**The portal has no application authentication and uses unencrypted HTTP.** Anyone with network access to the device can control the lights, change configuration and Wi-Fi credentials, and upload or delete scripts. The public setup password is not access control for the portal. Use an isolated network with trusted clients; do not port-forward it, expose it to the Internet, or place it on an untrusted/shared network. Only run trusted Lua scripts; script resource exhaustion is not prevented. This firmware is not a hardened network appliance.

## Using the portal

1. Complete Wi-Fi setup as described above, then open the device’s LAN address.
2. Use the mode and brightness controls to try the built-in lighting.
3. Open **Scripts**, select a slot, and enter a name and Lua source.
4. Choose **Run in dev mode** to try it in RAM. Choose **Save to slot** to persist it; **Exit dev mode** returns to normal playback.
5. Use the board button to cycle saved modes. Holding the button during boot resets Wi-Fi setup.

The application is in `main/`; browser assets are in `www/`. Firmware design notes are in [`docs/design/`](../docs/design/).
