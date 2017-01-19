#
# Copyright (C) 2009 OpenWrt.org
# Copyright (C) 2009 lantiq.com
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=gpon-omci-onu-ocs-lib
PKG_VERSION:=0.26.08
PKG_BASE_NAME:=gpon_omci_onu_ocs_lib
PKG_SOURCE:=$(PKG_BASE_NAME)-$(PKG_VERSION).tar.gz
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_BASE_NAME)-$(PKG_VERSION)
PKG_RELEASE:=1
PKG_SOURCE_URL:=http://localhost
PKG_BIN_DIR=opt/ifx/bin

include $(INCLUDE_DIR)/package-version-override.mk
include $(INCLUDE_DIR)/package.mk

# should depend on the DRV (make DEPENDS:=+ltq-gpon-onu)
define Package/$(PKG_NAME)
  SECTION:=application
  CATEGORY:=Lantiq
  TITLE:=OCS-OMCI Stack
  URL:=http://www.lantiq.com/
  MAINTAINER:=Lantiq
  DEPENDS:=
  SUBMENU:=GPON
endef

define Package/$(PKG_NAME)/description
	Implementation of OMCI Stack by OpenCon Systems
endef

define Build/Configure
	echo OCS-OMCI Configure was called. Do nothing.
endef

define Build/Compile
	echo OCS-OMCI Compile was called. Do nothing.
endef

define Build/InstallDev
	echo OCS-OMCI InstallDev was called. Copy libraries and files.
	$(INSTALL_DIR) $(STAGING_DIR)/usr/include/omci_onu_ocs/
	$(CP) $(PKG_BUILD_DIR)/ocsomci/include/*.h $(STAGING_DIR)/usr/include/omci_onu_ocs
	$(CP) $(PKG_BUILD_DIR)/ocsomci/pm/include/*.h $(STAGING_DIR)/usr/include/omci_onu_ocs
	$(CP) $(PKG_BUILD_DIR)/ossvc/include/*.h $(STAGING_DIR)/usr/include/omci_onu_ocs
	$(CP) $(PKG_BUILD_DIR)/user/include/*.h $(STAGING_DIR)/usr/include/omci_onu_ocs
	
	$(INSTALL_DIR) $(STAGING_DIR)/usr/lib
	$(AR) -r $(STAGING_DIR)/usr/lib/libocsomci.a $(PKG_BUILD_DIR)/ont/obj.lx/*.o
endef
  
define Package/$(PKG_NAME)/install
	echo OCS-OMCI Install was called. Do nothing.
endef 

$(eval $(call BuildPackage,$(PKG_NAME)))
