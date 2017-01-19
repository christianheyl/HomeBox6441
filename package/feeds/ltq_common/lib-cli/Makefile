#
# Copyright (C) 2006-2010 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=lib-cli
PKG_RELEASE:=1
PKG_BASE_NAME:=lib_cli
PKG_VERSION:=2.1.0

PKG_SOURCE:=$(PKG_BASE_NAME)-$(PKG_VERSION).tar.gz
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_BASE_NAME)-$(PKG_VERSION)
PKG_SOURCE_URL:=http://localhost

PKG_BIN_DIR=/opt/lantiq/bin
PKG_BUILD_DEPENDS:=ifxos

PKG_FIXUP:=libtool
PKG_INSTALL:=1

#PKG_IS_KERNEL_MODULE:=y
include $(INCLUDE_DIR)/package-version-override.mk
include $(INCLUDE_DIR)/package.mk

ifeq ($(DUMP),)
	STAMP_CONFIGURED:=$(strip $(STAMP_CONFIGURED))_$(shell $(SH_FUNC) grep '^CONFIG_COMMON_CLI_' $(TOPDIR)/.config | md5s)
endif

define Package/$(PKG_NAME)
  SECTION:=driver
  CATEGORY:=Lantiq
  TITLE:= CLI Library
  URL:=http://www.lantiq.com/
  MAINTAINER:=Lantiq Deutschland GmbH
  DEPENDS:=+ifxos +libpthread
  MENU:=1
  SUBMENU:=CLI
endef

define Package/$(PKG_NAME)/description
 The common command line interface (CLI) library.
endef

define Package/$(PKG_NAME)/config
	menu "configuration"
	depends on PACKAGE_$(PKG_NAME)
		source "$(SOURCE)/config/Config.in"
	endmenu
   $(call Package/$(PKG_NAME)/override_version)
   $(call Package/$(PKG_NAME)/override_source_path)
endef

# make the submake silent:
MAKE_FLAGS += -s

CONFIGURE_ARGS += \
	--enable-kernelincl="$(LINUX_DIR)/include"

ifeq ($(CONFIG_LINUX_2_4),)
CONFIGURE_ARGS += --enable-linux-26 --enable-kernelbuild="$(LINUX_DIR)" ARCH=$(LINUX_KARCH)
PKG_MODULE_EXTENSION:=.ko
endif

ifeq ($(CONFIG_LINUX_2_4),y)
ifneq ($(CONFIG_mips)$(CONFIG_mipsel),)
CONFIGURE_ARGS += --enable-add_drv_cflags="-fno-pic -mno-abicalls -mlong-calls -G 0"
endif
define KernelPackage/ifxos/install
	cd $(1)/lib/modules/$(LINUX_VERSION) && ln -snf mod_cli mod_cli.o
endef
endif

CONFIGURE_ARGS += \
	--enable-ifxos-include="-I$(STAGING_DIR)/usr/include/ifxos" \
	--enable-ifxos-library="-L$(STAGING_DIR)/usr/lib" \
	$(call autoconf_bool,CONFIG_COMMON_CLI_ERROR_PRINTS,error-print) \
	$(call autoconf_bool,CONFIG_COMMON_CLI_DEBUG_PRINTS,debug-print) \
	$(call autoconf_bool,CONFIG_COMMON_CLI_SUPPORT_BUFFER_OUT,cli-buffer-pout) \
	$(call autoconf_bool,CONFIG_COMMON_CLI_SUPPORT_FILE_OUT,cli-file-pout) \
	$(call autoconf_bool,CONFIG_COMMON_CLI_SUPPORT_CONSOLE,cli-console) \
	$(call autoconf_bool,CONFIG_COMMON_CLI_SUPPORT_PIPE,cli-pipe) \
	--with-max-pipes=$(strip CONFIG_COMMON_CLI_MAX_CLI_PIPES) \
	--enable-build-example --with-kernel-module

define KernelPackage/$(PKG_NAME)
  SUBMENU:=Lantiq
  DEPENDS:=+kmod-ifxos @BROKEN
  TITLE:=CLI Module (kernel space)
  URL:=http://www.lantiq.com/
  MAINTAINER:=Lantiq Deutschland GmbH
  FILES:=$(PKG_BUILD_DIR)/src/mod_cli$(PKG_MODULE_EXTENSION)
  AUTOLOAD:=$(call AutoLoad,76,drv_ifxos mod_cli)
endef

define KernelPackage/$(PKG_NAME)/description
  The common command line interface (CLI) kernel module.
endef

define Build/Clean
	@- rm -Rf $(PKG_BUILD_DIR)/ipkg*
	@- [ -e $(PKG_BUILD_DIR)/Makefile ] && $(MAKE) -C $(PKG_BUILD_DIR) clean
	$(call Build/Clean/Default)
endef

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/{lib,include}
	$(CP) $(PKG_INSTALL_DIR)/include/cli $(1)/usr/include/
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/libcli.* $(1)/usr/lib/
endef

define Build/UninstallDev
	- rm -Rf $(1)/usr/include/cli
	- rm -f $(1)/usr/lib/libcli.*
endef

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/libcli.so.* $(1)/usr/lib/
ifeq ($(CONFIG_COMMON_CLI_EXAMPLE),y)
	$(INSTALL_DIR) $(1)/$(PKG_BIN_DIR)
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/bin/cli_example $(1)/$(PKG_BIN_DIR)
endif
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
$(eval $(call KernelPackage,$(PKG_NAME)))
