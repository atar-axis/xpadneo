[![ko-fi](https://ko-fi.com/img/githubbutton_sm.svg)](https://ko-fi.com/O4O43SURE)

If you want to support me or accelerate the development of a special feature, consider a small donation :heart:
Just leave a message if your donation is for a specific use (like a new hardware or a specific function).

[![Build Status](https://dev.azure.com/dollingerflorian/dollingerflorian/_apis/build/status/atar-axis.xpadneo?branchName=master)](https://dev.azure.com/dollingerflorian/dollingerflorian/_build/latest?definitionId=1?branchName=master)
[![Average time to resolve an issue](http://isitmaintained.com/badge/resolution/atar-axis/xpadneo.svg)](http://isitmaintained.com/project/atar-axis/xpadneo "Average time to resolve an issue")
[![Packaging status](https://repology.org/badge/tiny-repos/xpadneo.svg)](https://repology.org/project/xpadneo/versions)
[![Discord](https://img.shields.io/discord/733964971842732042)](https://discord.gg/nCqfKa84KA)


# Advanced Linux Driver for Xbox One Wireless Gamepad

![xpadneo Logo](img/xpadneo.png)

Quote from [@atar-axis (Florian Dollinger)](https://github.com/atar-axis), creator of the initial driver:

> This is the first driver for the Xbox One Wireless Gamepad (which is shipped with the Xbox One S). I wrote it for a
> student project at fortiss GmbH and it is fully functional but does only support the connection via Bluetooth as
> yet - more will follow.
>
> Many thanks to *Kai Krakow* who **sponsored** me a Xbox One Wireless Controller :video_game: (including Wireless
> Adapter) and a pack of mouthwatering guarana cacao :coffee:


## Other Projects

* [xow](https://github.com/medusalix/xow) is a driver for the Xbox One S controllers, too, and supports the native
  dongles packaged with the controller. Kudos to [@medusalix](https://github.com/medusalix) for working together on
  finding some work-arounds for controller firmware bugs.
* [xpad](https://github.com/paroj/xpad) supports this and many other controllers in USB mode.
* [xone](https://github.com/medusalix/xone) is a driver aiming for fully supporting all Microsoft GIP devices thus
  replacing the xpad driver in the kernel while adding support for additional types of hardware.
* [MissionControl](https://github.com/ndeadly/MissionControl) aims to support the controller on Nintendo Switch via
  Bluetooth.

These other projects may not support some of the advanced features of xpadneo.


## Breaking Changes

### Kernel 4.18 or newer required

As of xpadneo v0.10, we require kernel 4.18 or later to utilize `HID_QUIRK_INPUT_PER_APP` which splits the gamepad into
multiple sub-devices to fix problems and incompatibilities at several layers.


### SDL2 2.28 Compatibility

Thanks to [@slouken](https://github.com/slouken) from SDL2, xpadneo mappings are now auto-detected in the upcoming
SDL2 2.28 release. This will fix long-standing problems with Steam Input and SDL2 games. With this release, we will
also have full paddle support.

If you still see problems, ensure that you didn't create custom controllerdb entries. See also:
- https://github.com/atar-axis/xpadneo/issues/428
- https://github.com/libsdl-org/SDL/commit/9567989eb3ce9c858f0fe76806c5ccad69da89ba
- https://github.com/libsdl-org/SDL/commit/0f4b15e16b7f07a46db6dc8e651f8c1849d658c5

Known issues:
- The Share button will currently not be recognized by SDL2, scheduled to be fixed in xpadneo v0.11
- If SDL2 uses hidraw, mappings will be wrong, export `SDL_JOYSTICK_HIDAPI=0` in your profile or find which software
  enabled hidraw device access to all drivers


### Quirks by Design

With BLE firmware, all models switched to a unified HID report descriptor, only the XBE2 controller identifies with
PID 0x0B22 while the other models identify with PID 0x0B13. This has some known consequences:

- All non-XBE2 controllers will claim to have a Share button no matter if it physically exists. As HID doesn't report
  the internal model number, xpadneo cannot fix it currently. The button is currently mapped to F12, so this has no
  consequences.
- All XBE2 controllers will claim to have a full keyboard and the Share button is actually the Profile button. Since
  Share is currently mapped to F12, this will have no consequences.


## Advantages of this Driver

* Supports Bluetooth
* Supports most force feedback and all rumble effects through Linux `ff-memless` effect emulation
* Supports [Trigger Force Feedback](https://www.youtube.com/watch?v=G4PHupKm2OQ) in every game by applying a
  pressure-dependent effect intensity to the current rumble effect (not even supported in Windows)
* Supports adjusting rumble intensity including disabling rumble
* Offers a consistent mapping, even if the Gamepad was paired to Windows/Xbox before, and independent of software
  layers (SDL2, Stadia via Chrome Gamepad API, etc)
* Working paddles (buttons on the backside of the controller)
* Correct axis range (signed, important for e.g. RPCS3)
* Supports battery level indication (including the Play 'n Charge Kit)
  ![Battery Level Indication](./img/battery_support.png)
* Easy installation
* Supports customization through profiles (work in progress)
* Optional high-precision mode for Wine/Proton users (disables dead zones so games don't apply an additional one)
* Share button support on supported controllers


## Unavailable Features

Across all models, xpadneo won't support audio features of the controllers because the firmware doesn't support audio
in Bluetooth mode. In the future, xpadneo may support audio when USB and dongle support will be added.


### Xbox One S Wireless Controller

This is the initial controller supported from the first version of xpadneo. All features are fully supported. This
controller uses emulated profile switching support (see below).


### Xbox Elite Series 2 Wireless Controller

Basic support for the Xbox Elite Series 2 Wireless controller is present, covering all the features of the driver.
The following features are missing:

- Upload of profile mappings and sensitivity curves is currently not supported.

This controller uses native profile switching support (see below).


### Xbox Series X|S Wireless Controller

Full support for the Xbox Series X|S controller is present including the share button. This is currently statically
mapped to keyboard event `KEY_F12` to take screenshots with Steam. It will be configurable in the future. This
controller uses emulated profile switching support (see below).

This controller uses BLE (Bluetooth low energy) and can only be supported if your Bluetooth dongle also supports BLE.

**Known problems:** The controller may not properly set its connection parameters, resulting in laggy and choppy
input experience. See also: [Troubleshooting](https://atar-axis.github.io/xpadneo/#troubleshooting).


### 8BitDo Controllers

This driver supports the Nintendo layout of those controllers to exposes them correctly as button A, B, X, and Y
as labelled on the device. This is swapped compared to the original Xbox controller layout. However, this feature is
not enabled by default. If you want to use this feature, you have to add a quirk flag to the module options:

```
# /etc/modprobe.conf
options hid_xpadneo quirks=E4:17:D8:xx:xx:xx+32
```

where you replace `xx:xx:xx` with the values from your controller MAC (as shown in `dmesg`). The value `32` enables
Nintendo layout. If you'll want to add other quirk flags, simply add the values,
e.g. `32` + `7` (default quirks for 8BitDo) = `39`. After changing this, reload the driver or reboot.

This controller uses emulated profile switching support (see below).

**Breaking change:** Users of previous versions of the driver may want to remove their custom SDL mappings. Full
support has been added for these controllers and broken mapping of previously versions no longer needs to be
applied. See also: [SDL](https://atar-axis.github.io/xpadneo/#troubleshooting#sdl).


### GuliKit KingKong Controller Family

This driver supports the GuliKit King Kong controller family, the driver was tested with model NS09 (using firmware
v2.0) and NS39 (aka KK3 MAX, firmware v3.6) but should work just fine for the older models, too. If in doubt, follow
the firmware upgrade guides on the GuliKit home page to receive the latest firmware. Both the Android mode and the
X-Input mode are supported but it may depend on your Bluetooth stack which mode works better for you (Android mode
didn't pair for me).

This driver supports the Nintendo layout of those controllers to exposes them correctly as button A, B, X, and Y
as labelled on the device. This is swapped compared to the original Xbox controller layout. However, this feature is
not enabled by default. If you want to use this feature, you have to add a quirk flag to the module options:

```
# /etc/modprobe.conf
options hid_xpadneo quirks=98:B6:EA:xx:xx:xx+32
```

where you replace `xx:xx:xx` with the values from your controller MAC (as shown in `dmesg`). The value `32` enables
Nintendo layout. If you'll want to add other quirk flags, simply add the values,
e.g. `32` + `131` (default quirks for GuliKit) = `163`. After changing this, reload the driver or reboot.

However, alternatively the controller supports swapping the buttons on the fly, too: Just press and hold the settings
button, the click the plus button. Thus, the quirks flag is just a matter of setting the defaults.

This controller uses emulated profile switching support (see below).


### GameSir T4 Cyclone Family

This driver supports the GameSir T4 Cyclone controller family, tested by the community. The Pro-models also support
trigger rumble but since we cannot distinguish both models by the Bluetooth MAC OUI, we simply enable the trigger
rumble protocol for both variants. This should not introduce any problems but if it does, and your model does not have
trigger rumble support, you can explicitly tell the driver to not use the trigger rumble motors by adding a quirk flag:

```
# /etc/modprobe.conf
options hid_xpadneo quirks=A0:5A:5D:xx:xx:xx+2
```

This controller uses emulated profile switching support (see below).


### GameSir T4 Nova Lite Family

This driver supports the GameSir T4 Nova Lite controller family, tested by the community. These models have a quirk of
only allowing rumble when all motor-enable bits are set and does not have trigger rumble motors. It looks like these
models are available with different MAC OUIs, so your particular controller may not be automatically detected. In this
case, manually add the quirk flags for your controller:

```
# /etc/modprobe.conf
options hid_xpadneo quirks=3E:42:6C:xx:xx:xx+6
```

This controller uses emulated profile switching support (see below).


## Profile Switching

The driver supports switching between different profiles, either through emulation or by using the hardware
switch that comes with some models. This switching can be done at any time even while in a game. The API for
customizing each profile does not exist yet.


### Native Profile Switching Support

The driver support native profile switching for the Xbox Elite Series 2 controller. However, the feature is not
finalized yet:

- The default profile (no LED) exposes the paddles as extra buttons.
- The other three profiles behave the same way by default. While there is no support for modifying them currently,
  configurations set in the [Xbox Accessories app (Windows only)](https://apps.microsoft.com/store/detail/xbox-accessories/9NBLGGH30XJ3)
  will carry over and operate as intended.


### Emulated Profile Switching Support

The driver emulates profile switching for controllers without a hardware profile switch by pressing buttons A, B, X,
or Y while holding down the Xbox logo button. However, the following caveats apply:

- Profiles currently behave all the same, and there is no support for configuring them.
- Full support will be available once the Xbox Elite Series 2 controller is fully supported.
- If you hold the button for too long, the controller will turn off - we cannot prevent that.

**Important:** Emulated profile switching won't work if you disabled the shift-mode of the Xbox logo button (module
parameter `disable_shift_mode`).


## Getting Started

### Distribution Packages

If your distribution has a maintained package, you can just use that and do not need to follow the manual install
instructions below:

[![Packaging status](https://repology.org/badge/vertical-allrepos/xpadneo.svg)](https://repology.org/project/xpadneo/versions)


### Notes for Package Maintainers

To properly support module signing and UEFI secure boot, `openssl` and `mokutil` are required additionally to the
prerequisites below. The [DKMS readme](https://github.com/dell/dkms/blob/master/README.md) has more instructions.


### Prerequisites

Make sure you have installed *dkms*, *linux headers* and a bluetooth implementation (e.g. *bluez*) and their
dependencies.

Kernel maintainers should also include the `uhid` module (`CONFIG_UHID`) because otherwise Bluetooth LE devices (all
models with firmware 5.x or higher) cannot create the HID input device which is handled in user-space by the bluez
daemon.

* On **Arch** and Arch-based distros (like **EndeavourOS**), try
  `sudo pacman -S dkms linux-headers bluez bluez-utils`
* On **Debian** based systems (like Ubuntu) you can install those packages by running
  ``sudo apt-get install dkms linux-headers-`uname -r` ``
* On **Fedora**, it is
  ``sudo dnf install dkms make bluez bluez-tools kernel-devel-`uname -r` kernel-headers ``
* On **Manjaro** try
  `sudo pacman -S dkms linux-latest-headers bluez bluez-utils`
* On **openSUSE** (tested on Tumbleweed, should work for Leap), it is
  `sudo zypper install dkms make bluez kernel-devel kernel-source`
* On **OSMC** you will have to run the following commands
  ``sudo apt-get install dkms rbp2-headers-`uname -r` ``
  ``sudo ln -s "/usr/src/rbp2-headers-`uname -r`" "/lib/modules/`uname -r`/build"`` (as a [workaround](https://github.com/osmc/osmc/issues/471))
* On **Raspbian**, it is
  `sudo apt-get install dkms raspberrypi-kernel-headers`
  If you recently updated your firmware using `rpi-update` the above package may not yet include the header files for
  your kernel. Please follow the steps described [here](https://github.com/notro/rpi-source/wiki) in this case.
* On **generic distributions**, it doesn't need DKMS but requires a configured kernel source tree, then:
  `cd hid-xpadneo && make modules && sudo make modules_install`
* **Module singing and UEFI secure boot:** If installing yourself, you may need to follow the instructions above for
  package maintainers.

Please feel free to add other distributions as well!


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
* `[bluetooth]# scan off` to stop scanning as it may interfere with properly pairing the controller
* `[bluetooth]# pair <MAC>`
* `[bluetooth]# trust <MAC>`
* `[bluetooth]# connect <MAC>` (should usually not be needed but there are [open bugs](https://github.com/atar-axis/xpadneo/issues/198))
* The `<MAC>` parameter is optional if the command line already shows the controller name

You know that everything works fine when you feel the gamepad rumble ;)


### Configuration

* If using DKMS: Use `sudo ./configure.sh` to configure the driver as you wish. The script will guide you through the
  available options.


### Update

In order to update xpadneo, do the following

* Update your cloned repo: `git pull`
* If using DKMS: Run `sudo ./update.sh`
* otherwise follow the steps above (generic distribution)


### Uninstallation

* If using DKMS: Run `sudo ./uninstall.sh` to remove all installed versions of hid-xpadneo
* otherwise follow the steps above (generic distribution)


## Further Information

For further information please visit the GitHub Page https://atar-axis.github.io/xpadneo/ which is generated
automatically from the content of the `/docs` folder.

You will find there e.g. the following sections

* [Troubleshooting](https://atar-axis.github.io/xpadneo/#troubleshooting)
* [Debugging](https://atar-axis.github.io/xpadneo/#debugging)
* [Compatible BT Dongles](https://atar-axis.github.io/xpadneo/#bt-dongles)
