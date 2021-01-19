KERNEL_SOURCE_DIR := /usr/src/linux
LD := ld.bfd

all: modules

.INTERMEDIATE: ../VERSION

../VERSION:
	git describe --tags --dirty >$@

src/version.h: src/version.h.in ../VERSION
	sed 's/"@DO_NOT_CHANGE@"/"'"$(shell cat ../VERSION)"'"/g' $< >$@

clean modules modules_install: src/version.h
	$(MAKE) -C $(KERNEL_SOURCE_DIR) INSTALL_MOD_DIR="kernel/drivers/hid" LD=$(LD) M=$(shell pwd)/src $@

reinstall: modules
	sudo make modules_install
	sudo rmmod hid-xpadneo || true
	sudo modprobe hid-xpadneo
