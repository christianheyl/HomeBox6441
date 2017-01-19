#
# Copyright (C) 2008 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

include $(INCLUDE_DIR)/image.mk

PKG_NAME:=open_uboot
PKG_VERSION:=2010.06
PKG_RELEASE:=1

PKG_BUILD_DIR:=$(KERNEL_BUILD_DIR)/u-boot-$(PKG_VERSION)
PKG_SOURCE:=u-boot-$(PKG_VERSION).tar.bz2
PKG_SOURCE_URL:=ftp://ftp.denx.de/pub/u-boot
PKG_MD5SUM:=cd42bc64b6edafa6930ce299a144503e
PKG_CAT:=bzcat
#PKG_TARGETS:=bin

include $(INCLUDE_DIR)/package.mk

ifeq ($(DUMP),)
  STAMP_CONFIGURED:=$(strip $(STAMP_CONFIGURED))_$(shell $(SH_FUNC) grep '^CONFIG_UBOOT_' $(TOPDIR)/.config | md5s)
endif
  
define Package/$(PKG_NAME)
  SECTION:=boot
  CATEGORY:=Boot Loaders
  DEPENDS:=@TARGET_ltqcpe||TARGET_lantiq
  TITLE:=U-Boot for Lantiq SoC based platforms
  URL:=http://www.denx.de/wiki/UBoot/WebHome
endef

EXT_SRC=src.VdslIadO2Hb2
PATCHES_SRC=patches_ugw54
CONFIG_SRC=config_ugw54

