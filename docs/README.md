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
* Supports all Force Feedback/Rumble effects through Linux `ff-memless` effect emulation
* Supports [Trigger Force Feedback](https://www.youtube.com/watch?v=G4PHupKm2OQ)
  in every game by applying a pressure-dependent effect intensity to the current rumble effect (not even supported in Windows)
* Supports disabling FF
* Supports multiple Gamepads at the same time (not even supported in Windows)
* Offers a consistent mapping, even if the Gamepad was paired to Windows/Xbox before, and independent of software layers (SDL2, Stadia via Chrome Gamepad API, etc)
* Working Select, Start, Mode buttons
* Supports Battery Level Indication (including the Play \`n Charge Kit)
  ![Battery Level Indication](./img/battery_support.png)
* Easy Installation
* Agile Support and Development

### Xbox Elite Series 2 Wireless controller

While basic support for the Xbox Elite Series 2 Wireless controller is present, the following features are missing:
- Profile support. All four profiles behave the same way currently, and there is no support for configuring them.
- The four paddles at the bottom are currently not supported.

## Getting started
### Prerequisites
Make sure you have installed *dkms*, *linux headers* and a bluetooth implementation (e.g. *bluez*) and their dependencies.

* On **Arch** and Arch-based distros (like **Antergos**), try  
  `sudo pacman -S dkms linux-headers bluez bluez-utils`  
* On **Debian** based systems (like Ubuntu) you can install those packages by running  
  ``sudo apt-get install dkms linux-headers-`uname -r` ``  
* On **Fedora**, it is  
  ``sudo dnf install dkms make bluez bluez-tools kernel-devel-`uname -r` kernel-headers-`uname -r` `` 
* On **Manjaro** try
  `sudo pacman -S dkms linux-latest-headers bluez bluez-utils`
* On **OSMC** you will have to run the following commands  
  ``sudo apt-get install dkms rbp2-headers-`uname -r` ``  
  ``sudo ln -s "/usr/src/rbp2-headers-`uname -r`" "/lib/modules/`uname -r`/build"`` (as a [workaround](https://github.com/osmc/osmc/issues/471))
* On **Raspbian**, it is  
  `sudo apt-get install dkms raspberrypi-kernel-headers`  
  If you recently updated your firmware using `rpi-update` the above package may not yet include the header files for your kernel. Please follow the steps described [here](https://github.com/notro/rpi-source/wiki) in this case.
* On **generic distributions**, it doesn't need DKMS but requires a configured kernel source tree, then:
  `cd hid-xpadneo && make modules && sudo make modules_install`
  
Please feel free to add other Distributions as well!

### Installation
* Download the Repository to your local machine 
  `git clone https://github.com/atar-axis/xpadneo.git`
* `cd xpadneo`
* If using DKMS, run `sudo ./install.sh`
* If not using DKMS, follow steps above (generic distribution)
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
* The `<MAC>` parameter is optional if the command line already shows the controller name

You know that everything works fine when you feel the gamepad rumble ;)

### Configuration

* If using DKMS: Use `sudo ./configure.sh` to configure the driver as you wish. The script will guide you through the available options.

### Update
In order to update xpadneo, do the following
* Update your cloned repo: `git pull`
* If using DKMS: Run `sudo ./update.sh`
* otherwise follow the steps above (generic distribution)

### Uninstallation
* If using DKSM: Run `sudo ./uninstall.sh` to remove all installed versions of hid-xpadneo
* otherwise follow the steps above (generic distribution)


## Further information
For further information please visit the GitHub Page https://atar-axis.github.io/xpadneo/ which is generated automatically from the content of the `/docs` folder.

You will find there e.g. the following sections
* [Troubleshooting](https://atar-axis.github.io/xpadneo/#troubleshooting)
* [Debugging](https://atar-axis.github.io/xpadneo/#debugging)
* [Compatible BT Dongles](https://atar-axis.github.io/xpadneo/#bt-dongles)
