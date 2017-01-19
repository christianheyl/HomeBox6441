#
# Copyright (C) 2008 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=u-boot
PKG_VERSION:=1.1.5
PKG_RELEASE:=0

PKG_BUILD_DIR:=$(KERNEL_BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.bz2
PKG_SOURCE_URL:=ftp://ftp.denx.de/pub/u-boot
PKG_MD5SUM:=579707c8ecbf1ab4127285d2aac2a9ee
PKG_INSTALL_DIR:=$(PKG_BUILD_DIR)/ipkg-install

include $(INCLUDE_DIR)/package.mk

UBOOT_CONFIG=$(strip $(subst ",, $(CONFIG_UBOOT_TARGET)))

define Package/uboot-amazon-se
  SECTION:=boot
  CATEGORY:=Boot Loaders
  DEPENDS:=@LINUX_2_4_AMAZON_SE 
  TITLE:=U-Boot for Infineon Amazon-SE based boards
  URL:=http://www.denx.de/wiki/UBoot/WebHome
endef

IFX_CONFIG_MEMORY_SIZE:=16
IFX_CONFIG_FLASH_SIZE:=4
IFX_UBOOT_RAM_TEXT_BASE=0x80400000
IFX_CONFIG_CPU:=AMAZON_SE
UBOOT_COMPRESS:=lzma

UBOOT_CFLAGS:=-DCONFIG_IFX_MIPS -DCONFIG_LZMA \
	-DIFX_CONFIG_MEMORY_SIZE=$(IFX_CONFIG_MEMORY_SIZE) \
	-DIFX_CONFIG_FLASH_SIZE=$(IFX_CONFIG_FLASH_SIZE) \
	-DIFX_CONFIG_ETHERNET_TYPE_EPHY \
	-DIFX_CONFIG_LED_TYPE_GPIO \
	-DIFX_ADSL_FIRMWARE_IN_ROOTFS=1

UBOOT_MAKE_OPTS:=\
	CROSS_COMPILE=$(TARGET_CROSS) \
	CROSS_COMPILE_UCLIBC=1 \
	COMPRESS=$(UBOOT_COMPRESS) \
	PLATFORM_CPU=4kc \
	IFX_CFLAGS="$(UBOOT_CFLAGS)" \
	UBOOT_RAM_TEXT_BASE=$(IFX_UBOOT_RAM_TEXT_BASE) \
	CPU_TYPE=$(IFX_CONFIG_CPU) \

#	BOOTSTRAP_PRINTF_STATUS=BOOTSTRAP_PRINTF_ENABLED 

define Build/Configure
	$(MAKE) -C $(PKG_BUILD_DIR) \
		$(UBOOT_MAKE_OPTS) \
		$(UBOOT_CONFIG)_config
endef

define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR) \
		$(UBOOT_MAKE_OPTS) \
		ifx_all
endef

define Build/InstallDev
	mkdir -p $(BIN_DIR)
	$(CP) $(PKG_BUILD_DIR)/u-boot.ifx $(BIN_DIR)/$(PKG_NAME)-amazon-se-$(PKG_VERSION)-$(PKG_RELEASE).pkg
endef

$(eval $(call BuildPackage,uboot-amazon-se))
