#!/bin/bash

if [[ $EUID != 0 ]]; then
  echo "ERROR: You most probably need superuser privileges to uninstall modules, please run me via sudo!"
  exit -3
fi


echo "* unloading current driver module"
modprobe -r hid_xpadneo

echo "* looking for registered instances"
VER=($(dkms status 2>/dev/null | grep '^hid-xpadneo' 2>/dev/null | sed -E 's/^hid-xpadneo, ([0-9]+.[0-9]+.[0-9]+).*/\1/'))
echo "found ${#VER[@]} registered instance(s) on your system"


for instance in "${VER[@]}"
do
    echo "* $instance"

    echo "  * uninstalling and removing $instance from DKMS"
    dkms remove -m hid-xpadneo -v $instance --all

    echo "  * removing $instance folder from /usr/src"
    rm --recursive /usr/src/hid-xpadneo-$instance/
done
