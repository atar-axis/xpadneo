Many many thanks to *Kai Krakow* who sponsored me a Xbox One Wireless Controller (including Wireless Adapter) and a pack of mouthwatering guarana cacao ;D

*This Driver is currently in development - basic functionality works, but there is still A LOT to do.*


# Advanced Linux Driver for Xbox One S Wireless Gamepad

This is a driver for the Xbox One S Wireless Gamepad which I created for a student research project at fortiss GmbH.

At the moment of development there was no driver available which supports force feedback (Rumble) - at least not for the wireless version (Bluetooth). There still is none at January 2018, but until this driver is in a bit more presentable condition I won't submit it to the linux kernel.

The buildsystem consists not only of the driver itself, it also fixes a bug I found in L2CAP (which initially forced us to disable ertm completely), futhermore it adds the new driver to the hid-core (this way it automatically loads the new driver whenever the controller is registered). As an alternative, it offers a Udev-rule to load the driver, this is a workaround which is useful whenever you are not able to recompile hid-core (e.g. if it is not a module - e.g. on Raspbian - and you don't want to recompile the whole kernel).

## Advantages

* Supports Bluetooth
* Supports Force Feedback over Bluetooth
* Supports Force Feedback at the triggers (not even supported at Windows)
  * take a look here https://www.youtube.com/watch?v=G4PHupKm2OQ
* Offers a consistent mapping, even if paired to Windows before
* Working Select, Start, Mode buttons
* Support for Battery Level Indication
  * On some KDE desktops it works, on others not. We've already fixed that and ~~are currently waiting for a merge request: https://phabricator.kde.org/D11331~~
  UPDATE: Merged in https://www.kde.org/announcements/kde-frameworks-5.45.0.php
  * Tested in GNOME
* Agile Development

## Build

You have to build the driver yourself if there is no suitable version available in out/.
To make life a bit easier, we offer you a Taskfile at the moment (you will need go-task https://github.com/go-task/task to execute that).

Furthermore, you need `git`, `build-essential` (gcc), `linux-headers` and `make` - please make sure those are installed.
For cross-compilation (raspberry) you also need: `arm-linux-gnueabihf-gcc-stage1`.

- build driver(s) for your local system
  ```
  task
  ```
  Alternatively, you can also run either `task default` or `task local`

- build driver(s) for raspberry pi 3 (linux-raspberrypi-kernel_1.20171029-1)
  ```
  task raspi3
  ```

The Taskfile downloads your current kernel, copies the driver into the extracted folder and patches hid-core and l2cap. Then it compiles the kernel modules and copies the resulting driver to your local ./out directory.

Until now the Taskfile is tested under the following Distributions
* ARCH
* Antergos
* Gentoo

## Installation

If Hid and Bluetooth are modules (not built-ins), then all you have to do is:

```
sudo task install
```

If bluetooth isn't a module but builtin, you can alternatively run
`echo 1 | sudo tee /sys/module/bluetooth/parameters/disable_ertm`
before connecting the controller to the computer.

If hid isn't a module, you can alternatively copy `99-xpadneo.rules` to `/etc/udev/rules.d/` (run `udevadmn control --reload` afterwards)


## Configuration

The driver can be reconfigured at runtime by accessing the following sysfs
files in `/sys/module/hid_xpadneo/parameters`:

- `dpad_to_buttons`: Set to Y to map dpad directional input to button events,
  defaults to N

## Caveats

From libSDL 3.3 onward, SDL contains a fix for the wrong mapping introduced
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

## Debugging

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


