#!/bin/bash

if [ ${EUID} -ne 0 ]; then
	echo >&2 "ERROR: You most probably need superuser privileges to use this script, please run me via sudo!"
	exit 3
fi

DKMS_BIN=$(type -p dkms)

# shellcheck disable=SC2086
: ${DKMS_BIN:?Please install dkms to continue}

get_dkms_versions_installed() {
	${DKMS_BIN} status | awk -F', ' -e'$1 == "hid-xpadneo" { print $2 }' | sort -nu
}

get_upstream_version_latest() {
	curl -Lq "https://raw.githubusercontent.com/atar-axis/xpadneo/master/VERSION"
}

# shellcheck disable=SC2034
INSTALLED=(
	$(get_dkms_versions_installed)
)
