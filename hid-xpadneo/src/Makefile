#
# Makefile for the HID driver
#
ccflags-y += -DVERSION=$(VERSION)
obj-m += hid-xpadneo.o

#FIXME(kakra) Work-around, cleanup after source code split
hid-xpadneo-y += xpadneo.o
$(obj)/xpadneo.c: $(src)/hid-xpadneo.c
	cp $< $@

hid-xpadneo-y += xpadneo/core.o xpadneo/consumer.o
