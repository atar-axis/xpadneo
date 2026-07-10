#!/bin/bash
# set -x

DKMS_DEFAULT_MOK_DIR="/var/lib/dkms"
UBUNTU_DEFAULT_MOK_DIR="/var/lib/shim-signed/mok"
USP_MOK_DIR="/var/lib/shim-signed/mok"

if [ ${EUID} -ne 0 ]
then
	echo >&2 "ERROR: You most probably need superuser privileges to use this script, \
please run me via sudo!"
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
# Return 0 if key already exists and is enrolled, 1 if it needs a restart to enroll.
function check_keys () {
  moks_dir="$1"

  # Load and use values set in DKMS (or Ubuntu) configuration file if no parameter
  if [ -z "$moks_dir" ]
  then
    source /etc/dkms/framework.conf
    [[ -r /etc/os-release ]] && source /etc/os-release

    # DKMS uses the default key created by update-secureboot-policy for Ubuntu and the likes
    if [[ -z "${mok_signing_key}" ]] \
      && { [ "$ID" = "ubuntu" ] || [[ "${ID_LIKE[0]}" == "ubuntu"* ]]; }
    then
      echo "Using default MOK key location for Ubuntu and derivatives"
      moks_dir="$UBUNTU_DEFAULT_MOK_DIR"
    else
      # Use MOK location defined in /etc/dkms/framework.conf. Fallback to default one
      if [ -z "$mok_signing_key" ]
      then mok_signing_key="$DKMS_DEFAULT_MOK_DIR/mok.key"
      fi
      if [ -z "$mok_certificate" ]
      then mok_certificate="$DKMS_DEFAULT_MOK_DIR/mok.pub"
      fi

      echo "Using DKMS configured or default MOK files locations"
      # Check if DKMS key already exists, create it otherwise
      if [ -f "$mok_signing_key" ] && [ -f "$mok_certificate" ]
      then
        # If key is already trusted, no need to enroll it
        if check_key "$mok_signing_key" "$mok_certificate"
        then return 0
        fi
      else generate_dkms_mok "$mok_signing_key" "$mok_certificate"
      fi

      # MOK was just created or is not trusted yet
      enroll_mok "$mok_certificate"
      return 1
    fi
  fi

  # Check if any key is present and enrolled
  if check_key "$moks_dir/MOK.priv" "$moks_dir/MOK.der" \
    || check_key "$moks_dir/mok.key" "$moks_dir/mok.pub"
  then return 0
  fi

  info "No Private key found or missing associated DER file in $moks_dir. \
Generating and importing it."
  # Try to use update-secureboot-policy if the given directory corresponds to where it stores MOK
  if [ "$USP_DEFAULT_MOK_DIR" = "$moks_dir" ]
  then
    generate_usp_mok
    enroll_usp_mok
  else
    echo "Using custom MOK key location"
    mkdir -p "$moks_dir"
    generate_mok "$moks_dir/MOK.priv" "$moks_dir/MOK.der"
    enroll_mok "$1/MOK.der"
  fi
  
  return 1
}

# Used by check_keys to check MOK is valid
# returns 0 if key pair exists and is valid, 1 if it does not exist or is not valid
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

# Used by check_keys to generate DKMS MOK files
function generate_dkms_mok () {
  # Use DKMS's generate_mok action, fallback on openssl
  dkms_help_output=$(bash -c 'dkms --help' 2>&1)
  if command -v dkms >/dev/null 2>&1 \
    && echo "$dkms_help_output" | grep -q "generate_mok"
  then
    echo "Using 'dkms generate_mok' to manage MOK creation"
    dkms generate_mok
  else generate_mok "$1" "$2"
  fi
}

# Used by check_keys to generate update-secureboot-policy MOK files
function generate_usp_mok () {
  # Use update-secureboot-policy by default, fallback on openssl
  usp_help_output=$(bash -c 'update-secureboot-policy --help' 2>&1)
  if command -v update-secureboot-policy >/dev/null 2>&1 \
    && echo "$usp_help_output" | grep -q "new-key"
  then
    echo "Using update-secureboot-policy to manage MOK creation"
    update-secureboot-policy --new-key
  else generate_mok "$USP_MOK_DIR/MOK.priv" "$USP_MOK_DIR/MOK.der"
  fi
}

# Used by check_keys or as fallback of generate_dkms_mok and generate_usp_mok to generate MOK files
function generate_mok () {
  mok_key="$1"
  mok_cert="$2"
  
  if command -v openssl >/dev/null 2>&1
  then
    echo "Using generic tools to manage MOK creation"

    cn="$(hostname) Secure Boot signature key (xpadneo)"
    # Max CN length is 64
    if [ ${#cn} -gt 64 ]
    then
      cn=${cn:0:63}
      cn="$cn…"
    fi

    openssl \
      req -new -x509 \
      -newkey rsa:2048 -keyout "$mok_key" \
      -outform DER -out "$mok_cert" \
      -days 36500 \
      -subj "/CN=$cn/" \
      -addext "extendedKeyUsage=codeSigning" || error "issue creating cert"

    chmod 600 "$mok_key"
    chmod 600 "$mok_cert"
  else
    echo "openssl not found, cannot generate key and certificate."
    return
  fi
}

# Used by check_keys to enroll MOK files
function enroll_usp_mok () {
  # Use update-secureboot-policy by default, fallback using mokutil
  if command -v update-secureboot-policy >/dev/null 2>&1 \
    && update-secureboot-policy --help 2>&1 | grep -q "enroll-key" \
    && ! update-secureboot-policy --enroll-key 2>&1 | grep -q "No DKMS modules installed"
  then
    warn "MOK imported, you will need to reboot to enroll it."
  else
    enroll_mok "$USP_MOK_DIR/MOK.der"
  fi
}

# Used by check_keys to enroll MOK files
function enroll_mok () {
  warn "Set a one-time import password. You will have to reboot and type it to enroll the MOK."
  mokutil --import "$1"
}
