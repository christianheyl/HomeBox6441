#
# Copyright (C) 2006 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
include $(TOPDIR)/rules.mk

PKG_NAME:=arc_cipher
PKG_VERSION:=1.00

HOST_BUILD_DIR:=$(BUILD_DIR_HOST)/$(PKG_NAME)-$(PKG_VERSION)

include $(INCLUDE_DIR)/host-build.mk

UTIL_DIR=$(HOST_BUILD_DIR)/src

define Host/Install
	$(MAKE) -C $(UTIL_DIR) install
endef

define Host/Configure
	$(UTIL_DIR)/configure --prefix=$(STAGING_DIR_HOST)/bin/ $(UTIL_DIR)
endef

define Host/Compile
	$(MAKE) -C $(UTIL_DIR)
endef

define Host/Prepare
	mkdir -p $(UTIL_DIR)
	$(CP) ./src/* $(UTIL_DIR)
	chmod 755 -R $(UTIL_DIR)
endef

define Host/Clean
	$(MAKE) -C $(UTIL_DIR) clean
endef

$(eval $(call HostBuild))
