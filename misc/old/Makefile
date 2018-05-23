# MODULE INFORMATION
MOD_NAME=hid-xpadneo
MOD_SUBDIR=drivers/hid

# FOLDERS IN BUILDSYSTEM
SRC_DIR=src
DL_DIR=dl
BUILD_DIR=build
OUT_DIR=out
TMP_DIR=tmp

# Raspberry Pi 3
ifeq ($(TARGET), raspi3)

	# RASPBERRY PI 3 SETTINGS
	# Default-Settings
	DEFCONFIG?=bcm2709
	export KERNEL?=kernel7
	export CROSS_COMPILE?=arm-linux-gnueabihf-
	export ARCH?=arm

	# KERNEL VERSION
	KERNEL_RELEASE_LONG=4.9.59-v7+
	KERNEL_RELEASE=$(shell echo $(KERNEL_RELEASE_LONG) | sed -e 's?-.*??')
	LINUX_SRC_DIR=$(BUILD_DIR)/linux-raspberrypi-kernel_1.20171029-1

# Default (Local System)
else

	# KERNEL VERSION
	KERNEL_RELEASE_LONG=$(shell uname -r)
	KERNEL_RELEASE=$(shell uname -r | sed -e 's?-.*??')
	LINUX_SRC_DIR=$(BUILD_DIR)/linux-$(KERNEL_RELEASE)
	ARCH=$(shell uname -m)

	# SYSTEM PATHS
	ERTM_CONFIG=/sys/module/bluetooth/parameters/disable_ertm
	EXTRAMODULES=/lib/modules/$(KERNEL_RELEASE_LONG)/extramodules

endif


##################
## BUILD TARGET ##
##################

.PHONY: clean clean_build clean_out clean_dl clean_all

build: $(OUT_DIR)/$(ARCH)/$(MOD_NAME).ko

