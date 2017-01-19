#
# Copyright (C) 2010 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
# $Id:  $

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=ltq-evtlog
PKG_VERSION:=1.1.2
PKG_RELEASE:=1
PKG_BASE_NAME:=drv_event_logger

include $(INCLUDE_DIR)/package-version-override.mk

PKG_SOURCE:=$(PKG_BASE_NAME)-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=http://localhost
PKG_SOURCE_DIR:=$(BUILD_DIR)/$(PKG_BASE_NAME)-$(PKG_VERSION)
PKG_BUILD_DIR:=$(PKG_SOURCE_DIR)
PKG_CAT:=zcat
PKG_BUILD_DEPENDS:=ifxos
LANTIQ_BIN_DIR=opt/lantiq/bin

include $(INCLUDE_DIR)/package.mk

define Package/$(PKG_NAME)
  SECTION:=driver
  CATEGORY:=Lantiq
  TITLE:=Event Logger Kernel Module
  URL:=http://www.Lantiq.com/
  MAINTAINER:=Lantiq Deutschland GmbH
  MENU:=1
  DEPENDS:=+kmod-ifxos
endef

define Package/$(PKG_NAME)/description
	Lantiq Event Logger (Kernel Module)
endef

CONFIGURE_ARGS += --disable-trace --with-kernel-incl="$(LINUX_DIR)/include" \
						--with-ifxos-incl="$(STAGING_DIR)/usr/include/ifxos" \
						--with-ifxos-lib="$(STAGING_DIR)/usr/lib"

ifeq ($(CONFIG_LINUX_2_4),)
CONFIGURE_ARGS += --enable-linux-26 --with-kernel-build="$(LINUX_DIR)" ARCH=$(LINUX_KARCH)
PKG_MODULE_EXTENSION=.ko
else
ifneq ($(CONFIG_mips)$(CONFIG_mipsel),)
CONFIGURE_ARGS += --with-cflags="-fno-pic -mno-abicalls -mlong-calls -G 0"
endif
endif

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/load_evtlog.sh $(1)/etc/init.d/
	$(INSTALL_DIR) $(1)/$(LANTIQ_BIN_DIR)
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/src/$(PKG_BASE_NAME)$(PKG_MODULE_EXTENSION) $(1)/$(LANTIQ_BIN_DIR)
	$(INSTALL_DIR) $(1)/$(MODULES_SUBDIR)
	ln -snf /$(LANTIQ_BIN_DIR)/$(PKG_BASE_NAME)$(PKG_MODULE_EXTENSION) $(1)/$(MODULES_SUBDIR)/$(PKG_BASE_NAME).$(LINUX_KMOD_SUFFIX)
	ln -snf /$(MODULES_SUBDIR)/drv_ifxos.$(LINUX_KMOD_SUFFIX) $(1)/$(LANTIQ_BIN_DIR)/drv_ifxos$(PKG_MODULE_EXTENSION)
endef

define KernelPackage/$(PKG_NAME)
  SUBMENU:=Lantiq
  DEPENDS:=+kmod-ifxos
  TITLE:=Event Logger Module
  URL:=http://www.lantiq.com/
  MAINTAINER:=Lantiq
  FILES:=$(PKG_BUILD_DIR)/src/drv_event_logger$(PKG_MODULE_EXTENSION)
  AUTOLOAD:=$(call AutoLoad,15,drv_event_logger)
endef

define KernelPackage/$(PKG_NAME)/description
   Event Logger Kernel Module
endef

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/include
	mkdir -p $(1)/usr/include/event_logger
	$(CP) $(PKG_SOURCE_DIR)/include/* $(1)/usr/include/event_logger
endef

define Build/UninstallDev
	- rm -Rf $(1)/usr/include/event_logger
endef

# compabillity for old name
define Package/ifx-evtlog
  SECTION:=driver
  CATEGORY:=Lantiq
  TITLE:=Event Logger Kernel Module
  URL:=http://www.Lantiq.com/
  MAINTAINER:=Lantiq Deutschland GmbH
  MENU:=1
  DEPENDS:=+kmod-ltq-evtlog
endef

define Package/ifx-evtlog/description
	Lantiq Event Logger (Kernel Module)
endef

define Package/ifx-evtlog/install
endef

$(eval $(call BuildPackage,ifx-evtlog))

$(eval $(call BuildPackage,$(PKG_NAME)))
$(eval $(call KernelPackage,$(PKG_NAME)))
