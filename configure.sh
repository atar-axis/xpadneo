#!/bin/bash
# configure.sh v0.0.1
# Written by CodeCanna

# Variables n such
VERSION=$(cat ./VERSION)
XPAD_DIR="$(ls /usr/src/ | grep hid-xpadneo)"
SOURCE_PATH=/usr/src/$XPAD_DIR
DETECTED_VERSION=$(echo $SOURCE_PATH | tr -d [:alpha:] | tr -d "-" | tr -d "/")
MODULE=/sys/module/hid_xpadneo/
PARAMS=/sys/module/hid_xpadneo/parameters/

#echo "Path:" $SOURCE_PATH
#echo "Detected Version:" $DETECTED_VERSION
#echo "Directory:" $XPAD_DIR
#echo "Current Version:" $VERSION

# ARGS=("$@")
NAME=$0

# Check if root
if [[ "$EUID" -ne 0 ]];
then
  echo "This script must be ran as root."
  exit -3
fi

if [[ $DETECTED_VERSION != $VERSION ]];
then
  echo "Version out of date!  Please update."
  echo "Run ./update.sh in the xpadneo git directory."
  exit
fi

# Detect installation
# echo "Detecting installation"
if [[ -d "$SOURCE_PATH" ]];
then
  # echo "Installation found in $SOURCE_DIR."
  # echo "Detecting if module is inserted."

  if [[ -d "$MODULE" ]];  # Detect if hid_xpadneo is inserted
  then
    # echo "Module detected"
    echo ""
  else
    echo "Module not inserted."
    echo "Try running 'modprobe hid_xpadneo' and try again."
    exit
  fi
else
  echo "Installation not detected, did you run ./install.sh?"
  exit
fi

###Arg Functions###
# Display Help
function disp_help() {
  cat ./docs/config_help
}

# Combine the triggers (ABS_Z and ABS_RZ) to form a single axis
#function z_axis() {
#  echo "Combined Z Axis"
#}

# Debug Output
#function dbg_level() {
#  echo "Debug Level"
#}

# Disable FF
# function disable_ff() {
#  echo "Disable FF"
#}

# Fake Dev Version
function fkdv() {
  echo "Enter fake dev version from 1 to 65535: "
  read version
  if [[ $version -lt 1 ]];
  then
    echo $NAME:"No value given. Value not changed"
    exit
  elif [[ $version -gt 65535 ]];
  then
    echo $NAME:"Value given too high.  Value not changed."
    exit
#  elif [[ $? -ne 0 ]];
#  then
#    echo $NAME:"Value must be a number. Value not changed."
  else
    echo $version > $PARAMS/fake_dev_version
    echo $NAME:"Fake Dev Version changed to $version"
  fi
}

# Trigger Rumble Dampening
function trig_rd() {
  echo "Enter in a value between 1 and 256: "
  read val
  if [[ $val -lt 1 ]];
  then
    echo $NAME:"No value given.  Value not changed!"
    exit
  elif [[ $val -gt 256 ]];
  then
    echo $NAME:"Value too high. Value not changed!"
    exit
#  elif [[ $? -ne 0 ]];
#  then
#    echo "Value must be a number.  Value not changed!"
  else
    echo $val > $PARAMS/trigger_rumble_damping
    echo "Trigger Rumble Damping set to $val"
  fi
}

# Parse arguments.
for arg in "$@"
do
  case $arg in
    -h | --help)
      #echo "Help is here!"
      disp_help
      ;;

      # Combine Z Axis
    -z=* | -z= | -z | --combined-z-axis=* | --combined-z-axis= | --combined-z-axis)
      #echo "Combined Z Axis"
      if [[ $arg == '-z=true' ]] || [[ $arg == '--combined-z-axis=true' ]];
      then
        echo 1 > $PARAMS/combined_z_axis
        echo $NAME:"Combined Z Axis Enabled"
      elif [[ $arg == '-z=false' ]] || [[ $arg == '--combined-z-axis=false' ]];
      then
        echo 0 > $PARAMS/combined_z_axis
        echo $NAME:"Combined Z Axis Disabled."
      else
        echo $NAME:"No values given set to default 0"
        echo 0 > $PARAMS/combined_z_axis
      fi
      ;;

      # Debug Level
    -d=* | -d= | -d | --debug-level=* | --debug-level= | --debug-level)
      # echo "Debug Level"
      if [[ $arg == '-d=3' ]] || [[ $arg == '--debug-level=3' ]];
      then
        echo 3 > $PARAMS/debug_level
        echo $NAME:"Debug Level = 3"
      elif [[ $arg == '-d=2' ]] || [[ $arg == '--debug-level=2' ]];
      then
        echo 2 > $PARAMS/debug_level
        echo $NAME:"Debug Level = 2"
      elif [[ $arg == '-d=1' ]] || [[ $arg == '--debug-level=1' ]];
      then
        echo 1 > $PARAMS/debug_level
        echo $NAME:"Debug Level = 1"
      else
        echo 0 > $PARAMS/debug_level
        echo $NAME:"Set To Default = 0"
      fi
      ;;

      # Disable FF
    -f=* | -f= | -f | --disable-ff=* | --disable-ff= | --dissable-ff)
      # echo "FF"
      if [[ $arg == '-f=true' ]] || [[ $arg == '--disable-ff=true' ]];
      then
        echo 1 > $PARAMS/disable_ff
        echo $NAME:"FF Disabled"
      elif [[ $arg == '-f=false' ]] || [[ $arg == '--disable-ff=false' ]];
      then
        echo 0 > $PARAMS/disable_ff
        echo $NAME:"FF Enabled."
      else
        echo $NAME:"No values given FF Disabled"
      fi
      ;;

      # Fake Dev Version
    -v | --fake-dev-version)
      # echo "Fake Dev"
      # TODO: Make it so you don't have to use read, see fkdv()
      # ex. ./configure.sh -v=4400
      fkdv
      ;;

      # Trigger Rumble Damping
    -r | --trigger-rumble-damping)
      # echo "Trigger rumble"
      # TODO: Make it so you don't have to use read, see trid_rd()
      # ex. ./configure.sh -r=4
      trig_rd
      ;;

    *)
      echo "Invalid Input"
      exit
  esac
done
