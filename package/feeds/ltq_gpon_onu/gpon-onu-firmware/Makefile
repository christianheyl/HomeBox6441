#
# Copyright (C) 2009 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
# $Id: $

include $(TOPDIR)/rules.mk

PKG_NAME:=gpon-onu-firmware
PKG_VERSION:=0.0.2
PKG_BASE_NAME:=gpon_onu_firmware
PKG_SOURCE:=$(PKG_BASE_NAME)-$(PKG_VERSION).tar.gz
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_BASE_NAME)-$(PKG_VERSION)
PKG_RELEASE:=1
PKG_SOURCE_URL:=http://localhost
PKG_BUILD_DEPENDS:=gpon-pe-firmware

ONU_LIB_PKG_NAME:=gpon-onu-lib
ONU_LIB_PKG_VERSION:=4.0.1
ONU_LIB_PKG_BASE_NAME:=gpon_onu_drv
ONU_LIB_PKG_SOURCE:=$(ONU_LIB_PKG_BASE_NAME)-$(ONU_LIB_PKG_VERSION).tar.gz
ONU_LIB_PKG_MD5SUM:=402869fb7a1358450c88eb45498a8969

OPTIC_LIB_PKG_NAME:=gpon-optic-lib
OPTIC_LIB_PKG_VERSION:=4.0.0
OPTIC_LIB_PKG_BASE_NAME:=gpon_optic_drv
OPTIC_LIB_PKG_SOURCE:=$(OPTIC_LIB_PKG_BASE_NAME)-$(OPTIC_LIB_PKG_VERSION).tar.gz
OPTIC_LIB_PKG_MD5SUM:=7c7d39d04447dafd635903d9180b4325

DRV_OPTIC_SOURCE_DIR:=$(PKG_BUILD_DIR)/$(OPTIC_LIB_PKG_NAME)
DRV_ONU_SOURCE_DIR:=$(PKG_BUILD_DIR)/$(ONU_LIB_PKG_NAME)
ONU_BINARIES_SOURCE_DIR:=$(STAGING_DIR)/usr/bin/pe_firmware

include $(INCLUDE_DIR)/package-version-override.mk
include $(INCLUDE_DIR)/package.mk

ifeq ($(DUMP),)
	STAMP_CONFIGURED:=$(strip $(STAMP_CONFIGURED))_$(shell $(SH_FUNC) grep '^CONFIG_GPON_' $(TOPDIR)/.config | md5s)
endif

define Package/$(PKG_NAME)/Default
  SECTION:=firmware
  CATEGORY:=Lantiq
  TITLE:= ONU Firmware for GPON
  URL:=http://www.lantiq.com/
  MAINTAINER:=Lantiq
  DEPENDS:=
  MENU:=1
  SUBMENU:=GPON
endef

define Package/$(PKG_NAME)
$(call Package/$(PKG_NAME)/Default)
  TITLE:=ONU Firmware for GPON
  DEPENDS:=+gpon-onu-lib +gpon-optic-lib +gpon-pe-firmware +tftpd-hpa
endef

define Package/$(PKG_NAME)/description
   Firmware for GPON Optical Network Unit device
endef

define Package/$(PKG_NAME)/config
	menu "configuration"
	depends on PACKAGE_$(PKG_NAME)
		source "$(SOURCE)/config/Config.in"
	endmenu
   $(call Package/$(PKG_NAME)/override_version)
   $(call Package/$(PKG_NAME)/override_source_path)
endef

define Package/$(ONU_LIB_PKG_NAME)
$(call Package/$(PKG_NAME)/Default)
  TITLE:=ONU library for GPON firmware
endef

define Package/$(ONU_LIB_PKG_NAME)/description
   ONU library for GPON Optical Network Unit device
endef

define Package/$(ONU_LIB_PKG_NAME)/config
	menu "custom source directory"
	depends on PACKAGE_$(ONU_LIB_PKG_NAME)
		source "$(SOURCE)/config/Config.onu"
	endmenu
endef

define Package/$(OPTIC_LIB_PKG_NAME)
$(call Package/$(PKG_NAME)/Default)
  TITLE:=Optic library for GPON firmware
endef

define Package/$(OPTIC_LIB_PKG_NAME)/description
   Optic library for GPON Optical Network Unit device
endef

define Package/$(OPTIC_LIB_PKG_NAME)/config
	menu "custom source directory"
	depends on PACKAGE_$(OPTIC_LIB_PKG_NAME)
		source "$(SOURCE)/config/Config.optic"
	endmenu
endef



define Download/files
ifneq ($(5),y)
 define Download/$(1)
  FILE:=$(2)-$(3).tar.gz
  URL:=http://localhost/
  MD5SUM:=$(4)
 endef
 define Prepare/$(1)
  mkdir -p $(BUILD_DIR)/$(1) && \
  tar xzf "$(DL_DIR)/$(2)-$(3).tar.gz" -C "$(BUILD_DIR)/$(1)" && \
  $(LN) $(BUILD_DIR)/$(1)/$(2)-$(3) $(PKG_BUILD_DIR)/$(1)
 endef
 $$(eval $$(call Download,$(1)))
