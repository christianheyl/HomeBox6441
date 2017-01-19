#
# Copyright (C) 2008 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
# $Id: $

include $(TOPDIR)/rules.mk

PKG_NAME:=ifx-dti
PKG_VERSION:=1.1.3
PKG_BASE_NAME:=lib_dti
PKG_SOURCE:=$(PKG_BASE_NAME)-$(PKG_VERSION).tar.gz
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_BASE_NAME)-$(PKG_VERSION)
PKG_RELEASE:=1
PKG_SOURCE_URL:=http://localhost
IFX_BIN_DIR=opt/ifx/bin

PKG_BUILD_DEPENDS:=lib_ifxos \
	PACKAGE_ifx-dsl-vinax:ifx-dsl-vinax \
	PACKAGE_ifx-dsl-vinax2:ifx-dsl-vinax2 \
	PACKAGE_ifx-dsl-vinax2-emul:ifx-dsl-vinax2-emul \
	PACKAGE_ifx-dsl-geminaxd:ifx-dsl-geminaxd

include $(INCLUDE_DIR)/package-version-override.mk
include $(INCLUDE_DIR)/package.mk

define Package/$(PKG_NAME)
  SECTION:=libs
  CATEGORY:=Lantiq
  TITLE:=Lantiq Debug and Trace Interface
  URL:=http://www.lantiq.com/
  MAINTAINER:=Lantiq GmbH
  DEPENDS:=+libpthread +kmod-ifxos
  SUBMENU:=DTI
  MENU:=1
endef

define Package/$(PKG_NAME)/description
  This library will be used by some Lantiq applications / drivers
endef

ifdef CONFIG_PACKAGE_ifx-dsl-vinax
CONFIGURE_ARGS += \
   --enable-vinax \
   --with-dti-max-lines-per-device=2\
   --with-dti-max-devices=32\
   --enable-dsl-driver-include="-I$(STAGING_DIR)/usr/include"
endif

ifdef CONFIG_PACKAGE_ifx-dsl-vinax2
CONFIGURE_ARGS += \
   --enable-vinax \
   --enable-dti-agent \
   --enable-easy88008 \
   --with-dti-max-lines-per-device=8\
   --with-dti-max-devices=8\
   --enable-dsl-driver-include="-I$(STAGING_DIR)/usr/include"
endif

ifdef CONFIG_PACKAGE_ifx-dsl-vinax2-emul
CONFIGURE_ARGS += \
   --enable-vinax \
   --enable-dti-agent \
   --enable-easy88008 \
   --with-dti-max-lines-per-device=8\
   --with-dti-max-devices=8\
   --enable-dsl-driver-include="-I$(STAGING_DIR)/usr/include"
endif

ifdef CONFIG_PACKAGE_ifx-dsl-geminaxd
CONFIGURE_ARGS += \
   --enable-geminax \
   --enable-dti-agent \
   --with-dti-max-lines-per-device=$(IFX_DSL_LINES_PER_DEVICE)\
   --with-dti-max-devices=$(IFX_DSL_MAX_DEVICE)\
   --enable-dsl-driver-include="-I$(STAGING_DIR)/usr/include"
endif

CONFIGURE_ARGS += \
	--enable-targetincl=$(STAGING_DIR)/usr/include \
	--enable-target=$(STAGING_DIR)/usr/lib \
	--enable-debug --enable-error-print\
   --enable-dev-generic \
	--enable-ifxos-include="-I$(STAGING_DIR)/usr/include/ifxos"\
	--enable-ifxos-library="-L$(STAGING_DIR)/usr/lib"\
	--with-dti-tcp-port=9000\
	--enable-cli

define Build/Clean
	@- [ -e $(PKG_BUILD_DIR)/Makefile ] && $(MAKE) -C $(PKG_BUILD_DIR) clean
	$(call Build/Clean/Default)
endef

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/{lib,include/dti}
	$(CP) $(PKG_BUILD_DIR)/src/include/* $(1)/usr/include/dti
	mkdir -p $(1)/usr/lib
	$(CP) $(PKG_BUILD_DIR)/src/libdti*.a $(1)/usr/lib/
endef

define Build/UninstallDev
	- rm -Rf $(1)/usr/include/dti
	- rm -f $(1)/usr/lib/libdti*.a
endef

ifdef CONFIG_PACKAGE_ifx-dsl-vinax2
define Package/ifx-dti/install
	$(INSTALL_DIR) $(1)/$(IFX_BIN_DIR)
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/src/dti_agent $(1)/$(IFX_BIN_DIR)
endef
endif

ifdef CONFIG_PACKAGE_ifx-dsl-vinax2-emul
define Package/ifx-dti/install
	$(INSTALL_DIR) $(1)/$(IFX_BIN_DIR)
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/src/dti_agent $(1)/$(IFX_BIN_DIR)
endef
endif

ifdef CONFIG_PACKAGE_ifx-dsl-geminaxd
define Package/ifx-dti/install
	$(INSTALL_DIR) $(1)/$(IFX_BIN_DIR)
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/src/dti_agent $(1)/$(IFX_BIN_DIR)
endef
endif

$(eval $(call BuildPackage,ifx-dti))
