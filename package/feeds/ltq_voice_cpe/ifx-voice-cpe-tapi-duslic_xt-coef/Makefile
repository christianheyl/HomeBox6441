#
# Copyright (C) 2008 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
# $Id: $

include $(TOPDIR)/rules.mk

PKG_NAME:=ifx-voice-cpe-tapi-duslic_xt-coef
PKG_VERSION:=0.0.7
PKG_MD5SUM:=7bf10ac8311dc0908db0ac5181856a2d
PKG_BASE_NAME:=coef_dxt

# IMPORTANT - include the version-override here after PKG_VERSION definition
include $(INCLUDE_DIR)/package-version-override.mk
# IMPORTANT - ... and before using PKG_VERSION below

PKG_SOURCE:=$(PKG_BASE_NAME)-$(PKG_VERSION).tar.gz
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_BASE_NAME)-$(PKG_VERSION)
PKG_RELEASE:=1
PKG_UNPACK:=gzip -dc $(DL_DIR)/$(PKG_SOURCE) | $(TAR) -C $(PKG_BUILD_DIR) $(TAR_OPTIONS)
PKG_SOURCE_URL:=http://localhost

LANTIQ_FIRMWARE_DIR=/lib/firmware
LANTIQ_DXT_BBD_FILE_NAME=dxt_bbd_fxs.bin
LANTIQ_DXT_BBD_DIR_PREFIX_IBB:=coef_dxt_IBB

include $(INCLUDE_DIR)/package.mk

define Package/$(PKG_NAME)
  SECTION:=firmware
  CATEGORY:=Lantiq
  TITLE:=DUSLIC-xT voice coefficient samples
  URL:=http://www.lantiq.com/
  MAINTAINER:=Lantiq Deutschland GmbH
  DEPENDS:=ifx-voice-cpe-tapi PACKAGE_ifx-voice-cpe-tapi-duslic_xt
  MENU:=1
endef

define Package/$(PKG_NAME)/config
$(call Package/$(PKG_NAME)/override_version)
$(call Package/$(PKG_NAME)/override_source_path)
	source "$(SOURCE)/config/Config.in"
endef

ifeq ($(CONFIG_IFX_VOICE_CPE_DXT_COEF_ETSI),y)
	PKG_TARGET_BASE_NAME:=ETSI_T3R10.BIN
endif

ifeq ($(CONFIG_IFX_VOICE_CPE_DXT_COEF_US600R),y)
	PKG_TARGET_BASE_NAME:=R600_T3R10.BIN
endif

ifeq ($(CONFIG_IFX_VOICE_CPE_DXT_COEF_GERMANY),y)
	PKG_TARGET_BASE_NAME:=GERM_T3R10.BIN
endif

ifeq ($(CONFIG_IFX_VOICE_CPE_DXT_COEF_USE_CUSTOM_FILE),y)
	PKG_TARGET_BASE_NAME:=$(CONFIG_IFX_VOICE_CPE_DXT_COEF_CUSTOM_FILE)
endif

define Package/$(PKG_NAME)/description
	DUSLIC-xT voice coefficient samples.
endef

define Build/Configure
endef

define Build/Compile
endef

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)$(LANTIQ_FIRMWARE_DIR)

	$(INSTALL_DIR) $(1)$(LANTIQ_FIRMWARE_DIR)/$(LANTIQ_DXT_BBD_DIR_PREFIX_IBB)
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/dcdc_IBB/README $(1)$(LANTIQ_FIRMWARE_DIR)/$(LANTIQ_DXT_BBD_DIR_PREFIX_IBB)/README
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/dcdc_IBB/*.BIN  $(1)$(LANTIQ_FIRMWARE_DIR)/$(LANTIQ_DXT_BBD_DIR_PREFIX_IBB)/

	(cd $(1)$(LANTIQ_FIRMWARE_DIR) && rm -f $(LANTIQ_DXT_BBD_FILE_NAME) && ln -sf $(LANTIQ_DXT_BBD_DIR_PREFIX_IBB)/$(PKG_TARGET_BASE_NAME) $(LANTIQ_DXT_BBD_FILE_NAME))
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
