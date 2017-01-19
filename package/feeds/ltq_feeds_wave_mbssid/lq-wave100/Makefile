#
# Openwrt Makefile for lq-wave
#

#### Includes ###################################
include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

#### Package Info ###############################
PKG_NAME:=lq-wave100
PKG_VERSION:=0.6.1
PKG_RELEASE:=1
PKG_BASE_NAME:=lq_wave100
PKG_SOURCE:=$(PKG_BASE_NAME)-$(PKG_VERSION).tar.gz
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_BASE_NAME)-$(PKG_VERSION)
PKG_SOURCE_URL:=http://localhost
PKG_BUILD_DEPENDS:=libopenssl
PKG_DEFAULT_CUSTOM_SOURCE_DIR:=$(TOPDIR)/../../../wlnsw/wireline_sw_wlan/ifx_sw
PKG_NO_AUTORECONF:=y

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/package-version-override.mk

define Package/$(PKG_NAME)
  SECTION:=driver
  CATEGORY:=Lantiq
  TITLE:=Lantiq Wave Sub System Package for GW 4.3 onwards
  URL:=http://www.lantiq.com
  MAINTAINER:=Lantiq GmbH
  MENU:=1
endef

define Package/$(PKG_NAME)/description
  WAVE driver and utilities
endef

define KernelPackage/ifxcpe_wave100_pci
  SUBMENU:=Lantiq
  TITLE:=LANTIQ PCI Support for WAVE100 
  DEPENDS:=@TARGET_ifxcpe_platform_ar9||TARGET_ifxcpe_platform_danube +kmod-ifxcpe_pci
  KCONFIG:= \
	CONFIG_IFX_PCI_CLOCK_DELAY_NANO_SECONDS=0 \
	CONFIG_IFX_PCI_CLOCK_DELAY_TENTH_NANO_SECOND=1 \
	CONFIG_FW_LOADER=y
endef

define KernelPackage/ifxcpe_wave100_pci/description
  Kernel Support for WAVE100 PCI 
endef

#### Directory Paths ############################
OPENSSL_INCLUDE_DIR:=$(wildcard $(BUILD_DIR)/openssl-*)
export LQ_OPENSSL_INCLUDE_DIR=$(OPENSSL_INCLUDE_DIR)
export LQ_PRODUCT_ROOT=$(PKG_BUILD_DIR)

### Menuconfig ##################################
define Package/$(PKG_NAME)/config
$(call Package/$(PKG_NAME)/override_source_path)
	menu "$(PKG_NAME) features"
	depends on PACKAGE_$(PKG_NAME)
	source "$(SOURCE)/config/Config.in"
	endmenu
	config PACKAGE_$(PKG_NAME)
	  select PACKAGE_kmod-ifxcpe_wave100_pci if TARGET_ifxcpe_platform_ar9
	  select FEATURE_IFX_WIRELESS if TARGET_ifxcpe_platform_ar9
endef

#### Target Rules ###############################
define Build/Prepare
	$(call Build/Prepare/Default)
	$(INSTALL_DIR) $(PKG_BUILD_DIR)/dest_dir
	$(if $(CONFIG_TARGET_ifxcpe_platform_amazon_se),\
		$(if $(CONFIG_FEATURE_LQ_WAVE100_EXT_PA),\
		$(CP) $(PKG_BUILD_DIR)/src/defconfig.ugw.ase.extPA $(PKG_BUILD_DIR)/src/.config,\
		$(CP) $(PKG_BUILD_DIR)/src/defconfig.ugw.ase $(PKG_BUILD_DIR)/src/.config)\
	,\
	$(if $(CONFIG_TARGET_ifxcpe_platform_ar9),$(CP) $(PKG_BUILD_DIR)/src/defconfig.ugw.ar9 $(PKG_BUILD_DIR)/src/.config) \
	)
	chmod +w $(PKG_BUILD_DIR)/src/.config
endef

define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR)/src KERNEL_DIR=$(LINUX_DIR) DEST_DIR=$(PKG_BUILD_DIR)/dest_dir
endef

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/lib/modules/$(LINUX_VERSION)
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/dest_dir/wlan.ko  $(1)/lib/modules/$(LINUX_VERSION)
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/dest_dir/wlan_acl.ko  $(1)/lib/modules/$(LINUX_VERSION)
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/dest_dir/wlan_tkip.ko  $(1)/lib/modules/$(LINUX_VERSION)
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/dest_dir/wlan_ccmp.ko  $(1)/lib/modules/$(LINUX_VERSION)
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/dest_dir/wlan_scan_sta.ko  $(1)/lib/modules/$(LINUX_VERSION)
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/dest_dir/wlan_scan_ap.ko  $(1)/lib/modules/$(LINUX_VERSION)
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/dest_dir/wlan_wep.ko  $(1)/lib/modules/$(LINUX_VERSION)
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/dest_dir/wlan_xauth.ko  $(1)/lib/modules/$(LINUX_VERSION)
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/dest_dir/wlan_1650_ap.ko  $(1)/lib/modules/$(LINUX_VERSION)
	$(INSTALL_DIR) $(1)/etc/rc.d
	$(INSTALL_BIN) ./files/scripts/* $(1)/etc/rc.d/
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/dest_dir/1650-AP_util $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/dest_dir/hostapd $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/dest_dir/hostapd_cli $(1)/usr/bin
endef

define Build/Clean
	if [ -d $(PKG_BUILD_DIR) ]; then $(MAKE) -C $(PKG_BUILD_DIR)/src KERNEL_DIR=$(LINUX_DIR) clean; \
	$(RM) -r $(PKG_BUILD_DIR)/ipkg-$(BOARD); fi
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
$(eval $(call KernelPackage,ifxcpe_wave100_pci))
