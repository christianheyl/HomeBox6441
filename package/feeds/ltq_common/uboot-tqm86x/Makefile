# 
# Copyright (C) 2008 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=u-boot
PKG_VERSION:=1.1.6
PKG_RELEASE:=1

PKG_BUILD_DIR:=$(KERNEL_BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.bz2
PKG_SOURCE_URL:=ftp://ftp.denx.de/pub/u-boot
PKG_MD5SUM:=5b1b1f7b3b1e06f75f5bfbd79891067b
PKG_TARGETS:=bin

include $(INCLUDE_DIR)/package.mk

UBOOT_CONFIG=$(strip $(subst ",, $(CONFIG_UBOOT_TARGET)))

define Package/uboot-tqm86x
  SECTION:=boot
  CATEGORY:=Boot Loaders
  DEPENDS:=@TARGET_mpc8xx_tqm8xx_24||@TARGET_mpc8xx_tqm8xx_26
  TITLE:=U-Boot for Lantiq PowerQUICC based boards
  URL:=http://www.denx.de/wiki/UBoot/WebHome
endef

define Build/Configure
	export CROSS_COMPILE=$(TARGET_CROSS); \
	$(MAKE) -C $(PKG_BUILD_DIR) $(UBOOT_CONFIG)_config
endef

define Build/Compile
	export CROSS_COMPILE=$(TARGET_CROSS); \
	$(MAKE) -C $(PKG_BUILD_DIR) 
endef

define Package/uboot-tqm86x/install
	$(CP) $(PKG_BUILD_DIR)/u-boot.bin $(1)/$(PKG_NAME)-tqm86x-$(PKG_VERSION)-$(PKG_RELEASE).pkg
endef

$(eval $(call BuildPackage,uboot-tqm86x))
