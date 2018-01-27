# Linux Driver for Xbox One S Wireless Gamepad

This is a driver for the Xbox One S Wireless Gamepad which I created for a student research project at fortiss GmbH.

At the moment of development there was no driver available (and there still is none at January 2018) which supports force feedback (Rumble) one the wireless version (over Bluetooth).

This one does, the buildsystem also fixes a bug I found in L2CAP (which forced us to disable ertm before) and adds the new driver to the hid-core (this automatically loads the new driver when the controller is attached).
Alternatively, it offers a way to load the driver via UDev whenever you are not able to recompile hid-core (e.g. if it is not a module - on Raspbian).

## Build

You have to build yourself if there is not suitable version available in out/.
To do so we offer a Taskfile (you will need go-task https://github.com/go-task/task).

- build driver(s) for your local system
  ```
  task
  alternatively: task default
  alternatively: task local
  ```

- build driver(s) for raspberry pi 3 (linux-raspberrypi-kernel_1.20171029-1)
  ```
  task raspi3
  ```

## Installation

To install the driver, copy the hid-xpadneo module to extramodules and replace the other two modules in you system:

### Remove the original versions

  ```
  sudo mv $(modinfo -n bluetooth) $(modinfo -n bluetooth)_bck
  sudo mv $(modinfo -n hid) $(modinfo -n hid)_bck
  ```

### Install the new ones

```
cd out/<YOUR_ARCH>/<YOUR_KERNEL>
sudo cp ./hid-xpadneo.ko /lib/modules/<YOUR_KERNEL>/extramodules/hid-xpadneo.ko
sudo cp ./hid.ko /lib/modules/<YOUR_KERNEL>/kernel/drivers/hid/hid.ko
sudo cp ./bluetooth.ko /lib/modules/<YOUR_KERNEL>/kernel/net/bluetooth/bluetooth.ko
```

Register the new drivers by running `sudo depmod` afterwards.


#### Problems

- If bluetooth isn't a module, you can alternatively run `sudo /bin/bash -c "echo 1 > /sys/module/bluetooth/parameters/disable_ertm"` before connecting the controller to the computer.

- If hid isn't a module, you can alternatively copy `99-xpadneo.rules` to `/etc/udev/rules.d/` (run `udevadmn control --reload` afterwards)
