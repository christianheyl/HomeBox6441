#
# Copyright (C) 2006-2009 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=dhrystone
PKG_VERSION:=2.1
PKG_RELEASE:=1

PKG_MD5SUM:=f17bf168821901c29ccb074fcfd2dfe9
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=\
	http://netsw.org/benchmark/system/ \
	http://ftp.unicamp.br/pub/unix-c/benchmark/system/

include $(INCLUDE_DIR)/package.mk

PKG_UNPACK:=gzip -dc $(DL_DIR)/$(PKG_SOURCE) | $(TAR) -C $(PKG_BUILD_DIR) -xvf - 

define Package/dhrystone
  SECTION:=utils
  CATEGORY:=Benchmarks
  TITLE:=Dhrystone benchmark
endef

define Package/dhrystone/description
  Dhrystone benchmark
endef

EXTRA_CFLAGS:=-fno-builtin-malloc -fno-builtin-strcpy

define Build/Compile_
	$(call Build/Compile/Default,clobber all)
endef


define Package/dhrystone/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/gcc_dry2 $(1)/usr/bin/dhrystone
endef

$(eval $(call BuildPackage,dhrystone))
