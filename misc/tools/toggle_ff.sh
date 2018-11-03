#!/bin/bash


# --- FUNCTIONS ---

enable_ff ()
{
	echo "* force feedback is disabled, enabling... "

	echo "N" > /sys/module/hid_xpadneo/parameters/disable_ff
	rm -f /etc/modprobe.d/xpadneo.conf
}


disable_ff ()
{
	echo "* force feedback is enabled, disabling... "

	echo "Y" > /sys/module/hid_xpadneo/parameters/disable_ff
	echo "options hid_xpadneo disable_ff=1" > /etc/modprobe.d/xpadneo.conf
}


# --- PROTECTION ---

if (( EUID != 0 )); then
	echo "ERROR: You need superuser privileges to toggle vibration, please run toggle-vibration.sh via sudo!"
	exit -3
fi


if [[ "$#" -eq 0 ]]; then
	PARAM="toggle"
elif [[ ( "$#" -eq 1 ) && ( ! -z $1 ) && ( $1 == "enable" || ! $1 == "disable" ) ]]; then
	PARAM=$1
else
	echo "Usage: sudo ./toggle_ff.sh [enable|disable]"
	echo "Default behaviour is toggling the current force-feedback settings"
	exit -2
fi

if [ ! -f /sys/module/hid_xpadneo/parameters/disable_ff ]; then
	echo "ERROR: file /sys/module/hid_xpadneo/parameters/disable_ff does not exist"
	exit -4
fi


# --- ACTUAL CODE ---

DISABLED_BOOT=0
DiSABLED_TMP=0

if grep -q "disable_ff=1" /etc/modprobe.d/xpadneo.conf 2>/dev/null; then
	echo "* ff currently disabled by modprobe parameter"
	DISABLED_BOOT=1
fi

if grep -q "Y" /sys/module/hid_xpadneo/parameters/disable_ff 2>/dev/null; then
	echo "* ff currently disabled by sysfs parameter"
	DiSABLED_TMP=1
fi


if [[ $PARAM == "toggle" ]]; then
	if [[ "$DiSABLED_TMP" -eq 1 || "$DISABLED_BOOT" -eq 1 ]]; then
		enable_ff
	else
		disable_ff
	fi

elif [[ $option == "disable" ]]; then
	disable_ff

elif [[ $option == "enable" ]]; then
	enable_ff

else
	echo "ERROR: Unexpected"
	exit -1
fi


echo "* done"
exit 0
