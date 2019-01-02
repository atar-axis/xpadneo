#!/bin/bash

if [[ $EUID != 0 ]]; then
  echo "ERROR: You most probably need superuser privileges to update modules, please run me via sudo!"
  exit -3
fi

LATEST=$(wget --quiet --output-document=/dev/stdout "https://raw.githubusercontent.com/atar-axis/xpadneo/master/VERSION" | sed -E 's/.*"([0-9]*.[0-9]*.[0-9]*)".*/\1/')
source VERSION


if [[ $LATEST == $VERSION ]]
then
    echo "You already have the latest version! Yay."
else
    echo "* uninstalling the outdated module"
    ./uninstall.sh

    echo "* updating the cloned git directory"
    git pull

    echo "* installing updated module"
    ./install.sh
fi
