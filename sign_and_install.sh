#!/usr/bin/env bash
#set -x
set -e

# https://docs.fedoraproject.org/en-US/fedora/rawhide/system-administrators-guide/kernel-module-driver-configuration/Working_with_Kernel_Modules/#sect-signing-kernel-modules-for-secure-boot

source "$(dirname "$0")/lib/mok.sh"

command -v openssl > /dev/null 2>&1 || warn "missing openssl, you will not be able to create a new MOK key"
command -v mokutil > /dev/null 2>&1 || warn "missing mokutil, you will not be able to sign the module"

function install_xpadneo () {
  info "start install_xpadneo"

  # Uninstall previous version
  source "$(dirname "$0")/uninstall.sh"
  source "$(dirname "$0")/install.sh"

  module_ko_file=$(modinfo hid-xpadneo | awk '/^filename:/ { print $2 }')
  if [ -z "$module_ko_file" ]
  then error "Module compilation or installation failed"
  fi

  depmod -a
  
  if modprobe hid-xpadneo
  then  
    modinfo hid-xpadneo
    info "finished install_xpadneo"

    if [ -d /sys/module/hid_xpadneo ]
    then
      info "hid_xpadneo loaded!"
      # suggest to load uhid to support Bluetooth LE (for firmware 5.x)
      if [ ! -d /sys/devices/virtual/misc/uhid ]
      then modprobe uhid
      fi
    else warn "failed to load hid_xpadneo (check for errors by running 'sudo dmesg')"
    fi
  else warn "failed to load hid_xpadneo"
  fi 
}

if [ "$#" -ne "0" ]
then
  signing_help
  exit 0
else
  if mokutil --sb-state | grep -q "SecureBoot enabled"
  then info "Secure boot is enabled, signing is needed"
  else warn "Secure boot is disabled, no need to sign module!"
  fi

  if ! [ -d /sys/module/hid_xpadneo ]
  then
    # Check in DKMS directory and enroll if a MOK is present
    info "If you want to use specific key, set mok_signing_key and mok_certificate in /etc/dkms/framework.conf." \
      "Otherwise, shim default will be used."
    if check_keys ""
    then install_xpadneo
    fi
  else info "hid_xpadneo is already loaded"
  fi
fi
