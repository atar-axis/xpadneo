#!/bin/bash
source VERSION

echo "* uninstalling and removing from DKMS"
sudo dkms remove -m hid-xpadneo -v $VERSION --all

echo "* removing folder from /usr/src"
sudo rm --recursive /usr/src/hid-xpadneo-$VERSION/
