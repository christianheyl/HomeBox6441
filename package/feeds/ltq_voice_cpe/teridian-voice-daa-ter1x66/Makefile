#
# Copyright (C) 2008 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=teridian-voice-daa-ter1x66
PKG_VERSION:=2.7.3lq7
PKG_MD5SUM:=8f2889494b8ca3b10cd985f153a12bfb
PKG_RELEASE:=1

# IMPORTANT - include the version-override here after PKG_VERSION definition
include $(INCLUDE_DIR)/package-version-override.mk
# IMPORTANT - ... and before using PKG_VERSION below

PKG_SOURCE:=drv_ter1x66-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=http://localhost
PKG_BUILD_DEPENDS:=ifx-voice-cpe-tapi
PKG_BUILD_DIR:=$(KERNEL_BUILD_DIR)/drv_ter1x66-$(PKG_VERSION)
PKG_START_SCRIPT:=teridian_load_voice_ter1x66.sh
IFX_BIN_DIR=opt/ifx/bin
LANTIQ_BIN_DIR=opt/lantiq/bin

include $(INCLUDE_DIR)/package.mk

define Package/$(PKG_NAME)
  SECTION:=driver
  CATEGORY:=Lantiq
  TITLE:=Teridian 1x66 DAA driver
  URL:=http://www.teridian.com/
  MAINTAINER:=Teridian Semiconductor
  MENU:=1
  DEPENDS:=ifx-voice-cpe-tapi +kmod-ltqcpe_spi
endef

define Package/$(PKG_NAME)/description
	Teridian 1x66 device driver with IFX TAPI compatible interface and
	therefore requires some TAPI headers.
endef

define Package/$(PKG_NAME)/config
$(call Package/$(PKG_NAME)/override_version)
$(call Package/$(PKG_NAME)/override_source_path)
	source "$(SOURCE)/config/Config.in"
endef

DRV_TAPI_INCLUDE:=$(STAGING_DIR)/usr/include/drv_tapi

CONFIGURE_ARGS += --with-kernel-incl="$(LINUX_DIR)/include"
CONFIGURE_ARGS += --with-tapi-incl="$(DRV_TAPI_INCLUDE)"
CONFIGURE_ARGS += --enable-warning

ifneq ($(CONFIG_LINUX_2_4),y)
CONFIGURE_ARGS += --enable-linux-26 --with-kernel-build="$(LINUX_DIR)" ARCH=$(LINUX_KARCH)
PKG_MODULE_EXTENSION=.ko
endif

# selecting driver's user configuration file
ifeq ($(CONFIG_TERIDIAN_DAA_BOARD_EASY50712),y)
DRV_CONFIG_USER_FILE := drv_config_user_danube.h
endif
ifeq ($(CONFIG_TERIDIAN_DAA_BOARD_EASY50712_V3),y)
DRV_CONFIG_USER_FILE := drv_config_user.ueip.easy50712V3_danubev3.h
endif
ifeq ($(CONFIG_TERIDIAN_DAA_BOARD_VR9),y)
DRV_CONFIG_USER_FILE := drv_config_user.ueip.easy80910_vr9.h
endif
ifeq ($(CONFIG_TERIDIAN_DAA_BOARD_EASY508xx),y)
DRV_CONFIG_USER_FILE := drv_config_user.ueip.easy50822_ar9.h
endif
ifeq ($(CONFIG_TERIDIAN_DAA_ENFORCE_POLLING),y)
DRV_CONFIG_USER_ENFORCE_POLLING := y
else
DRV_CONFIG_USER_ENFORCE_POLLING := n
endif


define Build/Configure
	$(call Build/Configure/Default)
	if [ "$(DRV_CONFIG_USER_ENFORCE_POLLING)" == "n" ] ; then \
		(cd $(PKG_BUILD_DIR) && rm -f drv_config_user.h && ln -snf $(PKG_BUILD_DIR)/src/$(DRV_CONFIG_USER_FILE) drv_config_user.h);\
	else \
		(cd $(PKG_BUILD_DIR) && rm -f drv_config_user.h && cat $(PKG_BUILD_DIR)/src/$(DRV_CONFIG_USER_FILE) | sed "s/^#define IRQ_NUMBER .*/#define IRQ_NUMBER 0/" > drv_config_user.h) ; \
	fi
endef

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/include
	mkdir -p $(1)/usr/include/drv_ter1x66
	$(CP) $(PKG_BUILD_DIR)/include/* $(1)/usr/include/drv_ter1x66
endef

define Build/UninstallDev
	rm -rf $(BUILD_DIR)/drv_ter1x66
endef

define Build/Clean
	@- [ -e $(PKG_BUILD_DIR)/ipkg ] && rm -Rf $(PKG_BUILD_DIR)/ipkg
	@- [ -e $(PKG_BUILD_DIR)/Makefile ] && $(MAKE) -C $(PKG_BUILD_DIR) clean
	$(call Build/Clean/Default)
endef

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/$(PKG_START_SCRIPT) $(1)/etc/init.d/
	$(INSTALL_DIR) $(1)/$(IFX_BIN_DIR)
	$(INSTALL_DIR) $(1)/$(LANTIQ_BIN_DIR)
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/src/drv_ter1x66$(PKG_MODULE_EXTENSION) $(1)/$(LANTIQ_BIN_DIR)
	$(INSTALL_DIR) $(1)/$(MODULES_SUBDIR)
	ln -snf /$(LANTIQ_BIN_DIR)/drv_ter1x66$(PKG_MODULE_EXTENSION) $(1)/$(MODULES_SUBDIR)/drv_ter1x66.$(LINUX_KMOD_SUFFIX)
	(cd $(1)/$(IFX_BIN_DIR); ln -snf ../../lantiq/bin/drv_ter1x66.$(LINUX_KMOD_SUFFIX) .)
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
