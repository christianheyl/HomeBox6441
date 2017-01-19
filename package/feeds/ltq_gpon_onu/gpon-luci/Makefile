#
# Copyright (C) 2009 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=gpon-luci
PKG_VERSION:=0.0.1
PKG_RELEASE:=1

PKG_BASE_NAME:=gpon-luci
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)

PKG_BUILD_DEPENDS:=luci-lib-core

include $(INCLUDE_DIR)/package.mk

ifeq ($(DUMP),)
	STAMP_CONFIGURED:=$(strip $(STAMP_CONFIGURED))_$(shell $(SH_FUNC) grep '^CONFIG_GPON_' $(TOPDIR)/.config | md5s)
endif

define Package/gpon-luci
  SECTION:=library
  CATEGORY:=Lantiq
  TITLE:=LuCI Extension for Lantiq GPON device
  URL:=http://www.lantiq.com/
  MAINTAINER:=Lantiq
  DEPENDS:=+luci-lib-core
  SUBMENU:=LuCI
endef

define Package/gpon-luci/description
	This package adds the GPON extension to the LuCI Web Interface
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(TAR) c -C ./files . | $(TAR) x -C $(PKG_BUILD_DIR)/
endef

define Build/Configure
endef 

define Build/Compile
endef 

define Package/gpon-luci/install
	$(INSTALL_DIR) $(1)/usr/lib/lua/luci
	$(CP) -aR ./files/* $(1)/usr/lib/lua/luci
endef

$(eval $(call BuildPackage,gpon-luci))
