#
# Copyright (C) 2006-2008 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
# $Id: Makefile 12369 2008-08-22 05:03:37Z nico $

include $(TOPDIR)/rules.mk

PKG_NAME:=ppp
PKG_VERSION:=2.3.11
PKG_RELEASE:=1

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION)-arcadyan.tar.gz
PKG_SOURCE_URL:=ftp://ftp.samba.org/pub/ppp/
#PKG_MD5SUM:=848f6c3cafeb6074ffeb293c3af79b7c
PKG_MD5SUM:=ea191a1d96ce83c080cc41e4868ad878

#PKG_BUILD_DEPENDS:=libpcap

include $(INCLUDE_DIR)/package.mk
#include $(INCLUDE_DIR)/kernel.mk

define Package/ppp/Default
  SECTION:=net
  CATEGORY:=Network
  URL:=http://ppp.samba.org/
endef

define Package/ppp
$(call Package/ppp/Default)
  DEPENDS:=
  TITLE:=PPPoE client daemon
endef

define Package/ppp/description
 This package contains the PPPoE (Point-to-Point Protocol over Ethernet) daemon.
endef

define Build/Configure
$(call Build/Configure/Default,, \
	UNAME_S="Linux" \
	UNAME_R="$(LINUX_VERSION)" \
	UNAME_M="$(ARCH)" \
)
endef

define Build/Prepare
	$(call Build/Prepare/Default,)
endef

define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR)/pppd \
	CC=$(TARGET_CC) \
	LD="$(TARGET_CROSS)ld" \
	LDFLAGS="-lcrypt" \
	LIBCRYPT=$(TOPDIR)/extern/target-install/lib/libcrypt.a
endef

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/include
endef

define Package/ppp/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/pppd/pppd $(1)/usr/sbin/
endef

$(eval $(call BuildPackage,ppp))
