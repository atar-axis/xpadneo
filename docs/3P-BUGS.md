<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

## Third Party Bugs

While developing this driver we recognized some bugs in KDE and linux itself,
some of which are fixed now - others are not:

* Broken Battery Indicator in KDE
  fixed! <https://www.kde.org/announcements/kde-frameworks-5.45.0.php>
* Cambridge Silicon Radio (CSR) chip sets do have problems while reconnecting (OUI 00:1A:7D)
  * Most of these problems may be fixed in kernel 5.10
* Qualcomm chip sets may have performance and lag problems (OUI 9C:B6:D0)
* Some Bluetooth dongles may need additional firmware for proper operation
  * Possible solution: try installing `linux-firmware` from your distribution
* Arch Linux based distributions: The pacman DKMS hook (`/usr/share/libalpm/scripts/dkms`) fails to rebuild xpadneo
  on kernel updates if the version contains dashes, e.g. development versions like `v0.11-pre-64-g9aa168e`
  * The hook splits `hid-xpadneo-<version>` at the last dash and registers a wrong module name and version; the build
    then fails with `specified external module directory ... does not exist`
  * This is a bug in the distribution hook (see also Arch Linux bug FS#73194); xpadneo itself installs the module as
    `hid-xpadneo/<version>` correctly
  * Work-around: Install the module manually after the kernel update, e.g.
    `sudo dkms install -m hid-xpadneo -v <version> -k <kernel version>`, and use `dkms status` to find the version
  * Release versions without dashes in the version (e.g. `v0.10`) are not affected
