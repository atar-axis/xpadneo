KVER ?= $(shell uname -r)

INSTALL_MOD_DIR := kernel/drivers/hid
KERNEL_SOURCE_DIR ?= /lib/modules/$(KVER)/build
KERNEL_DEST_DIR := /lib/modules/$(KVER)/$(INSTALL_MOD_DIR)
MOK_DIR ?= /var/lib/shim-signed/mok
ETC_PREFIX ?= /etc
DOC_PREFIX ?= /usr/share/doc/xpadneo
META_PREFIX ?= /usr/share/metainfo

LD := ld.bfd
MODPROBE_CONFS := xpadneo.conf
UDEV_RULES := 60-xpadneo.rules 70-xpadneo-disable-hidraw.rules
DOC_SRCS := ../NEWS.md $(wildcard ../docs/[0-9A-Z]*.md)
DOCS := $(notdir $(DOC_SRCS))

ifeq ($(PREFIX),)
  UDEVADM ?= udevadm
else
  $(warning Installing to prefix, udevadm commands will not be run!)
  UDEVADM ?= : SKIPPING udevadm
endif

all: modules

.INTERMEDIATE: ../VERSION

../VERSION:
	$(MAKE) -C .. $(@:../%=%)

# convenience rules for local development

clean modules modules_install: ../VERSION
	$(MAKE) -C $(KERNEL_SOURCE_DIR) INSTALL_MOD_DIR="$(INSTALL_MOD_DIR)" LD=$(LD) M=$(shell pwd)/src VERSION="$(shell cat ../VERSION)" $@

check_sudo:
	[ "$(shell id -u)" = "0"  ] || { echo >&2 "needs to be run with sudo"; exit 1; }

reinstall: modules check_sudo install-all unload load
reinstall-sign: modules check_sudo install-all sign unload load
install-all: install install-metainfo

install: modules_install
	mkdir -p $(PREFIX)$(ETC_PREFIX)/modprobe.d $(PREFIX)$(ETC_PREFIX)/udev/rules.d $(PREFIX)$(DOC_PREFIX)
	install -D -m 0644 -t $(PREFIX)$(ETC_PREFIX)/modprobe.d $(MODPROBE_CONFS:%=etc-modprobe.d/%)
	install -D -m 0644 -t $(PREFIX)$(ETC_PREFIX)/udev/rules.d $(UDEV_RULES:%=etc-udev-rules.d/%)
	install -D -m 0644 -t $(PREFIX)$(DOC_PREFIX) $(DOC_SRCS)
	$(UDEVADM) control --reload

install-metainfo:
	install -D -m 0644 -t $(PREFIX)$(META_PREFIX) ../io.github.atar_axis.xpadneo.metainfo.xml

uninstall: check_sudo unload ../VERSION
	rm -f $(KERNEL_DEST_DIR)/hid-xpadneo.ko*
	rm -f $(DOCS:%=$(PREFIX)$(DOC_PREFIX)/%)
	rm -f $(UDEV_RULES:%=$(PREFIX)$(ETC_PREFIX)/udev/rules.d/%)
	rm -f $(MODPROBE_CONFS:%=$(PREFIX)$(ETC_PREFIX)/modprobe.d/%)
	rm -f $(PREFIX)$(META_PREFIX)/io.github.atar_axis.xpadneo.metainfo.xml
	rmdir --ignore-fail-on-non-empty -p $(PREFIX)$(ETC_PREFIX)/modprobe.d $(PREFIX)$(ETC_PREFIX)/udev/rules.d $(PREFIX)$(DOC_PREFIX) || true
	$(UDEVADM) control --reload

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
	depmod -a
	modprobe hid-xpadneo $(MOD_PARAMS)

# DKMS support rules

dkms.conf: dkms.conf.in ../VERSION
	sed 's/"@DO_NOT_CHANGE@"/"$(shell cat ../VERSION)"/g' <"$<" >"$@"
