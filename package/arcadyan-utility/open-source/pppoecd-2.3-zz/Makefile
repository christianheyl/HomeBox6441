#
# Copyright (C) 2006-2008 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
# $Id: Makefile 12369 2008-08-22 05:03:37Z nico $

include $(TOPDIR)/rules.mk

PKG_NAME:=pppoecd
PKG_VERSION:=2.3
PKG_RELEASE:=1

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=ftp://ftp.samba.org/pub/ppp/
#PKG_MD5SUM:=848f6c3cafeb6074ffeb293c3af79b7c
PKG_MD5SUM:=2de8c67542125101b3c2565f5ba266cf

#PKG_BUILD_DEPENDS:=libpcap

include $(INCLUDE_DIR)/package.mk
#include $(INCLUDE_DIR)/kernel.mk

define Package/pppoecd/Default
  SECTION:=net
  CATEGORY:=Network
  URL:=http://ppp.samba.org/
endef

define Package/pppoecd
$(call Package/pppoecd/Default)
  DEPENDS:=
  TITLE:=PPPoE client daemon
endef

define Package/pppoecd/description
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
	$(MAKE) -C $(PKG_BUILD_DIR)/pppoecd \
	CC=$(TARGET_CC) \
	LD="$(TARGET_CROSS)ld" \
	LIBCRYPT=$(TOOLCHAIN_DIR)/usr/lib/libcrypt.a

	$(MAKE) -C $(PKG_BUILD_DIR)/chat \
	CC=$(TARGET_CC) \
	LD="$(TARGET_CROSS)ld" \
	LIBCRYPT=$(TOOLCHAIN_DIR)/usr/lib/libcrypt.a
endef

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/include
endef

define Package/pppoecd/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/pppoecd/pppoecd $(1)/usr/sbin/
endef

$(eval $(call BuildPackage,pppoecd))
