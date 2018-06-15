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
* Offers a consistent mapping, even if paired to Windows/Xbox before
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

* `dpad_to_buttons` (default `N`)
  * Set to `Y` to map dpad directional input to button events
  * Obsolete, will be removed in a future version
* `debug_level` (default `0`)
  * `0` (no debug output) to `3` (all)
  * For more information, please see [below](https://github.com/atar-axis/xpadneo#troubleshooting)
* `trigger_rumble_damping` (default `4`)
  * Damp the strength of the trigger force feedback
  * `1` (none) to `256` (max)
* `fake_dev_version` (default `0x1130`)
  * Fake the input device version to the given value (to prevent SDL from applying another mapping, see below)
  * Values from `1` to `0xFFFF` are handled as a version number, `0` will retain the original version


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
