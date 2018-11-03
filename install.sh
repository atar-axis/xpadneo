#!/bin/bash

# exit immediately if one command fails
set -e 

source VERSION

if [[ $EUID != 0 ]]; then
  echo "ERROR: You most probably need superuser privileges to install new modules, please run me via sudo!"
  exit -3
fi


echo "* copying module into /usr/src"
cp --recursive $PWD/hid-xpadneo/ /usr/src/hid-xpadneo-$VERSION

echo "* adding module to DKMS"
dkms add -m hid-xpadneo -v $VERSION

echo "* installing module (using DKMS)"
dkms install -m hid-xpadneo -v $VERSION
