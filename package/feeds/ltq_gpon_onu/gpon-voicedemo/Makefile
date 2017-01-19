#
# Copyright (C) 2009 OpenWrt.org
# Copyright (C) 2009 lantiq.com
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=gpon-voicedemo
PKG_VERSION:=0.0.2
PKG_BASE_NAME:=gpon_voicedemo
PKG_SOURCE:=$(PKG_BASE_NAME)-$(PKG_VERSION).tar.gz
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_BASE_NAME)-$(PKG_VERSION)
PKG_RELEASE:=2
PKG_SOURCE_URL:=http://localhost
PKG_BUILD_DEPENDS:=lib_ifxos PACKAGE_kmod-ifx-tapi:kmod-ifx-tapi PACKAGE_kmod-ifx-vmmc:kmod-ifx-vmmc \
	PACKAGE_kmod-ltq-tapi:kmod-ltq-tapi PACKAGE_kmod-ltq-vmmc:kmod-ltq-vmmc 
PKG_BIN_DIR=usr/sbin

include $(INCLUDE_DIR)/package-version-override.mk
include $(INCLUDE_DIR)/package.mk

ifeq ($(DUMP),)
	STAMP_CONFIGURED:=$(strip $(STAMP_CONFIGURED))_$(shell $(SH_FUNC) grep '^CONFIG_GPON_' $(TOPDIR)/.config | md5s)
endif

define Package/$(PKG_NAME)
  SECTION:=application
  CATEGORY:=Lantiq
  TITLE:= VoIP demo for GPON
  URL:=http://www.lantiq.com/
  MAINTAINER:=Lantiq
  DEPENDS:=+ifxos +kmod-ltq-tapi +kmod-ltq-vmmc +kmod-ltq-kpi2udp
  MENU:=1
  SUBMENU:=GPON
endef

define Package/$(PKG_NAME)/description
   VoIP demo using TAPI for GPON
endef

define Package/$(PKG_NAME)/config
   source "$(SOURCE)/Config.in"
   $(call Package/$(PKG_NAME)/override_version)
   $(call Package/$(PKG_NAME)/override_source_path)
endef

CONFIGURE_ARGS += \
	--enable-ifxos-include="-I$(STAGING_DIR)/usr/include/ifxos" \
	--enable-ifxos-library="-L$(STAGING_DIR)/usr/lib" \
	--enable-tapi-include="-I$(STAGING_DIR)/usr/include/drv_tapi" \
	--enable-drv-include="-I$(STAGING_DIR)/usr/include/drv_vmmc" --enable-board=EASY508XX

ifeq ($(CONFIG_GPON_VOICEDEMO_BOARD_EASY507XX),y)
	CONFIGURE_ARGS += --enable-board=EASY507XX
endif
ifeq ($(CONFIG_GPON_VOICEDEMO_BOARD_EASY508XX),y)
	CONFIGURE_ARGS += --enable-board=EASY508XX
endif

define Build/Clean
	@- rm -Rf $(PKG_BUILD_DIR)/ipkg*
	@- [ -e $(PKG_BUILD_DIR)/Makefile ] && $(MAKE) -C $(PKG_BUILD_DIR) clean
	$(call Build/Clean/Default)
endef

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/etc/init.d/
	$(INSTALL_BIN) ./files/gpon_voicedemo_init $(1)/etc/init.d/voicedemo
	$(INSTALL_DIR) $(1)/$(PKG_BIN_DIR)
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/src/gpon_voicedemo $(1)/$(PKG_BIN_DIR)
#
#	$(INSTALL_DIR) $(1)/etc/init.d
#	$(INSTALL_BIN) ./files/load_omci_daemon.sh $(1)/etc/init.d/
#	$(INSTALL_BIN) $(PKG_BUILD_DIR)/scripts/onu_omci_pipe.sh $(1)/$(PKG_BIN_DIR)/onu_omci_pipe.sh

	$(INSTALL_DIR) $(1)/etc/config
	$(INSTALL_DATA) ./files/voicedemo $(1)/etc/config
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
