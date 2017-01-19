# 
# Copyright (C) 2006 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
# $Id: Makefile 12228 2008-08-06 22:43:15Z nbd $

include $(TOPDIR)/rules.mk

PKG_NAME:=matrixssl
PKG_VERSION:=1.8.3
PKG_RELEASE:=1

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.bz2
PKG_SOURCE_URL:=http://nbd.vd-s.ath.cx/openwrt
PKG_MD5SUM:=c732627eedb9c4c2d5b772ccb18f582f
PKG_DEFAULT_CUSTOM_SOURCE_DIR:=$(TOPDIR)/../../../wlnsw/wireline_sw_linux24/open_matrixssl
PKG_NO_AUTORECONF=y

#PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/package-version-override.mk

define Package/matrixssl
  SECTION:=libs
  CATEGORY:=Libraries
  DEPENDS:=+libpthread
  TITLE:=Embedded SSL implementation
  URL:=http://www.matrixssl.org/
  MENU:=1
endef

define Build/Configure
endef

TARGET_CFLAGS += $(FPIC)

define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR)/matrixssl/src \
		$(TARGET_CONFIGURE_OPTS) \
		DFLAGS="$(TARGET_CFLAGS) -Wall"
endef

define Build/InstallDev
	mkdir -p $(1)/usr/include/matrixSsl
	$(CP) $(PKG_BUILD_DIR)/matrixssl/matrixSsl.h $(1)/usr/include/matrixSsl/
	ln -sf matrixSsl/matrixSsl.h $(1)/usr/include/matrixSsl.h
	mkdir -p $(1)/usr/lib
	$(CP) $(PKG_BUILD_DIR)/matrixssl/src/libmatrixssl.so* $(1)/usr/lib/
endef

define Package/matrixssl/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_BUILD_DIR)/matrixssl/src/libmatrixssl.so* $(1)/usr/lib/
endef

define Build/Clean
	if [ -d $(PKG_BUILD_DIR)/matrixssl ]; then $(MAKE) -C $(PKG_BUILD_DIR)/matrixssl/src clean; \
	$(RM) $(PKG_BUILD_DIR)/ipk; fi
endef

$(eval $(call BuildPackage,matrixssl))
