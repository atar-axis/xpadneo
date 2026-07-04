#!/bin/bash

cd "$(dirname "$0")"
source "lib/options.sh"
source "lib/installer.sh"

set -e

if [[ ! -d /sys/devices/virtual/misc/uhid ]]; then

    >&2 echo 'WARNING: Kernel uhid module not found, controller firmware 5.x may not be supported.'
    >&2 echo 'WARNING: This does not prevent xpadneo from being installed. If your kernel supports'
    >&2 echo 'WARNING: uhid, loading hid_xpadneo during boot should also load uhid; see'
    >&2 echo 'WARNING: `docs/TROUBLESHOOTING.md` for the `/etc/modules-load.d/xpadneo.conf`'
    >&2 echo 'WARNING: example.'

fi

if [ "$(uname -r | awk -F. '{ printf "%03d%03d",$1,$2 }')" -lt 005012 ]; then

    >&2 echo "WARNING: kernel 5.12 or lower will not be supported due to known ERTM issues"

fi

echo "* deploying DKMS package"
make "${MAKE_OPTS[@]}" VERSION="${VERSION}" install || maybe_already_installed

echo "* building module"
dkms build "${V[@]}" "hid-xpadneo/${VERSION}" || cat_dkms_make_log

echo "* installing module"
dkms install "${V[@]}" "${FORCE}" "hid-xpadneo/${VERSION}"
