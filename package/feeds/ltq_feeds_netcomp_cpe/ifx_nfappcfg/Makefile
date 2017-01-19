# 
# Openwrt Makefile for ifx-nfappcfg
#

#### Includes ###################################
include $(TOPDIR)/rules.mk

#### Package Info ###############################
PKG_NAME:=ifx-nfappcfg
PKG_VERSION:=2.3.15.1
PKG_RELEASE:=1
PKG_MD5SUM:=2c58d22c77761d7cd514308b01eccf3e
PKG_BASE_NAME:=ifx_nfappcfg
PKG_SOURCE:=$(PKG_BASE_NAME)-$(PKG_VERSION).tar.gz
PKG_CAT:=zcat
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_BASE_NAME)-$(PKG_VERSION)
PKG_SOURCE_URL:=http://localhost
#PKG_FILE_DEPENDS:=$(STAGING_DIR)/usr/include/ifx_config.h
PKG_DEFAULT_CUSTOM_SOURCE_DIR:=$(TOPDIR)/../../../wlnsw/wireline_sw_router/ifx_nfappcfg
PKG_NO_AUTORECONF=y

PKG_CONFIG_DEPENDS_REBUILD=y
PKG_CONFIG_DEPENDS:=\
	CONFIG_FEATURE_IFX_VOIP

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/package-version-override.mk

define Package/$(PKG_NAME)
	SECTION:=utils
	CATEGORY:=Lantiq
	SUBMENU:=Router Subsystem
	TITLE:=Lantiq NF Application Configuration module
	URL:=http://www.lantiq.com
	MAINTAINER:=Lantiq Technologies AG
	DEPENDS:=+ifx-IFXAPIs
	MENU:=1
endef

define Package/$(PKG_NAME)/description
	This package contain Lantiq NF Application Configuration
endef

#### Directory Paths ############################
IFX_NFAPPCFG_INC_DIR:=$(PKG_BUILD_DIR)

IFX_APIS_DIR:=$(wildcard $(BUILD_DIR)/ifx_IFXAPIs*)
IFX_APIS_INC_DIR:=$(IFX_APIS_DIR)/include
IFX_APIS_LIB_DIR:=$(IFX_APIS_DIR)/lib
IFX_APIS_ALGSRC_DIR:=$(IFX_APIS_DIR)/src/alg


### Configurations depends on Menuconfig ########
IFX_CFLAGS_y:=
IFX_LDFLAGS_y:=

IFX_CFLAGS_y +=$(TARGET_CFLAGS) -Wall -Werror
IFX_CFLAGS_y +=-I$(IFX_APIS_INC_DIR)
IFX_CFLAGS_y +=-I$(IFX_APIS_ALGSRC_DIR)
IFX_CFLAGS_y +=-I$(STAGING_DIR)/usr/include

IFX_LDFLAGS_y +=-L$(STAGING_DIR)/usr/lib -Wl,-rpath-link,$(STAGING_DIR)/usr/lib
IFX_LDFLAGS_y +=-L$(IFX_APIS_LIB_DIR)

IFX_CFLAGS:=$(IFX_CFLAGS_y)
IFX_LDFLAGS:=$(IFX_LDFLAGS_y)

#### Target Rules ###############################
define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR)/ \
	$(TARGET_CONFIGURE_OPTS) \
	IFX_CFLAGS="$(IFX_CFLAGS)" IFX_LDFLAGS="$(IFX_LDFLAGS)"
endef

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/nfappcfg $(1)/usr/sbin/
	#$(INSTALL_BIN) $(PKG_BUILD_DIR)/sipALGd $(1)/usr/sbin/
endef

define Build/Clean
	if [ -d $(PKG_BUILD_DIR) ]; then $(MAKE) -C $(PKG_BUILD_DIR) clean; \
	-rm -f .*.dep; \
	$(RM) $(PKG_BUILD_DIR)/ipkg-$(BOARD); fi
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
