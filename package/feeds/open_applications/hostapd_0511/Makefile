#
# Openwrt Makefile for hostapd
#

#### Includes ###################################
include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

#### Package Info ###############################
PKG_NAME:=hostapd_0511
PKG_VERSION:=0.5.11
PKG_RELEASE:=1
PKG_BASE_NAME:= hostapd
PKG_SOURCE:=hostapd-$(PKG_VERSION).tar.gz
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_BASE_NAME)-$(PKG_VERSION)
PKG_SOURCE_URL:=http://hostap.epitest.fi/releases
PKG_BUILD_DEPENDS:=libopenssl

include $(INCLUDE_DIR)/package.mk

define Package/$(PKG_NAME)
  SECTION:=net
  CATEGORY:=Network
  TITLE:=IEEE 802.1x Authenticator(0_5_11)
  URL:=http://hostap.epitest.fi
  MENU:=1
endef

define Package/$(PKG_NAME)/description
  IEEE 802.11 AP, IEEE 802.1X/WPA/WPA2/EAP/RADIUS Authenticator
endef

#### Directory Paths ############################
OPENSSL_DIR:=$(wildcard $(BUILD_DIR)/openssl-*)

IFX_CFLAGS += -I$(OPENSSL_DIR)/include
IFX_LDFLAGS += -L$(OPENSSL_DIR)/

#### Target Rules ###############################
define Build/Prepare
	$(call Build/Prepare/Default)
	$(INSTALL_DIR) $(PKG_BUILD_DIR)
endef

define Build/Compile
	$(MAKE) $(TARGET_CONFIGURE_OPTS) -C $(PKG_BUILD_DIR) \
	IFX_CFLAGS="$(IFX_CFLAGS)" IFX_LDFLAGS="$(IFX_LDFLAGS)"
endef

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/hostapd $(1)/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/hostapd_cli $(1)/bin
endef

define Build/Clean
	if [ -d $(PKG_BUILD_DIR) ]; then $(MAKE) -C $(PKG_BUILD_DIR) clean; \
	$(RM) -r $(PKG_BUILD_DIR)/ipkg-$(BOARD); fi
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