define Build/Prepare
	rm -f ./config
	ln -sf ./$(CONFIG_SRC) ./config
	rm -f ./patches
	ln -sf ./$(PATCHES_SRC) ./patches
	$(call Build/Prepare/Default)
	cp -r ./files/* $(PKG_BUILD_DIR)
	if [ -d "./$(EXT_SRC)" ] ; then \
		$(CP) ./$(EXT_SRC)/* $(PKG_BUILD_DIR)/. ; \
	else \
		cp -r ./src/* $(PKG_BUILD_DIR); \
	fi
endef

#define Build/Configure
#endef

# Target Macros
ifeq ($(CONFIG_TARGET_$(BOARD)_platform_ar9)$(CONFIG_TARGET_$(BOARD)_ar9),y)
  UBOOT_PLATFORM:=ar9
endif

ifdef CONFIG_TARGET_ltqcpe_platform_vbg400
UBOOT_PLATFORM:=vbg400
endif

ifeq ($(CONFIG_TARGET_$(BOARD)_platform_danube)$(CONFIG_TARGET_$(BOARD)_danube),y)
  UBOOT_PLATFORM:=danube
endif

ifeq ($(CONFIG_TARGET_$(BOARD)_platform_amazon_se)$(CONFIG_TARGET_$(BOARD)_ase),y)
  UBOOT_PLATFORM:=amazon_se
endif

ifeq ($(CONFIG_TARGET_$(BOARD)_platform_vr9)$(CONFIG_TARGET_$(BOARD)_vr9)$(CONFIG_TARGET_$(BOARD)_xrx200)$(CONFIG_TARGET_$(BOARD)_xrx500),y)
  UBOOT_PLATFORM:=vr9
endif

ifeq ($(CONFIG_TARGET_$(BOARD)_platform_ar10)$(CONFIG_TARGET_$(BOARD)_ar10)$(CONFIG_TARGET_$(BOARD)_xrx300),y)
  ifdef CONFIG_TARGET_ltqcpe_platform_ar10_GRX390 
    UBOOT_PLATFORM:=grx390
  else
    UBOOT_PLATFORM:=ar10
  endif
endif

ifeq ($(CONFIG_TARGET_$(BOARD)_xrx330),y)
  UBOOT_PLATFORM:=grx390
endif

ifeq ($(CONFIG_TARGET_$(BOARD)_platform_hn1)$(CONFIG_TARGET_$(BOARD)_hn1),y)
  UBOOT_PLATFORM:=hn1
endif

ifdef CONFIG_TARGET_ifxcpe_platform_vbg400
UBOOT_PLATFORM:=vbg400
endif

UBOOT_IMAGE :=$(PKG_BUILD_DIR)/u-boot.ltq

ifdef CONFIG_UBOOT_CONFIG_BOOT_FROM_NAND
UBOOT_IMAGE +=$(PKG_BUILD_DIR)/u-boot-nand.bin
ifeq ($(CONFIG_TARGET_$(BOARD)_platform_hn1)$(CONFIG_TARGET_$(BOARD)_hn1),y)
UBOOT_IMAGE +=$(PKG_BUILD_DIR)/u-boot.uart
ifdef CONFIG_UBOOT_CONFIG_ENABLE_DDR_DCDC
UBOOT_IMAGE +=$(PKG_BUILD_DIR)/dcdc_ddr_patch.uart
endif
ifndef CONFIG_UBOOT_CONFIG_HN1_GPHY_FW_EMBEDDED
UBOOT_IMAGE +=$(PKG_BUILD_DIR)/gphy_firmware.img
endif
endif
endif


ifdef CONFIG_UBOOT_CONFIG_BUILD_ENV_BLOCK
UBOOT_IMAGE +=$(PKG_BUILD_DIR)/ubootenv.img
endif



ifdef CONFIG_UBOOT_CONFIG_BOOT_FROM_SPI
ifeq ($(CONFIG_TARGET_$(BOARD)_platform_hn1)$(CONFIG_TARGET_$(BOARD)_hn1),y)
UBOOT_IMAGE +=$(PKG_BUILD_DIR)/u-boot.uart
ifdef CONFIG_UBOOT_CONFIG_ENABLE_DDR_DCDC
UBOOT_IMAGE +=$(PKG_BUILD_DIR)/dcdc_ddr_patch.uart
endif
ifndef CONFIG_UBOOT_CONFIG_HN1_GPHY_FW_EMBEDDED
UBOOT_IMAGE +=$(PKG_BUILD_DIR)/gphy_firmware.img
endif
endif
endif

define Package/open_uboot/config
        menu "Configuration"
                depends on PACKAGE_open_uboot
                source "$(SOURCE)/config/Kconfig"
        endmenu
endef


define Build/Configure
        rm -f $(PKG_BUILD_DIR)/.configured*
	rm -f ./config
	ln -sf ./$(CONFIG_SRC) ./config
endef

ifeq ($(CONFIG_KERNEL_CRYPTO),y)
NEW_CONFIG_BOOTCOMMAND=run flash_flash2
else
NEW_CONFIG_BOOTCOMMAND=run flash_flash
endif

define Build/Compile
	@echo "XXX Do Compile XXX"
	#ln -sdf $(TOPDIR)/../../../wlnsw/wireline_sw_linux26/open_uboot $(PKG_BUILD_DIR)/open_uboot

	cd $(PKG_BUILD_DIR);\
	chmod a+x ./scripts_platform/*;\
	make clean distclean;\
	make $(UBOOT_PLATFORM)_config;\
	echo "XXX Generate uboot .config XXX";\
	grep 'CONFIG_UBOOT_' $(TOPDIR)/.config | sed -e "s,\\(# \)\\?CONFIG_UBOOT_\\(.*\\),\\1\\2,g" > $(PKG_BUILD_DIR)/.config;\
	grep FLASH_FLASH $(PKG_BUILD_DIR)/.config;\
	echo "XXX hack in uboot .config & modify CONFIG_BOOTCOMMAND according to CONFIG_KERNEL_CRYPTO XXX";\
	cat $(PKG_BUILD_DIR)/.config | sed "s/CONFIG_BOOTCOMMAND.*/CONFIG_BOOTCOMMAND=\"$(NEW_CONFIG_BOOTCOMMAND)\"/" > $(PKG_BUILD_DIR)/.tmp_config;\
	cp $(PKG_BUILD_DIR)/.tmp_config $(PKG_BUILD_DIR)/.config;\
	rm $(PKG_BUILD_DIR)/.tmp_config;\
	make oldconfig CROSS_COMPILE=$(TARGET_CROSS);\
	grep FLASH_FLASH $(PKG_BUILD_DIR)/.config;\
	make clean all CROSS_COMPILE=$(TARGET_CROSS);\
	grep FLASH_FLASH $(PKG_BUILD_DIR)/.config
	
	# Terry 20131206, create ASC file for UART boot
ifeq ($(CONFIG_UBOOT_CONFIG_AR10_DDR_NANYA),y)
	$(PKG_BUILD_DIR)/scripts_platform/gct \
		$(PKG_BUILD_DIR)/board/ar10/ddr2/xrx300_ddr2_300M_settings_nanya_128Mx8.conf  \
		$(PKG_BUILD_DIR)/u-boot.srec \
		$(PKG_BUILD_DIR)/u-boot.asc
endif
endef

define Build/Clean
	@echo "XXX Do CLEAN XXX"
	rm -f ./patches
	rm -f ./config
	$(call Build/Clean/Default)
endef

define Package/$(PKG_NAME)/install
	mkdir -p $(TARGET_IMG_DIR)
	cp -f $(UBOOT_IMAGE) $(TARGET_IMG_DIR)/
ifeq ($(CONFIG_UBOOT_CONFIG_AR10_DDR_NANYA),y)
	cp -f $(PKG_BUILD_DIR)/u-boot.asc $(TARGET_IMG_DIR)/
endif

ifeq ($(CONFIG_KERNEL_CRYPTO),y)
	mkdir -p $(KDIR)/
	cp -f $(PKG_BUILD_DIR)/sec_boot.bin $(KDIR)/
	cp -f $(PKG_BUILD_DIR)/sec_boot.bin $(TARGET_IMG_DIR)/
endif
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
