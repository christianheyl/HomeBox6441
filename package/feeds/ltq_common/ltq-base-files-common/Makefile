#
# Copyright (C) 2009 OpenWrt.org
# Copyright (C) 2011 lantiq.com
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.

include $(TOPDIR)/rules.mk

PKG_NAME:=ltq-base-files-common
PKG_RELEASE:=1

include $(INCLUDE_DIR)/package.mk

define Package/$(PKG_NAME)
  SUBMENU:=Base Files
  SECTION:=base
  CATEGORY:=Lantiq
  TITLE:=Common files for Lantiq systems
  URL:=http://www.lantiq.com/
  MAINTAINER:=Lantiq
endef

define Package/$(PKG_NAME)/description
 This package contains files and system scripts for OpenWrt,
 common for all Lantiq targets.
endef

define Build/Prepare
endef

define Build/Compile
endef

define Package/$(PKG_NAME)/install
	$(CP) ./files/* $(1)/
	$(SED) 's,$$$$R,$(REVISION),g' $(1)/etc/banner
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
