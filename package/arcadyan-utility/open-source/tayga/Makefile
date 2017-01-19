#
# Copyright (C) 2011 Arcadyan Inc.
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=tayga
PKG_VERSION:=0.9.2
PKG_RELEASE:=1

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=http://www.litech.org/tayga/tayga-0.9.2.tar.bz2

include $(INCLUDE_DIR)/package.mk

define Package/tayga/Default
  SECTION:=net
  CATEGORY:=Network
  TITLE:=Simple, no-fuss NAT64 for Linux
endef

define Package/tayga
  $(call Package/tayga/Default)
endef

define Package/tayga/Compile
	$(call Package/tayga/Compile/Default, all)
endef

define Package/tayga/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/tayga $(1)/usr/sbin/
endef

$(eval $(call BuildPackage,tayga))
