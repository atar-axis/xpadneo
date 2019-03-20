#!/bin/bash


# --- FUNCTIONS ---

enable_ff ()
{
	echo "* enabling ff... "

	echo "N" > /sys/module/hid_xpadneo/parameters/disable_ff
	rm -f /etc/modprobe.d/99-xpadneo-bluetooth.conf
}


disable_ff ()
{
	echo "* disabling ff... "

	echo "Y" > /sys/module/hid_xpadneo/parameters/disable_ff
	echo "options hid_xpadneo disable_ff=1" > /etc/modprobe.d/xpadneo.conf
}


# --- PROTECTION ---

if [[ $EUID != 0 ]]; then
	echo "ERROR: You need superuser privileges to toggle vibration, please run toggle_ff.sh via sudo!"
	exit -3
fi


if [[ $# == 0 ]]; then
	PARAM="toggle"

elif [[ ( $# == 1 ) && ( ! -z $1 ) && ( $1 == "enable" || $1 == "disable" ) ]]; then
	PARAM=$1

else
	echo "Usage: sudo ./toggle_ff.sh [[enable|disable]]"
	echo "Default behaviour is toggling the current force-feedback settings"
	exit -2
fi

if [[ ! -f /sys/module/hid_xpadneo/parameters/disable_ff ]]; then
	echo "ERROR: file /sys/module/hid_xpadneo/parameters/disable_ff does not exist"
	exit -4
fi


# --- ACTUAL CODE ---

DISABLED_BOOT=0
DiSABLED_TMP=0

if grep -q "disable_ff=1" /etc/modprobe.d/xpadneo.conf 2>/dev/null; then
	echo "* ff is currently disabled by modprobe parameter"
	DISABLED_BOOT=1
fi

if grep -q "Y" /sys/module/hid_xpadneo/parameters/disable_ff 2>/dev/null; then
	echo "* ff is currently disabled by sysfs parameter"
	DiSABLED_TMP=1
fi


if [[ $PARAM == "toggle" ]]; then
	if [[ "$DiSABLED_TMP" -eq 1 || "$DISABLED_BOOT" -eq 1 ]]; then
		echo "* ff is disabled"
		enable_ff
	else
		echo "* ff is enabled"
		disable_ff
	fi

elif [[ $PARAM == "disable" ]]; then
	disable_ff

elif [[ $PARAM == "enable" ]]; then
	enable_ff

else
	echo "ERROR: Unexpected"
	exit -1
fi


echo "* done"
exit 0
