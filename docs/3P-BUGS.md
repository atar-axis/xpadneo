## Third party Bugs

While developing this driver we recognized some bugs in KDE and linux itself,
some of which are fixed now - others are not:

* Broken Battery Indicator in KDE
  fixed! https://www.kde.org/announcements/kde-frameworks-5.45.0.php
* Cambridge Silicon Radio (CSR) chipsets do have problems while reconnecting (OUI 00:1A:7D)
  * Most of these problems may be fixed in kernel 5.10
* Qualcomm chipsets may have performance and lag problems (OUI 9C:B6:D0)
* Some Bluetooth dongles may need additional firmware for proper operation
  * Possible solution: try installing `linux-firmware` from your distribution
