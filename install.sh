#!/bin/bash -e

cd "$(dirname "$0")" || exit 1
source "lib/verbose.sh"
source "lib/installer.sh"

DESTDIR="/usr/src/hid-xpadneo-${VERSION}"

if [[ -z "${INSTALLED[*]}" ]]; then

    echo "* copying module to '${DESTDIR}'"
    cp --recursive "hid-xpadneo" "${DESTDIR}"

    echo "* replacing version string if necessary"
    (
        cd "${DESTDIR}"
        sed -i 's/"@DO_NOT_CHANGE@"/"'"${VERSION}"'"/g' dkms.conf src/version.h
    )

    set -e

    echo "* adding module to DKMS"
    dkms add "${V[*]}" "hid-xpadneo/${VERSION}" || cat_dkms_make_log

    echo "* installing module (using DKMS)"
    dkms install "${V[*]}" "hid-xpadneo/${VERSION}" || cat_dkms_make_log

else

    echo "already installed!"

fi
