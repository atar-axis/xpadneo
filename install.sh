#!/bin/bash
VERSION="0.2.2"

echo "* copying module into /usr/src"
sudo cp --recursive $PWD/hid-xpadneo-$VERSION/ /usr/src/

echo "* adding module to DKMS"
sudo dkms add hid-xpadneo -v $VERSION

echo "* installing module (using DKMS)"
sudo dkms install hid-xpadneo -v $VERSION
