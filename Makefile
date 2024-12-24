ETC_PREFIX ?= /etc
DOC_PREFIX ?= /usr/share/doc/xpadneo

MODPROBE_CONFS := xpadneo.conf
UDEV_RULES := 60-xpadneo.rules 70-xpadneo-disable-hidraw.rules
DOC_SRCS := NEWS.md $(wildcard docs/[0-9A-Z]*.md)
DOCS := $(notdir $(DOC_SRCS))

ifeq ($(PREFIX),)
  DKMS ?= dkms
else
  $(warning Installing to prefix, dkms commands will not be run!)
  DKMS ?= : SKIPPING dkms
endif

all: build

help:
	@echo "Targets:"
	@echo "help        This help"
	@echo "build       Prepare the package for DKMS deployment"
	@echo "install     Install the package, documentation and DKMS source code"
	@echo "uninstall   Uninstall the package, documentation and DKMS source code"
	@echo
	@echo "Variables:"
	@echo "PREFIX      Install files into this prefix"
	@echo "DOC_PREFIX  Install doc files relative to the prefix (defaults to /usr/share/doc/xpadneo)"
	@echo "ETC_PREFIX  Install etc files relative to the prefix (defaults to /etc)"
	@echo
	@echo "Using PREFIX requires handling dkms commands in your package script."

.PHONY: build install

.INTERMEDIATE: VERSION

VERSION:
	{ [ -n "$(VERSION)" ] && echo $(VERSION) || git describe --tags --dirty; } >$@

build: VERSION
	$(MAKE) VERSION="$(shell cat VERSION)" -C hid-xpadneo dkms.conf

install: build
	mkdir -p $(PREFIX)$(ETC_PREFIX)/modprobe.d $(PREFIX)$(ETC_PREFIX)/udev/rules.d $(PREFIX)$(DOC_PREFIX)
	install -D -m 0644 -t $(PREFIX)$(ETC_PREFIX)/modprobe.d $(MODPROBE_CONFS:%=hid-xpadneo/etc-modprobe.d/%)
	install -D -m 0644 -t $(PREFIX)$(ETC_PREFIX)/udev/rules.d $(UDEV_RULES:%=hid-xpadneo/etc-udev-rules.d/%)
	install -D -m 0644 -t $(PREFIX)$(DOC_PREFIX) $(DOC_SRCS)
	$(DKMS) add hid-xpadneo

uninstall: VERSION
	$(DKMS) remove "hid-xpadneo/$(shell cat VERSION)" --all
	rm -Rf "$(PREFIX)/usr/src/hid-xpadneo-$(shell cat VERSION)"
	rm -f $(DOCS:%=$(PREFIX)$(DOC_PREFIX)/%)
	rm -f $(UDEV_RULES:%=$(PREFIX)$(ETC_PREFIX)/udev/rules.d/%)
	rm -f $(MODPROBE_CONFS:%=$(PREFIX)$(ETC_PREFIX)/modprobe.d/%)
	rmdir --ignore-fail-on-non-empty -p $(PREFIX)$(ETC_PREFIX)/modprobe.d $(PREFIX)$(ETC_PREFIX)/udev/rules.d $(PREFIX)$(DOC_PREFIX)
