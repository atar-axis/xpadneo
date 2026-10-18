<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

## Connecting the Gamepad

First, put the gamepad into pairing mode: Push the connect button on the upper side of the gamepad, and hold it down
until the light starts flashing fast. Then pair it either with the command line or with the Bluetooth manager of your
desktop.


### Using bluetoothctl

- `sudo bluetoothctl`
- `[bluetooth]# scan on`
- wait until all available devices are listed (otherwise it may be hard to identify which one is the gamepad)
- put the gamepad into pairing mode as described above
- wait for the gamepad to show up in bluetoothctl, remember the <MAC> address (e.g. `C8:3F:26:XX:XX:XX`)
- `[bluetooth]# scan off` to stop scanning as it may interfere with properly pairing the controller
- `[bluetooth]# pair <MAC>`
- `[bluetooth]# trust <MAC>`
- `[bluetooth]# connect <MAC>` (should usually not be needed but there are [open bugs](https://github.com/atar-axis/xpadneo/issues/198))
- the `<MAC>` parameter is optional if the command line already shows the controller name


### Using a Desktop Bluetooth Manager

- Start a search for new devices in your Bluetooth manager
- put the gamepad into pairing mode as described above
- once the gamepad shows up in the list, right-click it and select "Pair"
- then right-click it again and select "Connect", or double-click it

The exact names of the menu entries depend on your desktop environment.


### Reconnecting an Already Paired Gamepad

If the gamepad is already paired but does not work, you do not need to remove it first: Turn the gamepad off, turn it
on again in pairing mode, then select "Connect" in your desktop Bluetooth manager, or use `connect <MAC>` in
`bluetoothctl`. If connecting fails the first time, start a new search (scan) in the Bluetooth manager while the light
is still flashing fast, then try to connect again. This also works for third-party gamepads which offer a mode to be
recognized as an Xbox controller.


### Checking the Connection

You know that everything works fine when you feel the gamepad rumble ;)

If it doesn't, please check our documentation in [Troubleshooting](https://atar-axis.github.io/xpadneo/#troubleshooting).
