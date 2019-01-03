#!/bin/bash

# exit immediately if one command fails
set -e 

source VERSION

if [[ $EUID != 0 ]]; then
  echo "ERROR: You most probably need superuser privileges to install new modules, please run me via sudo!"
  exit -3
fi


echo "* replacing version string if necessary"
sed -i 's/PACKAGE_VERSION="DUMMY"/PACKAGE_VERSION="'$VERSION'"/g' hid-xpadneo/dkms.conf
sed -i 's/#define DRV_VER "DUMMY"/#define DRV_VER "'$VERSION'"/g' hid-xpadneo/src/hid-xpadneo.c


INSTALLED=$(dkms status 2>/dev/null | grep '^hid-xpadneo' 2>/dev/null | sed -E 's/^hid-xpadneo, ([0-9]+.[0-9]+.[0-9]+).*installed/\1/')

if [[ -z "$INSTALLED" ]]; then

    echo "* copying module into /usr/src"
    cp --recursive $PWD/hid-xpadneo/ /usr/src/hid-xpadneo-$VERSION

    echo "* adding module to DKMS"
    dkms add -m hid-xpadneo -v $VERSION

    echo "* installing module (using DKMS)"
    dkms install -m hid-xpadneo -v $VERSION

else

    echo "already installed"
    
fi
