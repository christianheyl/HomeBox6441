#
# Copyright (C) 2009 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=gpon-dti-agent
PKG_VERSION:=2.0.7
PKG_BASE_NAME:=gpon_dti_agent
PKG_SOURCE:=$(PKG_BASE_NAME)-$(PKG_VERSION).tar.gz
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_BASE_NAME)-$(PKG_VERSION)
PKG_RELEASE:=1
PKG_SOURCE_URL:=http://localhost
PKG_BUILD_DEPENDS:=lib-dti gpon-onu-drv gpon-optic-drv GPON_DTI_DEVICE_OCAL:gpon-ocal
PKG_BIN_DIR=opt/lantiq/bin

include $(INCLUDE_DIR)/package-version-override.mk
include $(INCLUDE_DIR)/package.mk

ifeq ($(DUMP),)
	STAMP_CONFIGURED:=$(strip $(STAMP_CONFIGURED))_$(shell $(SH_FUNC) grep '^CONFIG_GPON_' $(TOPDIR)/.config | md5s)
endif

define Package/$(PKG_NAME)
  SECTION:=application
  CATEGORY:=Lantiq
  TITLE:= DTI agent for GPON
  URL:=http://www.lantiq.com/
  MAINTAINER:=Lantiq
  DEPENDS:=+ifxos +lib-dti +gpon-onu-drv +gpon-optic-drv
  MENU:=1
  SUBMENU:=GPON
endef

define Package/$(PKG_NAME)/description
   The Debug & Trace Interface agent for GPON Optical Network Unit device.
endef

define Package/$(PKG_NAME)/config
	menu "configuration"
	depends on PACKAGE_$(PKG_NAME)
		source "$(SOURCE)/config/Config.in"
	endmenu
   $(call Package/$(PKG_NAME)/override_version)
   $(call Package/$(PKG_NAME)/override_source_path)
endef

CONFIGURE_ARGS += \
	--enable-ifxos-include="-I$(STAGING_DIR)/usr/include/ifxos" \
	--enable-dti-include="-I$(STAGING_DIR)/usr/include/dti" \
	--enable-start-devs \
	--disable-dev-generic \
	--enable-onu \
	--enable-onu-include="-I$(STAGING_DIR)/usr/include/onu_drv" \
	--enable-optic \
	--enable-optic-include="-I$(STAGING_DIR)/usr/include/optic_drv" \
	--enable-ifxos-library="$(STAGING_DIR)/usr/lib" \
	--enable-dti-library="$(STAGING_DIR)/usr/lib"

ifeq ($(CONFIG_mips),y)
CONFIGURE_ARGS += --enable-add_drv_cflags="-fno-pic -mno-abicalls -mlong-calls -G 0"
endif

ifeq ($(CONFIG_GPON_DTI_DEVICE_OCAL),y)
CONFIGURE_ARGS += \
	--enable-ocal \
	--enable-ocal-include="-I$(STAGING_DIR)/usr/include/ocal" \
	--enable-ocal-library="$(STAGING_DIR)/usr/lib"
endif

define Build/Clean
	@- rm -Rf $(PKG_BUILD_DIR)/ipkg*
	@- [ -e $(PKG_BUILD_DIR)/Makefile ] && $(MAKE) -C $(PKG_BUILD_DIR) clean
	$(call Build/Clean/Default)
endef

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/load_dti_agent.sh $(1)/etc/init.d/
	$(INSTALL_DIR) $(1)/$(PKG_BIN_DIR)
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/src/gpon_dti_agent $(1)/$(PKG_BIN_DIR)
	$(INSTALL_DIR) $(1)/$(MODULES_SUBDIR)
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
