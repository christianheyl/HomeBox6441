# Copyright (C) 2009 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
# $Id: $

include $(TOPDIR)/rules.mk
PATCH_DIR:=./patches

PKG_NAME:=gpon-sw-tests
PKG_VERSION:=1.0.0
PKG_BASE_NAME:=gpon_sw_tests
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_BASE_NAME)-$(PKG_VERSION)
PKG_RELEASE:=1

PKG_SOURCE_URL:=http://localhost
PKG_BIN_DIR=/opt/lantiq/bin
PKG_CAT:=zcat
PKG_BUILD_DEPENDS:=ifx-lib-tf ifxos

include $(INCLUDE_DIR)/package-version-override.mk
include $(INCLUDE_DIR)/package.mk

ifeq ($(DUMP),)
	STAMP_CONFIGURED:=$(strip $(STAMP_CONFIGURED))_$(shell $(SH_FUNC) grep '^CONFIG_GPON_' $(TOPDIR)/.config | md5s)
endif

define Package/$(PKG_NAME)
  SECTION:=application
  CATEGORY:=Lantiq
  TITLE:=GPON Software Test Application
  MAINTAINER:=Lantiq GmbH
  MENU:=1
  SUBMENU:=GPON
endef

define Package/$(PKG_NAME)/description
	GPON Software Test Application
endef

define Package/$(PKG_NAME)/config
$(call Package/$(PKG_NAME)/override_version)
$(call Package/$(PKG_NAME)/override_source_path)
	menu "configuration"
		depends on PACKAGE_$(PKG_NAME)
	endmenu
endef

CONFIGURE_ARGS += \
	--enable-driver-include="-I$(STAGING_DIR)/usr/include/onu_drv -I$(STAGING_DIR)/usr/include/optic_drv" \
	--enable-trace \
	--with-target-incl=$(STAGING_DIR)/usr/include \
	--with-target-lib=$(STAGING_DIR)/usr/lib \
	--with-ifxos-incl=$(STAGING_DIR)/usr/include/ifxos \
	--with-ifxos-lib=$(STAGING_DIR)/usr/lib \
	--with-tf-incl=$(STAGING_DIR)/usr/include/libtf \
	--with-tf-lib=$(STAGING_DIR)/usr/lib

#	--enable-vos-incl=$(STAGING_DIR)/usr/include \
#	--enable-vos-lib=$(STAGING_DIR)/usr/lib \
	
define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/etc/init.d
	#$(INSTALL_BIN) ./files/swtests.sh $(1)/etc/init.d/
	$(INSTALL_DIR) $(1)/opt/lantiq/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/prj_server/prj_server $(1)/opt/lantiq/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/prj_dispatch/prj_disp $(1)/opt/lantiq/bin
	$(TARGET_CROSS)strip $(1)/opt/lantiq/bin/prj_disp
	$(TARGET_CROSS)strip $(1)/opt/lantiq/bin/prj_server
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
