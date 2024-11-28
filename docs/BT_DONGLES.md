## BT Dongles

Please report your Dongles and how they work [here](https://github.com/atar-axis/xpadneo/issues/93)


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


### Cambridge Silicon Radio

* [Panda Bluetooth 4.0 USB Nano Adapter](https://www.amazon.com/gp/product/B00BCU4TZE/)
  * Chipset: CSR ???
  * `ID 0a12:0001 Cambridge Silicon Radio, Ltd Bluetooth Dongle (HCI mode)`
  * Performance:
    * Re-Connection Problems
    * Reliable once connected
  * Reported by @ugly95 [here](https://github.com/atar-axis/xpadneo/issues/76#issuecomment-462532230)
* [MIATONE Bluetooth Adapter Bluetooth CSR 4.0](https://www.amazon.com/gp/product/B00M1ATR4C/)
  * Chipset: CSR 8510
  * `ID 0a12:0001 Cambridge Silicon Radio, Ltd Bluetooth Dongle (HCI mode)`
  * Performance:
    * Re-Connection Problems
    * Reliable once connected
  * Reported by @ugly95 [here](https://github.com/atar-axis/xpadneo/issues/76#issuecomment-462532230)
* [CSL - Bluetooth 4.0 USB Adapter](https://www.amazon.de/dp/B01N0368AY)
  * Chipset: CSR 8510 A10
  * `ID 0a12:0001 Cambridge Silicon Radio, Ltd Bluetooth Dongle (HCI mode)`
  * Performance:
    * Re-Connection Problems
    * Initial Connection Problems
  * Reported by @germangoergs [here](https://github.com/atar-axis/xpadneo/issues/91) and [here](https://github.com/atar-axis/xpadneo/issues/93#issuecomment-480997846)
* [Sabrent USB Bluetooth 4.0 Micro Adapter for PC](https://www.amazon.com/gp/product/B06XHY5VXF/)
  * Chipset CSR ???
  * `ID 0a12:0001 Cambridge Silicon Radio, Ltd Bluetooth Dongle (HCI mode)`
  * Performance:
    * Re-Connection Problems
    * Reliable once connected
  * Reported by @ugly95 [here](https://github.com/atar-axis/xpadneo/issues/93#issuecomment-481065171)
* [Yizhet USB nano Bluetooth 4.0 Adapter](https://www.amazon.de/gp/product/B01LR8CNXU/)
  * Chipset CSR 8510 A10
  * `ID 0a12:0001 Cambridge Silicon Radio, Ltd Bluetooth Dongle (HCI mode)`
  * Performance:
    * Re-Connection Problems
    * Reliable once connected
  * Reported by @NoXPhasma [here](https://github.com/atar-axis/xpadneo/issues/91#issuecomment-484815264)
* [TP-Link USB Bluetooth Adapter Bluetooth 4.0 (UB400)](https://www.amazon.com/gp/product/B07V1SZCY6)
  * Chipset: CSR ???
  * `ID 0a12:0001 Cambridge Silicon Radio, Ltd Bluetooth Dongle (HCI mode)`
  * Performance:
    * Connection is flawless
  * Reported by @Arian8j2 [here](https://github.com/atar-axis/xpadneo/issues/389#issuecomment-1677012088)


### Broadcom

* [Plugable USB Bluetooth 4.0 Low Energy Micro Adapter](https://www.amazon.com/Plugable-Bluetooth-Adapter-Raspberry-Compatible/dp/B009ZIILLI/)
  * Chipset: BCM20702A0
  * `ID 0a5c:21e8 Broadcom Corp. BCM20702A0 Bluetooth 4.0`
  * Performance:
    * Connection flawless
    * Sometimes laggy while Gameplay
  * Reported by @ugly95 [here](https://github.com/atar-axis/xpadneo/issues/93#issuecomment-481065171) and [here](https://github.com/atar-axis/xpadneo/issues/76#issuecomment-464397584)
* [Targus BT 4.0 USB adapter](https://www.targus.com/au/acb75au)
  * Chipset: BCM20702A0
  * `ID 0a5c:21e8 Broadcom Corp. BCM20702A0 Bluetooth 4.0`
  * Performance:
    * Connection flawless
    * Sometimes laggy while Gameplay
  * Reported by Zowayix [here](https://github.com/atar-axis/xpadneo/issues/93#issuecomment-487280791)


### Qualcomm

* Unspecified model (https://github.com/atar-axis/xpadneo/issues/180):
  * `btmon` logs showed very low input report rate and high input lag (300ms+)


### Intel

* Status: incompatible (https://github.com/atar-axis/xpadneo/issues/270)
  * OUI: DC:1B:A1 (Intel)
  * Used as on-board chipset: Gigabyte B450 Aorus Pro WiFi 1.0 with integrated Bluetooth
* Status: bluetoothd logs "Request attribute has encountered an unlikely error"
  * Chipset: AX200
  * Used as on-board chipset: ASUS B550-i


### Realtek

Known bad firmware for RTL8761BU chipset is 0xdfc6d922. It causes frequent
reconnects. Firmware version can be found in the kernel log:
```
$ sudo dmesg | grep 'RTL: fw version'
[   21.193448] Bluetooth: hci0: RTL: fw version 0xdfc6d922
```

* [TP-Link USB Bluetooth Adapter Bluetooth 5.0 (UB500)](https://www.amazon.com/gp/product/B09DMP6T22)
  * Chipset: RTL8761BU
  * `ID 2357:0604 TP-Link TP-Link UB500 Adapter`
  * Performance:
    * Disconnects after some random interval and reconnects
    * When it's connected, it's good
  * Reported by @Arian8j2 [here](https://github.com/atar-axis/xpadneo/issues/389#issuecomment-1677012088)
* [Simplecom NB409 Bluetooth 5.0 USB Wireless Dongle with A2DP EDR](https://www.mwave.com.au/product/simplecom-nb409-bluetooth-50-usb-wireless-dongle-with-a2dp-edr-ac38550)
  * Chipset: RTL8761BU
  * `ID 0bda:8771 Realtek Semiconductor Corp. Bluetooth Radio`
  * Status: it works straight out of the box for a user on Manjaro KDE with kernel 5.15 and 6.1
  * Reported by @mscharley [here](https://github.com/atar-axis/xpadneo/issues/406)
* [UGREEN Bluetooth 5.0 USB Adapter (CM390)](https://www.amazon.com/gp/product/B08R8992YC/)
  * Chipset: RTL8761BU
  * `ID 0bda:8771 Realtek Semiconductor Corp. Bluetooth Radio`
  * Performance:
    * Connection flawless if good firmware is being used
