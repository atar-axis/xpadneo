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
# Accept a parameter for MOK location to check. Otherwise, use configured or default DKMS directory.
# If it exists, enroll MOK if necessary, but do not create new ones.
# If no key file found, we generate MOK.
function check_keys () {
  # Load and use values set in DKMS configuration file if no parameter
  if [ -z "$1" ]
  then
    source /etc/dkms/framework.conf
    
    # Check if key exists and is enrolled
    if [ -f "$mok_signing_key" ] && [ -f "$mok_certificate" ]
    then
      check_key "$mok_signing_key" "$mok_certificate"
      return $?
    fi
  fi
  
  # If a parameter was given or DKMS configured files were not set or did not exist
  moks_dir="$1"
  moks_dir="${moks_dir:-$SHIM_MOK_DIR}"
  # Check if any key is present and enrolled
  if check_key "$moks_dir/MOK.priv" "$moks_dir/MOK.der" || check_key "$moks_dir/mok.key" "$moks_dir/mok.pub"
  then return 0
  fi

  info "No Private key found or missing associated DER file in $moks_dir. Generating and importing it."
  generate_mok "$moks_dir"
  enroll_mok "$moks_dir"
  return 1
}

# Used by check_keys to check MOK is valid
# returns 0 if key pair exists and is valid, 1 if it exists but is not valid, or if it does not exist
function check_key () {
  if [ -f "$1" ] && [ -f "$2" ]
  then
    if mokutil --test-key "$2" | grep -q "already enrolled"
    then
      info "MOK Key $2 found, already trusted."
      return 0
    else
      info "$2 MOK key exists but is not trusted yet. Importing it."
      # Enrolling the MOK, even if not necessary
      enroll_mok "$moks_dir"
    fi
  fi

  return 1
}

# Used by check_keys to generate MOK files
function generate_mok () {
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
