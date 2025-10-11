KERNEL_SOURCE_DIR ?= /lib/modules/$(shell uname -r)/build
LD := ld.bfd
KVER ?= $(shell uname -r)
INSTALL_MOD_DIR := kernel/drivers/hid
ETC_PREFIX ?= /etc
DOC_PREFIX ?= /usr/share/doc/xpadneo
KSRC := /lib/modules/$(KVER)/build
KDST := /lib/modules/$(KVER)/$(INSTALL_MOD_DIR)
MOK_DIR ?= /var/lib/shim-signed/mok

MODPROBE_CONFS := xpadneo.conf
UDEV_RULES := 60-xpadneo.rules 70-xpadneo-disable-hidraw.rules
DOC_SRCS := ../NEWS.md $(wildcard ../docs/[0-9A-Z]*.md)

all: modules

.INTERMEDIATE: ../VERSION

../VERSION:
	$(MAKE) -C .. $(@:../%=%)

# convenience rules for local development

clean modules modules_install: ../VERSION
	$(MAKE) -C $(KERNEL_SOURCE_DIR) INSTALL_MOD_DIR="$(INSTALL_MOD_DIR)" LD=$(LD) M=$(shell pwd)/src VERSION="$(shell cat ../VERSION)" $@

reinstall: unload modules modules_install load
reinstall-sign: unload modules modules_install load

unload:
	[ "$(shell id -u)" = "0"  ] || { echo >&2 "need to run with sudo"; exit 1; }
	rmmod hid-xpadneo || echo "proceeding anyways"
	rm /etc/udev/rules.d/60-xpadneo.rules || echo "remove error: proceeding anyways"
	rm /etc/udev/rules.d/70-xpadneo-disable-hidraw.rules || echo "remove error: proceeding anyways"

load:
	mkdir -p $(PREFIX)$(ETC_PREFIX)/modprobe.d $(PREFIX)$(ETC_PREFIX)/udev/rules.d $(PREFIX)$(DOC_PREFIX)
	install -D -m 0644 -t $(PREFIX)$(ETC_PREFIX)/modprobe.d $(MODPROBE_CONFS:%=etc-modprobe.d/%)
	install -D -m 0644 -t $(PREFIX)$(ETC_PREFIX)/udev/rules.d $(UDEV_RULES:%=etc-udev-rules.d/%)
	install -D -m 0644 -t $(PREFIX)$(DOC_PREFIX) $(DOC_SRCS)
	depmod -a
	modprobe hid-xpadneo $(MOD_PARAMS)

uninstall: ../VERSION unload
	rm -f $(KDST)/hid-xpadneo.ko*
	rm -Rf "$(PREFIX)/usr/src/hid-xpadneo-$(shell cat $<)"
	rm -f $(DOCS:%=$(PREFIX)$(DOC_PREFIX)/%)
	rm -f $(UDEV_RULES:%=$(PREFIX)$(ETC_PREFIX)/udev/rules.d/%)
	rm -f $(MODPROBE_CONFS:%=$(PREFIX)$(ETC_PREFIX)/modprobe.d/%)
	rmdir --ignore-fail-on-non-empty -p $(PREFIX)$(ETC_PREFIX)/modprobe.d $(PREFIX)$(ETC_PREFIX)/udev/rules.d $(PREFIX)$(DOC_PREFIX) || true


$(MOK_DIR)/MOK.priv $(MOK_DIR)/MOK.der:
	[ "$(shell id -u)" = "0"  ] || { echo >&2 "need to run with sudo"; exit 1; }
#	Create the shim MOK keys if they do not exist
ifeq ($(shell update-secureboot-policy --help 2>&1 | grep 'new-key'),)
	@echo "update-secureboot-policy not installed or does not have 'new-key' option. You will have to manage the MOK yourself"
else
	@update-secureboot-policy --new-key
endif

sign: $(MOK_DIR)/MOK.priv $(MOK_DIR)/MOK.der $(KDST)/hid-xpadneo.ko
	[ "$(shell id -u)" = "0"  ] || { echo >&2 "need to run with sudo"; exit 1; }
#	Enroll the shim MOK keys
ifeq ($(shell update-secureboot-policy --help 2>&1 | grep 'enroll-key'),)
	@echo "update-secureboot-policy not installed or does not have 'enroll-key' option. You will have to enroll the MOK yourself if needed"
else
	update-secureboot-policy --enroll-key | grep -q --invert-match 'No DKMS modules installed' \
	  || mokutil --import "$(MOK_DIR)/MOK.der"
endif
	$(KSRC)/scripts/sign-file sha256 $^

install-sign: modules modules_install sign load

# DKMS support rules

dkms.conf: dkms.conf.in ../VERSION
	sed 's/"@DO_NOT_CHANGE@"/"$(shell cat ../VERSION)"/g' <"$<" >"$@"
