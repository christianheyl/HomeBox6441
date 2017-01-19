#
# Copyright (C) 2011 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=gpon-gphy-firmware
PKG_VERSION:=T0405
PKG_MD5SUM:=53cb3d67b7c73f8f6e9b4f11f881f9f9

include $(INCLUDE_DIR)/package-version-override.mk

PKG_BASE_NAME:=gpon-phy11g
PKG_SOURCE:=$(PKG_BASE_NAME)-$(PKG_VERSION).tar.gz
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)
PKG_RELEASE:=1
PKG_UNPACK:=$(TAR) -C $(PKG_BUILD_DIR) -xzf $(DL_DIR)/$(PKG_SOURCE)

include $(INCLUDE_DIR)/package.mk

PKG_SOURCE_URL:=http://localhost

ifeq ($(DUMP),)
	STAMP_CONFIGURED:=$(strip $(STAMP_CONFIGURED))_$(shell $(SH_FUNC) grep '^CONFIG_GPON_' $(TOPDIR)/.config | md5s)
endif

define Package/$(PKG_NAME)
  SECTION:=firmware
  CATEGORY:=Lantiq
  TITLE:=FALC-ON gphy firmware (phy11g)
  URL:=http://www.lantiq.com/
  MAINTAINER:=Lantiq Deutschland GmbH
  MENU:=1
  SUBMENU:=GPON
  DEPENDS:=gpon-onu-drv
endef

define Build/Compile
endef

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/lib/firmware
	$(CP) $(PKG_BUILD_DIR)/phy11g.bin $(1)/lib/firmware/
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
