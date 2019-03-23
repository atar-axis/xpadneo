#!/usr/bin/env bash
# Version 0.0.2
# Written by CodeCanna

set -o posix
# Define Variables
VERSION=$(cat ./VERSION)
SOURCE_PATH=$(find /usr/src -mindepth 1 -maxdepth 1 -type d -name "hid-xpadneo*")
DETECTED_VERSION=$(echo "$SOURCE_PATH" | sed "s/[^[:digit:].]//g")

MODULE=/sys/module/hid_xpadneo/
PARAMS=/sys/module/hid_xpadneo/parameters
CONF_FILE=$(find /etc/modprobe.d/ -mindepth 1 -maxdepth 1 -type f -name "*xpadneo*")

NAME=$0
OPTS=$(getopt -n $NAME -o hz:d:f:v:r: -l help,version,combined-z-axis:,debug-level:,disable-ff:,fake-dev-version:,trigger-rumble-damping: -- "$@")  # Use getopt NOT getopts to parse arguments.

# Check if ran as root
if [[ "$EUID" -ne 0 ]];
then
  echo "This script must be ran as root!"
  exit -3
fi

function main {
  check_version
}

# Check if version is out of date.
function check_version {
  if [[ "$VERSION" != "$DETECTED_VERSION" ]];
  then
    echo $NAME:"Your version of xpadneo seems to be out of date."
    echo $NAME:"Please run ./update.sh from the git directory to update to the latest version."
    echo $DETECTED_VERSION
    exit 1
  else
    is_installed
  fi
}

# Check if xpadneo is Installed
function is_installed {
  if [[ ! -d "$SOURCE_PATH" ]];
  then
    echo "Installation not found.  Did you run ./install.sh?"
    exit
  else
    parse_args # Function parse_args()
  fi
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
  value=$2

  if [[ "$value" -ne 0 ]] && [[ "$value" -ne 1 ]] && [[ "$value" -ne 2 ]] && [[ "$value" -ne 3 ]];
  then
    echo "Invalid Debug Level! Number must be between 0 and 3."
    exit 1
  fi

  # If module is inserted edit parameters.
  if [[ -d "$MODULE" ]];
  then
    echo $NAME:"Module inserted writing to $PARAMS"
    echo $value > $PARAMS/debug_level   # Write to parameters
    if [[ $? -ne 0 ]];
    then
      echo $NAME:"Problem writing to $PARAMS"
      exit 1
    fi
  fi

  local LINE_EXISTS=$(grep debug_level $CONF_FILE)

  if [[ $LINE_EXISTS ]];
  then
    sed -i 's/options hid_xpadneo debug_level=.*/options hid_xpadneo debug_level='"$value"'/' $CONF_FILE
    if [[ $? -ne 0 ]];  # Did above command run successfully?
    then
      echo "There was a problem writing to $CONF_FILE"
      exit 1
    else
      echo $NAME:"Debug Level set to $value."
      echo $NAME:"Config written to $CONF_FILE"
    fi
  else
    echo "options hid_xpadneo debug_level=$value" >> $CONF_FILE   # Write to config file.
    if [[ $? -ne 0 ]];
    then
      echo "There was a problem writing to $CONF_FILE."
      exit 1
    else
      echo $NAME:"Debug Level set to $value."
      echo $NAME:"Config written to $CONF_FILE"
    fi
  fi
}

## Set FF ##
function disable_ff {
  value=$2

  if [[ "$value" != "y" ]] && [[ "$value" != "n" ]];
  then
    echo $NAME:"Invalid Entry! please enter 'y' or 'n'."
    exit 1
  fi

  # If module is inserted edit parameters.
  if [[ -d $MODULE ]];
  then
    echo $NAME:"Module is inserted writing to $PARAMS."
    if [[ "$value" == "y" ]];
    then
      echo 0 > $PARAMS/disable_ff
    else
      echo 1 > $PARAMS/disable_ff
    fi

    if [[ $? -ne 0 ]];
    then
      echo $NAME:"ERROR! Problem writing to $PARAMS."
      exit 1
    fi
  fi

  # local LINE_EXISTS=$(cat $CONF_FILE | grep disable_ff)
  local LINE_EXISTS=$(grep disable_ff $CONF_FILE)

  if [[ $LINE_EXISTS ]];
  then
    sed -i 's/options hid_xpadneo disable_ff=.*/options hid_xpadneo disable_ff='"$value"'/' $CONF_FILE
    if [[ $? -ne 0 ]];
    then
      echo $NAME:"There was a problem writing to $CONF_FILE"
      exit 1
    else
      echo $NAME:"Disable_ff set to $value"
      echo $NAME:"Config written to $CONF_FILE."
    fi
  else
    echo "options hid_xpadneo disable_ff=$value" >> $CONF_FILE
    if [[ $? -ne 0 ]];
    then
      echo $NAME:"There was a problem writing to $CONF_FILE."
      exit 1
    else
      echo $NAME:"Disable_ff set to $value."
      echo $NAME:"Config written to $CONF_FILE."
    fi
  fi
}

