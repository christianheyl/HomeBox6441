#
# Copyright (C) 2006 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=ntpclient
PKG_VERSION:=2000_338
PKG_RELEASE:=1

PKG_SOURCE:=$(PKG_NAME)_$(PKG_VERSION)-arcadyan.tar.gz
PKG_SOURCE_URL:=http://doolittle.icarus.com/ntpclient/
#PKG_MD5SUM:=cb98711f11769cdd22fc592844cef414

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk

define Package/ntpclient
  SECTION:=net
  CATEGORY:=Network
  TITLE:=NTP (Network Time Protocol) client
  URL:=http://doolittle.icarus.com/ntpclient/
  DEPENDS:=+librt
endef

define Package/ntpclient/description
	NTP client for setting system time from NTP servers.
endef

MAKE_FLAGS += \
	all

define Package/ntpclient/install
#	$(INSTALL_DIR) $(1)/etc/hotplug.d/iface
#	$(INSTALL_DATA) ./files/ntpclient.hotplug $(1)/etc/hotplug.d/iface/20-ntpclient
	$(INSTALL_DIR) $(1)/etc/config
	$(INSTALL_CONF) ./files/ntpclient.config $(1)/etc/config/ntpclient
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/ntpclient $(1)/usr/sbin/
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/ntpclient.init $(1)/etc/init.d/ntpclient
	$(INSTALL_BIN) ./files/ntpclient2     $(1)/etc/init.d/ntpclient2
endef

$(eval $(call BuildPackage,ntpclient))
