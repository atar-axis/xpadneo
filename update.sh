#!/bin/bash

# exit immediately if one command fails
set -e 

if [[ $EUID != 0 ]]; then
  echo "ERROR: You most probably need superuser privileges to update modules, please run me via sudo!"
  exit -3
fi


echo "* uninstalling the outdated module"
./uninstall.sh

echo "* updating the git clone"
git pull

echo "* installing updated module"
./install
