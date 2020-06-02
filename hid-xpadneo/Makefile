KERNEL_SOURCE_DIR := /usr/src/linux
LD := ld.bfd

all: modules

clean modules modules_install:
	$(MAKE) -C $(KERNEL_SOURCE_DIR) INSTALL_MOD_DIR="kernel/drivers/hid" LD=$(LD) M=$(shell pwd)/src $@

reinstall: modules
	sudo make modules_install
	sudo rmmod hid-xpadneo || true
	sudo modprobe hid-xpadneo
