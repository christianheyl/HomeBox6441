#
# (C) Copyright 2008 Atheros Communications, Inc.
# All rights reserved.
#

#
# Path to the fusion top
#
WLAN_TOP := $(obj)/$(DEPTH)
#
# Path to the common source base
#
HTC_COMMON := $(WLAN_TOP)

# HTC/WMI/HIF include files
HTC_INC := $(HTC_COMMON)/htc/inc -I$(HTC_COMMON)/htc/miscdrv
ifeq ($(strip ${BUS}),GMAC)
    BUSNAME=gmac
endif

HIF_INC := $(HTC_COMMON)/htc/hif/${BUSNAME}

# ADF include files
ADF_ADF_INC := $(WLAN_TOP)/adf/os/linux
ADF_LNX_OS_INC := $(WLAN_TOP)/adf/include/os
ADF_LNX_NBUF_INC := $(WLAN_TOP)/adf/include/nbuf
ADF_LNX_INC := $(WLAN_TOP)/adf/nbuf/linux
ADF_LNX_NET_INC := $(WLAN_TOP)/adf/include/net  -I$(WLAN_TOP)/adf/net/linux   
