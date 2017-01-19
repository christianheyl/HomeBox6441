#
# Copyright (C) 2007-2008 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=jbed-cdca
PKG_VERSION:=BLD3491_RR4382
PKG_RELEASE:=1
PKG_BUILD_DIR:=jbed_cdca_fp11_CDC_MIPS_Linux
PKG_SOURCE:=JbedCDCA_FP11_Linux_MIPS_$(PKG_VERSION).tar.gz
#PKG_SOURCE_URL:=file:///home/zaharia/Work
PKG_SOURCE_URL:=http://localhost
PKG_MD5SUM:=7cb9d7b6224bb8cc030609f4799d5747
PKG_CAT:=zcat
#PKG_BUILD_DEPENDS:=

include $(INCLUDE_DIR)/package.mk

define Package/jbed-cdca
  SECTION:=utils
  CATEGORY:=Utilities
  TITLE:=A Java Virtual Machine
  #DESCRIPTION:=
  URL:=http://myriadgroup.com/
  DEPENDS:=@TARGET_ltqcpe_platform_vr9 @PACKAGE_kmod-ltqcpe_nand
endef

define Build/Compile
	:
endef

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_DIR) $(1)/usr/lib
	$(INSTALL_DIR) $(1)/usr/lib/security
	$(INSTALL_BIN)  $(PKG_BUILD_DIR)/../bin/cdca_Sa                       $(1)/usr/bin/
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/../lib/content-types.properties      $(1)/usr/lib/
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/../lib/security/cacerts              $(1)/usr/lib/security/
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/../lib/security/java.policy          $(1)/usr/lib/security/
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/../lib/security/java.security        $(1)/usr/lib/security/
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/../lib/security/java.security.readme $(1)/usr/lib/security/
	mkdir -p $(1)/opt/lantiq/
	$(CP) ./osgi $(1)/opt/lantiq/osgi/
endef

$(eval $(call BuildPackage,jbed-cdca))
