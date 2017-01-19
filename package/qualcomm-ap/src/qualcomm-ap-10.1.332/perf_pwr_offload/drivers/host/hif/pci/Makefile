#------------------------------------------------------------------------------
# <copyright file="makefile" company="Atheros">
#    Copyright (c) 2010 Atheros Corporation.  All rights reserved.
# $ATH_LICENSE_HOSTSDK0_C$
#------------------------------------------------------------------------------
#==============================================================================
# Author(s): ="Atheros"
#==============================================================================

#
# PCIe Host InterFace
#  

EXTRA_CFLAGS += -DHIF_MESSAGE_BASED 
EXTRA_CFLAGS += -DHIF_PCI

ATH_HIF_MESSAGE_BASED =yes

hif_pci-objs := \
	../../hif/pci/copy_engine.o \
	../../hif/pci/linux/ath_pci.o \
	../../hif/pci/linux/ath_sysfs.o \
	../../hif/pci/hif_pci.o

all:

clean:
