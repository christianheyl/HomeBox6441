#
# Copyright (C) 2009 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
# $Id: $

include $(TOPDIR)/rules.mk

PKG_NAME:=gpon-flow-api
PKG_VERSION:=1.1.5
PKG_BASE_NAME:=gpon_flow_api
PKG_SOURCE:=$(PKG_BASE_NAME)-$(PKG_VERSION).tar.gz
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_BASE_NAME)-$(PKG_VERSION)
PKG_RELEASE:=1
PKG_SOURCE_URL:=http://localhost
PKG_BIN_DIR=/opt/lantiq/bin
PKG_BUILD_DEPENDS:=GPON_FLOW_API_TEST_APP_READLINE:libncurses GPON_FLOW_API_TEST_APP_READLINE:libreadline GPON_FLOW_API_REMOTE:gpon-rpc

include $(INCLUDE_DIR)/package-version-override.mk
include $(INCLUDE_DIR)/package.mk

ifeq ($(DUMP),)
	STAMP_CONFIGURED:=$(strip $(STAMP_CONFIGURED))_$(shell $(SH_FUNC) grep '^CONFIG_GPON_' $(TOPDIR)/.config | md5s)
endif

define Package/$(PKG_NAME)
  SECTION:=driver
  CATEGORY:=Lantiq
  TITLE:= FLOW API for GPON
  URL:=http://www.lantiq.com/
  MAINTAINER:=Lantiq
  DEPENDS:=+gpon-onu-drv
  MENU:=1
  SUBMENU:=GPON
endef

define Package/$(PKG_NAME)/description
   The FLOW API for GPON Optical Network Unit device
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
	--enable-driver-include="-I$(STAGING_DIR)/usr/include/onu_drv"

ifeq ($(CONFIG_GPON_FLOW_API_DEBUG_PRINTS),y)
CONFIGURE_ARGS += --enable-debug-prints
endif

ifeq ($(CONFIG_GPON_FLOW_API_TEST_APP_READLINE),y)
CONFIGURE_ARGS += --with-readline
endif

CONFIGURE_ARGS += \
	$(call autoconf_bool,CONFIG_GPON_FLOW_API_REMOTE,remote-onu)

define Build/Clean
	@- [ -e $(PKG_BUILD_DIR)/Makefile ] && $(MAKE) -C $(PKG_BUILD_DIR) clean
	$(call Build/Clean/Default)
endef

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/include/flow_api
	$(CP) $(PKG_BUILD_DIR)/include/flow_api*.h $(1)/usr/include/flow_api
	$(INSTALL_DIR) $(1)/usr/lib
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/src/libflowapi.a $(1)/usr/lib
endef

ifeq ($(CONFIG_GPON_FLOW_API_TEST_APP),y)

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/$(PKG_BIN_DIR)
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/tools/fapi $(1)/$(PKG_BIN_DIR)
endef

endif

$(eval $(call BuildPackage,$(PKG_NAME)))
