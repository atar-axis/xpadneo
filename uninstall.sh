#!/bin/bash

set -e

cd "$(dirname "$0")"
source "lib/options.sh"
source "lib/installer.sh"

echo "* unloading current driver module"
modprobe -r hid_xpadneo || true

echo "* looking for registered instances"
echo "found ${#INSTALLED[@]} registered instance(s) on your system"

# NOTE: overrides VERSION from lib/installer.sh but we don't need it
for VERSION in "${INSTALLED[@]}"; do
    make "${MAKE_OPTS[@]}" VERSION="${VERSION}" uninstall
done
