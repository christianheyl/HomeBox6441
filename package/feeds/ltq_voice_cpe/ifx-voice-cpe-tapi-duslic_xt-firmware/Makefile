#
# Copyright (C) 2008 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
# $Id: $

include $(TOPDIR)/rules.mk

PKG_NAME:=ifx-voice-cpe-tapi-duslic_xt-firmware
PKG_VERSION:=5.25.11
PKG_MD5SUM:=74073c49e3a82eacd276a3ca88fb69c7
PKG_BASE_NAME:=fw_dxt


# IMPORTANT - include the version-override here after PKG_VERSION definition
include $(INCLUDE_DIR)/package-version-override.mk
# IMPORTANT - ... and before using PKG_VERSION below

PKG_SOURCE:=$(PKG_BASE_NAME)-$(PKG_VERSION).tar.gz
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_BASE_NAME)-$(PKG_VERSION)
PKG_RELEASE:=1
PKG_UNPACK:=gzip -dc $(DL_DIR)/$(PKG_SOURCE) | $(TAR) -C $(PKG_BUILD_DIR) $(TAR_OPTIONS)
PKG_SOURCE_URL:=http://localhost
PKG_TARGET_BASE_NAME:=dxt_firmware.bin
LANTIQ_FIRMWARE_DIR=/lib/firmware

include $(INCLUDE_DIR)/package.mk

define Package/$(PKG_NAME)
  SECTION:=firmware
  CATEGORY:=Lantiq
  TITLE:=DUSLIC-xT voice firmware
  URL:=http://www.lantiq.com/
  MAINTAINER:=Lantiq Deutschland GmbH
  DEPENDS:=ifx-voice-cpe-tapi PACKAGE_ifx-voice-cpe-tapi-duslic_xt
  MENU:=1
endef

define Package/$(PKG_NAME)/description
	DUSLIC-xT voice firmware.
endef

define Build/Configure
endef

define Build/Compile
endef

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)$(LANTIQ_FIRMWARE_DIR)
	$(CP) $(PKG_BUILD_DIR)/*.bin $(1)$(LANTIQ_FIRMWARE_DIR)/$(PKG_BASE_NAME)-$(PKG_VERSION).bin
	$(CP) $(PKG_BUILD_DIR)/LICENSE $(1)$(LANTIQ_FIRMWARE_DIR)/
	ln -sf $(PKG_BASE_NAME)-$(PKG_VERSION).bin $(1)$(LANTIQ_FIRMWARE_DIR)/$(PKG_TARGET_BASE_NAME)
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
