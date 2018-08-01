# Advanced Linux Driver for Xbox One S Wireless Gamepad

[![Build Status](https://travis-ci.org/atar-axis/xpadneo.svg?branch=master)](https://travis-ci.org/atar-axis/xpadneo)

This is a driver for the Xbox One S Wireless Gamepad which I made for a student project at fortiss GmbH.
It is fully functional but does only support the connection via Bluetooth as yet - more will follow.

Many thanks to *Kai Krakow* who **sponsored** me a Xbox One Wireless Controller :video_game: (including Wireless Adapter) and a pack of mouthwatering guarana cacao :coffee:

**Advantages of this driver**
* Supports Bluetooth
* Supports Force Feedback (Rumble) via BT
* Supports multiple Gamepads at the same time (not even supported in Windows)
* Supports [Trigger Force Feedback](https://www.youtube.com/watch?v=G4PHupKm2OQ) (not even supported in Windows)  
  see it in action: run `misc/tools/directional_rumble_test/direction_rumble_test <event# in /dev/input>`
* Offers a consistent mapping, even if paired to Windows/Xbox before
* Working Select, Start, Mode buttons
* Support for Battery Level Indication (including the Play \`n Charge Kit)  
  ![Battery Level Indication](./img/battery_support.png)
* Easy Installation
* Agile Support and Development

## Getting started
### Prerequisites
Make sure you have installed the following packages and their dependencies
* `dkms`
* `linux-headers`
* `bluez` and `bluez-utils`

On **Debian** based systems (like Ubuntu) you can install those packages by running  
``sudo apt-get install dkms linux-headers-`uname -r` ``  
On **Raspbian**, it is  
`sudo apt-get install dkms raspberrypi-kernel-headers`  
On **Arch** and similar distros (like Antergos), try  
`sudo pacman -S dkms linux-headers`

### Installation
* Download the Repository to your local machine 
  `git clone https://github.com/atar-axis/xpadneo.git`
* `cd xpadneo`
* Run `./install.sh`
* Done!

### Connection
* `sudo bluetoothctl`
* `[bluetooth]# scan on`
* push the connect button on upper side of the gamepad, and hold it down, until the light starts flashing fast
* wait for the gamepad to show up in bluetoothctl, remember the MAC address (C8:3F:26:XX:XX:XX)
* `[bluetooth]# pair <MAC>`
* `[bluetooth]# trust <MAC>`
* `[bluetooth]# connect <MAC>`

You know that everything works fine when you feel the gamepad rumble ;)

### Configuration
The driver can be reconfigured at runtime by accessing the following sysfs
files in `/sys/module/hid_xpadneo/parameters`:

* `dpad_to_buttons` (default `N`)
  * Set to `Y` to map dpad directional input to button events
  * Obsolete, will be removed in a future version
* `debug_level` (default `0`)
  * `0` (no debug output) to `3` (all)
  * For more information, please see [below](https://github.com/atar-axis/xpadneo#troubleshooting)
* `trigger_rumble_damping` (default `4`)
  * Damp the strength of the trigger force feedback
  * `1` (none) to `256` (max)
* `fake_dev_version` (default `0x1130`)
  * Fake the input device version to the given value (to prevent SDL from applying another mapping, see below)
  * Values from `1` to `0xFFFF` are handled as a version number, `0` will retain the original version


## Further information

Please abstain from merging/pushing the driver (or parts of it) to the official kernel. I will do so as soon as I consider it ready and stable enough. Thank you!

For further information, like instructions for troubleshooting, please visit the GitHub Page https://atar-axis.github.io/xpadneo/ which is generated automatically from the input of the `/docs` folder.
