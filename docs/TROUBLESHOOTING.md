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


#### High Latency or Lost Button Events with Bluetooth LE

**Affected models:** Xbox controllers using Bluetooth LE (Xbox Series X\|S or later)

While using new Xbox Series X\|S controller, you may experience laggy or choppy input, also button presses may be
lost or delayed. This problem only affects Bluetooth LE controllers, the older models are not affected by these
settings even if you think you may see such a problem.

A proper solution is still missing but we isolated it to the Bluetooth LE connection parameters for latency and
intervals. The bluez developers say that the connected device should suggest the best settings, the bluez daemon only
ships sensible default settings. It looks like the new Xbox controllers do not properly suggest their preferred
connection parameters, some BLE mice show the same problem. You can work around it by changing the bluez defaults
instead. This change is not recommended by the bluez developers but as long as you only use a very specific set of BLE
devices, this change should be fine.

The controller uses 100 Hz internally for its protocol, so we decided to use intervals of 8.75..11.25ms. Each tick is
1.25ms, so we end up with `MinConnectionInterval=7` and `MaxConnectionInterval=9`. If you already use a similar
work-around for other devices, you may need to adjust your settings to the proper bounds, i.e. do not increase the
min value, do not lower the max value.

Edit the following file and restart the Bluetooth service or reboot:
```
# /etc/bluetooth/main.conf
[LE]
MinConnectionInterval=7
MaxConnectionInterval=9
ConnectionLatency=0
```

References:

* https://www.novelbits.io/ble-connection-intervals/
* https://github.com/bluez/bluez/issues/156
* https://wiki.archlinux.org/title/Bluetooth_mouse#Mouse_lag


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
JustWorksRepairing = confirm
```


#### Xbox Wireless controller

The newest wireless controllers from Microsoft (Xbox One and Xbox Series X\|S) are known to cause a reconnect loop and
not pairing with Bluez. There are some specific workarounds:

- Plug your controller to a Windows 10 computer via a USB cord. Download
  the [Xbox Accessories application](https://xbox.com/accessories-app). When launching the app, it should ask you to
  update the firmware of the controller. When it's done, your controller should work just fine with any Linux system.
- If it didn't work, you can try these two workarounds:
    - Use a Windows 10 computer *on the same Bluetooth adapter* to pair with the controller. It must absolutly be on
      the same Bluetooth adapter, i.e. the same computer (can be inside a virtual machine with Bluetooth passthrough)
      if it's an internal Bluetooth adapter, or the same Bluetooth dongle. Then, you can get the pairing keys and
      install them within your Linux Bluetooth system.
    - Update to a newer kernel. Kernel 5.13 and higher might have patched a fix.
    - Use a different Bluetooth stack. Xbox controllers work fine with Fluoride (the bluetooth stack from Android).
      Sadly, it's hard to install on another Linux, and Bluez is the only stack easily provided on most Linux
      distributions.
- If none of these options worked, or you can't try them, then the only solution is to plug the controller using a USB
  cord. As for now, it won't load the xpadneo driver, but the default controller driver. USB support may be added soon
  to xpadneo.


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
