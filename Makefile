MODPROBE_CONFS := xpadneo.conf
UDEV_RULES := 60-xpadneo.rules 70-xpadneo-disable-hidraw.rules

ifeq ($(PREFIX),)
  DKMS ?= dkms
else
  $(warning Installing to prefix, dkms commands will not be run!)
  DKMS ?= : SKIPPING dkms
endif

ETC_PREFIX ?= $(PREFIX)/etc

all: build

help:
	@echo "Targets:"
	@echo "help        This help"
	@echo "build       Prepare the package for DKMS deployment"
	@echo "install     Install the package and DKMS source code"
	@echo "uninstall   Uninstall the package and DKMS source code"
	@echo
	@echo "Variables:"
	@echo "PREFIX      Install files into this prefix"
	@echo "ETC_PREFIX  Install etc files into this prefix (defaults to \$PREFIX/etc)"
	@echo
	@echo "Using PREFIX requires handling dkms commands in your package script."

.PHONY: build install

.INTERMEDIATE: VERSION

VERSION:
	{ [ -n "$(VERSION)" ] && echo $(VERSION) || git describe --tags --dirty; } >$@

build: VERSION
	$(MAKE) VERSION="$(shell cat VERSION)" -C hid-xpadneo dkms.conf

install: build
	mkdir -p $(PREFIX)/etc/modprobe.d $(PREFIX)/etc/udev/rules.d
	install -D -m 0644 -t $(PREFIX)/etc/modprobe.d $(MODPROBE_CONFS:%=hid-xpadneo/etc-modprobe.d/%)
	install -D -m 0644 -t $(PREFIX)/etc/udev/rules.d $(UDEV_RULES:%=hid-xpadneo/etc-udev-rules.d/%)
	$(DKMS) add hid-xpadneo

uninstall: VERSION
	$(DKMS) remove "hid-xpadneo/$(shell cat VERSION)" --all
	rm -Rf "$(PREFIX)/usr/src/hid-xpadneo-$(shell cat VERSION)"
	rm -f $(PREFIX)$(UDEV_RULES:%=$(ETC_PREFIX)/udev/rules.d/%)
	rm -f $(PREFIX)$(MODPROBE_CONFS:%=$(ETC_PREFIX)/modprobe.d/%)
	rmdir --ignore-fail-on-non-empty -p $(PREFIX)/etc/modprobe.d $(PREFIX)/etc/udev/rules.d
