#
# Copyright (C) 2008 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=ifx-switch
PKG_VERSION:=0.7.4
PKG_RELEASE:=1
PKG_SOURCE:=drv_ethsw-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=http://localhost
PKG_CAT:=zcat
PKG_SOURCE_DIR:=$(BUILD_DIR)/drv_ethsw-$(PKG_VERSION)
PKG_BUILD_DIR:=$(PKG_SOURCE_DIR)
#PKG_UNPACK += cp -f $(PKG_SOURCE_DIR)/include/ifx_ethsw.h $(LINUX_DIR)/include/asm/ifx/
IFX_BIN_DIR=opt/ifx/bin
PKG_START_SCRIPT:=ifx_load_switch.sh

include $(INCLUDE_DIR)/package-version-override.mk
include $(INCLUDE_DIR)/package.mk

define Package/$(PKG_NAME)
  SECTION:=driver
  CATEGORY:=Lantiq
  TITLE:=Ethernet Switch driver
  URL:=http://www.lantiq.com/
  MAINTAINER:=Lantiq
  MENU:=1
endef

define Package/$(PKG_NAME)/description
	Ethernet Switch driver with ioctl support
endef

CONFIGURE_ARGS += --enable-kernelincl="$(LINUX_DIR)/include"

ifeq ($(CONFIG_LINUX_2_6),y)
CONFIGURE_ARGS += --enable-linux-26 --enable-kernelbuild="$(LINUX_DIR)" ARCH=$(LINUX_KARCH)
PKG_MODULE_EXTENSION=.ko
endif

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/include/drv_ethsw
	mkdir -p $(1)/usr/include/drv_ethsw
	$(CP) $(PKG_SOURCE_DIR)/include/* $(1)/usr/include/drv_ethsw
endef

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/opt/ifx/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/src/switch_control $(1)/$(IFX_BIN_DIR)
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/$(PKG_START_SCRIPT) $(1)/etc/init.d/
	ln -snf /$(IFX_BIN_DIR)/switch_control $(1)/usr/bin/switch_control
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/src/drv_ethsw$(PKG_MODULE_EXTENSION) $(1)/$(IFX_BIN_DIR)
	$(INSTALL_DIR) $(1)/$(MODULES_SUBDIR)
	ln -snf /$(IFX_BIN_DIR)/drv_ethsw$(PKG_MODULE_EXTENSION) $(1)/$(MODULES_SUBDIR)/drv_ethsw.$(LINUX_KMOD_SUFFIX)
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
