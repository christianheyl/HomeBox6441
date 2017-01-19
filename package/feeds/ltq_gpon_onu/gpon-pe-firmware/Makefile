#
# Copyright (C) 2008 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
# $Id: $

include $(TOPDIR)/rules.mk

PKG_NAME:=gpon-pe-firmware
PKG_VERSION:=4.48.1.0
PKG_BASE_NAME:=fw_gpe_falcon
PKG_SOURCE:=$(PKG_BASE_NAME)-$(PKG_VERSION).tar.gz
PKG_RELEASE:=1
PKG_SOURCE_URL:=http://localhost

include $(INCLUDE_DIR)/package-version-override.mk

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)
PKG_UNPACK:=$(TAR) -C $(PKG_BUILD_DIR) -xzf $(DL_DIR)/$(PKG_SOURCE)

include $(INCLUDE_DIR)/package.mk

ifeq ($(DUMP),)
	STAMP_CONFIGURED:=$(strip $(STAMP_CONFIGURED))_$(shell $(SH_FUNC) grep '^CONFIG_GPON_' $(TOPDIR)/.config | md5s)
endif

define Package/$(PKG_NAME)
  SECTION:=firmware
  CATEGORY:=Lantiq
  TITLE:=FALC-ON packet engine firmware
  URL:=http://www.lantiq.com/
  MAINTAINER:=Lantiq Deutschland GmbH
  MENU:=1
  SUBMENU:=GPON
endef

define Package/$(PKG_NAME)/config
	config GPON_PE_FIRMWARE_REMOTE_ONLY
		bool "Enable remote-only ONU support"
		depends on PACKAGE_gpon-pe-firmware
		default y if GPON_ONU_REMOTE_ONLY
endef

define Build/Compile
endef

define Package/$(PKG_NAME)/install

ifneq ($(CONFIG_GPON_PE_FIRMWARE_REMOTE_ONLY),y)
	$(INSTALL_DIR) $(1)/lib/firmware/a2x
	- $(CP) $(PKG_BUILD_DIR)/a2x/gpefw.fw $(1)/lib/firmware/a2x/falcon_gpe_fw.bin
	- $(CP) $(PKG_BUILD_DIR)/a2x/*.bin $(1)/lib/firmware/a2x/
	$(INSTALL_DIR) $(1)/lib/firmware/a1x
	- $(CP) $(PKG_BUILD_DIR)/a1x/gpefw.fw $(1)/lib/firmware/a1x/falcon_gpe_fw.bin
	- $(CP) $(PKG_BUILD_DIR)/a1x/*.bin $(1)/lib/firmware/a1x/
	- $(CP) $(PKG_BUILD_DIR)/gpefw.fw $(1)/lib/firmware/a1x/falcon_gpe_fw.bin
	- $(CP) $(PKG_BUILD_DIR)/*.bin $(1)/lib/firmware/a1x/
endif

endef

define Build/InstallDev

ifeq ($(CONFIG_PACKAGE_gpon-onu-firmware),y)
	$(INSTALL_DIR) $(1)/usr/bin/pe_firmware
	$(CP) $(PKG_BUILD_DIR)/gpefw.fw $(1)/usr/bin/pe_firmware/falcon_gpe_fw.bin
	$(CP) $(PKG_BUILD_DIR)/*.bin $(1)/usr/bin/pe_firmware/
endif

endef

$(eval $(call BuildPackage,$(PKG_NAME)))
