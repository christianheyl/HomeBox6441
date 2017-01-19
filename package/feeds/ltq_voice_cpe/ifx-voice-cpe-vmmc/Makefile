#
# Copyright (C) 2008 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=ifx-voice-cpe-vmmc
PKG_VERSION:=1.15.0.4
PKG_MD5SUM:=7a306fa5046d4b214fe95ac9d8d710b9
PKG_RELEASE:=1

# IMPORTANT - include the version-override here after PKG_VERSION definition
include $(INCLUDE_DIR)/package-version-override.mk
# IMPORTANT - ... and before using PKG_VERSION below

PKG_SOURCE:=drv_vmmc-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=http://localhost
PKG_BUILD_DIR:=$(KERNEL_BUILD_DIR)/drv_vmmc-$(PKG_VERSION)
PKG_START_SCRIPT:=ltq_load_voice_vmmc.sh
PKG_BUILD_DEPENDS:=ifxos ifx-voice-cpe-tapi \
                   PACKAGE_ltq-evtlog:ltq-evtlog
#IFX_BIN_DIR=opt/ifx/bin
LANTIQ_BIN_DIR=opt/lantiq/bin

include $(INCLUDE_DIR)/package.mk

define Package/$(PKG_NAME)
  SECTION:=driver
  CATEGORY:=Lantiq
  TITLE:=TAPI LL driver for Voice Macro
  URL:=http://www.lantiq.com/
  MAINTAINER:=Lantiq Deutschland GmbH
  DEPENDS:=ifx-voice-cpe-tapi +kmod-ifxos
  MENU:=1
endef

define Package/$(PKG_NAME)/description
	Voice Subsystem Low Level Driver for XWAY(TM) xRX100, xRX200, xRX300 family devices
endef

define Package/$(PKG_NAME)/config
$(call Package/$(PKG_NAME)/override_version)
$(call Package/$(PKG_NAME)/override_source_path)
	source "$(SOURCE)/config/Config.in"
endef


# Add user specified CFLAGS and configure options

