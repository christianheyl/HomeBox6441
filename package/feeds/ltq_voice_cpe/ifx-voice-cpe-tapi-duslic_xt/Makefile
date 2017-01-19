#
# Copyright (C) 2008 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=ifx-voice-cpe-tapi-duslic_xt
PKG_VERSION:=1.5.4.2
PKG_MD5SUM:=f61714f672e99da07f3a0d1160505caf
PKG_RELEASE:=1

# IMPORTANT - include the version-override here after PKG_VERSION definition
include $(INCLUDE_DIR)/package-version-override.mk
# IMPORTANT - ... and before using PKG_VERSION below

PKG_SOURCE:=drv_dxt-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=http://localhost
PKG_BUILD_DIR:=$(KERNEL_BUILD_DIR)/drv_dxt-$(PKG_VERSION)
PKG_START_SCRIPT:=ifx_load_voice_dxt.sh
PKG_BUILD_DEPENDS:=ifxos ifx-voice-cpe-tapi \
                   PACKAGE_ltq-evtlog:ltq-evtlog
LANTIQ_BIN_DIR=opt/lantiq/bin

include $(INCLUDE_DIR)/package.mk

define Package/$(PKG_NAME)
  SECTION:=driver
  CATEGORY:=Lantiq
  TITLE:=TAPI LL driver for DUSLIC-xT
  URL:=http://www.lantiq.com/
  MAINTAINER:=Lantiq Deutschland GmbH
  DEPENDS:=ifx-voice-cpe-tapi +kmod-ifxos
  MENU:=1
endef
#  SUBMENU:=TAPI

define Package/$(PKG_NAME)/description
	Voice Subsystem Low Level Driver for DUSLIC-xT device
endef

define Package/$(PKG_NAME)/config
select PACKAGE_ifx-voice-cpe-tapi-duslic_xt-firmware
select PACKAGE_ifx-voice-cpe-tapi-duslic_xt-coef
$(call Package/$(PKG_NAME)/override_version)
$(call Package/$(PKG_NAME)/override_source_path)
	source "$(SOURCE)/config/Config.in"
endef

# Add user specified CFLAGS and configure options

ifneq ($(CONFIG_LTQ_VOICE_DXT_USER_FLAGS),"")
DXT_USER_CFLAGS=$(strip $(subst ",, $(CONFIG_LTQ_VOICE_DXT_USER_FLAGS)))
CONFIGURE_ARGS += --with-cflags="$(DXT_USER_CFLAGS)"
endif

ifneq ($(CONFIG_LTQ_VOICE_DXT_USER_OPTIONS),"")
CONFIGURE_ARGS += $(subst ",, $(CONFIG_LTQ_VOICE_DXT_USER_OPTIONS))
endif


# Options specific for TAPI-LL VMMC configuration

CONFIGURE_ARGS += --enable-proc \
                  --enable-direct-chip-access \
                  --enable-power-save \
                  --enable-dcdc-hw=IBB12


ifeq ($(CONFIG_LTQ_VOICE_DXT_GPIO),y)
CONFIGURE_ARGS += --enable-gpio
else
CONFIGURE_ARGS += --disable-gpio
endif


# configure options derived from TAPI configuration

ifeq ($(CONFIG_LTQ_VOICE_CPE_TAPI_CID),y)
CONFIGURE_ARGS += --enable-cid
else
CONFIGURE_ARGS += --disable-cid
endif

ifeq ($(CONFIG_LTQ_VOICE_CPE_TAPI_LT_GR909),y)
CONFIGURE_ARGS += --enable-lt
else
# no disable because it can interfere with phone detection
endif

ifeq ($(CONFIG_LTQ_VOICE_CPE_TAPI_METERING),y)
CONFIGURE_ARGS += --enable-metering
else
CONFIGURE_ARGS += --disable-metering
endif

ifeq ($(CONFIG_LTQ_VOICE_CPE_TAPI_CONT_MEASUREMENT),y)
CONFIGURE_ARGS += --enable-cont-measurement
else
CONFIGURE_ARGS += --disable-cont-measurement
endif

ifeq ($(CONFIG_LTQ_VOICE_CPE_TAPI_PHONE_DETECTION),y)
CONFIGURE_ARGS += --enable-cap-measurement
else
# no disable because it can interfere with linetesting
endif

ifeq ($(CONFIG_LTQ_VOICE_CPE_TAPI_TRACES),y)
CONFIGURE_ARGS += --enable-trace
else
CONFIGURE_ARGS += --disable-trace
endif

ifeq ($(CONFIG_LTQ_VOICE_CPE_TAPI_EVENT_LOGGER),y)
CONFIGURE_ARGS += --enable-el-debug --with-el-incl=$(STAGING_DIR)/usr/include/event_logger
else
CONFIGURE_ARGS += --disable-el-debug
endif

DRV_TAPI_INCLUDE:=$(STAGING_DIR)/usr/include/drv_tapi

CONFIGURE_ARGS += --with-kernel-incl="$(LINUX_DIR)/include"
CONFIGURE_ARGS += --with-tapi-incl="$(DRV_TAPI_INCLUDE)"
CONFIGURE_ARGS += --with-ifxos-incl=$(STAGING_DIR)/usr/include/ifxos

ifneq ($(CONFIG_LINUX_2_4),y)
CONFIGURE_ARGS += --enable-linux-26 --with-kernel-build="$(LINUX_DIR)" ARCH=$(LINUX_KARCH)
PKG_MODULE_EXTENSION=.ko
endif

define Build/Configure
	$(call Build/Configure/Default)
	(cd $(PKG_BUILD_DIR)/src && rm -f drv_config_user.h && ln -sf drv_config_user.ar9.h drv_config_user.h)
endef

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/include
	mkdir -p $(1)/usr/include/drv_duslic_xt
	$(CP) -v --dereference $(PKG_BUILD_DIR)/include/* $(1)/usr/include/drv_duslic_xt
endef

define Build/UninstallDev
	rm -rf $(BUILD_DIR)/drv_dxt
endef

define Build/Clean
	@- [ -e $(PKG_BUILD_DIR)/ipkg ] && rm -Rf $(PKG_BUILD_DIR)/ipkg
	@- [ -e $(PKG_BUILD_DIR)/Makefile ] && $(MAKE) -C $(PKG_BUILD_DIR) clean
	$(call Build/Clean/Default)
endef

define Package/$(PKG_NAME)/install
	# Startup script
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/$(PKG_START_SCRIPT) $(1)/etc/init.d/
	# Primary install location of the driver binary
	$(INSTALL_DIR) $(1)/$(MODULES_SUBDIR)
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/src/drv_dxt$(PKG_MODULE_EXTENSION) $(1)/$(MODULES_SUBDIR)
	# For easy access also create a link in Lantiq bin directory
	$(INSTALL_DIR) $(1)/$(LANTIQ_BIN_DIR)
	ln -snf /$(MODULES_SUBDIR)/drv_dxt$(PKG_MODULE_EXTENSION) $(1)/$(LANTIQ_BIN_DIR)/drv_dxt.$(LINUX_KMOD_SUFFIX)
#	$(INSTALL_DIR) $(1)/$(IFX_DWLD_DIR)
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
