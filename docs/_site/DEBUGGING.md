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
