# Advanced Linux Driver for Xbox One Wireless Gamepad

[![Build Status](https://travis-ci.org/atar-axis/xpadneo.svg?branch=master)](https://travis-ci.org/atar-axis/xpadneo)
[![Average time to resolve an issue](http://isitmaintained.com/badge/resolution/atar-axis/xpadneo.svg)](http://isitmaintained.com/project/atar-axis/xpadneo "Average time to resolve an issue")

This is the first and yet only driver for the Xbox One Wireless Gamepad (which is shipped with the Xbox One S). I wrote it for a student project at fortiss GmbH and it is fully functional but does only support the connection via Bluetooth as yet - more will follow.

Many thanks to *Kai Krakow* who **sponsored** me a Xbox One Wireless Controller :video_game: (including Wireless Adapter) and a pack of mouthwatering guarana cacao :coffee:

**Advantages of this driver**
* Supports Bluetooth
* Supports Force Feedback (Rumble) in General
* Supports [Trigger Force Feedback](https://www.youtube.com/watch?v=G4PHupKm2OQ) (not even supported in Windows)  
  see it in action: run `misc/tools/directional_rumble_test/direction_rumble_test <event# in /dev/input>`
* Supports disabling FF
* Supports multiple Gamepads at the same time (not even supported in Windows)
* Offers a consistent mapping, even if the Gamepad was paired to Windows/Xbox before
* Working Select, Start, Mode buttons
* Correct Axis Range (signed, important for e.g. RPCS3)
* Supports Battery Level Indication (including the Play \`n Charge Kit)  
  ![Battery Level Indication](./img/battery_support.png)
* Supports faking the Input Device Version in order to prevent SDL from trying to fix an unbroken mapping.
* Easy Installation
* Agile Support and Development

## Getting started
### Prerequisites
Make sure you have installed *dkms*, *linux headers* and a bluetooth implementation (e.g. *bluez*) and their dependencies.

* On **Arch** and Arch-based distros (like **Antergos**), try  
  `sudo pacman -S dkms linux-headers bluez bluez-utils`  
* On **Debian** based systems (like Ubuntu) you can install those packages by running  
  ``sudo apt-get install dkms linux-headers-`uname -r` ``  
* On **Fedora**, it is  
  ``sudo dnf install dkms bluez bluez-tools kernel-devel-`uname -r` kernel-headers-`uname -r` `` 
* On **OSMC** you will have to run the following commands  
  ``sudo apt-get install dkms rbp2-headers-`uname -r` ``  
  ``sudo ln -s "/usr/src/rbp2-headers-`uname -r`" "/lib/modules/`uname -r`/build"`` (as a [workaround](https://github.com/osmc/osmc/issues/471))
* On **Raspbian**, it is  
  `sudo apt-get install dkms raspberrypi-kernel-headers`  
  If you recently updated your firmware using `rpi-update` the above package may not yet include the header files for your kernel.  
  Please follow the steps described at [rpi-src](https://github.com/notro/rpi-source/wiki) to get the headers for your currently running kernel.  
  
Please feel free to add other Distributions as well!

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

* `debug_level` (default `0`)
  * `0` (no debug output) to `3` (all)
  * For more information, please take a look [here](https://atar-axis.github.io/xpadneo/#debugging)
* `disable_ff` (default `0`)
  * `0` (ff enabled) to `1` (ff disabled)
* `trigger_rumble_damping` (default `4`)
  * Damp the strength of the trigger force feedback
  * `1` (none) to `256` (max)
* `fake_dev_version` (default `0x1130`)
  * Fake the input device version to the given value (to prevent SDL from applying another mapping, see below)
  * Values from `1` to `0xFFFF` are handled as a version number, `0` will retain the original version

### Update
In order to update xpadneo, do the following
* Update your cloned directory by running `git pull`
* Run `./update.sh`

### Uninstallation
* Run `./uninstall.sh` to remove all installed versions of hid-xpadneo

## Further information

If someone is interested in helping me getting this driver merged into the kernel, tell me. I would really appreciate that.

For further information, like instructions for troubleshooting, please visit the GitHub Page https://atar-axis.github.io/xpadneo/ which is generated automatically from the input of the `/docs` folder.