clean:
	rm --force --recursive $(BUILD_DIR)/*

clean_build: clean

clean_out:
	rm --force --recursive $(OUT_DIR)/*

clean_dl:
	rm --force --recursive $(DL_DIR)/*

clean_all: clean_build clean_out clean_dl
	rm --force --recursive $(DL_DIR)/*


######################
## HANDLE ERTM (BT) ##
######################

.PHONY: disable_ertm enable_ertm

disable_ertm: $(ERTM_CONFIG)
	@sudo /bin/bash -c "echo 1 > $(ERTM_CONFIG)"

enable_ertm: $(ERTM_CONFIG)
	@sudo /bin/bash -c "echo 0 > $(ERTM_CONFIG)"


#################################
## DOWNLOAD AND UNPACK SOURCES ##
#################################

# create folders
$(DL_DIR)/:
	mkdir --parents $@

$(BUILD_DIR)/:
	mkdir --parents $@

$(OUT_DIR)/:
	mkdir --parents $@

$(TMP_DIR)/:
	mkdir --parents $@
	
$(OUT_DIR)/$(ARCH)/:
	mkdir --parents $@


# do not delete downloads
.PRECIOUS: $(DL_DIR)/linux-%.tar.gz $(DL_DIR)/raspberrypi-kernel_%.tar.gz

ifeq ($(TARGET), raspi3)
# download raspberry pi linux-kernel
$(DL_DIR)/linux-raspberrypi-kernel_%.tar.gz: | $(DL_DIR)/
	wget --quiet --show-progress --timestamping https://github.com/raspberrypi/linux/archive/raspberrypi-kernel_1.20171029-1.tar.gz --directory-prefix=$(DL_DIR)
else
# download official linux-kernel
$(DL_DIR)/linux-%.tar.gz: | $(DL_DIR)/
	wget --quiet --show-progress --timestamping https://www.kernel.org/pub/linux/kernel/v4.x/$(notdir $@) --directory-prefix=$(DL_DIR)
endif


.PHONY: unpack

unpack: $(LINUX_SRC_DIR)/

ifeq ($(TARGET), raspi3)
# unpack the raspberry pi linux kernel sources
$(BUILD_DIR)/linux-raspberrypi-kernel_%/: $(DL_DIR)/raspberrypi-kernel_%.tar.gz | $(BUILD_DIR)/
	tar --totals --extract --ungzip --file=$< --directory=$(BUILD_DIR)
else
# unpack the official linux kernel sources
$(BUILD_DIR)/linux-%/: $(DL_DIR)/linux-%.tar.gz | $(BUILD_DIR)/
	tar --totals --extract --ungzip --file=$< --directory=$(BUILD_DIR)
endif


#############################
## COPY FILES INTO SOURCES ##
#############################

ifdef DEFCONFIG

$(LINUX_SRC_DIR)/arch/$(ARCH)/configs/$(DEFCONFIG)_defconfig: $(SRC_DIR)/$(DEFCONFIG)_defconfig
	cp $< $@


$(LINUX_SRC_DIR)/.config: $(LINUX_SRC_DIR)/arch/$(ARCH)/configs/$(DEFCONFIG)_defconfig

	make --directory=$(LINUX_SRC_DIR) $(DEFCONFIG)_defconfig

	# merge the system-.config an the module-.config into one
	./$(LINUX_SRC_DIR)/scripts/kconfig/merge_config.sh -O $(LINUX_SRC_DIR) -m $(LINUX_SRC_DIR)/.config $(SRC_DIR)/config_setXpadNeo

	# run oldconfig
	# can we run allnoconfig instead?
	# make --directory=$(LINUX_SRC_DIR) -j6 oldconfig

	# adds a plus sign as desired by raspbian (localversion)
	# this has to be done after oldconfig, since oldconfig changes the localversion
	# thats not a very clean way since it is hardcoded, even if KERNEL!=kernel7...
	# TODO: workaround: check for KERNEL=kernel7 too
ifeq ($(TARGET), raspi3)
	./$(LINUX_SRC_DIR)/scripts/kconfig/merge_config.sh -O $(LINUX_SRC_DIR) -m $(LINUX_SRC_DIR)/.config $(SRC_DIR)/config_kernel7plus
endif

else #DEFCONFIG

# copy .config from local system into linux sources
$(LINUX_SRC_DIR)/.config: | $(LINUX_SRC_DIR)/ $(TMP_DIR)/

# search for a local .config file in various places
ifneq ("$(wildcard /usr/lib/modules/$(KERNEL_RELEASE_LONG)/build/.config)","")
	$(eval CONFIG_PATH=/usr/lib/modules/$(KERNEL_RELEASE_LONG)/build)

else ifneq ("$(wildcard /boot/config-$(KERNEL_RELEASE)/.config)","")
	$(eval CONFIG_PATH=/boot/config-$(KERNEL_RELEASE))

else ifneq ("$(wildcard /proc/config.gz)","")
	gzip --decompress < /proc/config.gz > $(TMP_DIR)/.config
	$(eval CONFIG_PATH=$(TMP_DIR))

else
	# abort if no local config was found
	$(error no config found - aborting...)

endif

	$(info using config found in $(CONFIG_PATH))

	# copy system-.config into our SRC directory
	cp $(CONFIG_PATH)/.config $(LINUX_SRC_DIR)/

	# merge the system-.config an the module-.config into one
	# this has to be done before running oldconfig, since oldconfig asks for the new driver the other way
	./$(LINUX_SRC_DIR)/scripts/kconfig/merge_config.sh -O $(LINUX_SRC_DIR) -m $(LINUX_SRC_DIR)/.config $(SRC_DIR)/config_setXpadNeo 

	# run oldconfig
	# can we run allnoconfig instead?
	# make --directory=$(LINUX_SRC_DIR) -j6 oldconfig

endif #DEFCONFIG


# copy driver from src into linux-build-dir
$(LINUX_SRC_DIR)/$(MOD_SUBDIR)/$(MOD_NAME).c: $(SRC_DIR)/$(MOD_NAME).c | $(LINUX_SRC_DIR)/ 
	cp --force $(SRC_DIR)/$(MOD_NAME).c $(LINUX_SRC_DIR)/$(MOD_SUBDIR)/


# prepare the Makefile and Kconfig
$(LINUX_SRC_DIR)/$(MOD_SUBDIR)/prepared: $(SRC_DIR)/Makefile.add $(SRC_DIR)/Kconfig.add | $(LINUX_SRC_DIR)/
	cat $(SRC_DIR)/Makefile.add >> $(LINUX_SRC_DIR)/$(MOD_SUBDIR)/Makefile
	cat $(SRC_DIR)/Kconfig.add >> $(LINUX_SRC_DIR)/$(MOD_SUBDIR)/Kconfig
	touch $@


############
## OUTPUT ##
############

# prepare the source for building
$(LINUX_SRC_DIR)/prepared: $(LINUX_SRC_DIR)/$(MOD_SUBDIR)/prepared $(LINUX_SRC_DIR)/.config | $(LINUX_SRC_DIR)/
	make --directory=$(LINUX_SRC_DIR) -j6 oldconfig
	make --directory=$(LINUX_SRC_DIR) -j6 prepare
	make --directory=$(LINUX_SRC_DIR) -j6 scripts
	touch $@

# generate Module.symvers once
$(LINUX_SRC_DIR)/Module.symvers: $(LINUX_SRC_DIR)/prepared | $(LINUX_SRC_DIR)/$(MOD_SUBDIR)/$(MOD_NAME).c $(LINUX_SRC_DIR)/
	make --directory=$(LINUX_SRC_DIR) -j6 modules

# compile the driver
$(LINUX_SRC_DIR)/$(MOD_SUBDIR)/$(MOD_NAME).ko: $(LINUX_SRC_DIR)/Module.symvers $(LINUX_SRC_DIR)/$(MOD_SUBDIR)/$(MOD_NAME).c | $(LINUX_SRC_DIR)/
	make --directory=$(LINUX_SRC_DIR) -j6 M=$(MOD_SUBDIR)

# copy from linux-build-directory to out-directory
$(OUT_DIR)/$(ARCH)/$(MOD_NAME).ko: $(LINUX_SRC_DIR)/$(MOD_SUBDIR)/$(MOD_NAME).ko | $(OUT_DIR)/$(ARCH)/
	cp $< $@


####################
## INSTALL TARGET ##
####################

.PHONY: install clean_install

install:
	sudo modprobe -r $(MOD_NAME)
	sudo cp -f $(OUT_DIR)/$(ARCH)/$(MOD_NAME).ko $(dir $(EXTRAMODULES)/$(MOD_NAME).ko)
	sudo depmod
	sudo modprobe $(MOD_NAME)
	@lsmod | grep xpadneo && echo "DRIVER SUCESSFULLY REGISTERED"

clean_install:
	sudo rm $(EXTRAMODULES)/$(MOD_NAME).ko




#################
## HELP TARGET ##
#################

help:
	@echo Buildsystem for XPADneo Driver
	@echo "by Florian Dollinger <dollinger.florian@gmx.de>"
	@echo
	@echo Run

	@echo '- make [build]' for your local system
	@echo '- make install' to install the driver on your local system
	@echo '- make TARGET=raspi3' for Raspberry Pi 3

	@echo '- make clean' to clean the build-folder
	@echo '- make clean_all' to clean download and build-folder
	@echo '- make clean_install' to remove the driver from your local system
