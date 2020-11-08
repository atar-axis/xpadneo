#!/bin/bash

# shellcheck disable=SC1090
source "$(dirname "$0")/lib/verbose.sh"

# shellcheck disable=SC1090
source "$(dirname "$0")/lib/installer.sh"

echo "* unloading current driver module"
modprobe -r hid_xpadneo || true

echo "* looking for registered instances"
echo "found ${#INSTALLED[@]} registered instance(s) on your system"


for instance in "${INSTALLED[@]}"
do
    echo "* $instance"

    set -e

    echo "  * uninstalling and removing $instance from DKMS"
    dkms remove "${V[*]}" "hid-xpadneo/${instance}" --all

    echo "  * removing $instance folder from /usr/src"
    rm --recursive "/usr/src/hid-xpadneo-$instance/"
done
