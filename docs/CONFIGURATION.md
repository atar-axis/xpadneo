## Configuration

**Alternatively** to using the config-script, you can also do it by hand:

The driver can be reconfigured at runtime by accessing the following sysfs
files in `/sys/module/hid_xpadneo/parameters`:

* `disable_deadzones` (default `0`)
  * `0` enables standard behavior to be compatible with `joydev` expectations
  * `1` enables raw passthrough of axis values without dead zones for high-precision use with modern Wine/Proton games
* `trigger_rumble_mode` (default `0`)
  * `0` rumbles triggers by pressure and current rumble effect
  * `1` rumbles triggers by force direction (non-conformant)
  * `2` disables trigger rumble
* `rumble_attenuation` (default `0,0`)
  * Attenuation the strength of the force feedback
  * `0` (none, full rumble) to `100` (max, no rumble)
  * If one or two values are given, the first value is the overall attenuation
  * If two values are given, the second value applies an extra attenuation to the triggers
  * Example 1: `0,100` turns trigger rumble off, `100,0` or `100` turn all rumble off
  * Example 2: `50,50` makes 50% rumble overall, and 25% for the triggers (50% of 50% = 25%)
  * Example 3: `50` makes 50% rumble overall (main and triggers)
  * Trigger-only rumble is not possible
* `quirks` (default empty)
  * Let's you adjust the quirk mode of your controller
  * Comma separated list of `address:flags` pairs (use `+flags` or `-flags` to change flags instead)
  * Specify your controller MAC address in the format `11:22:33:44:55:66`
  * Specify the flags as sum of the following:
    * `1` if your controller does not support pulse parameters (i.e., 8BitDo controllers)
    * `2` if your controller does not support trigger rumble (most clones in compat-mode)
    * `4` if your controller does not support individual motor programming (i.e., 8BitDo controllers)
    * `8` if your controller supports hardware profiles (auto-detected, do not set manually)
    * `16` if your controller boots in Linux mode (auto-detected, do not change manually)
    * `32` if you prefer to use Nintendo button mappings (i.e., 8BitDo controllers, defaults to off)
    * `64` if your controller has a awkwardly mapped Share button (auto-detected, do not set manually)

Some settings may need to be changed at loading time of the module, take a look at the following example to see how
that works:


### Example

To disable trigger rumbling temporarily, run
`echo 2 | sudo tee /sys/module/hid_xpadneo/parameters/trigger_rumble_mode`

To make the setting permanent and applied at loading time, try
`echo "options hid_xpadneo trigger_rumble_mode=2" | sudo tee /etc/modprobe.d/99-xpadneo-bluetooth.conf`
