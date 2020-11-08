#!/bin/bash -e

# shellcheck disable=SC1090
source "$(dirname "$0")/lib/verbose.sh"

# shellcheck disable=SC1090
source "$(dirname "$0")/lib/installer.sh"

DESTDIR="/usr/src/hid-xpadneo-${VERSION}"

if [[ -z "$INSTALLED" ]]; then

    echo "* copying module to '${DESTDIR}'"
    cp --recursive "hid-xpadneo" "${DESTDIR}"

	echo "* replacing version string if necessary"
	(
	 	cd "${DESTDIR}"
		sed -i 's/"@DO_NOT_CHANGE@"/"'"${VERSION}"'"/g' dkms.conf src/hid-xpadneo.c
	)

    echo "* adding module to DKMS"
    dkms add "${V[*]}" "hid-xpadneo/${VERSION}"

    echo "* installing module (using DKMS)"
    dkms install "${V[*]}" "hid-xpadneo/${VERSION}"

else

    echo "already installed!"

fi
