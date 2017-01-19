#
# Copyright (C) 2012 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=gpon-omci-api
PKG_VERSION:=4.5.0
PKG_BASE_NAME:=gpon_omci_api
PKG_SOURCE:=$(PKG_BASE_NAME)-$(PKG_VERSION).tar.gz
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_BASE_NAME)-$(PKG_VERSION)
PKG_RELEASE:=2
PKG_SOURCE_URL:=http://localhost
PKG_BUILD_DEPENDS:=GPON_OMCI_API_VOIP:pjsip GPON_OMCI_API_REMOTE:gpon-rpc
PKG_BIN_DIR=opt/lantiq/bin

include $(INCLUDE_DIR)/package-version-override.mk
include $(INCLUDE_DIR)/package.mk

ifeq ($(DUMP),)
	STAMP_CONFIGURED:=$(strip $(STAMP_CONFIGURED))_$(shell $(SH_FUNC) grep '^CONFIG_GPON_' $(TOPDIR)/.config | md5s)
endif

define Package/$(PKG_NAME)
  SECTION:=driver
  CATEGORY:=Lantiq
  TITLE:= OMCI API for GPON
  URL:=http://www.lantiq.com/
  MAINTAINER:=Lantiq
  DEPENDS:=+kmod-ifxos +gpon-onu-drv +gpon-optic-drv +GPON_OMCI_API_REMOTE:gpon-rpc
  MENU:=1
  SUBMENU:=GPON
endef

define Package/$(PKG_NAME)/description
   The OMCI API for GPON Optical Network Unit device
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
	--enable-ifxos-include="-I$(STAGING_DIR)/usr/include/ifxos"

CONFIGURE_ARGS += \
	--enable-driver-include="-I$(STAGING_DIR)/usr/include/onu_drv\
				-I$(STAGING_DIR)/usr/include/optic_drv"

ifeq ($(CONFIG_GPON_OMCI_API_DEBUG_PRINTS),y)
CONFIGURE_ARGS += --enable-debug-prints
endif

ifeq ($(CONFIG_GPON_OMCI_ONU_UDP_SIMULATION),y)
CONFIGURE_ARGS += --enable-omci-udp-simulation
endif

ifeq ($(CONFIG_GPON_OMCI_ONU_FALCON_SW_IMAGE_SUPPORT),y)
CONFIGURE_ARGS += --enable-falcon-sw-image-support
endif

ifeq ($(CONFIG_GPON_OMCI_API_DMALLOC),y)
CONFIGURE_ARGS += --enable-omci-dmalloc
endif

ifeq ($(CONFIG_GPON_OMCI_API_VOIP),y)
CONFIGURE_ARGS += \
	--enable-voip \
	--enable-pjsip-include="-I$(STAGING_DIR)/usr/pjsip-ltq-tapi/include" \
	--enable-tapi-include="-I$(STAGING_DIR)/usr/include/drv_tapi" \
	--enable-vmmc-include="-I$(STAGING_DIR)/usr/include/drv_vmmc"
endif

CONFIGURE_ARGS += \
	$(call autoconf_bool,CONFIG_GPON_OMCI_API_MCC,mcc) \
	$(call autoconf_bool,CONFIG_GPON_OMCI_API_REMOTE,remote-onu)

ifeq ($(CONFIG_GPON_ONU_DEVICE_PSB98010),y)
CONFIGURE_ARGS += --enable-device=PSB98010
endif

ifeq ($(CONFIG_GPON_ONU_DEVICE_PSB98020),y)
CONFIGURE_ARGS += --enable-device=PSB98020
endif

ifeq ($(CONFIG_GPON_ONU_DEVICE_PSB98030),y)
CONFIGURE_ARGS += --enable-device=PSB98030
endif

define Build/Clean
	@- [ -e $(PKG_BUILD_DIR)/Makefile ] && $(MAKE) -C $(PKG_BUILD_DIR) clean
	$(call Build/Clean/Default)
endef

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/include/omci_api
	$(CP) $(PKG_BUILD_DIR)/include/omci_api*.h $(1)/usr/include/omci_api
	$(INSTALL_DIR) $(1)/usr/include/omci_api/me
	$(CP) $(PKG_BUILD_DIR)/include/me/omci_api*.h $(1)/usr/include/omci_api/me
	$(INSTALL_DIR) $(1)/usr/include/omci_api/mcc
	$(CP) $(PKG_BUILD_DIR)/include/mcc/omci_api*.h $(1)/usr/include/omci_api/mcc
	$(INSTALL_DIR) $(1)/usr/include/omci_api/voip
	$(CP) $(PKG_BUILD_DIR)/include/voip/omci_api*.h $(1)/usr/include/omci_api/voip
	$(INSTALL_DIR) $(1)/usr/lib
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/src/libomciapi.a $(1)/usr/lib
endef

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/$(PKG_BIN_DIR)
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/tools/omci_usock_server $(1)/$(PKG_BIN_DIR)
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/usock.sh $(1)/etc/init.d/
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
