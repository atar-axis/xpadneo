## SDL Mapping

We fixed the following problem by pretending we are in Windows wireless mode
by faking the *input device PID* to `0x02E0`. The original PID `0x02FD`
triggeres several unwanted fixups at multiple layers, i.e. SDL or the HTML5
game controller API. The following paragraphs document the originally
wrong behaviour observed and we clearly don't want our fixed mappings to be
"fixed" again by layers detected a seemingly wrong button mapping:

Since after libSDL2 2.0.8, SDL contains a fix for the wrong mapping introduced
by the generic hid driver. Thus, you may experience wrong button mappings
again.

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

An alternative store location of user-defined mappings can be found here:
```
$HOME/.local/share/gamecontrollerdb.local.txt
```
