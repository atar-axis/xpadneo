## Troubleshooting

### Gamepad does not connect properly

#### Enhanced ReTransmission Mode is enabled

If your Gamepad is stuck in a Connected / Disconnected loop, then it may be caused by ERTM.
Usually the driver does disable this incompatible mode automatically, but sometimes things go wrong.

#### Incompatible Bluetooth Chipset

Some Chipsets, as the CSR 85xx, do have problems when you try to reconnect the Gamepad.

#### Secure Boot

Secure Boot may be enabled on your computer. On most Linux distribution, running `mokutil --sb-state` will tell you if it is the case. When Secure Boot is enabled, unsigned kernel module cannot be loaded. Two options are available:
1. Disable Secure Boot.
2. Sign the module yourself.

Instructions for both of these options are available [here](https://atar-axis.github.io/xpadneo/#working-with-secure-boot).
Secure Boot is not enabled and pairing still fails? See [Debugging](https://atar-axis.github.io/xpadneo/#debugging).


### Gamepad is connected but did not rumble

If the Gamepad does connect but it doesn't rumble, then mosty probably the wrong driver is loaded.
