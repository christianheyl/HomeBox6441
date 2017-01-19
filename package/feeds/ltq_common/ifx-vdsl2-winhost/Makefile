#
# Copyright (C) 2009 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=ifx-vdsl2-winhost
PKG_VERSION:=0.2.3
PKG_RELEASE:=1
PKG_BASE_NAME:=vdsl2_winhost
PKG_SOURCE:=$(PKG_BASE_NAME)-$(PKG_VERSION).tar.gz
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_BASE_NAME)-$(PKG_VERSION)
PKG_SOURCE_URL:=http://localhost
#PKG_MD5SUM:=

PKG_BUILD_DEPENDS:=lib_ifxos \
	PACKAGE_ifx-dsl-cpe-mei-vrx:ifx-dsl-cpe-mei-vrx \
	PACKAGE_ifx-dsl-vinax:ifx-dsl-vinax \
	PACKAGE_ifx-dsl-vinax2:ifx-dsl-vinax2

IFX_BIN_DIR=/opt/ifx/bin

include $(INCLUDE_DIR)/package-version-override.mk
include $(INCLUDE_DIR)/package.mk

define Package/$(PKG_NAME)
  SECTION:=application
  CATEGORY:=Lantiq
  TITLE:=Lantiq Winhost Agent for Vinax and VRx
  URL:=http://www.lantiq.com/
  MAINTAINER:=Lantiq
  DEPENDS:=+libpthread +kmod-ifxos @PACKAGE_ifx-dsl-vinax||PACKAGE_ifx-dsl-vinax2||PACKAGE_ifx-dsl-cpe-mei-vrx
endef

define Package/$(PKG_NAME)/description
  Winhost Agent for Vinax and VRx
endef

ifdef CONFIG_PACKAGE_ifx-dsl-vinax
CONFIGURE_ARGS += \
	--enable-device-driver=drv_vinax
endif

ifdef CONFIG_PACKAGE_ifx-dsl-vinax2
CONFIGURE_ARGS += \
	--enable-device-driver=drv_vinax
endif

ifdef CONFIG_PACKAGE_ifx-dsl-cpe-mei-vrx
CONFIGURE_ARGS += \
	--enable-device-driver=drv_mei_cpe
endif

CONFIGURE_ARGS += \
	--enable-targetincl=$(STAGING_DIR)/usr/include \
	--enable-target=$(STAGING_DIR)/usr/lib \
	--enable-debug --enable-error-print\
	--enable-ifxos-include="-I$(STAGING_DIR)/usr/include/ifxos" \
	--enable-ifxos-library="-L$(STAGING_DIR)/usr/lib" \
	--enable-dsl-driver-include="-I$(STAGING_DIR)/usr/include"


define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/vdsl2_winhost_init.sh $(1)/etc/init.d/
	$(INSTALL_DIR) $(1)/$(IFX_BIN_DIR)
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/src/vdsl2_winhost_agent $(1)/$(IFX_BIN_DIR)
endef

$(eval $(call BuildPackage,$(PKG_NAME)))

