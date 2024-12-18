#!/bin/bash

set -e

cd "$(dirname "$0")"
source "lib/options.sh"
source "lib/installer.sh"

if [[ ! -d /sys/devices/virtual/misc/uhid ]]; then

    >&2 echo "WARNING: kernel uhid module not found, controller firmware 5.x will not be supported"

fi

echo "* deploying DKMS package"
make "${MAKE_OPTS[@]}" VERSION="${VERSION}" install || maybe_already_installed

echo "* building module"
dkms build "${V[@]}" "hid-xpadneo/${VERSION}" || cat_dkms_make_log

echo "* installing module"
dkms install "${V[@]}" "${FORCE}" "hid-xpadneo/${VERSION}"
