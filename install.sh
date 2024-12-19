#!/bin/bash

set -e

cd "$(dirname "$0")"
source "lib/verbose.sh"
source "lib/installer.sh"

if [[ ! -d /sys/devices/virtual/misc/uhid ]]; then

    >&2 echo "WARNING: kernel uhid module not found, controller firmware 5.x will not be supported"

fi

if [[ -z "${INSTALLED[*]}" ]]; then

    echo "* creating dkms.conf"
    sed 's/"@DO_NOT_CHANGE@"/"'"${VERSION}"'"/g' <hid-xpadneo/dkms.conf.in >hid-xpadneo/dkms.conf

    # TODO: Works around https://github.com/dell/dkms/issues/177 for DKMS 3
    echo "* adding hid-xpadneo-${VERSION} folder to /usr/src"
    mkdir -p "/usr/src/hid-xpadneo-${VERSION}"
    cp --recursive "${V[@]}" hid-xpadneo/. "/usr/src/hid-xpadneo-${VERSION}/."

    echo "* installing module (using DKMS)"
    dkms install "${V[*]}" "hid-xpadneo/${VERSION}" --force || cat_dkms_make_log

else

    echo "already installed!"

fi
