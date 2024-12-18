#!/bin/bash

set -e

cd "$(dirname "$0")"
source "lib/options.sh"
source "lib/installer.sh"

if [[ ! -d /sys/devices/virtual/misc/uhid ]]; then

    >&2 echo "WARNING: kernel uhid module not found, controller firmware 5.x will not be supported"

fi

echo "* creating dkms.conf"
sed 's/"@DO_NOT_CHANGE@"/"'"${VERSION}"'"/g' <hid-xpadneo/dkms.conf.in >hid-xpadneo/dkms.conf

echo "* registering module"
dkms add "${V[@]}" "hid-xpadneo" || maybe_already_installed

echo "* building module"
dkms build "${V[@]}" "hid-xpadneo/${VERSION}" || cat_dkms_make_log

echo "* installing module"
dkms install "${V[@]}" "${FORCE}" "hid-xpadneo/${VERSION}"
