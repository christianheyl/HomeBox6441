#
# Makefile for the switch api
#
EXTRA_CFLAGS := -I./include/switch_api

ifeq ($(CONFIG_VR9),y)
EXTRA_CFLAGS += -DVR9
endif
ifeq ($(CONFIG_AR10),y)
EXTRA_CFLAGS += -DAR10
endif
ifeq ($(CONFIG_HN1),y)
#EXTRA_CFLAGS := -I$(srctree)/include/switch_api
EXTRA_CFLAGS += -DHN1
endif
ifeq ($(CONFIG_GE_MODE),y)
EXTRA_CFLAGS += -DGE_MODE
endif

ifeq ($(CONFIG_FE_MODE),y)
EXTRA_CFLAGS += -DFE_MODE
endif

ifeq ($(CONFIG_GEFE_MIXED_MODE),y)
EXTRA_CFLAGS += -DGEFE_MIXED_MODE
endif

  obj-$(CONFIG_GPHY_DRIVER)     += drv_ethsw_gphy.o 

ifeq ($(CONFIG_VR9),y)
  drv_ethsw_gphy-objs   += ltqmips_xrx_gphy.o
endif
ifeq ($(CONFIG_AR10),y)
  drv_ethsw_gphy-objs   += ltqmips_xrx_gphy.o
endif
ifeq ($(CONFIG_HN1),y)
  drv_ethsw_gphy-objs   += ltqmips_xrx_gphy.o
endif