ifneq ($(CONFIG_LTQ_VOICE_VMMC_USER_FLAGS),"")
VMMC_USER_CFLAGS=$(strip $(subst ",, $(CONFIG_LTQ_VOICE_VMMC_USER_FLAGS)))
CONFIGURE_ARGS += --with-cflags="$(VMMC_USER_CFLAGS)"
endif

ifeq ($(CONFIG_ARCADYAN_TARGET_kpn_VGV951ABWAC23-AB-30a-99),y)
CONFIGURE_ARGS += --with-cflags=-DARCADYAN_TARGET_kpn_VGV951ABWAC23
endif

ifeq ($(CONFIG_ARCADYAN_TARGET_o2_VRV9518SWAC33-B-49),y)
CONFIGURE_ARGS += --with-cflags=-DARCADYAN_TARGET_o2_VRV9518SWAC33
endif

ifeq ($(CONFIG_ARCADYAN_TARGET_o2_VRV9518SWAC24-B-49),y)
CONFIGURE_ARGS += --with-cflags=-DARCADYAN_TARGET_o2_VRV9518SWAC24
endif

ifeq ($(CONFIG_ARCADYAN_TARGET_zz_VRV9518SWAC33-B-ZZ),y)
CONFIGURE_ARGS += --with-cflags=-DARCADYAN_TARGET_zz_VRV9518SWAC33
endif

ifeq ($(CONFIG_ARCADYAN_TARGET_zz_VRV7518CW22-A-ZZ),y)
CONFIGURE_ARGS += --with-cflags=-DARCADYAN_TARGET_zz_VRV7518CW22
endif

ifeq ($(CONFIG_ARCADYAN_TARGET_tr_VRV7518CW22-B-TR),y)
CONFIGURE_ARGS += --with-cflags=-DARCADYAN_TARGET_tr_VRV7518CW22
endif

ifeq ($(CONFIG_ARCADYAN_TARGET_gr_VRV7006AW22-A-GR),y)
CONFIGURE_ARGS += --with-cflags=-DARCADYAN_TARGET_gr_VRV7518CW22
endif

ifeq ($(CONFIG_ARCADYAN_TARGET_gr_VRV7006AW22-A-GRT),y)
CONFIGURE_ARGS += --with-cflags=-DARCADYAN_TARGET_gr_VRV7518CW22
endif

ifeq ($(CONFIG_ARCADYAN_TARGET_ct_VRV7006AW22-A-CT),y)
CONFIGURE_ARGS += --with-cflags=-DARCADYAN_TARGET_gr_VRV7518CW22
endif

ifeq ($(CONFIG_ARCADYAN_TARGET_ta_VRV7006AW22-A-TA),y)
CONFIGURE_ARGS += --with-cflags=-DARCADYAN_TARGET_gr_VRV7518CW22
endif

ifneq ($(CONFIG_LTQ_VOICE_VMMC_USER_OPTIONS),"")
CONFIGURE_ARGS += $(subst ",, $(CONFIG_LTQ_VOICE_VMMC_USER_OPTIONS))
endif


# Options specific for TAPI-LL VMMC configuration

ifeq ($(CONFIG_LTQ_VOICE_CPE_VMMC_WITH_DEVICE_AR9),y)
CONFIGURE_ARGS += --with-device=AR9
endif

ifeq ($(CONFIG_LTQ_VOICE_CPE_VMMC_WITH_DEVICE_VR9),y)
CONFIGURE_ARGS += --with-device=VR9
endif

ifeq ($(CONFIG_LTQ_VOICE_CPE_VMMC_WITH_DEVICE_XRX300),y)
CONFIGURE_ARGS += --with-device=XRX300
endif

ifeq ($(CONFIG_LTQ_VOICE_CPE_VMMC_PMU),y)
CONFIGURE_ARGS += --enable-pmu
else
CONFIGURE_ARGS += --disable-pmu
endif

ifeq ($(CONFIG_LTQ_VOICE_CPE_VMMC_RTCP_XR),y)
CONFIGURE_ARGS += --enable-rtcp-xr
else
CONFIGURE_ARGS += --disable-rtcp-xr
endif

ifeq ($(CONFIG_LTQ_VOICE_CPE_VMMC_DISABLE_DECT_NIBBLE_SWAP),y)
CONFIGURE_ARGS += --disable-dect-nibble-swap
else
CONFIGURE_ARGS += --enable-dect-nibble-swap
endif

ifneq ($(CONFIG_LTQ_VOICE_CPE_VMMC_MPS_HISTORY_SIZE),128)
CONFIGURE_ARGS += --enable-history-buf=$(CONFIG_LTQ_VOICE_CPE_VMMC_MPS_HISTORY_SIZE)
endif

ifeq ($(CONFIG_LTQ_VOICE_CPE_VMMC_SSLIC_FXO_ACTIVATE),y)
CONFIGURE_ARGS += --enable-fxo-act
else
CONFIGURE_ARGS += --disable-fxo-act
endif


# configure options derived from TAPI configuration

ifeq ($(CONFIG_LTQ_VOICE_CPE_TAPI_FAX),y)
# it should be save to enable-t38 for all platforms here, the configure/driver
# will ignore the --enable-t38 on older platforms
CONFIGURE_ARGS += --enable-fax --enable-t38
endif

ifeq ($(CONFIG_LTQ_VOICE_CPE_TAPI_CID),y)
CONFIGURE_ARGS += --enable-cid
else
CONFIGURE_ARGS += --disable-cid
endif

ifeq ($(CONFIG_LTQ_VOICE_CPE_TAPI_DECT),y)
CONFIGURE_ARGS += --enable-dect
else
CONFIGURE_ARGS += --disable-dect
endif

ifeq ($(CONFIG_LTQ_VOICE_CPE_TAPI_PCM),y)
CONFIGURE_ARGS += --enable-pcm
else
CONFIGURE_ARGS += --disable-pcm
endif

ifeq ($(CONFIG_LTQ_VOICE_CPE_TAPI_KPI),y)
CONFIGURE_ARGS += --enable-kpi
else
CONFIGURE_ARGS += --disable-kpi
endif

ifeq ($(CONFIG_LTQ_VOICE_CPE_TAPI_LT_GR909),y)
CONFIGURE_ARGS += --enable-lt --enable-calibration
else
CONFIGURE_ARGS += --disable-lt
endif

ifeq ($(CONFIG_LTQ_VOICE_CPE_TAPI_METERING),y)
CONFIGURE_ARGS += --enable-metering
else
CONFIGURE_ARGS += --disable-metering
endif

ifeq ($(CONFIG_LTQ_VOICE_CPE_TAPI_HDLC),y)
CONFIGURE_ARGS += --enable-hdlc
else
CONFIGURE_ARGS += --disable-hdlc
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

ifeq ($(CONFIG_LTQ_VOICE_CPE_TAPI_PHONE_DETECTION),y)
CONFIGURE_ARGS += --enable-phone-detection
else
CONFIGURE_ARGS += --disable-phone-detection
endif

ifeq ($(CONFIG_LTQ_VOICE_CPE_TAPI_CONT_MEASUREMENT),y)
CONFIGURE_ARGS += --enable-cont-measurement
else
CONFIGURE_ARGS += --disable-cont-measurement
endif

ifeq ($(CONFIG_LTQ_VOICE_CPE_TAPI_PMC),y)
CONFIGURE_ARGS += --enable-pmc
else
CONFIGURE_ARGS += --disable-pmc
endif

DRV_TAPI_INCLUDE:=$(STAGING_DIR)/usr/include/drv_tapi

CONFIGURE_ARGS += --with-kernel-incl="$(LINUX_DIR)/include"
CONFIGURE_ARGS += --with-tapi-incl="$(DRV_TAPI_INCLUDE)"
CONFIGURE_ARGS += --with-ifxos-incl=$(STAGING_DIR)/usr/include/ifxos

ifneq ($(CONFIG_LINUX_2_4),y)
CONFIGURE_ARGS += --enable-linux-26 --with-kernel-build="$(LINUX_DIR)" ARCH=$(LINUX_KARCH)
PKG_MODULE_EXTENSION=.ko
endif

#define Build/Configure
#	@echo "#################### Tapi SUBTARGET: $(SUBTARGET)"
#	$(call Build/Configure/Default)
#endef

define Build/Prepare
	$(call Build/Prepare/Default)
ifeq ($(CONFIG_ARCADYAN_TARGET_gr_VRV7006AW22-A-GR),y)
	$(CP) ./src.gvt/* $(PKG_BUILD_DIR)/.
else ifeq ($(CONFIG_ARCADYAN_TARGET_gr_VRV7006AW22-A-GRT),y)
	$(CP) ./src.gvt/* $(PKG_BUILD_DIR)/.
else ifeq ($(CONFIG_ARCADYAN_TARGET_ct_VRV7006AW22-A-CT),y)
	$(CP) ./src.gvt/* $(PKG_BUILD_DIR)/.
else ifeq ($(CONFIG_ARCADYAN_TARGET_ta_VRV7006AW22-A-TA),y)
	$(CP) ./src.gvt/* $(PKG_BUILD_DIR)/.
else
	$(CP) ./src/* $(PKG_BUILD_DIR)/.
endif
endef

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/include
	mkdir -p $(1)/usr/include/drv_vmmc
	$(CP) -v --dereference $(PKG_BUILD_DIR)/include/* $(1)/usr/include/drv_vmmc
	(cd $(1)/usr/include/drv_vmmc && ln -snf . include)
endef

define Build/UninstallDev
	rm -rf $(BUILD_DIR)/drv_vmmc
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
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/src/drv_vmmc$(PKG_MODULE_EXTENSION) $(1)/$(MODULES_SUBDIR)
	# For easy access also create a link in Lantiq bin directory
	$(INSTALL_DIR) $(1)/$(LANTIQ_BIN_DIR)
	ln -snf /$(MODULES_SUBDIR)/drv_vmmc$(PKG_MODULE_EXTENSION) $(1)/$(LANTIQ_BIN_DIR)/drv_vmmc.$(LINUX_KMOD_SUFFIX)
#
#	$(INSTALL_DIR) $(1)/$(IFX_DWLD_DIR)
#
#	Historic install directory is no longer supported
#	$(INSTALL_DIR) $(1)/$(IFX_BIN_DIR)
#	(cd $(1)/$(IFX_BIN_DIR); ln -snf ../../lantiq/bin/drv_vmmc.$(LINUX_KMOD_SUFFIX) .)
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
