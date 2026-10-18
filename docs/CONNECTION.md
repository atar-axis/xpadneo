<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

## Connecting the Gamepad

- `sudo bluetoothctl`
- `[bluetooth]# scan on`
- wait until all available devices are listed (otherwise it may be hard to identify which one is the gamepad)
- push the connect button on upper side of the gamepad, and hold it down until the light starts flashing fast
- wait for the gamepad to show up in bluetoothctl, remember the <MAC> address (e.g. `C8:3F:26:XX:XX:XX`)
- `[bluetooth]# scan off` to stop scanning as it may interfere with properly pairing the controller
- `[bluetooth]# pair <MAC>`
- `[bluetooth]# trust <MAC>`
- `[bluetooth]# connect <MAC>` (should usually not be needed but there are [open bugs](https://github.com/atar-axis/xpadneo/issues/198))
- The `<MAC>` parameter is optional if the command line already shows the controller name

If the gamepad is already paired but does not work, you do not need to remove it first: Turn the gamepad off, turn it
on again in pairing mode (hold the connect button until the light flashes fast), then select "Connect" in your desktop
Bluetooth manager, or use `connect <MAC>` in `bluetoothctl`. If connecting fails the first time, start a new search
(scan) in the Bluetooth manager while the light is still flashing fast, then try to connect again. This also works for
third-party gamepads which offer a mode to be recognized as an Xbox controller.

You know that everything works fine when you feel the gamepad rumble ;)

If it doesn't, please check our documentation in [Troubleshooting](https://atar-axis.github.io/xpadneo/#troubleshooting).
