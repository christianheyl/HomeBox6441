#
# Copyright (C) 2009 Arcadyan Corporation
# All Rights Reserved.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=uboot_env
PKG_RELEASE:=1

PKG_BUILD_DIR := $(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/kernel.mk
include $(INCLUDE_DIR)/package.mk
CONFIG_UBOOT_CONFIG_NAND_FLASH_SIZE=128
CONFIG_BASE_FILES_CO_NOR_NAND=y

define Package/$(PKG_NAME)
	SUBMENU:=Utilities
	SECTION:=utils
	CATEGORY:=marvell
	TITLE:=common utilities
	MENU:=1
endef

define Package/$(PKG_NAME)/description
	uboot env utility
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

BOOT_FROM=CFG_BOOT_FROM_NAND
FLASH_SIZE=$(CONFIG_UBOOT_CONFIG_NAND_FLASH_SIZE)
IN_SUPERTASK=0

## ctc ###
define Build/Compile
	if [ A$(CONFIG_BASE_FILES_CO_NOR_NAND) = Ay ] ; then \
		$(MAKE) -C $(PKG_BUILD_DIR) \
			CC="$(TARGET_CC)" \
			STRIP=$(TARGET_CROSS)strip \
			CFLAGS="$(TARGET_CFLAGS) -Wall -I$(STAGING_DIR)/usr/include -I$(LINUX_DIR)/include -DIFX_CONFIG_MEMORY_SIZE=$(CONFIG_UBOOT_CONFIG_IFX_MEMORY_SIZE) -DCFG_BOOT_FROM_NOR -DIFX_CONFIG_FLASH_SIZE=$(FLASH_SIZE) -DIN_SUPERTASK=$(IN_SUPERTASK) $(DUAL_IMAGE)" \
			LDFLAGS="$(TARGET_LDFLAGS)" ; \
		mv $(PKG_BUILD_DIR)/uboot_env $(PKG_BUILD_DIR)/uboot_env_nor ; \
		$(MAKE) -C $(PKG_BUILD_DIR) clean ; \
		$(MAKE) -C $(PKG_BUILD_DIR) \
			CC="$(TARGET_CC)" \
			STRIP=$(TARGET_CROSS)strip \
			CFLAGS="$(TARGET_CFLAGS) -Wall -I$(STAGING_DIR)/usr/include -I$(LINUX_DIR)/include -DIFX_CONFIG_MEMORY_SIZE=$(CONFIG_UBOOT_CONFIG_IFX_MEMORY_SIZE) -DCFG_BOOT_FROM_NAND -DIFX_CONFIG_FLASH_SIZE=$(CONFIG_UBOOT_CONFIG_NAND_FLASH_SIZE) -DIN_SUPERTASK=$(IN_SUPERTASK) $(DUAL_IMAGE)" \
			LDFLAGS="$(TARGET_LDFLAGS)" ; \
		mv $(PKG_BUILD_DIR)/uboot_env $(PKG_BUILD_DIR)/uboot_env_nand ; \
	else \
		$(MAKE) -C $(PKG_BUILD_DIR) \
			CC="$(TARGET_CC)" \
			STRIP=$(TARGET_CROSS)strip \
			CFLAGS="$(TARGET_CFLAGS) -Wall -I$(STAGING_DIR)/usr/include -I$(LINUX_DIR)/include -DIFX_CONFIG_MEMORY_SIZE=$(CONFIG_UBOOT_CONFIG_IFX_MEMORY_SIZE) -D$(BOOT_FROM) -DIFX_CONFIG_FLASH_SIZE=$(FLASH_SIZE) -DIN_SUPERTASK=$(IN_SUPERTASK) $(DUAL_IMAGE)" \
			LDFLAGS="$(TARGET_LDFLAGS)" ; \
	fi
endef
###########

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/uboot_env_nand   $(1)/usr/sbin/uboot_env ;
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
