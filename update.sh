#!/bin/bash

if [[ $EUID != 0 ]]; then
  echo "ERROR: You most probably need superuser privileges to update modules, please run me via sudo!"
  exit -3
fi

LATEST=$(wget --quiet --output-document=/dev/stdout "https://raw.githubusercontent.com/atar-axis/xpadneo/master/VERSION" | sed -E 's/.*"([0-9]*.[0-9]*.[0-9]*)".*/\1/')
INSTALLED=$(dkms status 2>/dev/null | grep '^hid-xpadneo' 2>/dev/null | sed -E 's/^hid-xpadneo, ([0-9]+.[0-9]+.[0-9]+).*installed/\1/')
VERSION=$(cat VERSION)
IS_GIT=$(git rev-parse --is-inside-work-tree 2>/dev/null)


if [[ $LATEST == $VERSION ]]; then

    echo "Looks like the repo is up to date, fine!"
    
    if [[ $LATEST == $INSTALLED ]]; then
        echo "You have already installed the latest version! Yay."
    else
        echo "* uninstalling outdated modules"
        ./uninstall.sh
            
        echo "* installing latest version"
        ./install.sh
    fi
    
else
    
    if [[ $IS_GIT == "true" ]]; then
        echo "Please update this directory by running 'git reset --hard' and 'git pull', afterwards run this script again"
    else
        echo "Please update this directory by downloading the latest version from https://github.com/atar-axis/xpadneo/archive/master.zip"
    fi
    
    exit -4

fi


