# Advanced Linux Driver for Xbox One S Wireless Gamepad
This is a driver for the Xbox One S Wireless Gamepad which I created for a student research project at fortiss GmbH.
It is fully functional but does only support the connection via Bluetooth as yet - more will follow.

Many thanks to *Kai Krakow* who **sponsored** me a Xbox One Wireless Controller (including Wireless Adapter) and a pack of mouthwatering guarana cacao ;D

**Advantages of this driver**
* Supports Bluetooth
* Supports Force Feedback over Bluetooth
* Supports multiple Gamepads over Bluetooth (not even supported in Windows)
* Supports [Trigger Force Feedback](https://www.youtube.com/watch?v=G4PHupKm2OQ) (not even supported in Windows)
  * give it a try! run `misc/tools/directional_rumble_test/direction_rumble_test`
* Offers a consistent mapping, even if paired to Windows before
* Working Select, Start, Mode buttons
* Support for Battery Level Indication (including Play `n Charge Kit)
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
* Copy the `hid-xpadneo-<version>` folder into the `/usr/src/` directory  
  `sudo cp -r ~/xpadneo/hid-xpadneo-<version>/ /usr/src/`
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

* `dpad_to_buttons`:
  * Set to `Y` to map dpad directional input to button events, defaults to `N`
  * obsolete, will be removed in a future version
* `debug_level`:
  * 0 (none) to 3 (all)
  * for more information, please see [below](https://github.com/atar-axis/xpadneo#troubleshooting)
* `trigger_rumble_damping`:
  * Damp the strength of the trigger force feedback
  * 1 (none) to 256+ (max)
* `fake_dev_version`:
  * Fake the input device version to the given value (to prevent SDL from applying another mapping correction, see below)
  * Values from `1` to `0xFFFF` are handled as a version number
  * Higher or lower values (i.e. `0`) will result in an untouched version

## Things to know

### SDL Mapping
Since after libSDL2 2.0.8, SDL contains a fix for the wrong mapping introduced
by the generic hid driver. Thus, you may experience wrong button mappings
again despite using xpadneo as the driver.

Also, Wine since version 3.3 supports using SDL for `xinput*.dll`, and with
version 3.4 it includes a patch to detect the Xbox One S controller. Games
running in Wine and using xinput may thus also see wrong mappings.

The Steam client includes a correction for SDL based games since some
version, not depending on the SDL version. It provides a custom SDL
mapping the same way we are describing here.

To fix this and have both SDL-based software and software using the legacy
joystick interface using correct button mapping, you need to export an
environment variable which then overrides default behavior:

```
export SDL_GAMECONTROLLERCONFIG="\
  050000005e040000fd02000003090000,Xbox One Wireless Controller,\
  a:b0,b:b1,back:b6,dpdown:h0.4,dpleft:h0.8,dpright:h0.2,dpup:h0.1,\
  guide:b8,leftshoulder:b4,leftstick:b9,lefttrigger:a2,leftx:a0,lefty:a1,\
  rightshoulder:b5,rightstick:b10,righttrigger:a5,rightx:a3,righty:a4,\
  start:b7,x:b2,y:b3,"
```

You need to set this before starting the software. To apply it globally,
put this line into your logon scripts.

The id `050000005e040000fd02000003090000` is crafted from your device
id as four 32-bit words. It is, in LSB order, the bus number 5, the
vendor id `045e`, the device id `02fd`, and the interface version
or serial `0903` which is not a running number but firmware dependent.
This version number is not the same as shown in dmesg as the fourth
component.

You can find the values by looking at dmesg when `xpadneo` detects
your device. In dmesg, find the device path, then change to the
device path below `/sys` and look for the files in the `id` directory.

The name value after the id is purely cosmetical, you can name it
whatever you like. It may show up in games as a visual identifier.

If running Wine games, to properly support xpadneo, ensure you have
removed any previous xinput hacks (which includes redirecting
`xinput*.dll` to native and placing a hacked xinput dll in the
game directory. Also ensure your Wine built comes with SDL support
compiled in.

If you do not want to apply this setting globally, you can instead
put the SDL mapping inside Steam `config.vdf`. You can find this
file in `$STEAM_BASE/config/config.vdf`. Find the line containing
`"SDL_GamepadBind"` and adjust or add your own controller (see
above). Ensure correct quoting, and Steam is not running
while editing the file. This may not work for Steam in Wine
because the Wine SDL layer comes first, you still need to export
the variable before running Wine. An example with multiple
controllers looks like this:

```
        "SDL_GamepadBind"               "030000006d0400001fc2000005030000,Logitech F710 Gamepad (XInput),a:b0,b:b1,back:b6,dpdown:h0.4,dpleft:h0.8,dpright:h0.2,dpup:h0.1,guide:b8,leftshoulder:b4,leftstick:b9,lefttrigger:a2,leftx:a0,lefty:a1,rightshoulder:b5,rightstick:b10,righttrigger:a5,rightx:a3,righty:a4,start:b7,x:b2,y:b3,
03000000de280000fc11000001000000,Steam Controller,a:b0,b:b1,back:b6,dpdown:h0.4,dpleft:h0.8,dpright:h0.2,dpup:h0.1,guide:b8,leftshoulder:b4,leftstick:b9,lefttrigger:a2,leftx:a0,lefty:a1,rightshoulder:b5,rightstick:b10,righttrigger:a5,rightx:a3,righty:a4,start:b7,x:b2,y:b3,
03000000de280000ff11000001000000,Steam Virtual Gamepad,a:b0,b:b1,back:b6,dpdown:h0.4,dpleft:h0.8,dpright:h0.2,dpup:h0.1,guide:b8,leftshoulder:b4,leftstick:b9,lefttrigger:a2,leftx:a0,lefty:a1,rightshoulder:b5,rightstick:b10,righttrigger:a5,rightx:a3,righty:a4,start:b7,x:b2,y:b3,
030000006d04000019c2000011010000,Logitech F710 Gamepad (DInput),a:b1,b:b2,back:b8,dpdown:h0.4,dpleft:h0.8,dpright:h0.2,dpup:h0.1,leftshoulder:b4,leftstick:b10,lefttrigger:b6,leftx:a0,lefty:a1,rightshoulder:b5,rightstick:b11,righttrigger:b7,rightx:a2,righty:a3,start:b9,x:b0,y:b3,
050000005e040000fd02000003090000,Xbox One Wireless Controller,a:b0,b:b1,back:b6,dpdown:h0.4,dpleft:h0.8,dpright:h0.2,dpup:h0.1,guide:b8,leftshoulder:b4,leftstick:b9,lefttrigger:a2,leftx:a0,lefty:a1,rightshoulder:b5,rightstick:b10,righttrigger:a5,rightx:a3,righty:a4,start:b7,x:b2,y:b3,"
```

### Third party Bugs
While developing this driver we recognized some bugs in KDE and linux itself,
some of which are fixed now - others are not:
* Broken Battery Indicator in KDE
  fixed! https://www.kde.org/announcements/kde-frameworks-5.45.0.php
* L2CAP Layer does not handle ERTM requests
  * workaround: disable ERTM
  * unofficially fixed: see kernel_patches folder
* Binding of specialized drivers
  * before kernel 4.16
    * official way: Add driver to `hid_have_special_driver` an recompilation of HIDcore
    * workaround: UDEV rule (see src/udev_rules)
  * official way as from kernel 4.16: Specialized drivers are binded to the device [automatically](https://github.com/torvalds/linux/commit/e04a0442d33b8cf183bba38646447b891bb02123#diff-88d50bd989bbdf3bbd2f3c5dcd4edcb9) 


## Troubleshooting
### Debug Output
If you are asked to send debug info or want to fix bugs, enable debugging
first in the driver and upon request send the xpadneo related part:

```
echo 3 | sudo tee /sys/module/hid_xpadneo/parameters/debug_level
dmesg | grep xpadneo > ~/xpadneo_log
```

where `3` is the most verbose debug level. Disable debugging by setting the
value back to `0`.

You may want to set the debug level at load time of the driver. You can do
this by applying the setting to modprobe:

```
echo "options hid_xpadneo debug_level=3" | sudo tee /etc/modprobe.d/xpadneo.conf
```

Now, the driver will be initialized with debug level 3 during modprobe.

### Mapping
The Gamepad has different button mappings, thats the reason why sometimes
buttons like "select" and "start" do not work. I think this is fixed now,
but if you ever see any wrong mapping *please let me know*!

Please activate debug messages, reconnect your gamepad (turn it off and on
again), press every button (A, B, X, Y, L1, R1, Start, Select, X-Box, ThumbL,
ThumbR, DpadUp, DpadRight, DpadDown, DpadLeft) and axis once (X, Y, Rx, Ry,
Z, Rz) and upload your xpadneo-related `dmesg` output.
