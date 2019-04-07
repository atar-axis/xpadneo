#!/bin/bash
# Version 0.0.3
# Written by CodeCanna

set -o posix
#set -x
# Define Variables
VERSION=$(cat ./VERSION)
SOURCE_PATH=$(find /usr/src -mindepth 1 -maxdepth 1 -type d -name "hid-xpadneo*")
DETECTED_VERSION=($(dkms status 2>/dev/null | grep '^hid-xpadneo,' 2>/dev/null | sed -E 's/^hid-xpadneo, ([0-9]+.[0-9]+.[0-9]+).*/\1/'))

MODULE=/sys/module/hid_xpadneo/
PARAMS=/sys/module/hid_xpadneo/parameters
CONF_FILE=$(find /etc/modprobe.d/ -mindepth 1 -maxdepth 1 -type f -name "*xpadneo*")

NAME="$0"
OPTS=$(getopt -n "$NAME" -o hz:d:f:v:r: -l help,version,combined-z-axis:,debug-level:,disable-ff:,fake-dev-version:,trigger-rumble-damping: -- "$@")  # Use getopt NOT getopts to parse arguments.

echo "$VERSIONS"
exit 0

# Check if ran as root
if [[ "$EUID" -ne 0 ]];
then
  echo "This script must be ran as root!"
  exit 1
fi

function main {
  check_version "$@"
}

# Check if version is out of date.
function check_version {
  if [[ "$VERSION" != "$DETECTED_VERSION" ]];
  then
    echo "$NAME:Your version of xpadneo seems to be out of date."
    echo "$NAME:Please run ./update.sh from the git directory to update to the latest version."
    echo "$DETECTED_VERSION"
    exit 1
  else
    is_installed "$@"
  fi
}

# Check if xpadneo is Installed
function is_installed {
  if [[ ! -d "$SOURCE_PATH" ]];
  then
    echo "Installation not found.  Did you run ./install.sh?"
    exit
  else
    parse_args "$@" # Function parse_args()
  fi
}

function set_option {
  sed -i "/^[[:space:]]*options[[:space:]]\+hid_xpadneo/s/$key=[^[:space:]]*/$key=$value/g" "$CONF_FILE"
}

function set_param {
  PARAM="$PARAMS/$key"
  echo "$value" > "$PARAM"
}

### Arg Functions ###

## Help ##
function display_help {
  cat ./docs/config_help
}

## Version ##
function display_version {
  echo "Xpadneo Version: $DETECTED_VERSION"
}

## Set debug level ##
function debug_level {
  if [[ "$value" -ne 0 ]] && [[ "$value" -ne 1 ]] && [[ "$value" -ne 2 ]] && [[ "$value" -ne 3 ]];
  then
    echo "Invalid Debug Level! Number must be between 0 and 3."
    exit 1
  fi

  # If module is inserted edit parameters.
  if [[ -d "$MODULE" ]];
  then
    echo "$NAME:Module inserted writing to $PARAMS"
    if ! set_param "$key" "$value";  # Write to $PARAMS/debug_level
    then
      echo "$NAME:ERROR! Could not write to $PARAMS!"
      exit 1
    fi
  fi

  if ! set_option "$key" "$value";
  then
    echo "$NAME:ERROR! Could not write to $CONF_FILE"
    exit 1
  fi
  echo "$NAME: Config written to $CONF_FILE"
}

## Set FF ##
function disable_ff {
  #if [[ "${VALUES[1]}" != "y" ]] && [[ "${VALUES[1]}" != "n" ]];
  if [[ "$value" != "y" ]] && [[ "$value" != "n" ]];
  then
    echo "$NAME:Invalid Entry! please enter 'y' or 'n'."
    exit 1
  fi

  # If module is inserted edit parameters.
  if [[ -d "$MODULE" ]];
  then
    echo "$NAME:Module is inserted writing to $PARAMS."
    if ! set_param "$key" "$value";
    then
      echo "$NAME:ERROR! Could not write to $PARAMS!"
      exit 1
    fi
  fi

  if ! set_option "$key" "$value";
  then
    echo "$NAME:ERROR! Could not write to $CONF_FILE"
    exit 1
  fi
  echo "$NAME: Config written to $CONF_FILE"
}

## Set Trigger Damping ##
function trigger_damping {
  if [[ "$value" -gt 256 ]] || [[ "$value" -lt 1 ]];
  then
    echo "Invalid Entry! Value must be between 1 and 256."
    exit 1
  fi

  # If module is inserted edit parameters.
  if [[ -d "$MODULE" ]];
  then
    echo "$NAME:Module is inserted writing to $PARAMS."
    if ! set_param "$key" "$value";
    then
      echo "$NAME:ERROR! Could not write to $PARAMS"
      exit 1
    fi
  fi

  if ! set_option "$key" "$value";
  then
    echo "$NAME:ERROR! Could not write to $CONF_FILE"
    exit 1
  fi
  echo "$NAME: Config written to $CONF_FILE"
}

## Set Fake Dev Version ##
function fkdv {
  if [[ "$value" -gt 65535 ]] || [[ "$value" -lt 1 ]];
  then
    echo "Invalid Entry! Value must be between 1 and 65535."
    exit 1
  fi

  # If module is inserted edit parameters.
  if [[ -d "$MODULE" ]];
  then
    echo "$NAME:Module is inserted writing to $PARAMS."
    if ! set_param "$key" "$value";
    then
      echo "$NAME:ERROR! Could not write to $PARAMS."
      exit 1
    fi
  fi

  if ! set_option "$key" "$value";
  then
    echo "$NAME:ERROR! Could not write to $CONF_FILE!"
    exit 1
  fi
  echo "$NAME: Config written to $CONF_FILE"
}

## Combined Z Axis ##
function z_axis {
  if [[ "$value" != "y" ]] && [[ "$value" != "n" ]];
  then
    echo "NAME:Invalid Entry! please enter 'y' or 'n'."
    exit 1
  fi

  # If module is inserted edit parameters.
  if [[ -d $MODULE ]];
  then
    echo "NAME:Module is inserted writing to $PARAMS."
    if ! set_param "$key" "$value";
    then
      echo "$NAME:ERROR! Could not write to $PARAMS!"
      exit 1
    fi
  fi

  if ! set_option "$key" "$value";
  then
    echo "$NAME:ERROR! Could not write to $CONF_FILE"
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
        debug_level "$key" "$value"
        shift 2
        ;;

      -f | --disable-ff)
        key='disable_ff'
        value="${2#*=}"
        disable_ff "$key" "$value"
        shift 2
        ;;

      -r | --trigger-rumble-damping)
        key='trigger_rumble_damping'
        value="${2#*=}"
        trigger_damping "$key" "$value"
        shift 2
        ;;

      -v | --fake-dev-version)
        key='fake_dev_version'
        value="${2#*=}"
        fkdv "$key" "$value"
        shift 2
        ;;

      -z | --combined-z-axis)
        key='combined_z_axis'
        value="${2#*=}"
        z_axis "$key" "$value"
        shift 2
        ;;

      --)
        shift
        break
        ;;

      *)
        echo "$NAME:Invalid option"
        display_help
        exit 1
        ;;
    esac
  done
}

main "$@"