else
 define Prepare/$(1)
  mkdir -p $(BUILD_DIR)/$(1) && \
  cp -r $(6)/* "$(BUILD_DIR)/$(1)" && \
  $(LN) $(BUILD_DIR)/$(1) $(PKG_BUILD_DIR)/$(1)
 endef
endif
endef


ifneq ($(CONFIG_PACKAGE_gpon-onu-lib),)
 $(eval $(call Download/files,$(ONU_LIB_PKG_NAME),$(ONU_LIB_PKG_BASE_NAME),$(ONU_LIB_PKG_VERSION),$(ONU_LIB_PKG_MD5SUM),$(CONFIG_GPON_ONU_LIB_USE_CUSTOM_SOURCE_DIR),$(CONFIG_GPON_ONU_LIB_CUSTOM_SOURCE_DIR)))
endif

ifneq ($(CONFIG_PACKAGE_gpon-optic-lib),)
 $(eval $(call Download/files,$(OPTIC_LIB_PKG_NAME),$(OPTIC_LIB_PKG_BASE_NAME),$(OPTIC_LIB_PKG_VERSION),$(OPTIC_LIB_PKG_MD5SUM),$(CONFIG_GPON_OPTIC_LIB_USE_CUSTOM_SOURCE_DIR),$(CONFIG_GPON_OPTIC_LIB_CUSTOM_SOURCE_DIR)))
endif

COMMON_CFG_OPT= \
	--disable-linux \
	--without-cli \
	--without-cli-dump \
	--without-procfs \
	--build=$(GNU_HOST_NAME) \
	--host=$(GNU_TARGET_NAME) \
	--enable-maintainer-mode \
	--enable-ifxos-include=-I$(PKG_BUILD_DIR)/src/include

ONU_FIRMWARE_CFG_OPT= \
	$(call autoconf_bool,CONFIG_GPON_ONU_FIRMWARE_UBOOT,uboot) \
	--enable-onu-include="-I$(DRV_ONU_SOURCE_DIR)/src -I$(DRV_ONU_SOURCE_DIR)/src/include" \
	--enable-onu-library=-L$(DRV_ONU_SOURCE_DIR)/src \
	--enable-optic-include="-I$(DRV_OPTIC_SOURCE_DIR)/src -I$(DRV_OPTIC_SOURCE_DIR)/src/include " \
	--enable-optic-library=-L$(DRV_OPTIC_SOURCE_DIR)/src \
	--enable-pe-firmware="$(STAGING_DIR)/usr/bin/pe_firmware" \
	--with-ip-addr=$(CONFIG_GPON_ONU_FIRMWARE_IP_ADDR) \
	--with-ip-netmask=$(CONFIG_GPON_ONU_FIRMWARE_IP_NETMASK)

ONU_LIB_CFG_OPT:= \
	--enable-device=PSB98010 \
	--with-onu-library \
	--without-sce-dbg \
	--without-cop-dbg

OPTIC_LIB_CFG_OPT:= \
	--with-optic-library

TARGET_CFLAGS = \
	-DONU_GPE_LLT_MIN=0x2000 \
	-mips32r2 -mtune=mips32r2 -mips16 -msoft-float \
	-fstrict-aliasing -ffreestanding \
	-fno-builtin -fno-common -G0 -g -Os \
	-Wall -Wextra -Wundef -Wuninitialized -Wno-unused-parameter \
	-Wstrict-aliasing -Wredundant-decls -Wlogical-op -Wstrict-prototypes

TARGET_LDFLAGS=""

define Build/Clean
	@- [ -d $(BUILD_DIR)/$(ONU_LIB_PKG_NAME) ] && rm -Rf $(BUILD_DIR)/$(ONU_LIB_PKG_NAME)
	@- [ -d $(BUILD_DIR)/$(OPTIC_LIB_PKG_NAME) ] && rm -Rf $(BUILD_DIR)/$(OPTIC_LIB_PKG_NAME)
	$(call Build/Clean/Default)
endef

define Build/Prepare
	$(call Build/Prepare/Default)
	$(call Prepare/$(ONU_LIB_PKG_NAME))
	$(call Prepare/$(OPTIC_LIB_PKG_NAME))
endef

define Build/Configure
	(cd $(DRV_ONU_SOURCE_DIR); autoreconf -i; touch configure; \
		CFLAGS="$(TARGET_CFLAGS)" \
		LDFLAGS=$(TARGET_LDFLAGS) \
		./configure $(COMMON_CFG_OPT) $(ONU_LIB_CFG_OPT) \
	); \
	touch $(DRV_OPTIC_SOURCE_DIR)/.configured; \
	(cd $(DRV_OPTIC_SOURCE_DIR); autoreconf -i; touch configure; \
		CFLAGS="$(TARGET_CFLAGS) -DOPTIC_OCAL_SUPPORT=INACTIVE" \
		LDFLAGS=$(TARGET_LDFLAGS) \
		./configure $(COMMON_CFG_OPT) $(OPTIC_LIB_CFG_OPT) \
	); \
	touch $(PKG_BUILD_DIR)/.configured; \
	(cd $(PKG_BUILD_DIR); autoreconf -i; touch configure; \
		CFLAGS="$(TARGET_CFLAGS)" \
		LDFLAGS=$(TARGET_LDFLAGS) \
		./configure $(COMMON_CFG_OPT) $(ONU_FIRMWARE_CFG_OPT) \
	); \
	touch $(PKG_BUILD_DIR)/.configured;
endef

define Build/Compile
	$(MAKE) -C $(DRV_ONU_SOURCE_DIR)
	$(MAKE) -C $(DRV_OPTIC_SOURCE_DIR)
	$(MAKE) -C $(PKG_BUILD_DIR) $(TARGET_CONFIGURE_OPTS)
endef

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/opt/lantiq/firmware
	$(CP) $(PKG_BUILD_DIR)/src/gpon-onu-firmware.tar.gz $(1)/opt/lantiq/firmware/
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/onu_fw_init.sh $(1)/etc/init.d/
endef

define Build/InstallDev
#
#
endef


$(eval $(call BuildPackage,$(PKG_NAME)))
$(eval $(call BuildPackage,$(ONU_LIB_PKG_NAME)))
$(eval $(call BuildPackage,$(OPTIC_LIB_PKG_NAME)))
