#
# Copyright (C) 2006-2008 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=vsftpd
PKG_VERSION:=2.0.3
PKG_RELEASE:=1

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_MD5SUM:=74936cbd8e8251deb1cd99c5fb18b6f8
PKG_SOURCE_URL:=http://mirror2.openwrt.org/sources

include $(INCLUDE_DIR)/package.mk

define Package/$(PKG_NAME)
  SUBMENU:=FTP
  SECTION:=net
  CATEGORY:=Network
  TITLE:=vsftpd
endef

define Build/Prepare
	$(call Build/Prepare/Default)
	cp -rf src/* $(PKG_BUILD_DIR)
endef

define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR) \
		CC="$(TARGET_CROSS)gcc" \
		STRIP="$(TARGET_CROSS)strip"
endef

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/sbin
	$(INSTALL_DIR) $(1)/etc
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/vsftpd $(1)/sbin/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/vsftpd.conf $(1)/etc/
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
