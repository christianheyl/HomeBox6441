# 
# Copyright (C) 2007 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
# $Id$

include $(TOPDIR)/rules.mk

PKG_NAME:=memedit
PKG_VERSION:=0.7
PKG_RELEASE:=1

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=http://www.pengutronix.de/software/memedit/downloads
PKG_MD5SUM:=

include $(INCLUDE_DIR)/package.mk

define Package/memedit
  SECTION:=utils
  CATEGORY:=Utilities
  TITLE:=A small tool to manipulate memory mapped registers.
  DEPENDS:=+libncurses +libreadline
  URL:=http://www.pengutronix.de/software/memedit/index_en.html
endef

define Package/memedit/description
  A small tool to manipulate memory mapped registers.
endef

define Package/memedit/install
	$(INSTALL_DIR) $(1)/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/memedit $(1)/bin/
endef

$(eval $(call BuildPackage,memedit))
