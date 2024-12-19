#!/bin/bash

cd "$(dirname "$0")" || exit 1
source "lib/verbose.sh"
source "lib/installer.sh"

echo "* unloading current driver module"
modprobe -r hid_xpadneo || true

echo "* looking for registered instances"
echo "found ${#INSTALLED[@]} registered instance(s) on your system"

set -e

for VERSION in "${INSTALLED[@]}"; do
    echo "* ${VERSION}"

    echo "  * uninstalling and removing ${VERSION}"
    dkms remove "${V[*]}" "hid-xpadneo/${VERSION}" --all

    echo "  * removing ${VERSION} folder from /usr/src"
    rm --recursive "${V[@]}" "/usr/src/hid-xpadneo-${VERSION}/"
done
