#!/usr/bin/env python3
"""
Switch WCH-LinkE from ARM mode to RISC-V mode persistently.

The LinkE enumerates as:
  RISC-V mode  VID=0x1a86  PID=0x8010
  ARM mode     VID=0x1a86  PID=0x8012

Run once before flashing. The device will re-enumerate in RISC-V mode
and stay there across USB reconnects.

Requires: pip install pyusb
macOS:    brew install libusb
"""

import sys
import usb.core
import usb.util
import time

WCH_VID     = 0x1a86
PID_RV      = 0x8010
PID_ARM     = 0x8012

EP_OUT      = 0x02   # bulk out endpoint in ARM mode
EP_IN       = 0x83   # bulk in endpoint in ARM mode
CMD_SWITCH  = bytes([0x81, 0xff, 0x01, 0x52])

def find_device(pid):
    return usb.core.find(idVendor=WCH_VID, idProduct=pid)

def main():
    if find_device(PID_RV):
        print("WCH-LinkE is already in RISC-V mode — nothing to do.")
        sys.exit(0)

    dev = find_device(PID_ARM)
    if dev is None:
        print("Error: WCH-LinkE not found in either ARM or RISC-V mode.")
        print("Check the USB connection and try again.")
        sys.exit(1)

    print("Found WCH-LinkE in ARM mode. Switching to RISC-V mode...")

    if dev.is_kernel_driver_active(0):
        dev.detach_kernel_driver(0)

    dev.set_configuration()
    usb.util.claim_interface(dev, 0)
    dev.write(EP_OUT, CMD_SWITCH)

    # Device may disconnect immediately to re-enumerate — ignore read errors
    try:
        dev.read(EP_IN, 64, timeout=500)
    except (usb.core.USBTimeoutError, usb.core.USBError):
        pass

    print("Command sent. Waiting for device to re-enumerate...")
    time.sleep(1.5)

    if find_device(PID_RV):
        print("Success — WCH-LinkE is now in RISC-V mode.")
    else:
        print("Device did not re-enumerate as RISC-V. Unplug and replug the LinkE, then run again.")
        sys.exit(1)

if __name__ == "__main__":
    main()
