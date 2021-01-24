## Third party Bugs

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
    * workaround: UDEV rule (see `src/etc-udev-rules.d`)
  * official way as from kernel 4.16: Specialized drivers are bound to the device [automatically](https://github.com/torvalds/linux/commit/e04a0442d33b8cf183bba38646447b891bb02123#diff-88d50bd989bbdf3bbd2f3c5dcd4edcb9)
* Cambridge Silicon Radio (CSR) chipsets do have problems while reconnecting (OUI 00:1A:7D)
  * Most of these problems may be fixed in kernel 5.10
* Qualcomm chipsets may have performance and lag problems (OUI 9C:B6:D0)
* Some Bluetooth dongles may need additional firmware for proper operation
  * Possible solution: try installing `linux-firmware` from your distribution
