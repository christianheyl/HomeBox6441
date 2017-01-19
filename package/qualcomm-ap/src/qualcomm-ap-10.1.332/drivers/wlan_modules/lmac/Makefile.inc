#
# (C) Copyright 2007 Atheros Communications, Inc.
# All rights reserved.
#

#
# Path to the fusion top
#
ifeq ($(TARGET),ANDROID)
WLAN_TOP := $(ATH_DRIVER_TOPDIR)/drivers/wlan_modules
else
WLAN_TOP := $(obj)/$(DEPTH)
endif
#
# Path to the common source base
#
COMMON := $(WLAN_TOP)/lmac

#
# Path to common include files
#
COMINC := $(WLAN_TOP)/include

#
# Path to the HAL source code.
#
HAL := $(WLAN_TOP)/hal

#
# Path to the Atheros Device module.
#
ATH := $(COMMON)/ath_dev

#
# Path to the rate control algorithm.
#
ATH_RATE := $(COMMON)/ratectrl

#
# Path to the DFS module.
#
ATH_DFS := $(COMMON)/dfs

# Packet logging
ATH_PKTLOG := $(COMMON)/ath_pktlog

#
# Path to the SPECTRAL module.
#
ATH_SPECTRAL := $(COMMON)/spectral

#
# Path to the TX99 module.
#
TX99 := $(COMMON)/tx99
