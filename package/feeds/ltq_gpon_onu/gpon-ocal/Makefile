#
# Copyright (C) 2009 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
# $Id: $

include $(TOPDIR)/rules.mk

PKG_NAME:=gpon-ocal
PKG_VERSION:=3.5.0
PKG_BASE_NAME:=gpon_ocal
PKG_SOURCE:=$(PKG_BASE_NAME)-$(PKG_VERSION).tar.gz
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_BASE_NAME)-$(PKG_VERSION)
PKG_RELEASE:=2
PKG_SOURCE_URL:=http://localhost
PKG_BUILD_DEPENDS:=libuci gpon-optic-drv GPON_OCAL_CLI_DAEMON:lib-cli
PKG_BIN_DIR=opt/lantiq/bin

include $(INCLUDE_DIR)/package-version-override.mk
include $(INCLUDE_DIR)/package.mk

ifeq ($(DUMP),)
	STAMP_CONFIGURED:=$(strip $(STAMP_CONFIGURED))_$(shell $(SH_FUNC) grep '^CONFIG_GPON_' $(TOPDIR)/.config | md5s)
endif

define Package/$(PKG_NAME)
  SECTION:=application
  CATEGORY:=Lantiq
  TITLE:= OCAL for GPON ONU
  URL:=http://www.lantiq.com/
  MAINTAINER:=Lantiq
  DEPENDS:=+ifxos +gpon-optic-drv +libuci
  MENU:=1
  SUBMENU:=GPON
endef

define Package/$(PKG_NAME)/description
   The Optic CALibration for GPON Optical Network Unit device
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
	--enable-optic-include="-I$(STAGING_DIR)/usr/include/optic_drv" \
	--enable-ocal-uci

ifeq ($(CONFIG_GPON_OCAL_CLI_DAEMON),y)
CONFIGURE_ARGS += \
	--enable-cli-daemon \
	--enable-ifxos-include="-I$(STAGING_DIR)/usr/include/ifxos" \
	--enable-cli-include="-I$(STAGING_DIR)/usr/include/cli" \
	--enable-cli-library="-I$(STAGING_DIR)/usr/lib"
endif

define Build/Clean
	@- [ -e $(PKG_BUILD_DIR)/Makefile ] && $(MAKE) -C $(PKG_BUILD_DIR) clean
	$(call Build/Clean/Default)
endef

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/include/ocal
	$(CP) $(PKG_BUILD_DIR)/include/ocal*.h $(1)/usr/include/ocal

	$(INSTALL_DIR) $(1)/usr/lib
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/src/libocal.a $(1)/usr/lib
endef

ifeq ($(CONFIG_GPON_OCAL_CLI_DAEMON),y)
define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/load_ocal.sh $(1)/etc/init.d/

	$(INSTALL_DIR) $(1)/$(PKG_BIN_DIR)
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/tools/ocal $(1)/$(PKG_BIN_DIR)/

	$(INSTALL_BIN) $(PKG_BUILD_DIR)/scripts/ocal_pipe.sh $(1)/$(PKG_BIN_DIR)/
endef
else
define Package/$(PKG_NAME)/install
endef
endif

$(eval $(call BuildPackage,$(PKG_NAME)))
