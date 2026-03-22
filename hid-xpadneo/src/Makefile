#
# Makefile for the HID driver
#
ccflags-y += -DVERSION=$(VERSION)
obj-m += hid-xpadneo.o

hid-xpadneo-y += \
	xpadneo/consumer.o \
	xpadneo/core.o \
	xpadneo/debug.o \
	xpadneo/device.o \
	xpadneo/events.o \
	xpadneo/keyboard.o \
	xpadneo/mappings.o \
	xpadneo/mouse.o \
	xpadneo/power.o \
	xpadneo/quirks.o \
	xpadneo/rumble.o \
	xpadneo/synthetic.o
