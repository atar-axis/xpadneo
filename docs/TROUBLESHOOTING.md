## Troubleshooting

### Gamepad does not connect properly

#### Enhanced ReTransmission Mode is enabled

**Obsolete:** This section is scheduled for removal. Future kernels will include the proper fix for this.

Also, there seems to be a bug in the L2CAP handling of the kernel and you may need to force-disable
this setting:
```
# cat /sys/module/bluetooth/parameters/disable_ertm
Y
```

If it says `N`, write `Y` to the file and try again. You may need to remove your partially paired
controller from your Bluetooth settings completely before pairing again.


#### Incompatible Bluetooth Chipset

Some chipsets, e.g. the CSR 85xx, do have problems when you try to reconnect the Gamepad.

Some chipsets may need additional driver firmware to work correctly. Try installing
`linux-firmware` from your distribution.


#### Secure Boot

Secure Boot may be enabled on your computer. On most Linux distribution, running `mokutil --sb-state` will tell you if
it is the case. When Secure Boot is enabled, unsigned kernel module cannot be loaded. Two options are available:

1. Disable Secure Boot.
2. Sign the module yourself.

Instructions for both of these options are available
[here](https://atar-axis.github.io/xpadneo/#working-with-secure-boot).
Secure Boot is not enabled and pairing still fails? See [Debugging](https://atar-axis.github.io/xpadneo/#debugging).


### Gamepad is connected but did not rumble

If the gamepad does connect but it doesn't rumble, then mosty probably the wrong driver is loaded,
or the gamepad is quirky and doesn't fully support the protocol.

Check the output of the `dmesg` command to see whether xpadneo was loaded and logged your
gamepad.


### Gamepad has quirks (i.e., wrong rumble behavior)

You may want to try serveral combinations of quirk flags added to the module paramters.
See [Configuration](https://atar-axis.github.io/xpadneo/#configuration) and `modinfo hid-xpadneo`
for more information. You may also want to use the hidraw testing utility which bypasses the
driver and let's you try different combination of parameters. The utility is located at
`misc/examples/c_hidraw`.


### Gamepad does not connect at all, runs a reconnect loop, or immediately disconnects

Check whether ERTM was disabled (see above). Also, some newer models use a different Bluetooth protocol "Bluetooth
low energe" (BLE) which you may accidentally have disabled. Check the following settings in `/etc/bluetooth/main.conf`:
```
[General]
ControllerMode = dual
Privacy = device
```


### Gamepad axes are swapped, seemingly unresponsive or strange behavior

If you observe this problem with `jstest`, `systemsettings joystick` (KDE) or `jstest-gtk`, there's usually nothing
to do as these test programs use the old `joydev` API while most (modern) games use the `evdev` API. Some emulators,
tho, use the `joydev` API and do not respect the axes naming from `evdev`. In this case, please run the following
command to correct the axes mapping for the js interface:

```bash
jscal -u 8,0,1,3,4,2,5,16,17,10,304,305,307,308,310,311,314,315,317,318 /dev/input/js0
```

Explanation: `-u` set the mapping for `8` axes to axes code `0,1,3,4,...` and for `10` buttons to button codes
`304,305,307,308,...`. This only remaps the `joydev` API, **not** the `evdev` API. Making this change may have
unexpected consequences for applications using both APIs.

**IMPORTANT:** Now test all your axes to collect calibration data. You can then use the following command to store the
settings permanently:

```bash
sudo jscal-store /dev/input/js0
```

If the gamepad does not restore the mapping after disconnecting and reconnecting it, i.e., your distribution doesn't
ship a proper udev rule for that, you may want to add the this udev rule, then reboot (see also
`/misc/examples/udev-rules/99-xpadneo-joydev.rules`):

```bash
# /etc/udev/rules.d/99-xpadneo-joydev.rules
KERNEL=="js*", ACTION=="add", DRIVERS=="xpadneo", RUN+="/usr/bin/jscal-restore %E{DEVNAME}"
```

From now on, connecting the gamepad should restore the values from /var/lib/joystick/joystick.state. If you messed up,
simply remove your gamepad from the file and start over.

Please check, if `jscal-restore` is really in `/usr/bin`, otherwise use the correct path, found by running:

```bash
type -p jscal-restore
```

**IMPORTANT NOTE:** The Chrome gamepad API (used for Stadia and other browser games) is a hybrid user of both the
`joydev` and the `evdev` API bit it uses a hard-coded axes mapping for each controller model. Thus, when you run the
above commands, the API will be confused and now shows the problem you initially tried to fix. To use the Chrome
gamepad API, you'd need to revert that settings. There is currently no known work-around.
