#
# Copyright (C) 2009 OpenWrt.org
# Copyright (C) 2011 lantiq.com
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=gpon-onu-drv
PKG_VERSION:=4.5.0
PKG_BASE_NAME:=gpon_onu_drv
PKG_SOURCE:=$(PKG_BASE_NAME)-$(PKG_VERSION).tar.gz
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_BASE_NAME)-$(PKG_VERSION)
PKG_RELEASE:=3
PKG_SOURCE_URL:=http://localhost
PKG_BIN_DIR=opt/lantiq/bin
PKG_BUILD_DEPENDS:=GPON_DRV_ONU_EVENT_LOGGER:ltq-evtlog GPON_ONU_REMOTE_ONLY:gpon-rpc GPON_ONU_REMOTE:gpon-rpc
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
  TITLE:= GPON Optical Network Unit (ONU) Control Application
  URL:=http://www.lantiq.com/
  MAINTAINER:=Lantiq
  DEPENDS:=+kmod-ifxos +!GPON_ONU_REMOTE_ONLY:kmod-gpon-onu-drv +GPON_ONU_REMOTE_ONLY:gpon-rpc +GPON_ONU_REMOTE:gpon-rpc
  MENU:=1
  SUBMENU:=GPON
endef

define Package/$(PKG_NAME)/description
   The control application for GPON Optical Network Unit (ONU) device
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
	--enable-ifxos-library="-L$(STAGING_DIR)/usr/lib" \
	--with-max-eth-uni=$(CONFIG_GPON_ONU_MAX_ETH_UNI)

ifeq ($(CONFIG_GPON_ONU_DEVICE_PSB98010),y)
CONFIGURE_ARGS += --enable-device=PSB98010
endif

ifeq ($(CONFIG_GPON_ONU_DEVICE_PSB98020),y)
CONFIGURE_ARGS += --enable-device=PSB98020
endif

ifeq ($(CONFIG_GPON_ONU_DEVICE_PSB98030),y)
CONFIGURE_ARGS += --enable-device=PSB98030
endif

# make the submake silent:
MAKE_FLAGS += -s

ifeq ($(CONFIG_GPON_ONU_CLI_DUMP),y)
CONFIGURE_ARGS += --with-cli-dump
endif

ifeq ($(CONFIG_GPON_ONU_CLI),y)
CONFIGURE_ARGS += --with-cli="yes"
else
CONFIGURE_ARGS += --with-cli="no"
endif

ifeq ($(CONFIG_GPON_ONU_NO_REMOTE),y)
CONFIGURE_ARGS += --disable-remote-onu
endif

ifeq ($(CONFIG_GPON_ONU_REMOTE_ONLY),y)
CONFIGURE_ARGS += --enable-remote-only-onu
endif

# Event logger support for testing
ifeq ($(CONFIG_GPON_DRV_ONU_EVENT_LOGGER),y)
GPON_DRV_ONU_EVENT_LOGGER:=--with-el-incl=-I$(STAGING_DIR)/usr/include/event_logger
else
GPON_DRV_ONU_EVENT_LOGGER:=
endif

ifneq ($(CONFIG_LINUX_2_4),y)
CONFIGURE_ARGS += --enable-linux-26 --enable-kernelbuild="$(LINUX_DIR)" ARCH=$(LINUX_KARCH) $(GPON_DRV_ONU_EVENT_LOGGER)
PKG_MODULE_EXTENSION=.ko
endif

ifeq ($(CONFIG_GPON_ONU_TOD_ASC1),y)
CONFIGURE_ARGS += --with-tod-asc1
endif

ifeq ($(CONFIG_GPON_ONU_EXPORT_SYMBOLS),y)
CONFIGURE_ARGS += --with-dbg-exports
endif

ifeq ($(CONFIG_GPON_ONU_SCE_TABLES_WRAPPERS),y)
CONFIGURE_ARGS += --with-sce-tables-wrappers
endif

ifneq ($(CONFIG_GPON_ONU_DEBUG),y)
CONFIGURE_ARGS += --without-debug-support
endif

ifneq ($(CONFIG_GPON_ONU_COP_DEBUG),y)
CONFIGURE_ARGS += --without-cop-dbg
endif

ifneq ($(CONFIG_GPON_ONU_SCE_DEBUG),y)
CONFIGURE_ARGS += --without-sce-dbg
endif

ifneq ($(CONFIG_GPON_ONU_PROCFS),y)
CONFIGURE_ARGS += --without-procfs
endif

