#
# Copyright (C) 2009 OpenWrt.org
# Copyright (C) 2009 lantiq.com
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=gpon-sipdemo
PKG_VERSION:=0.0.1
PKG_BASE_NAME:=gpon_sipdemo
PKG_SOURCE:=$(PKG_BASE_NAME)-$(PKG_VERSION).tar.gz
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_BASE_NAME)-$(PKG_VERSION)
PKG_RELEASE:=2
PKG_SOURCE_URL:=http://localhost
PKG_BUILD_DEPENDS:=ltq-tapi ltq-vmmc ltq-kpi2udp pjsip-ltq-tapi
PKG_BIN_DIR=usr/sbin

include $(INCLUDE_DIR)/package-version-override.mk
include $(INCLUDE_DIR)/package.mk

ifeq ($(DUMP),)
	STAMP_CONFIGURED:=$(strip $(STAMP_CONFIGURED))_$(shell $(SH_FUNC) grep '^CONFIG_GPON_' $(TOPDIR)/.config | md5s)
endif

define Package/$(PKG_NAME)
  SECTION:=application
  CATEGORY:=Lantiq
  TITLE:= SIP demo for GPON
  URL:=http://www.lantiq.com/
  MAINTAINER:=Lantiq
  DEPENDS:=+kmod-ifxos +kmod-ltq-tapi +kmod-ltq-kpi2udp +kmod-ltq-vmmc +pjsip-ltq-tapi
  MENU:=1
  SUBMENU:=GPON
endef

define Package/$(PKG_NAME)/description
   PJSUA based SIP demo using TAPI and PJSIP for GPON
endef

define Package/$(PKG_NAME)/config
    source "$(SOURCE)/Config.in"
    $(call Package/$(PKG_NAME)/override_version)
    $(call Package/$(PKG_NAME)/override_source_path)
endef

CONFIGURE_ARGS += \
	--enable-ifxos-include="-I$(STAGING_DIR)/usr/include/ifxos" \
	--enable-ifxos-library="-L$(STAGING_DIR)/usr/lib" \
	--enable-pjsip-include="-I$(STAGING_DIR)/usr/pjsip-ltq-tapi/include" \
	--enable-pjsip-library="-L$(STAGING_DIR)/usr/pjsip-ltq-tapi/lib" \
	--enable-tapi-include="-I$(STAGING_DIR)/usr/include/drv_tapi" \
	--enable-drv-include="-I$(STAGING_DIR)/usr/include/drv_vmmc"

ifeq ($(CONFIG_GPON_SIPDEMO_BOARD_EASY507XX),y)
	CONFIGURE_ARGS += --enable-board=EASY507XX
else ifeq ($(CONFIG_GPON_SIPDEMO_BOARD_EASY508XX),y)
	CONFIGURE_ARGS += --enable-board=EASY508XX
endif

define Build/Clean
	@- rm -Rf $(PKG_BUILD_DIR)/ipkg*
	@- [ -e $(PKG_BUILD_DIR)/Makefile ] && $(MAKE) -C $(PKG_BUILD_DIR) clean
	$(call Build/Clean/Default)
endef

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/etc/init.d/
	$(INSTALL_BIN) ./files/gpon_sipdemo_init $(1)/etc/init.d/sipdemo
	$(INSTALL_DIR) $(1)/$(PKG_BIN_DIR)
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/src/gpon_sipdemo $(1)/$(PKG_BIN_DIR)
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
