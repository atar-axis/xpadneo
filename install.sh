#!/bin/bash -e

cd "$(dirname "$0")" || exit 1
source "lib/verbose.sh"
source "lib/installer.sh"

if [[ -z "${INSTALLED[*]}" ]]; then

    set -e

    echo "* creating dkms.conf"
    sed 's/"@DO_NOT_CHANGE@"/"'"${VERSION}"'"/g' <hid-xpadneo/dkms.conf.in >hid-xpadneo/dkms.conf

    echo "* installing sources (using DKMS)"
    # TODO: Works around https://github.com/dell/dkms/issues/177 for DKMS 3
    cp -a hid-xpadneo/ /usr/src/hid-xpadneo-${VERSION}
    dkms add -v "hid-xpadneo/${VERSION}" --force || cat_dkms_make_log

    echo "* building modules (using DKMS)"
    dkms build "hid-xpadneo/${VERSION}" --force || cat_dkms_make_log

    echo "* installing module (using DKMS)"
    dkms install "${V[*]}" "hid-xpadneo/${VERSION}" --force || cat_dkms_make_log

else

    echo "already installed!"

fi
