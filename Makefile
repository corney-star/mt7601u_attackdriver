obj-m := mt7601u.o

mt7601u-objs := usb.o init.o main.o mcu.o trace.o dma.o core.o eeprom.o phy.o mac.o util.o debugfs.o tx.o

KDIR := /lib/modules/$(shell uname -r)/build
PWD  := $(shell pwd)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean