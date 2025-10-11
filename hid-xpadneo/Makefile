KVER ?= $(shell uname -r)
INSTALL_MOD_DIR := kernel/drivers/hid
KERNEL_SOURCE_DIR ?= /lib/modules/$(KVER)/build
KERNEL_DEST_DIR := /lib/modules/$(KVER)/$(INSTALL_MOD_DIR)
MOK_DIR ?= /var/lib/shim-signed/mok
LD := ld.bfd

all: modules

.INTERMEDIATE: ../VERSION

../VERSION:
	$(MAKE) -C .. $(@:../%=%)

# convenience rules for local development

clean modules modules_install: ../VERSION
	$(MAKE) -C $(KERNEL_SOURCE_DIR) INSTALL_MOD_DIR="$(INSTALL_MOD_DIR)" LD=$(LD) M=$(shell pwd)/src VERSION="$(shell cat ../VERSION)" $@

check_sudo:
	[ "$(shell id -u)" = "0"  ] || { echo >&2 "needs to be run with sudo"; exit 1; }

reinstall: modules check_sudo modules_install unload load
reinstall-sign: modules check_sudo modules_install sign unload load

$(MOK_DIR)/MOK.priv $(MOK_DIR)/MOK.der:
# Create the shim MOK keys if they do not exist, using update-secureboot-policy.
# Users have to handle it themselves if it is not installed or not a good version.
ifeq ($(shell update-secureboot-policy --help 2>&1 | grep 'new-key'),)
	@echo "update-secureboot-policy not installed or does not have 'new-key' option. You will have to create the MOK yourself in $(MOK_DIR)"
else
	make check_sudo
	@update-secureboot-policy --new-key
endif

sign: $(MOK_DIR)/MOK.priv $(MOK_DIR)/MOK.der $(KERNEL_DEST_DIR)/hid-xpadneo.ko
	make check_sudo
# Enroll the shim MOK public key
ifeq ($(shell update-secureboot-policy --help 2>&1 | grep 'enroll-key'),)
	mokutil --import "$(MOK_DIR)/MOK.der"
else
	update-secureboot-policy --enroll-key | grep -q --invert-match 'No DKMS modules installed' \
	  || mokutil --import "$(MOK_DIR)/MOK.der"
endif
	$(KERNEL_SOURCE_DIR)/scripts/sign-file sha256 $^

unload: check_sudo
	rmmod hid-xpadneo || echo "proceeding anyways"

load: check_sudo
	modprobe hid-xpadneo $(MOD_PARAMS)

# DKMS support rules

dkms.conf: dkms.conf.in ../VERSION
	sed 's/"@DO_NOT_CHANGE@"/"$(shell cat ../VERSION)"/g' <"$<" >"$@"
