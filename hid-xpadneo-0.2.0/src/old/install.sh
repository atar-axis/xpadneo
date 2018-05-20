#!/bin/bash

# Check if current user is root
if [[ $EUID -ne 0 ]]; then
   echo "This script must be run as root" 1>&2
   exit 1
fi

hid_xpadneo=$(modinfo -n hid-xpadneo 2> /dev/null)
hid=$(modinfo -n hid 2> /dev/null)
bluetooth=$(modinfo -n bluetooth 2> /dev/null)

extramodules="/lib/modules/"$(uname -r)"/extramodules/"


sudo [ ! -d "$extramodules"_bak ] && sudo mkdir --parents $extramodules

# install xpadneo
if [ "$hid_xpadneo" == "" ]
then
  echo "** install xpadneo"
  sudo cp -f ./hid-xpadneo.ko "$extramodules"
else
  echo "** replace xpadneo"
  sudo [ ! -f "$hid_xpadneo"_bak ] && sudo mv "$hid_xpadneo" "$hid_xpadneo"_bak
  sudo cp -f ./hid-xpadneo.ko $(dirname "$hid_xpadneo")
fi

# install hid
if [ "$hid" == "" ]
then
  echo "** install hid"
  sudo cp -f ./hid.ko "$extramodules"
else
  echo "** replace hid"
  sudo [ ! -f "$hid"_bak ] && sudo mv "$hid" "$hid"_bak
  sudo cp -f ./hid.ko $(dirname "$hid")
fi

# install bluetooth
if [ "$bluetooth" == "" ]
then
  echo "** install bluetooth"
  sudo cp -f ./bluetooth.ko "$extramodules"
else
  echo "** replace bluetooth"
  sudo [ ! -f "$bluetooth"_bak ] && sudo mv "$bluetooth" "$bluetooth"_bak
  sudo cp -f ./bluetooth.ko $(dirname "$bluetooth")
fi

# Register new modules if not yet
sudo depmod

# Load modules if not yet
sudo modprobe hid-xpadneo
sudo modprobe hid
sudo modprobe bluetooth

# Check if modules are installed correctly
if [[ $(cksum < "$hid") != $(cksum < ./hid.ko) \
   || $(cksum < "$bluetooth") != $(cksum < ./bluetooth.ko) \
   || $(cksum < "$hid_xpadneo") != $(cksum < ./hid-xpadneo.ko) ]]
then
  echo something went wrong, checksums are not correct
  exit 1
else 
  echo "* installed/replaced successfully"
fi;

# Update initramfs on Ubuntu
if [[ "$(uname -v | grep -o Ubuntu)" != "" ]]
then
  # fix for non-working mouse after update-initramfs
  # TODO: use patch instead of append it blind
  sudo /bin/bash -c "echo 'hid' >> /etc/initramfs-tools/modules"
  sudo /bin/bash -c "echo 'usbhid' >> /etc/initramfs-tools/modules"
  sudo /bin/bash -c "echo 'hid_generic' >> /etc/initramfs-tools/modules"
  sudo /bin/bash -c "echo 'ohci_pci' >> /etc/initramfs-tools/modules"
  # update initramfs (reload hid.ko and bluetooth.ko to ram at startup)
  sudo update-initramfs -u -k all
fi;
