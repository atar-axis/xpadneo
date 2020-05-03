## Configuration

**Alternatively** to using the config-script, you can also do it by hand:

The driver can be reconfigured at runtime by accessing the following sysfs
files in `/sys/module/hid_xpadneo/parameters`:

* `debug_level` (default `0`)
  * `0` (no debug output) to `3` (all)
  * For more information, please take a look [here](https://atar-axis.github.io/xpadneo/#debugging)
* `disable_ff` (default `0`)
  * `0` (ff enabled) to `1` (ff disabled)
* `trigger_rumble_damping` (default `4`)
  * Damp the strength of the trigger force feedback
  * `1` (none) to `256` (max)
* `combined_z_axis` (default `n`)
  * Combine the triggers (`ABS_Z` and `ABS_RZ`) to form a single axis `ABS_Z` which is used e.g. in flight simulators
  * The left and right trigger will work against each other.

Some settings may need to be changed at loading time of the module, take a look at the following example to see how that works:
  
**Example**
To set the highest level of debug verbosity temporarily, run  
`echo 3 | sudo tee /sys/module/hid_xpadneo/parameters/debug_level`  
To make the setting permanent and applied at loading time, try  
`echo "options hid_xpadneo debug_level=3" | sudo tee /etc/modprobe.d/99-xpadneo-bluetooth.conf`
