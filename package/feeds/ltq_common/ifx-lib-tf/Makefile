#
# Copyright (C) 2010 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
# $Id: $

include $(TOPDIR)/rules.mk

PKG_NAME:=ifx-lib-tf
PKG_VERSION:=2.1.0
include $(INCLUDE_DIR)/package-version-override.mk

PKG_BASE_NAME:=lib_tf
PKG_SOURCE:=$(PKG_BASE_NAME)-$(PKG_VERSION).tar.gz
PKG_SOURCE_DIR:=$(BUILD_DIR)/$(PKG_BASE_NAME)-$(PKG_VERSION)
PKG_BUILD_DIR:=$(PKG_SOURCE_DIR)
PKG_RELEASE:=1
PKG_SOURCE_URL:=http://localhost
#PKG_MD5SUM:=
PKG_BUILD_DEPENDS:=ifxos
PKG_FIXUP:=autoreconf

include $(INCLUDE_DIR)/package.mk

ifeq ($(DUMP),)
	STAMP_CONFIGURED:=$(strip $(STAMP_CONFIGURED))_$(shell $(SH_FUNC) grep '^CONFIG_PACKAGE_lib-dti' $(TOPDIR)/.config | md5s)
endif

define Package/$(PKG_NAME)
  SECTION:=libs
  CATEGORY:=Lantiq
  TITLE:=Lantiq Test Framework library
  URL:=http://www.Lantiq.com/
  MAINTAINER:=Lantiq Deutschland GmbH
  DEPENDS:=+libpthread
  MENU:=1
endef

define Package/$(PKG_NAME)/description
	This library will be used by some Lantiq applications.
endef

#CONFIGURE_VARS += CC="$(TARGET_CROSS)gcc"

CONFIGURE_ARGS += \
	--enable-warnings \
	--enable-messages \
	--enable-trace \
	--enable-soap \
	--with-ifxos-incl=$(STAGING_DIR)/usr/include/ifxos \
	--with-ifxos-lib=$(STAGING_DIR)/usr/lib \
	--enable-target=$(STAGING_DIR)


define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/{lib,include/libtf/include}
	$(CP) $(PKG_SOURCE_DIR)/include/* $(1)/usr/include/libtf/include
	$(CP) $(PKG_SOURCE_DIR)/tc_server/report.h $(1)/usr/include/libtf/include/
	$(CP) $(PKG_SOURCE_DIR)/tc_dispatch_loc/tc_dispatch_loc.h $(1)/usr/include/libtf/include/
	mkdir -p $(1)/usr/lib/tc_server
	mkdir -p $(1)/usr/lib/tc_dispatch_loc
	$(CP) $(PKG_BUILD_DIR)/tc_server/libtc_server.a $(1)/usr/lib/tc_server/libtc_server.a
	$(CP) $(PKG_BUILD_DIR)/tc_dispatch_loc/libtc_disploc.a $(1)/usr/lib/tc_dispatch_loc/libtc_disploc.a
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
