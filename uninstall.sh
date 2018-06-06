#!/bin/bash
VERSION="0.2.3"

echo "* uninstalling and removing from DKMS"
sudo dkms remove hid-xpadneo -v $VERSION --all

echo "* removing folder from /usr/src"
sudo rm --recursive /usr/src/hid-xpadneo-$VERSION/
