#
# Copyright (C) 2011 OpenWrt.org
# Copyright (C) 2011 lantiq.com
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=gpon-uboot
PKG_BASE_NAME:=u-boot
PKG_VERSION:=2011.12
PKG_MD5SUM:=7f29b9f6da44d6e46e988e7561fd1d5f
PKG_RELEASE:=1.2.7.1

PKG_BUILD_DIR:=$(KERNEL_BUILD_DIR)/$(PKG_BASE_NAME)-$(PKG_VERSION)
PKG_SOURCE:=$(PKG_BASE_NAME)-$(PKG_VERSION).tar.bz2
PKG_SOURCE_URL:=ftp://ftp.denx.de/pub/u-boot
PKG_TARGETS:=bin
PKG_BUILD_PARALLEL:=1

include $(INCLUDE_DIR)/package-version-override.mk
include $(INCLUDE_DIR)/package.mk

ifeq ($(DUMP),)
  STAMP_CONFIGURED:=$(PKG_BUILD_DIR)/$(BUILD_VARIANT)/.configured_$(shell $(SH_FUNC) \
	grep '^CONFIG_PACKAGE_uboot-$(BUILD_VARIANT)' $(TOPDIR)/.config | md5s)$(shell $(SH_FUNC) \
	grep '^CONFIG_GPON-UBOOT' $(TOPDIR)/.config | md5s)
  STAMP_BUILT:=$(PKG_BUILD_DIR)/$(BUILD_VARIANT)/.built
endif

define Package/$(PKG_NAME)
  SECTION:=boot
  CATEGORY:=Boot Loaders
  SUBMENU:=gpon-uboot
  DEPENDS:=@TARGET_ifxmips||TARGET_lantiq 
  TITLE:=U-Boot Meta package (no binaries)
  DEFAULT:=y if TARGET_lantiq_falcon
  URL:=http://www.denx.de/wiki/U-Boot
endef

# only override_source_path
define Package/$(PKG_NAME)/config
config GPON-UBOOT-DEBUG
	depends PACKAGE_gpon-uboot
	bool "add compile options for better debugging"
	default n

config GPON-UBOOT-CSCOPE
	depends PACKAGE_gpon-uboot
	bool "enable creation of cscope database"
	default n

  $(call Package/$(PKG_NAME)/override_source_path)
endef

define Package/uboot/template
define Package/uboot-$(1)
  SECTION:=boot
  CATEGORY:=Boot Loaders
  SUBMENU:=gpon-uboot
  DEPENDS:=gpon-uboot
  $(if $(2),DEPENDS+=@$(2))
  TITLE:=$(1)
  DEFAULT:=$(if $(3),y if $(3))
  VARIANT:=$(1)
  URL:=http://www.denx.de/wiki/U-Boot
endef
endef

UBOOTS:=

define BuildUbootPackage
# params:
# $(1) name (TILE, build-variant)
# $(2) config-name (only if different from build-variant)
# $(3) depends (only for "BROKEN")
# $(4) "default y" dependency
	UBOOTS+=$(1)
	CONFIG_$(1):=$(2)
	$(call Package/uboot/template,$(1),$(3),$(4))
endef

################################################################################
# definitions for supported boards/configs
################################################################################

$(eval $(call BuildUbootPackage,easy50712,easy50712_DDR166M))
$(eval $(call BuildUbootPackage,easy50712_ramboot,easy50712_DDR166M_ramboot))
DDR_CONFIG_easy50712_ramboot:=easy50712_DDR166M

$(eval $(call BuildUbootPackage,easy50812,easy50812_DDR166M,BROKEN))
$(eval $(call BuildUbootPackage,easy50812_ramboot,easy50812_DDR166M_ramboot,BROKEN))
DDR_CONFIG_easy50812_ramboot:=easy50812

