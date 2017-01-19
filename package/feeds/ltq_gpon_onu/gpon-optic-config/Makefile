#
# Copyright (C) 2009 OpenWrt.org
# Copyright (C) 2011 lantiq.com
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=gpon-optic-config
PKG_VERSION:=3.0.9
PKG_RELEASE:=3

include $(INCLUDE_DIR)/package.mk

ifeq ($(DUMP),)
	STAMP_CONFIGURED:=$(strip $(STAMP_CONFIGURED))_$(shell $(SH_FUNC) grep '^CONFIG_GPON_' $(TOPDIR)/.config | md5s)
endif

define Package/$(PKG_NAME)
  #SECTION:=driver
  CATEGORY:=Lantiq
  TITLE:=GPON Optic configurations files
  URL:=http://www.lantiq.com/
  MAINTAINER:=Lantiq
  MENU:=1
  SUBMENU:=GPON
  DEBUG_LEVEL:=1
endef

define Package/$(PKG_NAME)/description
   The configurations files for the GPON device (optic part)
endef

define Package/$(PKG_NAME)/config_disabled
	menu "configuration"
	depends on PACKAGE_$(PKG_NAME)
		source "$(SOURCE)/config/Config.in"
	endmenu
	$(call Package/$(PKG_NAME)/override_version)
	$(call Package/$(PKG_NAME)/override_source_path)
endef

define Package/$(PKG_NAME)/conffiles
/etc/config/goi_config
/etc/optic/goi_age
/etc/optic/goi*.csv
endef

define Build/Prepare
endef

define Build/Compile
endef

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/etc/config
	$(INSTALL_DIR) $(1)/etc/optic
	$(INSTALL_BIN) files/config/goi_config $(1)/etc/config
	$(INSTALL_BIN) files/config/goi_age $(1)/etc/optic
	$(INSTALL_BIN) files/config/*.csv $(1)/etc/optic
	$(INSTALL_BIN) files/config/*.sh $(1)/etc/optic
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
