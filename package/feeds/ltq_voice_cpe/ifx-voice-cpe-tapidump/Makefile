#
# Copyright (C) 2008 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
# $Id: $

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=ifx-voice-cpe-tapidump
PKG_VERSION:=1.0.2.1
PKG_MD5SUM:=c20a46747b55aae25e105198a85111af
PKG_RELEASE:=1

# IMPORTANT - include the version-override here after PKG_VERSION definition
include $(INCLUDE_DIR)/package-version-override.mk
# IMPORTANT - ... and before using PKG_VERSION below

PKG_SOURCE:=tapidump-$(PKG_VERSION).tar.gz
PKG_BUILD_DIR:=$(BUILD_DIR)/tapidump-$(PKG_VERSION)
PKG_BUILD_DEPENDS:=ifx-voice-cpe-tapi
DRV_TAPI_INCLUDE:=$(STAGING_DIR)/usr/include
PKG_SOURCE_URL:=http://localhost
PKG_START_SCRIPT:=tapidump_fw_cfg
IFX_BIN_DIR=/opt/ifx/bin
LANTIQ_BIN_DIR=/opt/lantiq/bin

include $(INCLUDE_DIR)/package.mk

define Package/$(PKG_NAME)
  SECTION:=application
  CATEGORY:=Lantiq
  TITLE:=TAPIdump debug tool
  URL:=http://www.lantiq.com/
  MAINTAINER:=Lantiq Deutschland GmbH
  DEPENDS:=ifx-voice-cpe-tapi
  MENU:=1
endef

define Package/$(PKG_NAME)/description
	tapidump debug tool
endef

define Package/$(PKG_NAME)/config
$(call Package/$(PKG_NAME)/override_version)
$(call Package/$(PKG_NAME)/override_source_path)
    source "$(SOURCE)/config/Config.in"
endef

CONFIGURE_ARGS += --with-drv-incl=$(STAGING_DIR)/usr/include/ \
    --with-ifxos-incl=$(STAGING_DIR)/usr/include/ifxos \
    --enable-warnings

ifneq ($(CONFIG_LINUX_2_4),y)
CONFIGURE_ARGS += --enable-linux-26 ARCH=$(LINUX_KARCH)
endif

define Build/Clean
	@- [ -e $(PKG_BUILD_DIR)/ipkg ] && rm -Rf $(PKG_BUILD_DIR)/ipkg
	@- [ -e $(PKG_BUILD_DIR)/Makefile ] && $(MAKE) -C $(PKG_BUILD_DIR) clean
	$(call Build/Clean/Default)
endef

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)$(IFX_BIN_DIR)
	$(INSTALL_DIR) $(1)$(LANTIQ_BIN_DIR)
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/src/tapidump $(1)$(LANTIQ_BIN_DIR)
	$(INSTALL_DIR) $(1)/usr/sbin
	(cd $(1)/usr/sbin; ln -snf ../../opt/lantiq/bin/tapidump .)
	(cd $(1)$(IFX_BIN_DIR); ln -snf ../../lantiq/bin/tapidump .)
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/src/scripts/$(PKG_START_SCRIPT) $(1)/usr/sbin/
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