## Set Trigger Damping ##
function trigger_damping {
  shift
  value=$1

  if [[ "$value" -gt 256 ]] || [[ "$value" -lt 1 ]];
  then
    echo "Invalid Entry! Value must be between 1 and 256."
    exit 1
  fi

  # If module is inserted edit parameters.
  if [[ -d $MODULE ]];
  then
    echo $NAME:"Module is inserted writing to $PARAMS."
    echo $value > $PARAMS/trigger_rumble_damping
    if [[ $? -ne 0 ]];
    then
      echo $NAME:"ERROR! Could not write to $PARAMS."
      exit 1
    fi
  fi

  # local LINE_EXISTS=$(cat $CONF_FILE | grep trigger_rumble_damping)
  local LINE_EXISTS=$(grep trigger_rumble_damping $CONF_FILE)

  if [[ $LINE_EXISTS ]];
  then
    sed -i 's/options hid_xpadneo trigger_rumble_damping=.*/options hid_xpadneo trigger_rumble_damping='"$value"'/' $CONF_FILE
    if [[ $? -ne 0 ]];
    then
      echo $NAME:"There was a problem writing to $CONF_FILE"
      exit 1
    else
      echo $NAME:"Trigger Rumble Damping set to $value."
      echo $NAME:"Config written to $CONF_FILE"
    fi
  else
    echo "options hid_xpadneo trigger_rumble_damping=$value" >> $CONF_FILE
    if [[ $? -ne 0 ]];
    then
      echo $NAME:"There was a problem writing to $CONF_FILE!"
      exit 1
    else
      echo $NAME:"Trigger Rumble Damping set to $value."
      echo $NAME:"Config written to $CONF_FILE"
    fi
  fi
}

## Set Fake Dev Version ##
function fkdv {
  shift
  value=$1

  if [[ "$value" -gt 65535 ]] || [[ "$value" -lt 1 ]];
  then
    echo "Invalid Entry! Value must be between 1 and 65535."
    exit 1
  fi

  # If module is inserted edit parameters.
  if [[ -d $MODULE ]];
  then
    echo $NAME:"Module is inserted writing to $PARAMS."
    echo $value > $PARAMS/fake_dev_version
    if [[ $? -ne 0 ]];
    then
      echo $NAME:"ERROR! Could not write to $PARAMS."
      exit 1
    fi
  fi

  # local LINE_EXISTS=$(cat $CONF_FILE | grep fake_dev_version)
  local LINE_EXISTS=$(grep fake_dev_version $CONF_FILE)

  if [[ $LINE_EXISTS ]];
  then
    sed -i 's/options hid_xpadneo fake_dev_version=.*/options hid_xpadneo fake_dev_version='"$value"'/' $CONF_FILE
    if [[ $? -ne 0 ]];
    then
      echo $NAME:"There was a problem writing to $CONF_FILE"
      exit 1
    else
      echo $NAME:"Fake Dev Version set to $value."
      echo $NAME:"Config written to $CONF_FILE"
    fi
  else
    echo "options hid_xpadneo fake_dev_version=$value" >> $CONF_FILE
    if [[ $? -ne 0 ]];
    then
      echo $NAME:"There was a problem writing to $CONF_FILE!"
      exit 1
    else
      echo $NAME:"Fake Dev Version set to $value."
      echo $NAME:"Config written to $CONF_FILE"
    fi
  fi
}

## Combined Z Axis ##
function z_axis {
  shift
  value=$1

  if [[ "$value" != "y" ]] && [[ "$value" != "n" ]];
  then
    echo $NAME:"Invalid Entry! please enter 'y' or 'n'."
    exit 1
  fi

  # If module is inserted edit parameters.
  if [[ -d $MODULE ]];
  then
    echo $NAME:"Module is inserted writing to $PARAMS."
    if [[ "$value" == "y" ]];
    then
      echo 1 > $PARAMS/combined_z_axis
    else
      echo 0 > $PARAMS/combined_z_axis
    fi

    if [[ $? -ne 0 ]];
    then
      echo $NAME:"ERROR! Problem writing to $PARAMS."
      exit 1
    fi
  fi

  # local LINE_EXISTS=$(cat $CONF_FILE | grep combined_z_axis)
  local LINE_EXISTS=$(grep combined_z_axis $CONF_FILE)

  if [[ $LINE_EXISTS ]];
  then
    sed -i 's/options hid_xpadneo combined_z_axis=.*/options hid_xpadneo combined_z_axis='"$value"'/' $CONF_FILE
    if [[ $? -ne 0 ]];
    then
      echo $NAME:"There was a problem writing to $CONF_FILE"
      exit 1
    else
      echo $NAME:"Combined Z Axis set to $value"
      echo $NAME:"Config written to $CONF_FILE."
    fi
  else
    echo "options hid_xpadneo combined_z_axis=$value" >> $CONF_FILE
    if [[ $? -ne 0 ]];
    then
      echo $NAME:"There was a problem writing to $CONF_FILE."
      exit 1
    else
      echo $NAME:"combined_z_axis set to $value."
      echo $NAME:"Config written to $CONF_FILE."
    fi
  fi
}

# Parse arguments.
function parse_args {
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
        debug_level $@
        shift 2
        ;;

      -f | --disable-ff)
        disable_ff $@
        shift 2
        ;;

      -r | --trigger-rumble-damping)
        trigger_damping $@
        shift 2
        ;;

      -v | --fake-dev-version)
        fkdv $@
        shift 2
        ;;

      -z | --combined-z-axis)
        z_axis $@
        shift 2
        ;;

      --)
        shift
        break
        ;;

      *)
        echo $NAME:"Invalid option"
        display_help
        exit 1
        ;;
    esac
  done
}

main $@