define KernelPackage/$(PKG_NAME)
  SUBMENU:=Lantiq
  DEPENDS:=+kmod-ifxos
  TITLE:=GPON Optical Network Unit (ONU) driver
  URL:=http://www.lantiq.com/
  MAINTAINER:=Lantiq
  FILES:=$(PKG_BUILD_DIR)/src/mod_onu$(PKG_MODULE_EXTENSION)
  AUTOLOAD:=$(call AutoLoad,80,drv_ifxos mod_onu)
endef

define KernelPackage/$(PKG_NAME)/description
   The driver for GPON Optical Network Unit (ONU) device
endef

define KernelPackage/$(PKG_NAME)_netdev/description
   The network driver for ONU device
endef

define KernelPackage/$(PKG_NAME)_netdev
  SUBMENU:=Lantiq
  DEPENDS:=kmod-$(PKG_NAME)
  TITLE:=ONU network driver
  URL:=http://www.lantiq.com/
  MAINTAINER:=Lantiq
  FILES:=$(PKG_BUILD_DIR)/netdev/mod_onu_netdev$(PKG_MODULE_EXTENSION)
  AUTOLOAD:=$(call AutoLoad,81,mod_onu mod_onu_netdev)
endef

define KernelPackage/$(PKG_NAME)_led_trigger/description
   The LED trigger driver for ONU device
endef

define KernelPackage/$(PKG_NAME)_led_trigger
  SUBMENU:=Lantiq
  DEPENDS:=kmod-$(PKG_NAME)
  TITLE:=ONU LED trigger driver
  URL:=http://www.lantiq.com/
  MAINTAINER:=Lantiq
  FILES:=$(PKG_BUILD_DIR)/led-trigger/mod_onu_led_trigger$(PKG_MODULE_EXTENSION)
  AUTOLOAD:=$(call AutoLoad,82,mod_onu mod_onu_netdev mod_onu_led_trigger)
endef

define Build/Configure
	# remove "configured" stamps of other configs
	@- rm $(PKG_BUILD_DIR)/.configured*
	$(call Build/Configure/Default)
endef

define Build/Clean
	@- rm -Rf $(PKG_BUILD_DIR)/ipkg*
	@- [ -e $(PKG_BUILD_DIR)/Makefile ] && $(MAKE) -C $(PKG_BUILD_DIR) clean
	$(call Build/Clean/Default)
endef

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/include/onu_drv
	$(CP) $(PKG_INSTALL_DIR)/include/* $(1)/usr/include/onu_drv
ifeq ($(CONFIG_GPON_ONU_EXPORT_SYMBOLS),y)
	$(INSTALL_DIR) $(1)/usr/include/onu_drv_dev
	$(CP) $(PKG_BUILD_DIR)/src/*.h $(1)/usr/include/onu_drv_dev
endif
ifeq ($(CONFIG_GPON_ONU_SCE_TABLES_WRAPPERS),y)
	$(CP) $(PKG_BUILD_DIR)/tools/onu/include/onu_sce_xml_wrappers.h $(1)/usr/include/onu_drv
	$(CP) $(PKG_BUILD_DIR)/tools/onu/include/onu_sce_json_wrappers.h $(1)/usr/include/onu_drv
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_BUILD_DIR)/tools/onu/libscewrappers.a $(1)/usr/lib
endif
endef

define Package/$(PKG_NAME)/install

ifneq ($(CONFIG_GPON_ONU_REMOTE_ONLY),y)
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) files/onu.sh $(1)/etc/init.d
	$(INSTALL_BIN) files/led.sh $(1)/etc/init.d
	$(INSTALL_BIN) files/lan.sh $(1)/etc/init.d
	$(INSTALL_DIR) $(1)/etc/hotplug.d/gpon
	$(INSTALL_DATA) ./files/10-net $(1)/etc/hotplug.d/gpon/10-net
endif
	$(INSTALL_DIR) $(1)/$(PKG_BIN_DIR)
	$(INSTALL_BIN) files/debug_fw.sh $(1)/$(PKG_BIN_DIR)
	$(INSTALL_BIN) files/gci_xml_table_dump.sh $(1)/$(PKG_BIN_DIR)
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/tools/onu/onu $(1)/$(PKG_BIN_DIR)
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/tools/gtop/gtop $(1)/$(PKG_BIN_DIR)
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/tools/gexdump/gexdump $(1)/$(PKG_BIN_DIR)
ifneq ($(CONFIG_GPON_ONU_NO_REMOTE),y)
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/tools/rpc/grpc_client $(1)/$(PKG_BIN_DIR)
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/tools/rpc/grpc_server $(1)/$(PKG_BIN_DIR)
endif
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
$(eval $(call KernelPackage,$(PKG_NAME)))
$(eval $(call KernelPackage,$(PKG_NAME)_netdev))
$(eval $(call KernelPackage,$(PKG_NAME)_led_trigger))
