#
# Copyright (C) 2008 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=ifx-voice-cpe-wineasy
PKG_VERSION:=1.5.2.16-1
PKG_MD5SUM:=3d06f2cc3020b36d3e354925b359f391
PKG_RELEASE:=2

# IMPORTANT - include the version-override here after PKG_VERSION definition
include $(INCLUDE_DIR)/package-version-override.mk
# IMPORTANT - ... and before using PKG_VERSION below

PKG_SOURCE:=tooling-$(PKG_VERSION).tar.gz
PKG_BUILD_DIR:=$(BUILD_DIR)/tooling-$(PKG_VERSION)
PKG_BUILD_DEPENDS:=ifx-vos \
                   PACKAGE_ifx-voice-cpe-tapi:ifx-voice-cpe-tapi \
                   PACKAGE_ifx-voice-cpe-vmmc:ifx-voice-cpe-vmmc \
                   PACKAGE_ifx-voice-cpe-tapi-duslic_xt:ifx-voice-cpe-tapi-duslic_xt \
                   PACKAGE_ifx-voice-cpe-tapi:ifx-voice-cpe-lib-nlt \
                   PACKAGE_ltq-tapi:ltq-tapi \
                   PACKAGE_ltq-tapi:ifx-voice-cpe-lib-nlt
DRV_TAPI_INCLUDE:=$(STAGING_DIR)/usr/include
PKG_SOURCE_URL:=http://localhost
IFX_BIN_DIR=/opt/ifx/bin
LANTIQ_BIN_DIR=/opt/lantiq/bin


include $(INCLUDE_DIR)/package.mk

define Package/$(PKG_NAME)
  SECTION:=application
  CATEGORY:=Lantiq
  TITLE:=WinEasy Tooling
  URL:=http://www.lantiq.com/
  MAINTAINER:=Lantiq Deutschland GmbH
  DEPENDS:=ifx-voice-cpe-tapi +ifx-vos +ifx-voice-cpe-lib-nlt
  MENU:=1
endef

define Package/$(PKG_NAME)/description
	WinEasy voice access tooling
endef


define Package/$(PKG_NAME)/config
$(call Package/$(PKG_NAME)/override_version)
$(call Package/$(PKG_NAME)/override_source_path)
    source "$(SOURCE)/config/Config.in"
endef

ifeq ($(CONFIG_LTQ_VOICE_CPE_WINEASY_BOARD_EASY50712),y)
CONFIGURE_ARGS += --with-system=EASY50712
endif
ifeq ($(CONFIG_LTQ_VOICE_CPE_WINEASY_BOARD_EASY50712_V3),y)
CONFIGURE_ARGS += --with-system=EASY50712
endif
ifeq ($(CONFIG_LTQ_VOICE_CPE_WINEASY_BOARD_EASY508xx),y)
CONFIGURE_ARGS += --with-system=EASY508XX
endif
ifeq ($(CONFIG_LTQ_VOICE_CPE_WINEASY_BOARD_XWAY_XRX300),y)
CONFIGURE_ARGS += --with-system=XWAY_XRX300
endif
ifeq ($(CONFIG_LTQ_VOICE_CPE_WINEASY_BOARD_EASY80910),y)
CONFIGURE_ARGS += --with-system=EASY80910
endif
#to be extended...

ifeq ($(CONFIG_LTQ_VOICE_CPE_WINEASY_DXT_EXT),y)
CONFIGURE_ARGS += --enable-dxt-ext
endif

ifeq ($(CONFIG_LTQ_VOICE_CPE_WINEASY_IFXOS_SUPPORT),y)
CONFIGURE_ARGS += --enable-ifxos \
	--with-ifxos-incl="$(STAGING_DIR)/usr/include/ifxos" \
  --with-ifxos-lib=$(STAGING_DIR)/usr/lib
endif

ifneq ($(CONFIG_LTQ_VOICE_CPE_WINEASY_ADDITIONAL_FLAGS),"")
CONFIGURE_ARGS += \
   $(subst ",, $(CONFIG_LTQ_VOICE_CPE_WINEASY_ADDITIONAL_FLAGS))
endif

CONFIGURE_ARGS += --enable-trace --enable-fs \
   --with-kernel-incl=$(LINUX_DIR)/include \
   --with-drv-incl=$(STAGING_DIR)/usr/include/ \
   --with-vos-incl=$(STAGING_DIR)/usr/include/ \
   --with-vos-lib=$(STAGING_DIR)/usr/lib \
   --with-lib-tapi-nlt-incl=$(STAGING_DIR)/usr/include/lib_tapi_nlt_cpe \
   --with-lib-tapi-nlt-lib=$(STAGING_DIR)/usr/lib

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
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/src/tooling $(1)$(LANTIQ_BIN_DIR)
	$(INSTALL_DIR) $(1)/usr/sbin
	(cd $(1)/usr/sbin; ln -snf ../../opt/lantiq/bin/tooling .)
	(cd $(1)$(IFX_BIN_DIR); ln -snf ../../lantiq/bin/tooling .)
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
