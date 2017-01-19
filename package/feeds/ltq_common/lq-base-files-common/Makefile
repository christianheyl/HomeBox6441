#
# Copyright (C) 2009 OpenWrt.org
# Copyright (C) 2011 lantiq.com
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.

include $(TOPDIR)/rules.mk

PKG_NAME:=lq-base-files-common
PKG_RELEASE:=1

include $(INCLUDE_DIR)/package.mk

define Package/$(PKG_NAME)
  SUBMENU:=Base Files
  SECTION:=base
  CATEGORY:=Lantiq
  TITLE:=Common files for Lantiq systems (transitional package)
  URL:=http://www.lantiq.com/
  MAINTAINER:=Lantiq
  DEPENDS:=+ltq-base-files-common
endef

define Package/$(PKG_NAME)/description
 This package is only transitional to help renaming to ltq-base-files-common.
 Can be unselected/removed later.
endef

define Build/Prepare
endef

define Build/Compile
endef

define Package/$(PKG_NAME)/install
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
