#
# Openwrt feed Makefile for ltq-gphy-firmware
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=ltq-gphy-firmware
PKG_VERSION:=1.4
PKG_BASE_NAME:=ltq_fw_PHY_IP-$(PKG_VERSION)
PKG_SOURCE:=$(PKG_BASE_NAME).tar.gz
PKG_RELEASE:=1
PKG_SOURCE_URL:=http://localhost
PKG_BUILD_DIR:=$(KERNEL_BUILD_DIR)/$(PKG_BASE_NAME)

include $(INCLUDE_DIR)/package-version-override.mk
include $(INCLUDE_DIR)/package.mk

ifeq ($(DUMP),)
  STAMP_CONFIGURED:=$(strip $(STAMP_CONFIGURED))_$(shell grep '_SW_PORT2_MII' $(TOPDIR)/.config | md5s)
endif

GPHY_FIRMWARE_IMAGE	:=gphy_firmware.img
MKIMAGE			:=$(STAGING_DIR_HOST)/bin/mkimage
LZMA			:=$(STAGING_DIR_HOST)/bin/lzma
PLATFORM_NAME		:=$(subst lantiq_,,$(subst platform_,,$(SUBTARGET)))

ifeq ("$(PLATFORM_NAME)","vr9")
  XWAY_NAME		:=xRx2xx
  ifeq ("$(CONFIG_UBOOT_CONFIG_VR9_SW_PORT2_MII)","y")
    FIRMWARE_1=ltq_fw_PHY22F_IP_1v5_$(XWAY_NAME)_A22_*.bin
    FIRMWARE_1_STRING="VR9 V1.2 GPHY FE"

    FIRMWARE_2=ltq_fw_PHY22F_IP_1v4_$(XWAY_NAME)_A14_*.bin
    FIRMWARE_2_STRING="VR9 V1.1 GPHY FE"
  else
    FIRMWARE_1=ltq_fw_PHY11G_IP_1v5_$(XWAY_NAME)_A22_*.bin
    FIRMWARE_1_STRING="VR9 V1.2 GPHY GE"

    FIRMWARE_2=ltq_fw_PHY11G_IP_1v4_$(XWAY_NAME)_A14_*.bin
    FIRMWARE_2_STRING="VR9 V1.1 GPHY GE"
  endif
endif

ifeq ("$(PLATFORM_NAME)","ar10")
  XWAY_NAME		:=xRx3xx
  FIRMWARE_1=ltq_fw_PHY11G_IP_1v5_$(XWAY_NAME)_A21_*.bin
  FIRMWARE_1_STRING="VR9 V1.2 GPHY GE"

  FIRMWARE_2=ltq_fw_PHY22F_IP_1v5_$(XWAY_NAME)_A21_*.bin
  FIRMWARE_2_STRING="VR9 V1.2 GPHY FE"
endif


define Package/$(PKG_NAME)
  SECTION:=firmware
  CATEGORY:=Lantiq
  TITLE:=GPHY Firmware
  URL:=http://www.lantiq.com/
  MAINTAINER:=Lantiq Deutschland GmbH
  DEPENDS:=@(!UBOOT_CONFIG_VR9_GPHY_FW_EMBEDDED)
  MENU:=1
endef

define Package/$(PKG_NAME)/description
	Lantiq GPHY Firmware
endef

define Build/Configure
endef


define Build/Compile
	cd $(PKG_BUILD_DIR); \
	[ -d $(XWAY_NAME) ] && cd $(XWAY_NAME); \
	$(MKIMAGE) -A mips -T firmware -C none -n $(FIRMWARE_1_STRING) -d $(FIRMWARE_1) tmp1.img; \
	$(MKIMAGE) -A mips -T firmware -C none -n $(FIRMWARE_2_STRING) -d $(FIRMWARE_2) tmp2.img; \
	cat tmp2.img >> tmp1.img; \
	$(LZMA) e tmp1.img  tmp1.lzma; \
	$(MKIMAGE) -A mips -T multi -C lzma -n "GPHY Firmware" -d tmp1.lzma $(GPHY_FIRMWARE_IMAGE)
endef

define Package/$(PKG_NAME)/install
	mkdir -p $(TARGET_IMG_DIR)
	mkdir -p $(1)/etc/init.d
	cp -f $(PKG_BUILD_DIR)/$(XWAY_NAME)/$(GPHY_FIRMWARE_IMAGE) $(TARGET_IMG_DIR)/
	$(INSTALL_BIN) ./files/* $(1)/etc/init.d
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
