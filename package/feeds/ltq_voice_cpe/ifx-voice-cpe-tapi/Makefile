#
# Copyright (C) 2008 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=ifx-voice-cpe-tapi
PKG_VERSION:=4.10.4.2
PKG_MD5SUM:=deb7be0f483fde1126af7ed3e31f903e
PKG_RELEASE:=1

# IMPORTANT - include the version-override here after PKG_VERSION definition
include $(INCLUDE_DIR)/package-version-override.mk
# IMPORTANT - ... and before using PKG_VERSION below

PKG_SOURCE:=drv_tapi-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=http://localhost
PKG_BUILD_DIR:=$(KERNEL_BUILD_DIR)/drv_tapi-$(PKG_VERSION)
PKG_START_SCRIPT:=ltq_load_voice_tapi.sh
PKG_BUILD_DEPENDS:=ifxos \
                   PACKAGE_ltq-evtlog:ltq-evtlog
#IFX_BIN_DIR=opt/ifx/bin
LANTIQ_BIN_DIR=opt/lantiq/bin

include $(INCLUDE_DIR)/package.mk

define Package/$(PKG_NAME)
  SECTION:=driver
  CATEGORY:=Lantiq
  TITLE:=TAPI subsystem
  URL:=http://www.lantiq.com/
  MAINTAINER:=Lantiq Deutschland GmbH
  MENU:=1
  DEPENDS:=+kmod-ifxos @TARGET_ltqcpe_ar9||TARGET_ltqcpe_vr9||TARGET_ltqcpe_ar10||TARGET_ltqcpe_platform_ar9||TARGET_ltqcpe_platform_vr9||TARGET_ltqcpe_platform_danube||TARGET_ltqcpe_platform_ar10
endef

define Package/$(PKG_NAME)/description
	Voice Subsystem Telephony API High Level Driver
endef

define Package/$(PKG_NAME)/config
$(call Package/$(PKG_NAME)/override_version)
$(call Package/$(PKG_NAME)/override_source_path)
	source "$(SOURCE)/config/Config.in"
endef

# Set TAPI API interface version
CONFIGURE_ARGS += --enable-tapi3


# Add user specified CFLAGS and configure options

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



ifneq ($(CONFIG_LTQ_VOICE_TAPI_USER_FLAGS),"")
TAPI_USER_CFLAGS=$(strip $(subst ",, $(CONFIG_LTQ_VOICE_TAPI_USER_FLAGS)))
CONFIGURE_ARGS += --with-cflags="$(TAPI_USER_CFLAGS)"
endif

ifneq ($(CONFIG_LTQ_VOICE_TAPI_USER_OPTIONS),"")
CONFIGURE_ARGS += $(subst ",, $(CONFIG_LTQ_VOICE_TAPI_USER_OPTIONS))
endif


# Configuration Options

ifeq ($(CONFIG_LTQ_VOICE_CPE_TAPI_FAX),y)
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
CONFIGURE_ARGS += --enable-kpi --enable-kpi-tasklet
else
CONFIGURE_ARGS += --disable-kpi
endif

ifeq ($(CONFIG_LTQ_VOICE_CPE_TAPI_QOS),y)
CONFIGURE_ARGS += --enable-qos
else
CONFIGURE_ARGS += --disable-qos
endif

ifeq ($(CONFIG_LTQ_VOICE_CPE_TAPI_LT_GR909),y)
CONFIGURE_ARGS += --enable-nlt
else
CONFIGURE_ARGS += --disable-nlt
endif

ifeq ($(CONFIG_LTQ_VOICE_CPE_TAPI_STATISTICS),y)
CONFIGURE_ARGS += --enable-statistics
else
CONFIGURE_ARGS += --disable-statistics
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

ifeq ($(CONFIG_LTQ_VOICE_CPE_TAPI_HOTPLUG),y)
CONFIGURE_ARGS += --enable-hotplug
else
CONFIGURE_ARGS += --disable-hotplug
endif

ifeq ($(CONFIG_LTQ_VOICE_CPE_TAPI_EVENT_LOGGER),y)
CONFIGURE_ARGS += --enable-el-debug --with-el-incl=$(STAGING_DIR)/usr/include/event_logger
else
CONFIGURE_ARGS += --disable-el-debug
endif

# Fixed options for TAPI V3 API
CONFIGURE_ARGS += --enable-voice \
                  --enable-packet \
                  --enable-hsm \
                  --enable-dtmf \
                  --disable-peakd \
                  --disable-mfr2 \
                  --disable-event-counter \
                  --disable-srtp \

CONFIGURE_ARGS += --with-kernel-incl="$(LINUX_DIR)/include"
CONFIGURE_ARGS += --with-ifxos-incl=$(STAGING_DIR)/usr/include/ifxos
CONFIGURE_ARGS += --with-ifxos-lib=$(STAGING_DIR)/usr/lib

ifneq ($(CONFIG_LINUX_2_4),y)
CONFIGURE_ARGS += --enable-linux-26 --with-kernel-build="$(LINUX_DIR)" ARCH=$(LINUX_KARCH)
PKG_MODULE_EXTENSION=.ko
endif

#define Build/Configure
#	#@echo "#################### Tapi SUBTARGET: $(SUBTARGET)"
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
	mkdir -p $(1)/usr/include/drv_tapi
	$(CP) --dereference $(PKG_BUILD_DIR)/include/* $(1)/usr/include/drv_tapi
	(cd $(1)/usr/include/drv_tapi && ln -s ../ifxos/ifx_types.h .)
endef

define Build/UninstallDev
	rm -rf $(BUILD_DIR)/drv_tapi
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
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/src/drv_tapi$(PKG_MODULE_EXTENSION) $(1)/$(MODULES_SUBDIR)
	# For easy access also create a link in Lantiq bin directory
	$(INSTALL_DIR) $(1)/$(LANTIQ_BIN_DIR)
	ln -snf /$(MODULES_SUBDIR)/drv_tapi$(PKG_MODULE_EXTENSION) $(1)/$(LANTIQ_BIN_DIR)/drv_tapi.$(LINUX_KMOD_SUFFIX)
#	Historic install directory is no longer supported
#	$(INSTALL_DIR) $(1)/$(IFX_BIN_DIR)
#	(cd $(1)/$(IFX_BIN_DIR); ln -snf ../../lantiq/bin/drv_tapi.$(LINUX_KMOD_SUFFIX) .)
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
