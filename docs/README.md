[![ko-fi](https://www.ko-fi.com/img/donate_sm.png)](https://ko-fi.com/R6R5P6FN)  
If you want to support me or accelerate the development of a special feature, consider a small donation :heart:  
Just leave a message if your donation is for a specific use (like a new hardware or a specific function).


# Advanced Linux Driver for Xbox One Wireless Gamepad

[![Build Status](https://dev.azure.com/dollingerflorian/dollingerflorian/_apis/build/status/atar-axis.xpadneo?branchName=master)](https://dev.azure.com/dollingerflorian/dollingerflorian/_build/latest?definitionId=1?branchName=master)
[![Codacy Badge](https://api.codacy.com/project/badge/Grade/7bba9bae5e6e493189969dd2a80ac09e)](https://www.codacy.com/app/atar-axis/xpadneo?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=atar-axis/xpadneo&amp;utm_campaign=Badge_Grade)
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
  If you recently updated your firmware using `rpi-update` the above package may not yet include the header files for your kernel. Please follow the steps described [here](https://github.com/notro/rpi-source/wiki) in this case.
  
Please feel free to add other Distributions as well!

### Installation
* Download the Repository to your local machine 
  `git clone https://github.com/atar-axis/xpadneo.git`
* `cd xpadneo`
* Run `sudo ./install.sh`
* Done!

### Connection
* `sudo bluetoothctl`
* `[bluetooth]# scan on`
* wait until all available devices are listed (otherwise it may be hard to identify which one is the gamepad)
* push the connect button on upper side of the gamepad, and hold it down until the light starts flashing fast
* wait for the gamepad to show up in bluetoothctl, remember the <MAC> address (e.g. `C8:3F:26:XX:XX:XX`)
* `[bluetooth]# pair <MAC>`
* `[bluetooth]# trust <MAC>`
* `[bluetooth]# connect <MAC>`

You know that everything works fine when you feel the gamepad rumble ;)

### Configuration

* Use `sudo ./configure.sh` to configure the driver as you wish. The script will guide you through the available options.

### Update
In order to update xpadneo, do the following
* Update your cloned repo: `git pull`
* Run `sudo ./update.sh`

### Uninstallation
* Run `sudo ./uninstall.sh` to remove all installed versions of hid-xpadneo


## Further information
For further information please visit the GitHub Page https://atar-axis.github.io/xpadneo/ which is generated automatically from the content of the `/docs` folder.

You will find there e.g. the following sections
* [Troubleshooting](https://atar-axis.github.io/xpadneo/#troubleshooting)
* [Debugging](https://atar-axis.github.io/xpadneo/#debugging)
* [Compatible BT Dongles](https://atar-axis.github.io/xpadneo/#bt-dongles)
