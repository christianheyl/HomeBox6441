#
# Copyright (C) 2009 OpenWrt.org
# Copyright (C) 2009 lantiq.com
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=gpon-omci-onu-ocs
PKG_VERSION:=0.0.9
PKG_BASE_NAME:=gpon_omci_onu_ocs
PKG_SOURCE:=$(PKG_BASE_NAME)-$(PKG_VERSION).tar.gz
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_BASE_NAME)-$(PKG_VERSION)
PKG_RELEASE:=1
PKG_SOURCE_URL:=http://localhost
PKG_BUILD_DEPENDS:=gpon-omci-api gpon-omci-onu-ocs-lib
PKG_BIN_DIR=opt/lantiq/bin

include $(INCLUDE_DIR)/package-version-override.mk
include $(INCLUDE_DIR)/package.mk

define Package/$(PKG_NAME)
  SECTION:=application
  CATEGORY:=Lantiq
  TITLE:= GPON Optical Network Unit (ONU) OCS-OMCI stack Daemon
  URL:=http://www.lantiq.com/
  MAINTAINER:=Lantiq
  DEPENDS:=+ifxos +gpon-omci-api +gpon-omci-onu-ocs-lib
  MENU:=1
  SUBMENU:=GPON
endef

define Package/$(PKG_NAME)/description
   The Daemon and Portals implementation for OCS-OMCI Stack
endef

define Package/$(PKG_NAME)/config
	menu "configuration"
	depends on PACKAGE_$(PKG_NAME)
		source "$(SOURCE)/config/Config.in"
	endmenu
   $(call Package/$(PKG_NAME)/override_version)
   $(call Package/$(PKG_NAME)/override_source_path)
endef

LIB_OMCI_ONU_OCS_CFLAGS := -DOG_ONT

ifeq ($(CONFIG_GPON_OMCI_ONU_OCS_LITTLE_ENDIAN),y)
LIB_OMCI_ONU_OCS_CFLAGS += \
	-DSVC_LITTLE_ENDIAN
else
LIB_OMCI_ONU_OCS_CFLAGS += \
	-DSVC_BIG_ENDIAN_RESTRICTED
endif

ifeq ($(CONFIG_GPON_OMCI_ONU_OCS_TRACE),y)
LIB_OMCI_ONU_OCS_CFLAGS += \
	-DOG_TRACE
endif

ifeq ($(CONFIG_GPON_OMCI_ONU_OCS_UNKNOWNCLASS_IGNORE),y)
LIB_OMCI_ONU_OCS_CFLAGS += \
	-DOG_UNKNOWNCLASS_IGNORE
endif

ifeq ($(CONFIG_GPON_OMCI_ONU_OCS_INTEGRATED_ONT),y)
LIB_OMCI_ONU_OCS_CFLAGS += \
	-DOG_ANI_FLAG_0 \
	-DOG_INTEGRATED_ONT
endif

CONFIGURE_ARGS += \
	--enable-ifxos-include="-I$(STAGING_DIR)/usr/include/ifxos" \
	--enable-ifxos-library="-L$(STAGING_DIR)/usr/lib" \
	--enable-gpon-al-include="-I$(STAGING_DIR)/usr/include/omci_api" \
	--enable-gpon-al-library="-L$(STAGING_DIR)/usr/lib" \
	--enable-ocsomci-include="-I$(STAGING_DIR)/usr/include/omci_onu_ocs" \
	--enable-ocsomci-library="-L$(STAGING_DIR)/usr/lib/" \
	--enable-ocsomci-cflags="$(LIB_OMCI_ONU_OCS_CFLAGS)"

ifeq ($(CONFIG_GPON_OMCI_ONU_CLI),y)
CONFIGURE_ARGS += --enable-cli
endif

ifeq ($(CONFIG_GPON_OMCI_ONU_CLI_PIPE),y)
CONFIGURE_ARGS += --enable-cli-pipe
endif

ifeq ($(CONFIG_GPON_OMCI_ONU_SELF_DESCRIPTION),y)
CONFIGURE_ARGS += --enable-omci-self-desc
endif

ifeq ($(CONFIG_GPON_OMCI_ONU_UDP_SIMULATION),y)
CONFIGURE_ARGS += --enable-omci-udp-simulation
# TODO add configurable -o option to the load_omci_daemon.sh
endif

define Build/Clean
	@- rm -Rf $(PKG_BUILD_DIR)/ipkg*
	@- [ -e $(PKG_BUILD_DIR)/Makefile ] && $(MAKE) -C $(PKG_BUILD_DIR) clean
	$(call Build/Clean/Default)
endef

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/$(PKG_BIN_DIR)
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/src/onu_omci_daemon $(1)/$(PKG_BIN_DIR)

	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/load_omci_daemon_ocs.sh $(1)/etc/init.d/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/scripts/onu_omci_pipe.sh $(1)/$(PKG_BIN_DIR)/onu_omci_ocs_pipe.sh

ifeq ($(GPON_OMCI_ONU_FALCON_SW_IMAGE_SUPPORT),y)
	$(KERNEL_BUILD_DIR)/u-boot-1.1.5/tools/mkimage \
		-T script -C none \
		-n "SW Image selection HUSH script" \
		-d $(PKG_BUILD_DIR)/scripts/bootscript.hush \
		$(BIN_DIR)/bootscript.img
endif

endef

$(eval $(call BuildPackage,$(PKG_NAME)))
