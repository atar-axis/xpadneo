## BT Dongles

Please report your Dongles and how they work [here](https://github.com/atar-axis/xpadneo/issues/93)

> **Note:**
> Product names are not reliable indicators for Bluetooth chipsets. Some vendors ship different hardware revisions
> under the same name. Always check the USB ID (`lsusb`) and chipset information.

> **Note on integrated Bluetooth adapters:**
> PCIe and on-board Bluetooth chipsets may behave very differently from USB adapters.
> Stability can vary significantly depending on chipset generation and firmware.

To identify your Bluetooth adapter:

```bash
lsusb
dmesg | grep -i bluetooth
```

Pay special attention to USB IDs and firmware versions, not product names.


## Bluetooth Low Energy

Some newer controller may work in Bluetooth low energy mode (BLE). One of
those controllers is the XBOX Series X|S controller.

If your distribution supports the command, run `btmgmt info` and look for
`le` in supported and current settings, example:
```
# btmgmt info
Index list with 1 item
hci0:   Primary controller
        addr 00:1A:7D:XX:XX:XX version 6 manufacturer 10 class 0x100104
        supported settings: powered connectable fast-connectable discoverable bondable link-security ssp br/edr hs le advertising secure-conn debug-keys privacy static-addr phy-configuration
        current settings: powered ssp br/edr le secure-conn
        name jupiter
        short name
```
If `btmgmt` command is not available, try `bluetoothctl mgmt.info` instead.


### Actions Semiconductor

* [eppfun Bluetooth 5.3 USB Adapter](https://www.amazon.co.uk/dp/B0BG5YTK9P)
  * Chip set: CM591 (reported, exact variant unclear)
  * `ID 10d7:b012 Actions general adapter`
  * Performance:
    * Stable connection
    * No disconnects reported
    * Suitable for long gaming sessions
  * Tested with:
    * Xbox Elite Series 2 controller
    * Ubuntu 24.10 (GNOME)
  * Reported by @ashleyhinton [here](https://github.com/atar-axis/xpadneo/issues/527)


### Cambridge Silicon Radio

* [Panda Bluetooth 4.0 USB Nano Adapter](https://www.amazon.com/gp/product/B00BCU4TZE/)
  * Chip set: CSR ???
  * `ID 0a12:0001 Cambridge Silicon Radio, Ltd Bluetooth Dongle (HCI mode)`
  * Performance:
    * Re-Connection Problems
    * Reliable once connected
  * Reported by @ugly95 [here](https://github.com/atar-axis/xpadneo/issues/76#issuecomment-462532230)
* [MIATONE Bluetooth Adapter Bluetooth CSR 4.0](https://www.amazon.com/gp/product/B00M1ATR4C/)
  * Chip set: CSR 8510
  * `ID 0a12:0001 Cambridge Silicon Radio, Ltd Bluetooth Dongle (HCI mode)`
  * Performance:
    * Re-Connection Problems
    * Reliable once connected
  * Reported by @ugly95 [here](https://github.com/atar-axis/xpadneo/issues/76#issuecomment-462532230)
* [CSL - Bluetooth 4.0 USB Adapter](https://www.amazon.de/dp/B01N0368AY)
  * Chip set: CSR 8510 A10
  * `ID 0a12:0001 Cambridge Silicon Radio, Ltd Bluetooth Dongle (HCI mode)`
  * Performance:
    * Re-Connection Problems
    * Initial Connection Problems
  * Reported by @germangoergs [here](https://github.com/atar-axis/xpadneo/issues/91) and [here](https://github.com/atar-axis/xpadneo/issues/93#issuecomment-480997846)
* [Sabrent USB Bluetooth 4.0 Micro Adapter for PC](https://www.amazon.com/gp/product/B06XHY5VXF/)
  * Chip set CSR ???
  * `ID 0a12:0001 Cambridge Silicon Radio, Ltd Bluetooth Dongle (HCI mode)`
  * Performance:
    * Re-Connection Problems
    * Reliable once connected
  * Reported by @ugly95 [here](https://github.com/atar-axis/xpadneo/issues/93#issuecomment-481065171)
* [Yizhet USB nano Bluetooth 4.0 Adapter](https://www.amazon.de/gp/product/B01LR8CNXU/)
  * Chip set CSR 8510 A10
  * `ID 0a12:0001 Cambridge Silicon Radio, Ltd Bluetooth Dongle (HCI mode)`
  * Performance:
    * Re-Connection Problems
    * Reliable once connected
  * Reported by @NoXPhasma [here](https://github.com/atar-axis/xpadneo/issues/91#issuecomment-484815264)
* [TP-Link USB Bluetooth Adapter Bluetooth 4.0 (UB400)](https://www.amazon.com/gp/product/B07V1SZCY6)
  * Chip set: CSR ???
  * `ID 0a12:0001 Cambridge Silicon Radio, Ltd Bluetooth Dongle (HCI mode)`
  * Performance:
    * Connection is flawless
  * Reported by @Arian8j2 [here](https://github.com/atar-axis/xpadneo/issues/389#issuecomment-1677012088)


### Broadcom

* [Plugable USB Bluetooth 4.0 Low Energy Micro Adapter](https://www.amazon.com/Plugable-Bluetooth-Adapter-Raspberry-Compatible/dp/B009ZIILLI/)
  * Chip set: BCM20702A0
  * `ID 0a5c:21e8 Broadcom Corp. BCM20702A0 Bluetooth 4.0`
  * Performance:
    * Connection flawless
    * Sometimes laggy in games
  * Reported by @ugly95 [here](https://github.com/atar-axis/xpadneo/issues/93#issuecomment-481065171) and [here](https://github.com/atar-axis/xpadneo/issues/76#issuecomment-464397584)
* [Targus BT 4.0 USB adapter](https://www.targus.com/au/acb75au)
  * Chip set: BCM20702A0
  * `ID 0a5c:21e8 Broadcom Corp. BCM20702A0 Bluetooth 4.0`
  * Performance:
    * Connection flawless
    * Sometimes laggy in games
  * Reported by @Zowayix [here](https://github.com/atar-axis/xpadneo/issues/93#issuecomment-487280791)


### Qualcomm

* Unspecified model (<https://github.com/atar-axis/xpadneo/issues/180>):
  * `btmon` logs showed very low input report rate and high input lag (300ms+)


### Intel

* Intel 7265 (NGFF / PCIe)
  * Chip set: Intel 7265
  * `ID 8087:0a2a Intel Corp. Bluetooth wireless interface`
  * Performance:
    * Rock solid pairing and connection
    * Stable with multiple controllers (5 connected simultaneously)
    * No special configuration required
  * Tested on:
    * Ubuntu 25.04
  * Reported by @teeedubb [here](https://github.com/atar-axis/xpadneo/issues/541)
* Status: incompatible (<https://github.com/atar-axis/xpadneo/issues/270>)
  * OUI: DC:1B:A1 (Intel)
  * Used as on-board chip set: Gigabyte B450 AORUS Pro WiFi 1.0 with integrated Bluetooth
* Status: bluetoothd logs "Request attribute has encountered an unlikely error"
  * Chip set: AX200
  * Used as on-board chip set: ASUS B550-i


### Realtek

Known bad firmware for RTL8761BU chip set is 0xdfc6d922. It causes frequent
reconnects. Firmware version can be found in the kernel log:
```
$ sudo dmesg | grep 'RTL: fw version'
[   21.193448] Bluetooth: hci0: RTL: fw version 0xdfc6d922
```

Several users reported that switching away from RTL8761BU-based adapters
resolved frequent disconnect issues entirely.

* Realtek RTL8852AE (NGFF / PCIe)
  * Chip set: RTL8852AE
  * Performance:
    * Frequent disconnects and reconnects
    * Unreliable pairing with some Xbox One controllers
    * Other controllers may fail to pair entirely
  * Tested on:
    * Ubuntu 25.04 (distribution-provided drivers)
  * Notes:
    * Multiple users report instability with game controllers
  * Reported by @teeedubb [here](https://github.com/atar-axis/xpadneo/issues/541)
* TP-Link USB Bluetooth Adapter (marketed as "UB400" or "UB500")
  * Multiple hardware revisions exist under the same product name
  * Some units report as:
    * `ID 2357:0604` (RTL8761BU, problematic firmware)
  * Performance:
    * Frequent disconnects with known bad firmware
  * Notes:
    * Devices sold as "UB400" may internally be UB500-class hardware
    * Always verify using `lsusb`
* [TP-Link USB Bluetooth Adapter Bluetooth 5.0 (UB500)](https://www.amazon.com/gp/product/B09DMP6T22)
  * Chip set: RTL8761BU
  * `ID 2357:0604 TP-Link TP-Link UB500 Adapter`
  * Performance:
    * Disconnects after some random interval and reconnects
    * When it's connected, it's good
  * Reported by @Arian8j2 [here](https://github.com/atar-axis/xpadneo/issues/389#issuecomment-1677012088)
* [Simplecom NB409 Bluetooth 5.0 USB Wireless Dongle with A2DP EDR](https://www.mwave.com.au/product/simplecom-nb409-bluetooth-50-usb-wireless-dongle-with-a2dp-edr-ac38550)
  * Chip set: RTL8761BU
  * `ID 0bda:8771 Realtek Semiconductor Corp. Bluetooth Radio`
  * Status: it works straight out of the box for a user on Manjaro KDE with kernel 5.15 and 6.1
  * Reported by @mscharley [here](https://github.com/atar-axis/xpadneo/issues/406)
* [UGREEN Bluetooth 5.0 USB Adapter (CM390)](https://www.amazon.com/gp/product/B08R8992YC/)
  * Chip set: RTL8761BU
  * `ID 0bda:8771 Realtek Semiconductor Corp. Bluetooth Radio`
  * Performance:
    * Connection flawless if good firmware is being used
