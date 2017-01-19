#
# Copyright (C) 2006 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
# $Id: Makefile 12175 2008-08-05 22:04:10Z florian $

include $(TOPDIR)/rules.mk

PKG_NAME:=zebra
PKG_VERSION:=0.92a
PKG_RELEASE:=1
#PKG_MD5SUM:=ff1633f1ac026b720fa37b1856be3f48
#PKG_FIXUP=libtool
#PKG_NO_AUTORECONF=y

PKG_SOURCE_URL:=ftp://ftp.zebra.org 

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION)-arcadyan.tar.gz
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)

include $(INCLUDE_DIR)/package.mk
# include $(INCLUDE_DIR)/package-version-override.mk

define Package/$(PKG_NAME)
  SECTION:=net
  CATEGORY:=Network
  TITLE:=The zebra Software Routing Suite
  URL:=ftp://ftp.zebra.org
  MENU:=1
endef

define Package/$(PKG_NAME)/description
	GNU Zebra is free software that manages TCP/IP based routing protocols
endef

define Package/$(PKG_NAME)/config
$(call Package/$(PKG_NAME)/override_version)
$(call Package/$(PKG_NAME)/override_source_path)
	config FEATURE_RIP
		default y
		depends on PACKAGE_$(PKG_NAME)
		bool 'RIP Support'
endef

CONFIG_FULL_PACKAGE=y

define Build/Configure
	( cd $(PKG_BUILD_DIR) && CC=$(TARGET_CC) ./configure --disable-bgpd --disable-ripngd --disable-ospfd --disable-ospf6d --disable-bgp-announce )
endef

define Build/Compile	
	$(MAKE) -C $(PKG_BUILD_DIR) \
		CC=$(TARGET_CC) \
		AR=$(TARGET_CROSS)ar \
		LD=$(TARGET_CROSS)ld \
		RANLIB=$(TARGET_CROSS)ranlib
endef

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/zebra/zebra $(1)/usr/sbin/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/ripd/ripd $(1)/usr/sbin/
	$(INSTALL_DIR) $(1)/usr/lib
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/lib/libzebra.so $(1)/usr/lib/libzebra.so
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/etc/init.d/ripd.init $(1)/etc/init.d/ripd
	$(INSTALL_DIR) $(1)/etc/config
	$(INSTALL_DATA) ./files/etc/config/ripd.conf $(1)/etc/config/ripd.conf
endef

define Build/Clean
	if [ -d $(PKG_BUILD_DIR) ]; then $(MAKE) -C $(PKG_BUILD_DIR) clean; \
	$(RM) $(PKG_BUILD_DIR)/ipk; fi
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
