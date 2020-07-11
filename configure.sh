#!/bin/bash
# Written by CodeCanna, refined by atar-axis

# shellcheck disable=SC1090
source "$(dirname "$0")/lib/installer.sh"

set -o posix

NAME="$0"

# Define Variables
MODULE="/sys/module/hid_xpadneo/"
PARAMS="/sys/module/hid_xpadneo/parameters"
CONF_FILE=$(grep -sl '^options hid_xpadneo' /etc/modprobe.d/*.conf | tail -1)
: "${CONF_FILE:="/etc/modprobe.d/99-xpadneo-options.conf"}"

# Use getopt NOT getopts to parse arguments.
OPTS=$(getopt -n "$NAME" -o hz:d:f:v:r: -l help,version,combined-z-axis:,disable-ff:,rumble-attenuation: -- "$@")



## Print Help ##
function display_help {
  cat ./docs/config_help
}

## Print Version ##
function display_version {
  echo "$NAME: Installed xpadneo version: $INSTALLED"
}

## Parameter Validation ##
function check_param {
    key=$1
    value=$2

    case $key in
    "disable_ff")
        if [[ "$value" -gt 3 ]] || [[ "$value" -lt 0 ]];
        then
            echo "$NAME: $key: Invalid value! Value must be between 0 and 3."
            exit 1
        fi
        ;;
    "rumble_attenuation")
        if [[ "$value" -gt 100 ]] || [[ "$value" -lt 0 ]];
        then
            echo "$NAME: $key: Invalid value! Value must be between 0 and 100."
            exit 1
        fi
        ;;
    "combined_z_axis")
        if [[ "$value" != "y" ]] && [[ "$value" != "n" ]];
        then
            echo "$NAME: $key: Invalid value! Value must be 'y' or 'n'."
            exit 1
        fi
        ;;
    *)
        # key not known, should not be possible
        exit 2
        ;;
    esac
}

## Parameter Setting Helpers ##
function set_modprobe_param {
  sed -i "/^[[:space:]]*options[[:space:]]\+hid_xpadneo/s/$1=[^[:space:]]*/$1=$2/g" "$CONF_FILE"
}

function set_sysfs_param {
  echo "$2" > "$PARAMS/$1"
}


## Parameter Setting ##
function set_param {

  key=$1
  value=$2

  # check for valid parameters first
  check_param "$key" "$value"

  # edit sysfs parameter if module is inserted
  if [[ -d "$MODULE" ]];
  then
    echo "$NAME: Module inserted - writing to $PARAMS"
    if ! set_sysfs_param "$key" "$value";
    then
      echo "$NAME: ERROR! Could not write to $PARAMS!"
      exit 1
    fi
  fi

  # edit modprobe config file
  if ! set_modprobe_param "$key" "$value";
  then
    echo "$NAME: ERROR! Could not write to $CONF_FILE!"
    exit 1
  fi
  echo "$NAME: $key: parameter written to $CONF_FILE"

}

## Argument Parsing ##
function parse_args {
  if ! grep -sq 'options hid_xpadneo' "${CONF_FILE}";
  then
    # If line doesn't exist echo all of the defaults.
    mkdir -p "$(dirname "${CONF_FILE}")"
    touch "${CONF_FILE}"
    echo "options hid_xpadneo disable_ff=0 disable_deadzones=0 rumble_attenuation=0 trigger_rumble_mode=0 combined_z_axis=n" >> "$CONF_FILE"
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

      -f | --disable-ff)
        key='disable_ff'
        value="${2#*=}"
        set_param "$key" "$value"
        shift 2
        ;;

      -r | --rumble-attenuation)
        key='rumble_attenuation'
        value="${2#*=}"
        set_param "$key" "$value"
        shift 2
        ;;

      -z | --combined-z-axis)
        key='combined_z_axis'
        value="${2#*=}"
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


### Main Entry Point ###

PARAMETERS=( "$@" )

# Check if xpadneo is installed
if [[ -z "$INSTALLED" ]];
then
    echo "$NAME: Installation not found. Did you run ./install.sh?"
    exit 1
fi

# Check if the correct version is installed
if [[ "$VERSION" != "$INSTALLED" ]];
then
    echo "$NAME: Your version of xpadneo seems to be out of date."
    echo "$NAME: Please run ./update.sh from the git directory to update to the latest version."
    echo "$NAME: Installed version is $INSTALLED, but this script is for version $VERSION"
    exit 2
fi

parse_args "${PARAMETERS[@]}"

