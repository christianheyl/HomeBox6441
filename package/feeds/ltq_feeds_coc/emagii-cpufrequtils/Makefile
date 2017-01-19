#
# Copyright (C) 2006-2012 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=cpufrequtils
PKG_RELEASE:=1
PKG_BASE_NAME:=emagii-cpufrequtils
PKG_VERSION:=a2f0c39
PKG_MD5SUM:=9bc1e3a1152bd147512a5e139ea4904d
PKG_SOURCE_URL:=http://github.com/emagii/cpufrequtils/tarball/$(PKG_VERSION)
PKG_SOURCE:=$(PKG_BASE_NAME)-$(PKG_VERSION).tar.gz
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_BASE_NAME)-$(PKG_VERSION)

PKG_INSTALL:=1

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/nls.mk

define Package/$(PKG_NAME)
  SECTION:=utils
  CATEGORY:=Utilities
  TITLE:=Utils based on linux cpufreq driver
  URL:=http://www.kernel.org/pub/linux/utils/kernel/cpufreq/
  MAINTAINER:=kernel.org
  DEPENDS:=
endef

define Package/$(PKG_NAME)/description
   Cpufrequtils is a set of utilities designed to assist CPU frequency scaling.
   Features:
      - set new frequency
      - set new governor
      - set new policy, frequency table
   Example: cpufreq-set -c 0 -g userspace -f 500khz
endef

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/usr/{bin,lib}
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/bin/{cpufreq-info,cpufreq-set} $(1)/usr/bin/
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/libcpufreq.so.* $(1)/usr/lib/
endef

define Package/InstallDev
	$(INSTALL_DIR) $(1)/usr/{lib,include}
	$(CP) $(PKG_INSTALL_DIR)/usr/include/* $(1)/usr/include
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/libcpufreq.so.* $(1)/usr/lib/
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
