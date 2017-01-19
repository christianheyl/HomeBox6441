#
# Copyright (C) 2009 OpenWrt.org
# Copyright (C) 2012 Lantiq
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=gpon-base-files-mdu
PKG_RELEASE:=3

include $(INCLUDE_DIR)/package.mk

ifeq ($(DUMP),)
	STAMP_CONFIGURED:=$(strip $(STAMP_CONFIGURED))_$(shell $(SH_FUNC) grep '^CONFIG_GPON_' $(TOPDIR)/.config | md5s)
endif

define Package/$(PKG_NAME)
  SUBMENU:=Base Files
  SECTION:=base
  CATEGORY:=Lantiq
  TITLE:=MDU base files.
  URL:=http://www.lantiq.com/
  MAINTAINER:=Lantiq
  DEPENDS:=
endef


define Package/$(PKG_NAME)/description
 This package contains files and system scripts for OpenWrt.
endef

define Build/Prepare
endef

define Build/Compile
endef

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/opt/lantiq/bin/reset-vinax.sh $(1)/etc/init.d/
	$(INSTALL_BIN) ./files/opt/lantiq/bin/vinax.sh $(1)/etc/init.d/
	$(INSTALL_DIR) $(1)/opt/lantiq/bin
	$(INSTALL_BIN) ./files/opt/lantiq/bin/config-vinax.sh $(1)/opt/lantiq/bin/dms_default_config.sh
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
