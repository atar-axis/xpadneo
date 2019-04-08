#!/bin/bash
# Version 0.0.3
# Written by CodeCanna, refined by atar-axis

set -o posix

# Define Variables
VERSION="$(cat ./VERSION)"
INSTALLED_VERSION="$(dkms status 2>/dev/null | grep '^hid-xpadneo,' 2>/dev/null | sed -E 's/^hid-xpadneo, ([0-9]+.[0-9]+.[0-9]+).*/\1/')"

MODULE="/sys/module/hid_xpadneo/"
PARAMS="/sys/module/hid_xpadneo/parameters"
CONF_FILE="$(find /etc/modprobe.d/ -mindepth 1 -maxdepth 1 -type f -name "*xpadneo*")"

NAME="$0"
# Use getopt NOT getopts to parse arguments.
OPTS="$(getopt -n $NAME -o hz:d:f:v:r: -l help,version,combined-z-axis:,debug-level:,disable-ff:,fake-dev-version:,trigger-rumble-damping: -- $@)"

# Check if ran as root
if [[ "$EUID" -ne 0 ]];
then
  echo "This script must be ran as root!"
  exit 1
fi

### Arg Functions ###

## Help ##
function display_help {
  cat ./docs/config_help
}

## Version ##
function display_version {
  echo "Xpadneo Version: $INSTALLED_VERSION"
}


function check_param {
    key=$1
    value=$2
    
    case $key in
    "debug_level")
        if [[ "$value" -ne 0 ]] && [[ "$value" -ne 1 ]] && [[ "$value" -ne 2 ]] && [[ "$value" -ne 3 ]];
        then
            echo "$NAME: $key: Invalid value! Number must be between 0 and 3."
            exit 1
        fi
        ;;
    "disable_ff")
        if [[ "$value" != "y" ]] && [[ "$value" != "n" ]];
        then
            echo "$NAME: $key: Invalid value! please enter 'y' or 'n'."
            exit 1
        fi
        ;;
    "trigger_rumble_damping")
        if [[ "$value" -gt 256 ]] || [[ "$value" -lt 1 ]];
        then
            echo "$NAME: $key: Invalid value! Value must be between 1 and 256."
            exit 1
        fi
        ;;
    "fake_dev_version")
        if [[ "$value" -gt 65535 ]] || [[ "$value" -lt 1 ]];
        then
            echo "$NAME: $key: Invalid value! Value must be between 1 and 65535."
            exit 1
        fi
        ;;
    "combined_z_axis")
        if [[ "$value" != "y" ]] && [[ "$value" != "n" ]];
        then
            echo "$NAME: $key: Invalid value! please enter 'y' or 'n'."
            exit 1
        fi
        ;;
    *)
        # key not known, should not be possible
        exit 2
        ;;
    esac
}


function set_modprobe_param {
  sed -i "/^[[:space:]]*options[[:space:]]\+hid_xpadneo/s/$1=[^[:space:]]*/$1=$2/g" "$CONF_FILE"
}

function set_sysfs_param {
  echo "$2" > "$PARAMS/$1"
}

function set_param {

  # edit sysfs parameter if module is inserted
  if [[ -d "$MODULE" ]];
  then
    echo "$NAME: Module inserted writing to $PARAMS"
    if ! set_sysfs_param "$key" "$value";  # Write to $PARAMS/debug_level
    then
      echo "$NAME: ERROR! Could not write to $PARAMS!"
      exit 1
    fi
  fi
  
  # edit modprobe config file
  if ! set_modprobe_param "$key" "$value";
  then
    echo "$NAME: ERROR! Could not write to $CONF_FILE"
    exit 1
  fi
  echo "$NAME: Config written to $CONF_FILE"
}

## Parse Arguments ##
function parse_args {
  LINE_EXISTS=$(grep 'options hid_xpadneo' "$CONF_FILE")
  if [[ -z "$LINE_EXISTS" ]];
  then
    # If line doesn't exist echo all of the defaults.
    echo "options hid_xpadneo debug_level=0 disable_ff=n trigger_rumble_damping=4 fake_dev_version=4400 combined_z_axis=n" >> "$CONF_FILE"
  fi

  if [[ $1 == "" ]];
  then
    display_help
    exit 0
  fi

  eval set -- "$OPTS"

  while true;
  do
    case "$1" in
      -h | --help)
        display_help
        shift
        ;;

      --version)
        display_version
        shift
        ;;

      -d | --debug-level)
        key='debug_level'
        value="${2#*=}"
        check_param "$key" "$value"
        set_param "$key" "$value"
        shift 2
        ;;

      -f | --disable-ff)
        key='disable_ff'
        value="${2#*=}"
        check_param "$key" "$value"
        set_param "$key" "$value"
        shift 2
        ;;

      -r | --trigger-rumble-damping)
        key='trigger_rumble_damping'
        value="${2#*=}"
        check_param "$key" "$value"
        set_param "$key" "$value"
        shift 2
        ;;

      -v | --fake-dev-version)
        key='fake_dev_version'
        value="${2#*=}"
        check_param "$key" "$value"
        set_param "$key" "$value"
        shift 2
        ;;

      -z | --combined-z-axis)
        key='combined_z_axis'
        value="${2#*=}"
        check_param "$key" "$value"
        set_param "$key" "$value"
        shift 2
        ;;

      --)
        shift
        break
        ;;

      *)
        echo "$NAME: Invalid option"
        display_help
        exit 1
        ;;
    esac
  done
}


# MAIN ENTRY POINT

PARAMETERS=$@

# Check if xpadneo is installed
if [[ -z "$INSTALLED_VERSION" ]];
then
    echo "Installation not found. Did you run ./install.sh?"
    exit 1
fi

# Check if the correct version is installed
if [[ "$VERSION" != "$INSTALLED_VERSION" ]];
then
    echo "$NAME: Your version of xpadneo seems to be out of date."
    echo "$NAME: Please run ./update.sh from the git directory to update to the latest version."
    echo "$NAME: Installed version is $INSTALLED_VERSION, but this script is for version $VERSION"
    exit 2
fi

parse_args "$PARAMETERS"

