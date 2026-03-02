#!/usr/bin/env bash
#set -x
set -e

# https://docs.fedoraproject.org/en-US/fedora/rawhide/system-administrators-guide/kernel-module-driver-configuration/Working_with_Kernel_Modules/#sect-signing-kernel-modules-for-secure-boot

source "$(dirname "$0")/../lib/mok.sh"

if [ "$EUID" -ne 0 ]
then error "Please run as root"
fi

command -v openssl > /dev/null 2>&1 || warn "missing openssl, you will not be able to create a new MOK key"
command -v mokutil > /dev/null 2>&1 || warn "missing mokutil, you will not be able to sign the module"

function install_xpadneo () {
  info "start install_xpadneo"

  # Uninstall previous version
	rmmod hid-xpadneo > /dev/null 2>&1 || true

  make modules
  make modules_install
  make sign

  depmod -a
  modprobe hid-xpadneo
  modinfo hid-xpadneo

  if [ -d /sys/module/hid_xpadneo ]
  then
    info "hid_xpadneo loaded!"
    # suggest to load uhid to support Bluetooth LE (for firmware 5.x)
    if [ ! -d /sys/devices/virtual/misc/uhid ]
    then info "you may need to run 'sudo modprobe uhid' to support firmware 5.x"
    fi
  else warn "failed to load hid_xpadneo"
  fi

  info "finished install_xpadneo"
}

if [ "$#" -ne "0" ]
then
  signing_help
  exit 0
else
  if mokutil --sb-state | grep -q "SecureBoot enabled"; then
    if ! [ -d /sys/module/hid_xpadneo ]
    then
      info "Secure boot is enabled, signing is needed"
      # Check/enroll keys in default shim directory
      if check_keys "/var/lib/shim-signed/mok"
      then install_xpadneo
      fi
    else
      info "hid_xpadneo is loaded"
    fi
  else
    warn "Secure boot is disabled, no need to sign module!"
    export skip_signing=1
    if ! [ -d /sys/module/hid_xpadneo ]
    then install_xpadneo
    fi
  fi
fi