# falcon boards
$(eval $(call BuildUbootPackage,easy980x0))
$(eval $(call BuildUbootPackage,easy98020,,,(TARGET_lantiq_falcon_Generic || TARGET_lantiq_falcon_EASY98020)))
$(eval $(call BuildUbootPackage,easy980x0_nandflash,,,(TARGET_lantiq_falcon_Generic || TARGET_lantiq_falcon_EASY98000)))
$(eval $(call BuildUbootPackage,easy980x0_norflash,,,(TARGET_lantiq_falcon_Generic || TARGET_lantiq_falcon_EASY98000)))
$(eval $(call BuildUbootPackage,easy980x0_serialflash,,,(TARGET_lantiq_falcon_Generic || TARGET_lantiq_falcon_EASY98000)))
$(eval $(call BuildUbootPackage,easy980x0_netboot))
$(eval $(call BuildUbootPackage,cm95c3am1,95c3am1,,TARGET_lantiq_falcon_EASY98020))
# falcon emulation targets
$(eval $(call BuildUbootPackage,easy980x0_emul,,BROKEN))
$(eval $(call BuildUbootPackage,easy980x0_emul_venus,,BROKEN))
$(eval $(call BuildUbootPackage,easy980x0_nandflash_emul,,BROKEN))
$(eval $(call BuildUbootPackage,easy980x0_serialflash_emul,,BROKEN))

################################################################################

ifdef BUILD_VARIANT
UBOOT_CONFIG:=$(if $(CONFIG_$(BUILD_VARIANT)),$(CONFIG_$(BUILD_VARIANT)),$(BUILD_VARIANT))
UBOOT_RAMBOOT:=$(if $(DDR_CONFIG_$(BUILD_VARIANT)),$(DDR_CONFIG_$(BUILD_VARIANT)).conf,)
endif

ifneq ($(CONFIG_gpon-uboot_USE_CUSTOM_SOURCE_DIR),y)
define Build/Prepare
	$(Build/Prepare/Default)
	echo "-lantiq-gpon-$(PKG_RELEASE)" > $(PKG_BUILD_DIR)/localversion-gpon
endef
endif

ifeq ($(CONFIG_GPON-UBOOT-DEBUG),y)
UBOOT_DEBUG:=-fno-schedule-insns -fno-schedule-insns2
endif

ifeq ($(CONFIG_GPON-UBOOT-CSCOPE),y)
CSCOPE:=cscope
endif

UBOOT_MAKE_OPTS:= -s V=0 \
	CROSS_COMPILE=$(TARGET_CROSS) \
	ENDIANNESS="$(UBOOT_DEBUG)" \
	BUILD_TAG="$(BUILD_VARIANT)" \
	O=$(PKG_BUILD_DIR)/$(BUILD_VARIANT)

define Build/Configure
	$(MAKE) -C $(PKG_BUILD_DIR) \
		$(UBOOT_MAKE_OPTS) \
		$(UBOOT_CONFIG)_config
endef

define Build/Compile
	$(MAKE) $(PKG_JOBS) -C $(PKG_BUILD_DIR) \
		$(UBOOT_MAKE_OPTS) \
		all $(CSCOPE)
endef

define Package/uboot-$(BUILD_VARIANT)/install
	$(INSTALL_DIR) $(1)
	if [ -e $(PKG_BUILD_DIR)/$(BUILD_VARIANT)/u-boot.bri ]; then \
		$(CP) $(PKG_BUILD_DIR)/$(BUILD_VARIANT)/u-boot.bri $(1)/u-boot.img; \
	elif [ -e $(PKG_BUILD_DIR)/$(BUILD_VARIANT)/u-boot.bin ]; then \
		$(CP) $(PKG_BUILD_DIR)/$(BUILD_VARIANT)/u-boot.bin $(1)/u-boot.bin; \
	fi
	if [ "$(UBOOT_RAMBOOT)" ]; then \
		perl ./gct \
			$(UBOOT_RAMBOOT) \
			$(PKG_BUILD_DIR)/$(BUILD_VARIANT)/u-boot.srec \
			$(1)/u-boot.asc; \
	fi
endef

# dummy package for global config options
$(eval $(call BuildPackage,gpon-uboot))

$(foreach u,$(UBOOTS), \
	$(eval $(call BuildPackage,uboot-$(u))) \
)

