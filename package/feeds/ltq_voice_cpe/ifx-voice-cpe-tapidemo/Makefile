#
# Copyright (C) 2008 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
# $Id: $

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=ifx-voice-cpe-tapidemo
PKG_VERSION:=5.1.1.52
PKG_MD5SUM:=dc178747bb5f89acad67e4fe065eae39
PKG_RELEASE:=1

# IMPORTANT - include the version-override here after PKG_VERSION definition
include $(INCLUDE_DIR)/package-version-override.mk
# IMPORTANT - ... and before using PKG_VERSION below

PKG_SOURCE:=tapidemo-$(PKG_VERSION).tar.gz
PKG_BUILD_DIR:=$(BUILD_DIR)/tapidemo-$(PKG_VERSION)

PKG_BUILD_DEPENDS:=ifx-voice-cpe-tapi \
                   PACKAGE_ifx-voice-cpe-vmmc:ifx-voice-cpe-vmmc \
                   PACKAGE_teridian-voice-daa-ter1x66:teridian-voice-daa-ter1x66 \
                   PACKAGE_ifx-voice-cpe-tapi-duslic_xt:ifx-voice-cpe-tapi-duslic_xt \
                   PACKAGE_ifx-dect:ifx-dect
DRV_TAPI_INCLUDE:=$(STAGING_DIR)/usr/include
PKG_SOURCE_URL:=http://localhost
PKG_START_SCRIPT:=bringup_tapidemo
IFX_BIN_DIR=/opt/ifx/bin
LANTIQ_BIN_DIR=/opt/lantiq/bin
LANTIQ_FIRMWARE_DIR=/lib/firmware

include $(INCLUDE_DIR)/package.mk

define Package/$(PKG_NAME)
  SECTION:=application
  CATEGORY:=Lantiq
  TITLE:=TAPIdemo application
  URL:=http://www.lantiq.com/
  MAINTAINER:=Lantiq Deutschland GmbH
  DEPENDS:=ifx-voice-cpe-tapi
  MENU:=1
endef

define Package/$(PKG_NAME)/description
	Voice Access mini-PBX Demo Application
endef


define Package/$(PKG_NAME)/config
$(call Package/$(PKG_NAME)/override_version)
$(call Package/$(PKG_NAME)/override_source_path)
    source "$(SOURCE)/config/Config.in"
endef

ifeq ($(CONFIG_LTQ_VOICE_CPE_TAPIDEMO_BOARD_EASY50712),y)
CONFIGURE_ARGS += --enable-boardname=EASY50712
endif
ifeq ($(CONFIG_LTQ_VOICE_CPE_TAPIDEMO_BOARD_EASY50712_V3),y)
CONFIGURE_ARGS += --enable-boardname=EASY50712_V3
endif
ifeq ($(CONFIG_LTQ_VOICE_CPE_TAPIDEMO_BOARD_EASY508xx),y)
CONFIGURE_ARGS += --enable-boardname=EASY508XX
endif
ifeq ($(CONFIG_LTQ_VOICE_CPE_TAPIDEMO_BOARD_XWAY_XRX300),y)
CONFIGURE_ARGS += --enable-boardname=XWAY_XRX300
endif
ifeq ($(CONFIG_LTQ_VOICE_CPE_TAPIDEMO_BOARD_EASY80910),y)
CONFIGURE_ARGS += --enable-boardname=EASY80910
endif
#to be extended...


ifeq ($(CONFIG_LTQ_VOICE_CPE_TAPIDEMO_QOS),y)
CONFIGURE_ARGS += --enable-qos
endif

ifeq ($(CONFIG_LTQ_VOICE_CPE_TAPIDEMO_FAX_T_38_FW),y)
CONFIGURE_ARGS += --enable-fax-t38
else
#--enable-fax-t38 enables also functionality that is enabled by --enable-fax-modem ,
#when --enable-fax-t38 was use then using --enable-fax-modem doesn't change configuration
ifeq ($(CONFIG_LTQ_VOICE_CPE_TAPIDEMO_FAX_MODEM),y)
CONFIGURE_ARGS += --enable-fax-modem
endif
endif

