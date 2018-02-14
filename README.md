*This is a ALPHA version! It works, but there is still A LOT to do.*

---


# Linux Driver for Xbox One S Wireless Gamepad

This is a driver for the Xbox One S Wireless Gamepad which I created for a student research project at fortiss GmbH.

At the moment of development there was no driver available which supports force feedback (Rumble) - at least not for the wireless version (Bluetooth). There still is none at January 2018, but until this driver is in a bit more presentable condition I won't submit it to the linux kernel.

The buildsystem consists not only of the driver itself, it also fixes a bug I found in L2CAP (which initially forced us to disable ertm completely), futhermore it adds the new driver to the hid-core (this way it automatically loads the new driver whenever the controller is registered). As an alternative, it offers a Udev-rule to load the driver, this is a workaround which is useful whenever you are not able to recompile hid-core (e.g. if it is not a module - e.g. on Raspbian - and you don't want to recompile the whole kernel).

## Build

You have to build the driver yourself if there is no suitable version available in out/.
To make life a bit easier, we offer you a Taskfile at the moment (you will need go-task https://github.com/go-task/task to execute that).

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
* Ubunutu

## Installation

To install the driver after sucessfull building, simply copy the hid-xpadneo module to extramodules and replace the other two modules in your system:

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


## Known Bugs

The Gamepad has different button mappings, thats the reason why sometimes buttons like "select" and "start" do not work. It is somehow related to different firmware versions and other circumstances I do not fully understand at the moment. I already know how the other mappings look like (which button is which HID-usage), but I don't know yet how to differentiate them.
Please! If you recognize a wrong mapping, please run `dmesg` and tell me what you see (dollinger.florian@gmx.de).

What I need looks the following:
```
report-descriptor:
05 01 09 05 a1 01 85 01 09 01 a1 00 09 30 09 31 15 00 27 ff ff 00 00 95 02 75 10 81 02 c0 09 01
a1 00 09 32 09 35 15 00 27 ff ff 00 00 95 02 75 10 81 02 c0 05 02 09 c5 15 00 26 ff 03 95 01 75
0a 81 02 15 00 25 00 75 06 95 01 81 03 05 02 09 c4 15 00 26 ff 03 95 01 75 0a 81 02 15 00 25 00
75 06 95 01 81 03 05 01 09 39 15 01 25 08 35 00 46 3b 01 66 14 00 75 04 95 01 81 42 75 04 95 01
15 00 25 00 35 00 45 00 65 00 81 03 05 09 19 01 29 0f 15 00 25 01 75 01 95 0f 81 02 15 00 25 00
75 01 95 01 81 03 05 0c 0a 24 02 15 00 25 01 95 01 75 01 81 02 15 00 25 00 75 07 95 01 81 03 05
0c 09 01 85 02 a1 01 05 0c 0a 23 02 15 00 25 01 95 01 75 01 81 02 15 00 25 00 75 07 95 01 81 03
c0 05 0f 09 21 85 03 a1 02 09 97 15 00 25 01 75 04 95 01 91 02 15 00 25 00 75 04 95 01 91 03 09
70 15 00 25 64 75 08 95 04 91 02 09 50 66 01 10 55 0e 15 00 26 ff 00 75 08 95 01 91 02 09 a7 15
00 26 ff 00 75 08 95 01 91 02 65 00 55 00 09 7c 15 00 26 ff 00 75 08 95 01 91 02 c0 85 04 05 06
09 20 15 00 26 ff 00 75 08 95 01 81 02 c0 00
hdev:
- dev_rdesc: (see above)
- dev_rsize (original report size): 335
- bus: 0x0005
- report group: 0
- vendor: 0x0000045E
- version: 0x00000903
- product: 0x000002FD
- country: 33
- driverdata: 0
```
