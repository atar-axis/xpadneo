#!/bin/bash
# set -x

SHIM_MOK_DIR="/var/lib/shim-signed/mok"

if [ ${EUID} -ne 0 ]; then
	echo >&2 "ERROR: You most probably need superuser privileges to use this script, please run me via sudo!"
	exit 3
fi

function info  { echo -e "\e[32m[info] $*\e[39m"; }
function warn  { echo -e "\e[33m[warn] $*\e[39m"; }
function error { echo -e "\e[31m[error] $*\e[39m"; exit 1; }

function signing_help () {
  echo "
    just run the script, reboot (you will have to type a pass you set) and run again!
  "
}

# Check that Machine Owner Keys exist and are enrolled. Generate new ones if necessary.
# Accept two parameters for MOK locations checked.
# If any exist, enroll those if necessary, but do not create new ones.
# If key files found in none, we generate them in both locations.
function check_keys () {
  moks_main_dir=${1:-}
  moks_secondary_dir=${2:-$SHIM_MOK_DIR}

  # Check if any key are present and enrolled ()
  check_key "$moks_main_dir"
  main_dir_keys_status=$?
  check_key "$moks_secondary_dir"
  second_dir_keys_status=$?
  if [ $main_dir_keys_status -eq 0 ] || [ $second_dir_keys_status -eq 0 ]
  then
    # If any key is found, we suppose that it will be used
    return 0
  else
    # If no MOK found, generate it, in both directories if they exist
    if [ $main_dir_keys_status -eq 2 ] && [ $second_dir_keys_status -eq 2 ]
    then
      info "No existing MOK found."
      # Do not try to generate a key in moks_main_dir if it has not been set
      if [ -z "$moks_main_dir" ]
      then info "Generating Private key and associated DER file in $moks_secondary_dir."
      else
        info "Generating Private key and associated DER file in $moks_secondary_dir in addition to $moks_main_dir."
        generate_moks "$moks_main_dir"
      fi
      generate_moks "$moks_secondary_dir"
    fi
  fi

  return 1
}

# Used by check_keys to check MOK in a directory is valid
# returns 0 if key exists and is valid, 1 if it exists but is not valid, 2 if it does not exist
function check_key () {
  if [ -z "$1" ]
  then return 2
  fi

  moks_dir=$1
  # Check if any key are present and enrolled ()
  if { [ -f "$moks_dir/MOK.priv" ] || [ -f "$moks_dir/mok.key" ]; } \
    && { [ -f "$moks_dir/MOK.der" ] || [ -f "$moks_dir/mok.pub" ]; }
  then
    if mokutil --test-key "$moks_dir/MOK.der" 2> /dev/null | grep -q "already enrolled" \
      || mokutil --test-key "$moks_dir/mok.pub" 2> /dev/null | grep -q "already enrolled"
    then
      info "MOK Key found in $moks_dir, already trusted."
      return 0
    else
      info "A MOK key exist in $moks_dir but is not trusted yet. Importing it."
      # Enrolling the MOK, even if not necessary
      enroll_mok "$moks_dir"
      return 1
    fi
  else
    info "No Private key found or missing associated DER file in $moks_dir."
    return 2
  fi
}

# Used by check_keys to generate MOK files
function generate_moks () {
  moks_dir=$1
  # Use update-secureboot-policy by default, fallback using openssl
  usp_help_output=$(bash -c 'update-secureboot-policy --help' 2>&1)
  if command -v update-secureboot-policy >/dev/null 2>&1 && echo "$usp_help_output" | grep -q "new-key"
  then
    update-secureboot-policy --new-key
  else
    cn="$(hostname) Secure Boot Module Signature key (xpadneo)"
    # Max CN length is 64
    if [ ${#cn} -gt 64 ]
    then
      cn=${cn:0:63}
      cn="$cn…"
    fi

    mkdir -p "$moks_dir"
    
    openssl \
      req -new -x509 \
      -newkey rsa:2048 -keyout "$moks_dir/MOK.priv" \
      -outform DER -out "$moks_dir/MOK.der" \
      -days 36500 \
      -subj "/CN=$(hostname) Secure Boot Module Signature key (xpadneo)/" \
      -addext "extendedKeyUsage=codeSigning" || error "issue creating cert"

    chmod 600 "$moks_dir/MOK"*
  fi
}

# Used by check_keys to enroll MOK files
function enroll_mok () {
  # Use update-secureboot-policy by default, fallback using mokutil
  if command -v update-secureboot-policy >/dev/null 2>&1 \
    && update-secureboot-policy --help 2>&1 | grep -q "enroll-key" \
    && ! update-secureboot-policy --enroll-key 2>&1 | grep -q "No DKMS modules installed"
  then
    warn "MOK imported, you will need to reboot to enroll it."
  else
    warn "Set a one-time import password. You will have to reboot and type it to enroll the MOK."
    # Enroll all existing key files
    if [ -f "$1/MOK.der" ]; then mokutil --import "$1/MOK.der"; fi
    if [ -f "$1/mok.pub" ]; then mokutil --import "$1/mok.pub"; fi
  fi
}
