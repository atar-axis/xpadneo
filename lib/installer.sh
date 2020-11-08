#!/bin/bash

if [ ${EUID} -ne 0 ]; then
	echo >&2 "ERROR: You most probably need superuser privileges to use this script, please run me via sudo!"
	exit 3
fi

# shellcheck disable=SC2034
GIT_ROOT=$(git rev-parse --show-toplevel 2>/dev/null || true)

__version() {
	git describe --tags --dirty 2>/dev/null || sed -re's/v?(.*)/v\1/' "$(dirname "${BASH_SOURCE[0]}")/../VERSION"
}

__version_lte() {
	[ "$1" = "$(echo -e "$1\n${2/-/.9999-}" | sort -V | head -n1)" ]
}

# shellcheck disable=SC2034
VERSION=$(__version)

DKMS_BIN=$(type -p dkms)

# shellcheck disable=SC2086
: ${DKMS_BIN:?Please install dkms to continue}

get_dkms_versions_installed() {
	${DKMS_BIN} status | tr -s ':, ' ' ' | awk -- '$1 == "hid-xpadneo" { print $2 }' | sort -nu
}

get_upstream_version_latest() {
	curl -sI "https://github.com/atar-axis/xpadneo/releases/latest" | \
	awk -- 'BEGIN{IGNORECASE=1} /^location:/ { n = split($2, v, /\//); print v[n]; exit }' | \
	tr -d '[:space:]'
}

cat_dkms_make_log() {
	local last_error=$?
	if [ -n "${V[*]}" ]; then
		cat "/var/lib/dkms/hid-xpadneo/${VERSION}/build/make.log" || true
	fi
	exit ${last_error}
}

# shellcheck disable=SC2034
INSTALLED=(
	$(get_dkms_versions_installed)
)
