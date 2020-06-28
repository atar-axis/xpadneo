## Troubleshooting

### Gamepad does not connect properly

#### Enhanced ReTransmission Mode is enabled

If your Gamepad is stuck in a Connected / Disconnected loop, then it may be caused by ERTM.
Usually the driver does disable this incompatible mode automatically, but sometimes things go wrong.

#### Incompatible Bluetooth Chipset

Some chipsets, e.g. the CSR 85xx, do have problems when you try to reconnect the Gamepad.

Some chipsets may need additional driver firmware to work correctly. Try installing
`linux-firmware` from your distribution.

#### Secure Boot

Secure Boot may be enabled on your computer. On most Linux distribution, running `mokutil --sb-state` will tell you if it is the case. When Secure Boot is enabled, unsigned kernel module cannot be loaded. Two options are available:
1. Disable Secure Boot.
2. Sign the module yourself.

Instructions for both of these options are available [here](https://atar-axis.github.io/xpadneo/#working-with-secure-boot).
Secure Boot is not enabled and pairing still fails? See [Debugging](https://atar-axis.github.io/xpadneo/#debugging).

### Gamepad is connected but did not rumble

If the gamepad does connect but it doesn't rumble, then mosty probably the wrong driver is loaded,
or the gamepad is quirky and doesn't fully support the protocol.

### Gamepad has quirks (i.e., wrong rumble behavior)

You may want to try serveral combinations of quirk flags added to the module paramters.
See [Configuration](https://atar-axis.github.io/xpadneo/#configuration) and `modinfo hid-xpadneo`
for more information. You may also want to use the hidraw testing utility which bypasses the
driver and let's you try different combination of parameters. The utility is located at
`misc/examples/c_hidraw`.
