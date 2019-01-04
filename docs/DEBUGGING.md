## Debugging
### Enable Debug Output
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

Useful information can now be aquired with the commands:

* `dmesg`: I advise you to run `dmesg -wH` in a terminal while you connect your controller from a second terminal to get hardware information in realtime.
* `modinfo hid_xpadneo`: get information on xpadneo as a kernel module.
* When your gamepad is connected, run
  ```console
  sudo find "/sys/kernel/debug/hid/" -name "0005:045E:*" -exec sh -c 'echo "{}" && head -1 "{}/rdesc" | tee /dev/tty | cksum && echo' \;
  ```
  to get the rdesc identifier of your gamepad.
