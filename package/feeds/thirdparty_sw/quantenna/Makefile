#
# Copyright (C) 2007 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
# $Id: Makefile 13904 2009-01-06 21:56:22Z jow $

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=quantenna
PKG_VERSION:=1.1

include $(INCLUDE_DIR)/package.mk

define Package/$(PKG_NAME)
	SECTION:=utils
	CATEGORY:=Lantiq
	TITLE:= Quantenna scripts/binaries
	DEPENDS:=@LINUX_2_6&&TARGET_ltqcpe_platform_ar10&&TARGET_ltqcpe_platform_ar10_GRX390
	URL:=http://www.lantiq.com
	MAINTAINER:=LANTIQ
	MENU:=1
endef

define Package/$(PKG_NAME)/description
 This package contains quantenna scripts/binaries
endef

define Package/$(PKG_NAME)/config
  config PACKAGE_$(PKG_NAME)

  config PACKAGE_QUANTENNA_RGMII_TARGET_IP
	depends on PACKAGE_$(PKG_NAME)
        string 'Quantenna Target IP address'
        default "192.168.1.100"
	help
          'IP address of Quantenna RGMII target'

  config PACKAGE_QUANTENNA_FIRMWARE_EMBEDDED
	depends on PACKAGE_$(PKG_NAME)
	bool 'Quantenna firmware is embedded in host image'
	help
	  'For flash-less Quantenna RGMII module, firmware is embedded in host image'

  config PACKAGE_QUANTENNA_TYPE_SINGLE_RGMII
	depends on PACKAGE_$(PKG_NAME)
	bool 'Single RGMII'
	help
	  'Supprt Quantenna 11ac module which is connected to single RGMII'
	
  config PACKAGE_QUANTENNA_TYPE_TWO_RGMII
	depends on PACKAGE_$(PKG_NAME)
	bool 'Double RGMII'
	help
	  'Supprt Quantenna 11ac module which is connected to two RGMIIs'

  config PACKAGE_QUANTENNA_LINK_AGGREGATION_PORTS
	depends on PACKAGE_QUANTENNA_TYPE_TWO_RGMII
        string 'Link aggregation ports'
        default "0 5"
	help
          'RGMII ports which will be aggregated'

  config PACKAGE_QUANTENNA_ETH_LAN_PORTS
	depends on PACKAGE_$(PKG_NAME)
        string 'Ethernet LAN ports'
        default "0 1 4 5"
	help
          'Ethernet LAN ports'

  config PACKAGE_QUANTENNA_ETH_WAN_PORT
	depends on PACKAGE_$(PKG_NAME)
        string 'Ethernet WAN port'
        default "2"
	help
          'Ethernet WAN port'
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
endef

define Build/Compile
        echo -n;
endef

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/etc/rc.d/
	$(INSTALL_DIR) $(1)/lib/firmware/
	$(INSTALL_DIR) $(1)/lib/
	$(INSTALL_DIR) $(1)/sbin/
	$(if $(CONFIG_PACKAGE_QUANTENNA_FIRMWARE_EMBEDDED), \
		$(INSTALL_BIN) ./files/qtn_binaries/u-boot.bin $(1)/lib/firmware/; \
		$(INSTALL_BIN) ./files/qtn_binaries/topaz-linux.lzma.img $(1)/lib/firmware/; \
        )
	$(INSTALL_BIN)  ./files/qtn_binaries/call_qcsapi_sockrpc $(1)/sbin/
	$(INSTALL_BIN)  ./files/qtn_binaries/librpc.so $(1)/lib/
	$(CP)		./files/qtn_scripts/* $(1)/etc/rc.d/
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
