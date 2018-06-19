#!/bin/bash

# exit immediately if one command fails
set -e 

source VERSION

echo "* copying module into /usr/src"
sudo cp --recursive $PWD/hid-xpadneo/ /usr/src/hid-xpadneo-$VERSION

echo "* adding module to DKMS"
sudo dkms add -m hid-xpadneo -v $VERSION

echo "* installing module (using DKMS)"
sudo dkms install -m hid-xpadneo -v $VERSION
