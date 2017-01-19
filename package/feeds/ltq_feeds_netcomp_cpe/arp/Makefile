#
# Copyright (C) 2010 LANTIQ AG
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

#### Includes ###################################
include $(TOPDIR)/rules.mk

#### Package Info ###############################
PKG_NAME:=arp
PKG_VERSION:=1.0
PKG_RELEASE:=1

include $(INCLUDE_DIR)/package.mk

define Package/$(PKG_NAME)
	SECTION:=net
	CATEGORY:=Network
	TITLE:=ARP utility support
	URL:=http://www.lantiq.com
	MAINTAINER:=LANTIQ Technologies AG
endef

define Package/$(PKG_NAME)/description
	ARP This package contains arp utility support
endef

#### Target Rules ###############################
define Build/Prepare
	$(call Build/Prepare/Default)
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Build/Clean
	rm -rf $(PKG_BUILD_DIR)/$(PKG_NAME)
endef

define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR)/ \
	$(TARGET_CONFIGURE_OPTS)
endef

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/arp $(1)/usr/bin/
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
