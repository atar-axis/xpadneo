#!/bin/bash

source VERSION

if [[ $EUID != 0 ]]; then
  echo "ERROR: You most probably need superuser privileges to uninstall modules, please run me via sudo!"
  exit -3
fi


echo "* unloading driver module"
modprobe -r hid_xpadneo

echo "* uninstalling and removing from DKMS"
dkms remove -m hid-xpadneo -v $VERSION --all

echo "* removing folder from /usr/src"
rm --recursive /usr/src/hid-xpadneo-$VERSION/
