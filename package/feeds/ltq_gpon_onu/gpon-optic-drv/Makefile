#
# Copyright (C) 2009 OpenWrt.org
# Copyright (C) 2011 lantiq.com
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=gpon-optic-drv
PKG_VERSION:=4.5.0
PKG_BASE_NAME:=gpon_optic_drv
PKG_SOURCE:=$(PKG_BASE_NAME)-$(PKG_VERSION).tar.gz
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_BASE_NAME)-$(PKG_VERSION)
PKG_BUILD_DEPENDS:=GPON_DRV_OPTIC_EVENT_LOGGER:ltq-evtlog GPON_OPTIC_REMOTE:gpon-rpc GPON_OPTIC_REMOTE_ONLY:gpon-rpc
PKG_RELEASE:=3
PKG_SOURCE_URL:=http://localhost
PKG_BIN_DIR=opt/lantiq/bin
PKG_INSTALL:=1

PKG_FIXUP:=autoreconf

include $(INCLUDE_DIR)/package-version-override.mk
include $(INCLUDE_DIR)/package.mk

ifeq ($(DUMP),)
	STAMP_CONFIGURED:=$(strip $(STAMP_CONFIGURED))_$(shell $(SH_FUNC) grep '^CONFIG_GPON_' $(TOPDIR)/.config | md5s)
endif

define Package/$(PKG_NAME)
  SECTION:=driver
  CATEGORY:=Lantiq
  TITLE:= GPON Optic Control Application
  URL:=http://www.lantiq.com/
  MAINTAINER:=Lantiq
  DEPENDS:=+kmod-ifxos +!GPON_OPTIC_REMOTE_ONLY:kmod-gpon-optic-drv +gpon-optic-config +GPON_OPTIC_REMOTE:gpon-rpc +GPON_OPTIC_REMOTE_ONLY:gpon-rpc
  MENU:=1
  SUBMENU:=GPON
  DEBUG_LEVEL:=1
endef

define Package/$(PKG_NAME)/description
   The optic control application for the GPON device
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
	--enable-kernelincl="$(LINUX_DIR)/include" \
	--enable-ifxos-include="-I$(STAGING_DIR)/usr/include/ifxos" \
	--enable-ifxos-library="-L$(STAGING_DIR)/usr/lib"

#CONFIGURE_ARGS += \
#	--with-optic-register-simulation

ifeq ($(CONFIG_GPON_OPTIC_CLI),y)
CONFIGURE_ARGS += --with-cli="yes"
else
CONFIGURE_ARGS += --with-cli="no"
endif

ifeq ($(CONFIG_GPON_OPTIC_PROCFS),y)
CONFIGURE_ARGS += --with-procfs="yes"
else
CONFIGURE_ARGS += --with-procfs="no"
endif

# Event logger support for testing
ifeq ($(CONFIG_GPON_DRV_OPTIC_EVENT_LOGGER),y)
GPON_DRV_OPTIC_EVENT_LOGGER:=--with-el-incl=-I$(STAGING_DIR)/usr/include/event_logger
else
GPON_DRV_OPTIC_EVENT_LOGGER:=
endif

ifneq ($(CONFIG_GPON_OPTIC_DEBUG),y)
CONFIGURE_ARGS += --without-debug-support
endif

ifeq ($(CONFIG_GPON_OPTIC_NO_REMOTE),y)
CONFIGURE_ARGS += --disable-remote-onu
endif

ifeq ($(CONFIG_GPON_OPTIC_REMOTE_ONLY),y)
CONFIGURE_ARGS += --enable-remote-only-onu
endif

ifneq ($(CONFIG_LINUX_2_4),y)
CONFIGURE_ARGS += --enable-linux-26 --enable-kernelbuild="$(LINUX_DIR)" ARCH=$(LINUX_KARCH) $(GPON_DRV_OPTIC_EVENT_LOGGER)
PKG_MODULE_EXTENSION=.ko
endif

define KernelPackage/$(PKG_NAME)
  SUBMENU:=Lantiq
  DEPENDS:=+kmod-ifxos
  TITLE:=GPON optic driver
  URL:=http://www.lantiq.com/
  MAINTAINER:=Lantiq
  FILES:=$(PKG_BUILD_DIR)/src/mod_optic$(PKG_MODULE_EXTENSION)
  AUTOLOAD:=$(call AutoLoad,70,drv_ifxos mod_optic)
endef

define KernelPackage/$(PKG_NAME)/description
   The optic driver for GPON device
endef

define KernelPackage/$(PKG_NAME)/install
	sed -i -r -e 's/mod_optic$$$$$$$$/mod_optic debug=$(DEBUG_LEVEL)/' $(1)/etc/modules.d/??-$(PKG_NAME)
endef

define Build/Configure
	# remove "configured" stamps of other configs
	@-rm $(PKG_BUILD_DIR)/.configured*
	$(call Build/Configure/Default)
endef

define Build/Clean
	@- rm -Rf $(PKG_BUILD_DIR)/ipkg*
	@- [ -e $(PKG_BUILD_DIR)/Makefile ] && $(MAKE) -C $(PKG_BUILD_DIR) clean
	$(call Build/Clean/Default)
endef

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/include/optic_drv
	$(CP) $(PKG_INSTALL_DIR)/include/* $(1)/usr/include/optic_drv
endef

define Package/$(PKG_NAME)/install

ifneq ($(CONFIG_GPON_OPTIC_REMOTE_ONLY),y)
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_DIR) $(1)/etc/hotplug.d/gpon
	$(INSTALL_BIN) files/optic.sh $(1)/etc/init.d
	$(INSTALL_BIN) files/10-optic $(1)/etc/hotplug.d/gpon
endif
	$(INSTALL_DIR) $(1)/$(PKG_BIN_DIR)
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/tools/optic/optic $(1)/$(PKG_BIN_DIR)
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/tools/otop/optic_top $(1)/$(PKG_BIN_DIR)/otop
endef

diststore: distcheck
scp ${PACKAGE_TARNAME}-${PACKAGE_VERSION}.tar.gz mts.lantiq.com:~wlnsw/dist/

$(eval $(call BuildPackage,$(PKG_NAME)))
$(eval $(call KernelPackage,$(PKG_NAME)))
