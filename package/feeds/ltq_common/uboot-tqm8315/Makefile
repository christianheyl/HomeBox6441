# 
# Copyright (C) 2008 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=u-boot
PKG_VERSION:=2009.08-rc1
PKG_RELEASE:=1

PKG_BUILD_DIR:=$(KERNEL_BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.bz2
PKG_SOURCE_URL:=ftp://ftp.denx.de/pub/u-boot
PKG_MD5SUM:=efef9b182fad55a57f419ccb0dfcd3e6
PKG_TARGETS:=bin

include $(INCLUDE_DIR)/package.mk

# UBOOT_CONFIG=$(strip $(subst ",, $(CONFIG_UBOOT_TARGET)))
UBOOT_CONFIG:=TQM8315

define Package/uboot-tqm8315
  SECTION:=boot
  CATEGORY:=Boot Loaders
  #DEPENDS:=@TARGET_mpc83xx_tqm8315
  TITLE:=U-Boot for Lantiq PowerQUICC II based boards
  URL:=http://www.denx.de/wiki/UBoot/WebHome
endef


# Create hardlinks for files. This will only work if this feed and the build_dir are on the same filesystem!
ifneq ($(QUILT),)
  CP_OPTS=-l
endif

define Build/Prepare
	$(PKG_UNPACK)
	cp -rL $(CP_OPTS) $(FILES_DIR)/* $(PKG_BUILD_DIR)/
	$(Build/Patch/Default)
endef

define Build/Configure
	export CROSS_COMPILE=$(TARGET_CROSS); \
	$(MAKE) -C $(PKG_BUILD_DIR) $(UBOOT_CONFIG)_config
endef

define Build/Compile
	export CROSS_COMPILE=$(TARGET_CROSS); \
	$(MAKE) -C $(PKG_BUILD_DIR) 
endef

define Package/uboot-tqm8315/install
	$(CP) $(PKG_BUILD_DIR)/u-boot.bin $(1)/$(PKG_NAME)-tqm8315-$(PKG_VERSION)-$(PKG_RELEASE).pkg
endef

$(eval $(call BuildPackage,uboot-tqm8315))
