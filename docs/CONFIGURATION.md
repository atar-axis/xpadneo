## Configuration

**Alternatively** to using the config-script, you can also do it by hand:

The driver can be reconfigured at runtime by accessing the following sysfs
files in `/sys/module/hid_xpadneo/parameters`:

* `trigger_rumble_mode` (default `0`)
  * `0` rumbles triggers by pressure and current rumble effect
  * `1` rumbles triggers by force direction (non-conformant)
  * `2` disables trigger rumble
* `trigger_rumble_damping` (default `4`)
  * Damp the strength of the trigger force feedback
  * `1` (none) to `256` (max)
* `combined_z_axis` (default `n`)
  * Combine the triggers (`ABS_Z` and `ABS_RZ`) to form a single axis `ABS_Z` which is used e.g. in flight simulators
  * The left and right trigger will work against each other.
* `quirks` (default empty)
  * Let's you adjust the quirk mode of your controller
  * Comma separated list of `address:flags` pairs
  * Specify your controller MAC address in the format `11:22:33:44:55:66`
  * Specify the flags as sum of the following:
    * `1` if your controller does not support pulse parameters (i.e., 8BitDo controllers)
    * `2` if your controller does not support trigger rumble (most clones in compat-mode)
    * `4` if your controller does not support individual motor programming (i.e., 8BitDo controllers)

Some settings may need to be changed at loading time of the module, take a look at the following example to see how
that works:


### Example

To disable trigger rumbling temporarily, run
`echo 2 | sudo tee /sys/module/hid_xpadneo/parameters/trigger_rumble_mode`

To make the setting permanent and applied at loading time, try
`echo "options hid_xpadneo trigger_rumble_mode=2" | sudo tee /etc/modprobe.d/99-xpadneo-bluetooth.conf`