ifeq ($(CONFIG_LTQ_VOICE_CPE_TAPIDEMO_DXT_EXT),y)
CONFIGURE_ARGS += --enable-dxt-ext
CONFIGURE_ARGS += --with-dxt-incl=$(STAGING_DIR)/usr/include/drv_duslic_xt
endif

ifeq ($(CONFIG_LTQ_VOICE_CPE_TAPIDEMO_DECT),y)
CONFIGURE_ARGS += --enable-dect
CONFIGURE_ARGS += --with-dect-lib=$(STAGING_DIR)/usr/lib
CONFIGURE_ARGS += --with-dect-incl=$(STAGING_DIR)/usr/include
CONFIGURE_ARGS += --with-voip-common-incl=$(STAGING_DIR)/usr/include
CONFIGURE_ARGS += --with-voip-common-lib=$(STAGING_DIR)/usr/lib
endif

ifeq ($(CONFIG_LTQ_VOICE_CPE_TAPIDEMO_EVENT_LOGGER),y)
CONFIGURE_ARGS += --enable-el_debug --with-el-incl=$(STAGING_DIR)/usr/include/event_logger
endif

ifeq ($(CONFIG_PACKAGE_teridian-voice-daa-ter1x66),y)
CONFIGURE_ARGS += --with-teridian-incl=$(STAGING_DIR)/usr/include/drv_ter1x66
endif

CONFIGURE_ARGS +=  \
	--enable-trace --enable-fs  --with-kernel-incl=$(LINUX_DIR)/include \
	--with-tapi-incl=$(STAGING_DIR)/usr/include/drv_tapi \
	--with-vmmc-incl=$(STAGING_DIR)/usr/include/drv_vmmc --enable-warnings \
	--with-ifxos-incl=$(STAGING_DIR)/usr/include/ifxos \
	--with-ifxos-lib=$(STAGING_DIR)/usr/lib

ifneq ($(CONFIG_LTQ_VOICE_CPE_TAPIDEMO_ADDITIONAL_FLAGS),"")
CONFIGURE_ARGS += \
   $(subst ",, $(CONFIG_LTQ_VOICE_CPE_TAPIDEMO_ADDITIONAL_FLAGS))
endif

ifneq ($(CONFIG_LINUX_2_4),y)
CONFIGURE_ARGS += --enable-linux-26 ARCH=$(LINUX_KARCH)
endif

define Build/Clean
	@- [ -e $(PKG_BUILD_DIR)/ipkg ] && rm -Rf $(PKG_BUILD_DIR)/ipkg
	@- [ -e $(PKG_BUILD_DIR)/Makefile ] && $(MAKE) -C $(PKG_BUILD_DIR) clean
	$(call Build/Clean/Default)
endef


define Package/$(PKG_NAME)/install
   $(INSTALL_DIR) $(1)$(IFX_BIN_DIR)
	$(INSTALL_DIR) $(1)$(LANTIQ_BIN_DIR)
	$(INSTALL_DIR) $(1)/$(LANTIQ_FIRMWARE_DIR)
	$(INSTALL_DIR) $(1)/usr/sbin/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/src/tapidemo $(1)$(LANTIQ_BIN_DIR)
	$(INSTALL_BIN) ./files/$(PKG_START_SCRIPT) $(1)/usr/sbin/
	(cd $(1)/usr/sbin; ln -snf ../../opt/lantiq/bin/tapidemo .)
	(cd $(1)$(IFX_BIN_DIR); ln -snf ../../lantiq/bin/tapidemo .)
	$(INSTALL_BIN) ./files/ter1x66_bbd.bin $(1)$(LANTIQ_FIRMWARE_DIR)
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
