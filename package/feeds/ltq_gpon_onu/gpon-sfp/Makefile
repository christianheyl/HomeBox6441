#
# Copyright (C) 2009 OpenWrt.org
# Copyright (C) 2009 lantiq.com
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=gpon-sfp
PKG_VERSION:=0.0.1
PKG_BASE_NAME:=gpon_sfp
PKG_SOURCE:=$(PKG_BASE_NAME)-$(PKG_VERSION).tar.gz
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_BASE_NAME)-$(PKG_VERSION)
PKG_RELEASE:=1
PKG_SOURCE_URL:=http://localhost
PKG_BUILD_DEPENDS:=ecos
PKG_TARGETS:=bin

include $(INCLUDE_DIR)/package-version-override.mk
include $(INCLUDE_DIR)/package.mk

ifeq ($(DUMP),)
	STAMP_CONFIGURED:=$(strip $(STAMP_CONFIGURED))_$(shell $(SH_FUNC) grep '^CONFIG_GPON_' $(TOPDIR)/.config | md5s)
endif

define Package/$(PKG_NAME)
  SECTION:=driver
  CATEGORY:=Lantiq
  TITLE:= GPON SFP eCos based application
  URL:=http://www.lantiq.com/
  MAINTAINER:=Lantiq
  DEPENDS:=+ecos
  MENU:=1
  SUBMENU:=GPON
endef

define Package/$(PKG_NAME)/description
   SFP Application for GPON Optical Network Unit device
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
	--enable-ecos-include="-I$(STAGING_DIR)/usr/include/ecos" \
	--enable-ecos-library="-L$(STAGING_DIR)/usr/lib/ecos" \
	--enable-ecos-linker="$(STAGING_DIR)/usr/lib/ecos"

define Build/Clean
	@- rm -Rf $(PKG_BUILD_DIR)/ipkg*
	@- [ -e $(PKG_BUILD_DIR)/Makefile ] && $(MAKE) -C $(PKG_BUILD_DIR) clean
	$(call Build/Clean/Default)
endef

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)
	$(CP) $(PKG_BUILD_DIR)/src/gpon-sfp.bin $(1)/gpon-sfp.bin
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
