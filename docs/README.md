# Advanced Linux Driver for Xbox One S Wireless Gamepad [![Build Status](https://travis-ci.org/atar-axis/xpadneo.svg?branch=master)](https://travis-ci.org/atar-axis/xpadneo)
This is a driver for the Xbox One S Wireless Gamepad which I created for a student research project at fortiss GmbH.
It is fully functional but does only support the connection via Bluetooth as yet - more will follow.

Many thanks to *Kai Krakow* who **sponsored** me a Xbox One Wireless Controller (including Wireless Adapter) and a pack of mouthwatering guarana cacao ;D

**Advantages of this driver**
* Supports Bluetooth <i class="fab fa-bluetooth"></i>
* Supports Force Feedback over Bluetooth
* Supports multiple Gamepads over Bluetooth (not even supported in Windows)
* Supports [Trigger Force Feedback](https://www.youtube.com/watch?v=G4PHupKm2OQ) (not even supported in Windows)  
  see it in action! run `misc/tools/directional_rumble_test/direction_rumble_test <event# in /dev/input>`
* Offers a consistent mapping, even if paired to Windows/Xbox before
* Working Select, Start, Mode buttons
* Support for Battery Level Indication (including Play `n Charge Kit)  
  ![Battery Level Indication](./img/battery_support.png)
* Agile Development

## Getting started
### Prerequisites
Make sure you have installed the following packages and their dependencies
* dkms
* linux-headers

On debian based systems (like Ubuntu or Raspbian) you can install those packages by running  
`sudo apt-get install dkms linux-headers`

### Installation
* Download the Repository to your local machine 
  `git clone https://github.com/atar-axis/xpadneo.git`
* Run the `install.sh` script
* Done!

If something wents wrong you can always install it by hand like so:
* Let's say you have downloaded the driver into your home directory
* Read out your version: `cat ~/xpadneo/VERSION`
* Copy the `hid-xpadneo` folder into the `/usr/src/` directory (and append the version)  
  `sudo cp -r ~/xpadneo/hid-xpadneo/ /usr/src/hid-xpadneo-<version>`
* Add the driver to DKMS and install it  
  `sudo dkms install hid-xpadneo -v <version>`

### Connection
* `sudo bluetoothctl`
* `scan on`
* push the connect button on upper side of the gamepad until the light starts flashing fast
* wait for the gamepad to show up 
* `pair <MAC>`
* `trust <MAC>`
* `connect <MAC>`

You know that everything works fine when you feel the gamepad rumbling ;)


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

For further information, like instructions for troubleshooting, please visit the GitHub Page https://atar-axis.github.io/xpadneo/ which is generated automatically from the input of the `/docs` folder.
