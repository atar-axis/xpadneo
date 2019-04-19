#!/bin/bash

# by atar-axis (dollinger.florian@gmx.de)
# vercomp() by Dennis Williamson
# https://stackoverflow.com/questions/4023830/how-to-compare-two-strings-in-dot-separated-version-format-in-bash


vercomp () {
    if [[ $1 == $2 ]]
    then
        return 0
    fi
    local IFS=.
    local i ver1=($1) ver2=($2)
    # fill empty fields in ver1 with zeros
    for ((i=${#ver1[@]}; i<${#ver2[@]}; i++))
    do
        ver1[i]=0
    done
    for ((i=0; i<${#ver1[@]}; i++))
    do
        if [[ -z ${ver2[i]} ]]
        then
            # fill empty fields in ver2 with zeros
            ver2[i]=0
        fi
        if ((10#${ver1[i]} > 10#${ver2[i]}))
        then
            return 1
        fi
        if ((10#${ver1[i]} < 10#${ver2[i]}))
        then
            return 2
        fi
    done
    return 0
}


# kernel versions       bind supported          hid_generic greedy      hid_microsoft support   gamepad     method
# ---------------       --------------          ------------------      ---------------------   -------     ------
# [  ... - 4.14 [       no                      yes                     no                      *           1: add, hid_generic
# [ 4.14 - 4.16 [       yes                     yes                     no                      *           2: bind, hid_generc
# [ 4.16 - 4.20 [       yes                     no                      no                      *           none
# [ 4.20 - ...  ]       yes                     no                      yes                     02FD        3: bind, microsoft
#                                                                                               02E0        none


krnl=$(uname -r | sed -E 's/^([0-9]+.[0-9]+).*/\1/')

$(vercomp $krnl "4.20") 
if [[ $? -le 1 ]]; then
    # kernel version is from 4.20 upwards
    echo "3"
    exit 0
fi

$(vercomp $krnl "4.14")
if [[ $? -le 1 ]]; then
    # kernel version is from 4.14 upwards
    # bind uevents are supported
    echo "2"
    exit 0
fi

echo "1"
exit 0
