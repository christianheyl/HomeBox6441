#
# Copyright (C) 2006 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
# $Id: Makefile 12755 2008-09-28 08:34:06Z florian $

include $(TOPDIR)/rules.mk

PKG_NAME:=rp-pppoe
PKG_VERSION:=3.5
PKG_RELEASE:=1

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=http://localhost
PKG_MD5SUM:=97972f8f8f6a3ab9b7070333a6a29c4b
PKG_DEFAULT_CUSTOM_SOURCE_DIR:=$(TOPDIR)/../../../wlnsw/wireline_sw_linux24/open_rp_pppoe
PKG_NO_AUTORECONF=y

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/package-version-override.mk

#define Package/rp-pppoe
#  SECTION:=net
#  CATEGORY:=Network
#  DEPENDS:=+ppp
#  TITLE:=PPPoE (PPP over Ethernet)
#  URL:=http://roaringpenguin.com/products/pppoe
#  SUBMENU:=dial-in/up
#endef

define Package/rp-pppoe/Default
  SECTION:=net
  CATEGORY:=Network
  DEPENDS:=+ppp
  TITLE:=PPPoE (PPP over Ethernet)
  URL:=http://roaringpenguin.com/products/pppoe
  SUBMENU:=dial-in/up
endef

define Package/rp-pppoe/Default/description
	Dubbed RP-PPPoE, this open-source product is ideal for Linux users with
	a DSL "modem" whose Internet service provider uses PPPoE.
endef

#define Package/rp-pppoe-client
#  $(call Package/rp-pppoe/Default)
#  TITLE+= client
#endef

#define Package/rp-pppoe-client/description
#        $(call Package/rp-pppoe/Default/description)
#        This package contains pppoe-connect.
#endef

#define Package/rp-pppoe-relay
define Package/rp-pppoe
  $(call Package/rp-pppoe/Default)
  TITLE+= relay
endef

#define Package/rp-pppoe-relay/description
define Package/rp-pppoe/description
        $(call Package/rp-pppoe/Default/description)
        This package contains pppoe-relay.
endef

#define Package/rp-pppoe-server
#  $(call Package/rp-pppoe/Default)
#  TITLE+= server
#endef

#define Package/rp-pppoe-server/description
#        $(call Package/rp-pppoe/Default/description)
#        This package contains pppoe-server.
#endef

#define Package/rp-pppoe-sniff
#  $(call Package/rp-pppoe/Default)
#  TITLE+= sniffer
#endef

#define Package/rp-pppoe-sniff/description
#        $(call Package/rp-pppoe/Default/description)
#        This package contains pppoe-sniff.
#endef

TARGET_CFLAGS += -I$(PKG_BUILD_DIR)/src/libevent
CONFIGURE_PATH := ./src
CONFIGURE_ARGS += ac_cv_path_PPPD=/usr/sbin/pppd --enable-plugin=../../ppp-2.4.3/ipkg-install/usr/include/
#MAKE_FLAGS := DESTDIR="$(PKG_INSTALL_DIR)" install
MAKE_FLAGS := DESTDIR="$(PKG_INSTALL_DIR)" 
MAKE_PATH := ./src

define Build/Configure
	cd $(PKG_BUILD_DIR)/src; \
	chmod +x ./ifx-configure; \
	$(BUILD_FLAGS) ./ifx-configure
endef

define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR)/src \
        DFLAGS="$(TARGET_CFLAGS) -Wall \
        $(CONFIGURE_ARGS) $(MAKE_FLAGS)" pppoe-relay
endef

#define Build/InstallDev
#    $(INSTALL_DIR) $(1)/usr/include
#    $(CP) $(PKG_INSTALL_DIR)/usr/include/pppd $(1)/usr/include/
#endef

#define Package/rp-pppoe-client/install	
#	$(INSTALL_DIR) $(1)/etc/init.d
#	$(INSTALL_BIN) ./files/pppoe-client.init $(1)/etc/init.d/pppoe-client
#	$(INSTALL_DIR) $(1)/etc/ppp
#	$(INSTALL_DATA) $(PKG_INSTALL_DIR)/etc/ppp/pppoe.conf $(1)/etc/ppp/
#	$(INSTALL_DIR) $(1)/usr/sbin
#	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/sbin/pppoe $(1)/usr/sbin/
#	$(INSTALL_BIN) $(PKG_BUILD_DIR)/scripts/pppoe-connect $(1)/usr/sbin/
#	$(INSTALL_BIN) $(PKG_BUILD_DIR)/scripts/pppoe-start $(1)/usr/sbin/
#	$(INSTALL_BIN) $(PKG_BUILD_DIR)/scripts/pppoe-stop $(1)/usr/sbin/
#	$(SED) 's,modprobe,insmod,g' $(1)/usr/sbin/pppoe-connect
#endef

#define Package/rp-pppoe-relay/install	
define Package/rp-pppoe/install	
	#$(INSTALL_DIR) $(1)/etc/default
	#$(INSTALL_BIN) ./files/pppoe-relay.default $(1)/etc/default/pppoe-relay
	#$(INSTALL_DIR) $(1)/etc/init.d
	#$(INSTALL_BIN) ./files/pppoe-relay.init $(1)/etc/init.d/pppoe-relay
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_DIR) $(PKG_INSTALL_DIR)/usr/sbin
	$(CP) $(PKG_BUILD_DIR)/src/pppoe-relay $(PKG_INSTALL_DIR)/usr/sbin/pppoe-relay
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/sbin/pppoe-relay $(1)/usr/sbin/
endef

#define Package/rp-pppoe-server/install	
#	$(INSTALL_DIR) $(1)/etc/default
#	$(INSTALL_BIN) ./files/pppoe-server.default $(1)/etc/default/pppoe-server
#	$(INSTALL_DIR) $(1)/etc/init.d
#	$(INSTALL_BIN) ./files/pppoe-server.init $(1)/etc/init.d/pppoe-server
#	$(INSTALL_DIR) $(1)/etc/ppp
#	$(INSTALL_DIR) $(1)/etc/ppp/plugins
#	$(INSTALL_DATA) $(PKG_INSTALL_DIR)/etc/ppp/pppoe-server-options $(1)/etc/ppp/
#	$(INSTALL_DIR) $(1)/usr/sbin
#	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/sbin/pppoe-server $(1)/usr/sbin/
#	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/etc/ppp/plugins/rp-pppoe.so $(1)/etc/ppp/plugins/
#endef

#define Package/rp-pppoe-sniff/install	
#	$(INSTALL_DIR) $(1)/usr/sbin
#	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/sbin/pppoe-sniff $(1)/usr/sbin/
#endef

define Build/Clean
	if [ -d $(PKG_BUILD_DIR) ]; then $(MAKE) -C $(PKG_BUILD_DIR)/src clean; \
	$(RM) $(PKG_BUILD_DIR)/ipk; fi
endef

$(eval $(call BuildPackage,rp-pppoe))
#$(eval $(call BuildPackage,rp-pppoe-client))
#$(eval $(call BuildPackage,rp-pppoe-relay))
#$(eval $(call BuildPackage,rp-pppoe-server))
#$(eval $(call BuildPackage,rp-pppoe-sniff))
