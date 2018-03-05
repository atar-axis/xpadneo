*This is a ALPHA version! It works, but there is still A LOT to do.*

---


# Linux Driver for Xbox One S Wireless Gamepad

This is a driver for the Xbox One S Wireless Gamepad which I created for a student research project at fortiss GmbH.

At the moment of development there was no driver available which supports force feedback (Rumble) - at least not for the wireless version (Bluetooth). There still is none at January 2018, but until this driver is in a bit more presentable condition I won't submit it to the linux kernel.

The buildsystem consists not only of the driver itself, it also fixes a bug I found in L2CAP (which initially forced us to disable ertm completely), futhermore it adds the new driver to the hid-core (this way it automatically loads the new driver whenever the controller is registered). As an alternative, it offers a Udev-rule to load the driver, this is a workaround which is useful whenever you are not able to recompile hid-core (e.g. if it is not a module - e.g. on Raspbian - and you don't want to recompile the whole kernel).

## Build

You have to build the driver yourself if there is no suitable version available in out/.
To make life a bit easier, we offer you a Taskfile at the moment (you will need go-task https://github.com/go-task/task to execute that).

Furthermore, you need `git`, `build-essential` (gcc), `linux-headers` and `make` - please make sure those are installed.

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
* Gentoo

## Installation

To install the driver after sucessfull building, simply copy the hid-xpadneo module to extramodules and replace the other two modules in your system:

### Remove the original versions

  ```
  sudo mv $(modinfo -n bluetooth) $(modinfo -n bluetooth)_bck
  sudo mv $(modinfo -n hid) $(modinfo -n hid)_bck
  ```

### Install the new ones

```
sudo task install
```

#### Problems

- If bluetooth isn't a module but builtin, you can alternatively run `sudo /bin/bash -c "echo 1 > /sys/module/bluetooth/parameters/disable_ertm"` before connecting the controller to the computer.

- If hid isn't a module, you can alternatively copy `99-xpadneo.rules` to `/etc/udev/rules.d/` (run `udevadmn control --reload` afterwards)


## Configuration

The driver can be reconfigured at runtime by accessing the following sysfs
files in `/sys/module/hid_xpadneo/parameters`:

- `dpad_to_buttons`: Set to Y to map dpad directional input to button events,
  defaults to N


## Debugging

If you are asked to send debug info or want to fix bugs, enable debugging
first in the driver:

`echo 3 > /sys/module/hid_xpadneo/parameters/debug_level`

where `3` is the most verbose debug level. Disable debugging by setting the
value back to `0`.

You may want to set the debug level at load time of the driver. You can do
this by applying the setting to modprobe:

```
$ cat /etc/modprobe.d/xpadneo.conf
options hid_xpadneo debug_level=3
```

Now, the driver will be initialized with debug level 3 during modprobe.


## Support

The Gamepad has different button mappings, thats the reason why sometimes buttons like "select" and "start" do not work. I think this is fixed now, but if you ever see any wrong mapping *please let me know*!
In order to make the debugging easier, please run:
`sudo /bin/bash -c 'echo 3 > /sys/module/hid_xpadneo/parameters/debug_level'`
`sudo dmesg -C`
Please reconnect your gamepad, press every button once and send me your xpadneo-realted `dmesg` output
`dmesg | grep xpadneo > ~/xpadneo_log`
