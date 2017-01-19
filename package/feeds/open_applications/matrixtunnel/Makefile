# 
# Copyright (C) 2006 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
# $Id: Makefile 12175 2008-08-05 22:04:10Z florian $

include $(TOPDIR)/rules.mk

PKG_NAME:=matrixtunnel
PKG_VERSION:=0.2
PKG_RELEASE:=1

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=http://localhost
PKG_MD5SUM:=af169751efc7d87d500716a48d74ddc5

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)

PKG_BUILD_DEPENDS:=matrixssl
PKG_DEFAULT_CUSTOM_SOURCE_DIR:=$(TOPDIR)/../../../wlnsw/wireline_sw_linux24/open_matrixtunnel
PKG_NO_AUTORECONF=y

MATRIXSSL_DIR:=$(wildcard $(BUILD_DIR)/matrixssl*/matrixssl)

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/package-version-override.mk

define Package/matrixtunnel
  SECTION:=net
  CATEGORY:=Network
  SUBMENU:=VPN
  DEPENDS:=+matrixssl
  TITLE:=SSL tunnel based on matrixssl
  URL:=http://localhost
endef

define Package/matrixtunnel/description
	matrixtunnel is a stripped down version of stunnel 
	based on matrixssl library.
endef

define Build/Prepare
$(call Build/Prepare/Default)
	touch $(PKG_BUILD_DIR)/configure.in
endef

define Build/Configure
	if [ ! -e $(PKG_BUILD_DIR)/configure ];then \
		cd $(PKG_BUILD_DIR); \
		autoreconf -v --install; \
	fi;\
	$(call Build/Configure/Default, \
		--without-libiconv-prefix \
		--without-libintl-prefix \
		--with-matrixssl-src="$(MATRIXSSL_DIR)/" \
	)
endef

define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR)/src \
		DESTDIR="$(PKG_INSTALL_DIR)" \
		all install
endef

define Package/matrixtunnel/install
	$(INSTALL_DIR) $(1)/bin
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/bin/matrixtunnel $(1)/bin/tunnel
endef

define Build/Clean
	if [ -d $(PKG_BUILD_DIR) ]; then $(MAKE) -C $(PKG_BUILD_DIR) clean; \
	$(RM) $(PKG_BUILD_DIR)/ipk; fi
endef

$(eval $(call BuildPackage,matrixtunnel))
