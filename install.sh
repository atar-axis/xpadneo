#!/bin/bash

# exit immediately if one command fails
set -e 

if [[ $EUID != 0 ]]; then
  echo "ERROR: You most probably need superuser privileges to install new modules, please run me via sudo!"
  exit -3
fi

VERSION=$(cat VERSION)

# backup original files
cp hid-xpadneo/dkms.conf hid-xpadneo/dkms.conf_bck
cp hid-xpadneo/src/hid-xpadneo.c hid-xpadneo/src/hid-xpadneo.c_bck

echo "* replacing version string if necessary"
sed -i 's/PACKAGE_VERSION="@DO_NOT_CHANGE@"/PACKAGE_VERSION="'$VERSION'"/g' hid-xpadneo/dkms.conf
sed -i 's/#define DRV_VER "@DO_NOT_CHANGE@"/#define DRV_VER "'$VERSION'"/g' hid-xpadneo/src/hid-xpadneo.c


INSTALLED=$(dkms status 2>/dev/null | grep '^hid-xpadneo,' 2>/dev/null | sed -E 's/^hid-xpadneo, ([0-9]+.[0-9]+.[0-9]+).*installed/\1/')

if [[ -z "$INSTALLED" ]]; then

    echo "* copying module into /usr/src"
    cp --recursive $PWD/hid-xpadneo/ /usr/src/hid-xpadneo-$VERSION

    echo "* adding module to DKMS"
    dkms add -m hid-xpadneo -v $VERSION

    echo "* installing module (using DKMS)"
    dkms install -m hid-xpadneo -v $VERSION

else

    echo "already installed!"
    
fi

# restore original files
mv hid-xpadneo/dkms.conf_bck hid-xpadneo/dkms.conf
mv hid-xpadneo/src/hid-xpadneo.c_bck hid-xpadneo/src/hid-xpadneo.c
