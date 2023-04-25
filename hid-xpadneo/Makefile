ifndef KERNEL_SOURCES
	KERNEL_UNAME ?= $(shell uname -r)
	KERNEL_SOURCES := /lib/modules/$(KERNEL_UNAME)/build
endif

all: modules

.INTERMEDIATE: ../VERSION

../VERSION:
	git describe --tags --dirty >$@

clean modules modules_install: ../VERSION
	$(MAKE) -C $(KERNEL_SOURCES) INSTALL_MOD_DIR="kernel/drivers/hid" M="$(shell pwd)/src" VERSION=$(shell cat ../VERSION) $@

reinstall: modules
	sudo make modules_install
	sudo rmmod hid-xpadneo || true
	sudo modprobe hid-xpadneo $(MOD_PARAMS)
