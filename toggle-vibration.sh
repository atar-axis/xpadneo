#!/bin/bash
#This script can be ran without a parameter to toggle, or with a toggle parameter with value "enable" to enable vibration and "disable" to disable vibration.
#Usage: sudo ./toggle-vibration.sh or: sudo ./toggle-vibration.sh enable or: sudo ./toggle-vibration.sh disable
if (( EUID != 0 )); then
  echo "ERROR: You most probably need superuser privileges to toggle vibration, please run toggle-vibration.sh via sudo!"
  exit -3
fi

if [[ ! -z $1 ]]; then
    param=$1
fi

if [[ $param == "enable" ]] || [[ $param == "disable" ]]; then
    option=$param
else
    if [ ! -f /etc/modprobe.d/xpadneo.conf ]; then
        option="disable"
    else 
        option="enable"
    fi
fi

if [[ $option == "disable" ]]; then
    echo "Vibration exists, now disabling..."
    echo "options hid_xpadneo disable_ff=1" | sudo tee /etc/modprobe.d/xpadneo.conf
    echo "Vibration has been disabled."
else 
    echo "Vibration doesn't exist, now enabling..."
    sudo rm /etc/modprobe.d/xpadneo.conf
    echo "Vibration has been enabled."
fi