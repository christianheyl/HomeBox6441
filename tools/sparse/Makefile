#
# Copyright (C) 2010 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
include $(TOPDIR)/rules.mk

PKG_NAME:=sparse
PKG_VERSION:=0.4.3
PKG_RELEASE:=1

PKG_SOURCE_URL:=http://www.kernel.org/pub/software/devel/sparse/dist/
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.bz2
PKG_MD5SUM:=a5c0b07bd00ad5ea292f804d7af1adbc

include $(INCLUDE_DIR)/host-build.mk

define Host/Configure
endef

define Host/Compile
	$(MAKE) CC="$(HOSTCC)" -C $(HOST_BUILD_DIR) SHELL="$(BASH)"
endef

define Host/Install
	$(INSTALL_DIR) $(STAGING_DIR_HOST)/bin
	$(INSTALL_BIN) $(HOST_BUILD_DIR)/sparse $(STAGING_DIR_HOST)/bin/
	$(INSTALL_BIN) $(HOST_BUILD_DIR)/cgcc $(STAGING_DIR_HOST)/bin/
endef

$(eval $(call HostBuild))
