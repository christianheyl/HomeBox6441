# Makefile for Atheros Driver Framework, Atheros Test Driver 
# & Atheros Portable Driver (aka ADF,ATD & APD)
#
# Copyright (C) 2002 - 2008 Atheros Communications Inc.
#

ADF_DIR = ../adf
ASF_DIR = ../../asf
ADF_LINUX = $(ADF_DIR)/linux
MODULE_NAME = adf
$(MODULE_NAME)-objs += nbuf/adf_nbuf.o
$(MODULE_NAME)-objs += net/adf_net.o
$(MODULE_NAME)-objs += net/adf_net_ioctl.o
$(MODULE_NAME)-objs += os/adf_os_irq_pvt.o
$(MODULE_NAME)-objs += os/adf_os_defer_pvt.o

RM      = @rm -rf
JUNK	= *~ *.bak DEADJOE

EXTRA_CFLAGS += -I$(PWD)/$(ASF_DIR)/include/
EXTRA_CFLAGS += -I$(PWD)/../include/net
EXTRA_CFLAGS += -I$(PWD)/../include/nbuf
EXTRA_CFLAGS += -I$(PWD)/../include/os
EXTRA_CFLAGS += -I$(PWD)/os
EXTRA_CFLAGS += -I$(PWD)/net
EXTRA_CFLAGS += -I$(PWD)/nbuf



# First pass, kernel Makefile reads module objects
ifneq ($(KERNELRELEASE),)
obj-m	:= $(MODULE_NAME).o

# Second pass, the actual build.
else
KVER    ?= $(shell uname -r)
KDIR	:= /lib/modules/$(KVER)/build
PWD	:= $(shell pwd)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

distclean: clean
	$(RM) $(JUNK) $(OBJS)

help:
	$(MAKE) -C $(KDIR) M=$(PWD) help


# Indents the kernel source the way linux/Documentation/CodingStyle.txt
# wants it to be.
indent:
	indent -kr -i8 $($(MODULE_NAME)-objs:.o=.c)

install:
	$(MAKE) -C $(KDIR) M=$(PWD) modules_install

endif
