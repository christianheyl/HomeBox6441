#
# Copyright (C) 2008 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
# 

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=switch_cli
PKG_VERSION:=1.4.0
#PKG_VERSION:=1.3.3
PKG_RELEASE:=1
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=http://localhost
PKG_CAT:=zcat
PKG_SOURCE_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)
PKG_BUILD_DIR:=$(PKG_SOURCE_DIR)
#PKG_UNPACK += cp -f $(PKG_SOURCE_DIR)/include/ifx_ethsw.h $(LINUX_DIR)/include/asm/ifx/
IFX_BIN_DIR=opt/ifx/bin
#PKG_START_SCRIPT:=ifx_load_switch.sh

include $(INCLUDE_DIR)/package-version-override.mk
include $(INCLUDE_DIR)/package.mk

define Package/$(PKG_NAME)
  SECTION:=driver
  CATEGORY:=Lantiq
  TITLE:=Ethernet Switch New Command Line Utility
  URL:=http://www.lantiq.com/
  MAINTAINER:=Lantiq GmbH
  MENU:=1
endef

define Package/$(PKG_NAME)/description
	Advanced Ulitity for Ethernet Switch driver with ioctl support
endef

### Menuconfig ##################################
define Package/$(PKG_NAME)/config
        menu "$(PKG_NAME) features"
        depends on PACKAGE_$(PKG_NAME)
        source "$(SOURCE)/config/Config.in"
        endmenu
endef
CONFIG_SWITCH_CLI_SYSTEM:=
### Configurations depends on Menuconfig ########
ifeq ($(CONFIG_VR9),y)
	CONFIG_SWITCH_CLI_SYSTEM += --enable-gswip
endif
ifeq ($(CONFIG_AR10),y)
	CONFIG_SWITCH_CLI_SYSTEM += --enable-gswip
endif

ifeq ($(CONFIG_AR9),y)
	CONFIG_SWITCH_CLI_SYSTEM += --enable-tantos
endif

ifeq ($(CONFIG_DANUBE),y)
	CONFIG_SWITCH_CLI_SYSTEM += --enable-tantos
endif

ifeq ($(CONFIG_AMAZON_SE),y)
	CONFIG_SWITCH_CLI_SYSTEM += --enable-tantos
endif

ifeq ($(CONFIG_LTQ_ETHSW_CLI_HELP_PRINTOUT),y)
	CONFIG_SWITCH_CLI_SYSTEM += --enable-printhelp
endif

CONFIGURE_ARGS += --enable-kernelincl="$(LINUX_DIR)/include" \
	--with-cflags="-I$(LINUX_DIR)/include/switch_api" \
	$(CONFIG_SWITCH_CLI_SYSTEM)

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/src/switch_cli $(1)/usr/bin/
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
