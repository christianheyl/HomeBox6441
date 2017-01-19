#
# Copyright (C) 2006 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
# $Id: Makefile 14825 2009-03-09 21:57:11Z florian $

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=qualcomm-ap
PKG_VERSION:=10.1.467
PKG_RELEASE:=1

include $(INCLUDE_DIR)/package.mk

define KernelPackage/$(PKG_NAME)
  SUBMENU:=Wireless Drivers
  CATEGORY:=Kernel modules
  SECTION:=kernel
  TITLE:=Kernel driver for Qualcomm 9880 chipsets
  FILES:= $(PKG_BUILD_DIR)/apps/wireless_tools.29/iwconfig \
  $(PKG_BUILD_DIR)/apps/wireless_tools.29/iwpriv
  KCONFIG+= \
    CONFIG_KMALLOC_HIGH_ATHEROS=y \
    CONFIG_IFX_PCIE_HW_SWAP=y \
    CONFIG_QUALCOMM_AP_PPA=y
endef

define KernelPackage/$(PKG_NAME)/description
 This package contains wlan_ap example.
endef

# ARCADYAN #ARC_RC_SD2#, 2013/6/18, Dino, support of loading 9880 cal data from flash
	export AH_CAL_IN_FLASH=1
	export ATH_CAL_NAND_FLASH=1
	export ATH_CAL_NAND_PARTITION=manuf
	export AH_CAL_LOCATIONS=0x4000
	export AH_CAL_IN_FLASH_PCI=1
	export AH_CAL_RADIOS_PCI=1
	#export AH_CAL_LOCATIONS_PCI=0xbfff4000
	#export AH_CAL_LOCATIONS_PCI=0x4000


MAKE_KMOD := $(MAKE) -C "$(LINUX_DIR)" \
	CROSS_COMPILE="$(TARGET_CROSS)" \
	ARCH="$(LINUX_KARCH)" \
	PATH="$(TARGET_PATH)" \

define Build/Prepare
	rm -rf $(PKG_BUILD_DIR)
	mkdir -p $(PKG_BUILD_DIR)
	mkdir -p $(PKG_INSTALL_DIR)

	if [ -d "./src/$(PKG_NAME)-$(PKG_VERSION)" ] ; then \
		$(CP) ./src/$(PKG_NAME)-$(PKG_VERSION)/* $(PKG_INSTALL_DIR)/ ; \
	fi

	$(CP) ./src/$(PKG_NAME)-$(PKG_VERSION)/* $(PKG_BUILD_DIR)/
	$(CP) ./src/rootfs/ $(PKG_BUILD_DIR)
endef

define Build/Compile
	$(CP) ./src/$(PKG_NAME)-$(PKG_VERSION)/* $(PKG_BUILD_DIR)/
	$(MAKE) -C $(PKG_BUILD_DIR)/build BOARD_TYPE=lq-platform BUILD_UGW=y BUILD_LANTIQ=UGW_5.2_VR9_NAND PKG_BUILD_DIR=$(PKG_BUILD_DIR) \
		CC=$(TARGET_CC) \
		AR=$(TARGET_CROSS)ar \
		LD=$(TARGET_CROSS)ld
endef

define Build/Install
endef

define Build/Uninstall
endef

define KernelPackage/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/lib/network/
endef

$(eval $(call KernelPackage,$(PKG_NAME)))
