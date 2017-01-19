#
# Copyright (C) 2009 OpenWrt.org
# Copyright (C) 2009 lantiq.com
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=gpon-omci-onu
PKG_VERSION:=4.5.0
PKG_BASE_NAME:=gpon_omci_onu
PKG_SOURCE:=$(PKG_BASE_NAME)-$(PKG_VERSION).tar.gz
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_BASE_NAME)-$(PKG_VERSION)
PKG_RELEASE:=2
PKG_SOURCE_URL:=http://localhost
PKG_BUILD_DEPENDS:=GPON_OMCI_ONU_UCI:libuci GPON_OMCI_ONU_DMALLOC:libdmalloc GPON_OMCI_ONU_CLI:lib-cli
PKG_BIN_DIR=opt/lantiq/bin

include $(INCLUDE_DIR)/package-version-override.mk
include $(INCLUDE_DIR)/package.mk

ifeq ($(DUMP),)
	STAMP_CONFIGURED:=$(strip $(STAMP_CONFIGURED))_$(shell $(SH_FUNC) grep '^CONFIG_GPON_' $(TOPDIR)/.config | md5s)
endif

$(STAMP_BUILT): $(STAMP_CONFIGURED) $(STAGING_DIR)/usr/lib/libomciapi.a

define Package/$(PKG_NAME)
  SECTION:=driver
  CATEGORY:=Lantiq
  TITLE:= OMCI stack for GPON
  URL:=http://www.lantiq.com/
  MAINTAINER:=Lantiq
  DEPENDS:=+ifxos +gpon-omci-api #+GPON_OMCI_ONU_REMOTE_ONLY:gpon-onu-firmware
  MENU:=1
  SUBMENU:=GPON
endef

define Package/$(PKG_NAME)/description
   The OMCI stack for GPON Optical Network Unit device
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
	--enable-ifxos-include="-I$(STAGING_DIR)/usr/include/ifxos" \
	--enable-ifxos-library="-L$(STAGING_DIR)/usr/lib" \
	--enable-omci-api-include="-I$(STAGING_DIR)/usr/include/omci_api" \
	--enable-omci-api-library="-L$(STAGING_DIR)/usr/lib"

ifeq ($(CONFIG_GPON_OMCI_ONU_CLI),y)
CONFIGURE_ARGS += \
	--enable-cli \
	--enable-cli-include="-I$(STAGING_DIR)/usr/include/cli" \
	--enable-cli-library="-L$(STAGING_DIR)/usr/lib"
endif

ifeq ($(CONFIG_GPON_OMCI_ONU_CLI_PIPE),y)
CONFIGURE_ARGS += --enable-cli-pipe
endif

ifeq ($(CONFIG_GPON_OMCI_ONU_DEBUG_PRINTS),y)
CONFIGURE_ARGS += --enable-debug-prints
endif

ifeq ($(CONFIG_GPON_OMCI_ONU_FORMATTED_OMCI_DUMP),y)
CONFIGURE_ARGS += --enable-formatted-omci-dump
endif

ifeq ($(CONFIG_GPON_OMCI_ONU_SELF_DESCRIPTION),y)
CONFIGURE_ARGS += --enable-omci-self-desc
endif

ifeq ($(CONFIG_GPON_OMCI_ONU_PM),y)
CONFIGURE_ARGS += --enable-pm
endif

ifeq ($(CONFIG_GPON_OMCI_ONU_VOIP),y)
CONFIGURE_ARGS += \
	--enable-voip \
	--enable-pjsip-library="-L$(STAGING_DIR)/usr/pjsip-ltq-tapi/lib"
endif

ifeq ($(CONFIG_GPON_OMCI_ONU_UCI),y)
CONFIGURE_ARGS += --enable-omci-uci
endif

ifneq ($(CONFIG_GPON_OMCI_ONU_UCI_MAP_LIB),y)
CONFIGURE_ARGS += --enable-libucimap=no
endif

ifeq ($(CONFIG_GPON_OMCI_ONU_DMALLOC),y)
CONFIGURE_ARGS += --enable-omci-dmalloc
endif

ifeq ($(CONFIG_GPON_OMCI_ONU_REMOTE),y)
CONFIGURE_ARGS += \
	--enable-remote-onu \
	--enable-omci-extra-dep="$(STAGING_DIR)/usr/lib/libomciapi.a $(STAGING_DIR)/usr/lib/libgponrpc.a"
else
CONFIGURE_ARGS += \
	--disable-remote-onu \
	--enable-omci-extra-dep="$(STAGING_DIR)/usr/lib/libomciapi.a"
endif

define Build/Clean
	@- rm -Rf $(PKG_BUILD_DIR)/ipkg*
	@- [ -e $(PKG_BUILD_DIR)/Makefile ] && $(MAKE) -C $(PKG_BUILD_DIR) clean
	$(call Build/Clean/Default)
endef

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/$(PKG_BIN_DIR)
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/src/omcid $(1)/$(PKG_BIN_DIR)
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/tools/omci_simulate $(1)/$(PKG_BIN_DIR)
	$(INSTALL_BIN) ./files/omci_hook.sh $(1)/$(PKG_BIN_DIR)
	$(INSTALL_BIN) ./files/omci_simulate.sh $(1)/$(PKG_BIN_DIR)
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/scripts/omci_pipe.sh $(1)/$(PKG_BIN_DIR)/
	$(INSTALL_BIN) ./files/host2lan.sh $(1)/$(PKG_BIN_DIR)
	$(INSTALL_BIN) ./files/local_bridge.sh $(1)/$(PKG_BIN_DIR)
	$(INSTALL_DIR) $(1)/$(PKG_BIN_DIR)/config
	$(INSTALL_BIN) ./files/onu_setup_no_vlan.txt $(1)/$(PKG_BIN_DIR)/config
	$(INSTALL_BIN) ./files/onu_setup_vlan.txt $(1)/$(PKG_BIN_DIR)/config
	$(INSTALL_BIN) ./files/local_bridge.txt $(1)/$(PKG_BIN_DIR)/config

	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/omcid.sh $(1)/etc/init.d/

	$(INSTALL_DIR) $(1)/etc/mibs
	$(INSTALL_DATA) ./files/mibs/*.ini $(1)/etc/mibs/

ifeq ($(GPON_OMCI_ONU_FALCON_SW_IMAGE_SUPPORT),y)
	$(KERNEL_BUILD_DIR)/u-boot-1.1.5/tools/mkimage \
		-T script -C none \
		-n "SW Image selection HUSH script" \
		-d $(PKG_BUILD_DIR)/scripts/bootscript.hush \
		$(BIN_DIR)/bootscript.img
endif

ifeq ($(CONFIG_GPON_OMCI_ONU_UDP_SIMULATION),y)
	$(SED) 's/$$$$O/-o$(strip $(subst ",,$(CONFIG_GPON_OMCI_ONU_UDP_SIMULATION_OLT)))/' $(1)/etc/init.d/omcid.sh
else
	$(SED) 's/$$$$O//' $(1)/etc/init.d/omcid.sh
endif

	$(SED) 's/$$$$M/$(strip $(subst ",,$(CONFIG_GPON_OMCI_ONU_DEFAULT_MIB)))/' $(1)/etc/init.d/omcid.sh

endef

$(eval $(call BuildPackage,$(PKG_NAME)))
