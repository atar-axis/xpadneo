#!/bin/bash

#if [[ $(modinfo hid-xpadneo |& grep "not found") == "" ]]; then echo "already exists"; else echo "not yet registered"; fi
sudo modprobe -r hid-xpadneo
sudo cp ./hid-xpadneo.ko /lib/modules/4.14.12-1-ARCH/extramodules/hid-xpadneo.ko
sudo cp ./hid.ko /lib/modules/4.14.12-1-ARCH/kernel/drivers/hid/hid.ko
sudo cp ./bluetooth.ko /lib/modules/4.14.12-1-ARCH/kernel/net/bluetooth/bluetooth.ko
sudo modprobe hid-xpadneo
