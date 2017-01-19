# 
# Copyright (C) 2006 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
# $Id: $

include $(TOPDIR)/rules.mk

PKG_NAME:=ifx-vos
PKG_VERSION:=0.0.9
include $(INCLUDE_DIR)/package-version-override.mk

PKG_BASE_NAME:=vos-library
PKG_SOURCE:=$(PKG_BASE_NAME)-$(PKG_VERSION).tar.gz
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_BASE_NAME)-$(PKG_VERSION)
PKG_RELEASE:=1
PKG_SOURCE_URL:=http://localhost
#PKG_MD5SUM:=

include $(INCLUDE_DIR)/package.mk

define Package/$(PKG_NAME)
  SECTION:=libs
  CATEGORY:=Lantiq
  TITLE:=Virtual OS abstraction library (deprecated!)
  URL:=http://www.lantiq.com/
  MAINTAINER:=Lantiq
  DEPENDS:=+libpthread
endef

define Package/$(PKG_NAME)/description
	This library will be used by some Infineon applications.
	DEPRECATED: Please use ifx-os for new applications!
endef

CONFIGURE_ARGS += \
	--enable-warnings \
	--enable-debug \
	--enable-messages \
	--enable-wineasy

define Build/InstallDev
	mkdir -p $(STAGING_DIR)/usr/lib
	$(CP) $(PKG_BUILD_DIR)/src/libvos_all.a $(STAGING_DIR)/usr/lib/libvos.a
	$(CP) $(PKG_BUILD_DIR)/src/libvos_all.a $(STAGING_DIR)/usr/lib/
	$(INSTALL_DIR) $(1)/usr/include
	$(CP) $(PKG_BUILD_DIR)/include/* $(1)/usr/include/
endef



$(eval $(call BuildPackage,$(PKG_NAME)))

