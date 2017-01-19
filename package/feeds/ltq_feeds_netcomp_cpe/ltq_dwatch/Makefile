# 
# Openwrt Makefile for ltq-dwatch
#

#### Includes ###################################
include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

#### Package Info ##############################
PKG_NAME:=ltq-dwatch
PKG_VERSION:=2.3.15.0
PKG_RELEASE:=1
PKG_MD5SUM:=b0ae4d724ff45fcb1479bfcd54050b28
PKG_BASE_NAME:=ltq-dwatch
PKG_SOURCE:=$(PKG_BASE_NAME)-$(PKG_VERSION).tar.gz
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_BASE_NAME)-$(PKG_VERSION)
PKG_SOURCE_URL:=http://localhost
#PKG_FILE_DEPENDS:=$(STAGING_DIR)/usr/include/ifx_config.h
PKG_NO_AUTORECONF=y

PKG_CONFIG_DEPENDS_REBUILD=y

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/package-version-override.mk

define Package/$(PKG_NAME)
	SECTION:=utils
	CATEGORY:=Lantiq
	SUBMENU:=Router Subsystem
	TITLE:=Lantiq dwatch daemon
	URL:=http://www.lantiq.com
	MAINTAINER:=Lantiq Deutschland GmbH
	MENU:=1
endef

define Package/$(PKG_NAME)/description
	This package contain Lantiq dwatch daemon
endef

#### Directory Paths ############################

### Configurations depends on Menuconfig ########


#### Target Rules ###############################
define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR)/ \
	$(TARGET_CONFIGURE_OPTS) 
endef

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/dwatch $(1)/usr/sbin/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/dwatchctl $(1)/usr/sbin/
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/dwatch $(1)/etc/init.d/dwatch
endef

define Build/Clean
	if [ -d $(PKG_BUILD_DIR) ]; then $(MAKE) -C $(PKG_BUILD_DIR) clean; \
	$(RM) -r $(PKG_BUILD_DIR)/ipkg-$(BOARD); fi
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
