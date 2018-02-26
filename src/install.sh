#!/bin/bash

hid_xpadneo=$(modinfo -n hid-xpadneo 2> /dev/null)
hid=$(modinfo -n hid 2> /dev/null)
bluetooth=$(modinfo -n bluetooth 2> /dev/null)

extramodules="/lib/modules/"$(uname -r)"/extramodules"


# install xpadneo

if [ "$hid_xpadneo" == "" ]
then
  echo "* install xpadneo"
  sudo cp -f ./hid-xpadneo.ko "$extramodules"
else
  echo "* replace xpadneo"
  sudo mv "$hid_xpadneo" "$hid_xpadneo"_bak
  sudo cp -f ./hid-xpadneo.ko $(dirname "$hid_xpadneo")
fi

# install hid

if [ "$hid" == "" ]
then
  echo "* install hid"
  sudo cp -f ./hid.ko "$extramodules"
else
  echo "* replace hid"
  sudo mv "$hid" "$hid"_bak
  sudo cp -f ./hid.ko $(dirname "$hid")
fi

# install bluetooth

if [ "$bluetooth" == "" ]
then
  echo "* install bluetooth"
  sudo cp -f ./bluetooth.ko "$extramodules"
else
  echo "* replace bluetooth"
  sudo mv "$bluetooth" "$bluetooth"_bak
  sudo cp -f ./bluetooth.ko $(dirname "$bluetooth")
fi


sudo depmod


sudo modprobe hid-xpadneo
sudo modprobe hid
sudo modprobe bluetooth
