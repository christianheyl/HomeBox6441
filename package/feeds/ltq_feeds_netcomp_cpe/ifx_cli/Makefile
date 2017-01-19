# 
# Openwrt Makefile for ifx-cli
#

#### Includes ###################################
include $(TOPDIR)/rules.mk

#### Package Info ##############################
PKG_NAME:=ifx-cli
PKG_VERSION:=2.3.15.1
PKG_RELEASE:=1
PKG_MD5SUM:=ad756c5ad0e3d6eac2318ea73412614f
PKG_BASE_NAME:=ifx_cli
PKG_SOURCE:=$(PKG_BASE_NAME)-$(PKG_VERSION).tar.gz
PKG_CAT:=zcat
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_BASE_NAME)-$(PKG_VERSION)
PKG_SOURCE_URL:=http://localhost
PKG_BUILD_DEPENDS:=PACKAGE_ifx-dsl-cpe-api-danube:ifx-dsl-cpe-api-danube PACKAGE_ifx-dsl-cpe-control-danube:ifx-dsl-cpe-control-danube PACKAGE_ifx-dsl-cpe-api-vrx:ifx-dsl-cpe-api-vrx PACKAGE_ifx-dsl-cpe-api-vrx-bonding:ifx-dsl-cpe-api-vrx-bonding PACKAGE_ifx-dsl-cpe-control-vrx:ifx-dsl-cpe-control-vrx PACKAGE_ifx-dsl-cpe-control-vrx-bonding:ifx-dsl-cpe-control-vrx-bonding
#PKG_FILE_DEPENDS:=$(STAGING_DIR)/usr/include/ifx_config.h
PKG_DEFAULT_CUSTOM_SOURCE_DIR:=$(TOPDIR)/../../../wlnsw/wireline_sw_router/ifx_cli
PKG_NO_AUTORECONF=y

PKG_CONFIG_DEPENDS_REBUILD=y
PKG_CONFIG_DEPENDS:=\
	CONFIG_IFX_CONFIG_CPU \
	CONFIG_PACKAGE_samba3 \
	CONFIG_FEATURE_SAMBA \
	CONFIG_PACKAGE_IFX_DSL_CPE_API \
	CONFIG_PACKAGE_ifx-dsl-cpe-api-danube \
	CONFIG_PACKAGE_ifx-dsl-cpe-api-vrx \
	CONFIG_PACKAGE_ifx-dsl-cpe-api-vrx-bonding

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/package-version-override.mk

define Package/$(PKG_NAME)
	SECTION:=utils
	CATEGORY:=Lantiq
	SUBMENU:=Router Subsystem
	TITLE:=Lantiq CLI module
	URL:=http://www.lantiq.com
	MAINTAINER:=Lantiq Technologies AG
	DEPENDS:=+ifx-oam +ifx-IFXAPIs +ifx-httpd
	MENU:=1
endef

define Package/$(PKG_NAME)/description
	This package contain Lantiq Command Line Interface Application
endef

#### Directory Paths ############################
IFX_APIS_DIR:=$(wildcard $(BUILD_DIR)/ifx_IFXAPIs*)
IFX_HTTPD_DIR:=$(wildcard $(BUILD_DIR)/ifx_httpd*)
IFX_DSL_CPE_API_INCLUDES:=$(STAGING_DIR)/usr/include/drv_dsl_cpe_api/
IFX_DSL_CPE_CTRL_DIR:=$(wildcard $(BUILD_DIR)/dsl_cpe_control*)

### Configurations depends on Menuconfig ########
IFX_CFLAGS_y:=
IFX_LDFLAGS_y:=

IFX_CFLAGS_y +=$(TARGET_CFLAGS) -Wall -Werror -DBUILD_FROM_LTQ_APPS
ifeq ($(CONFIG_PACKAGE_samba3),y)
  IFX_CFLAGS_y +=-DCONFIG_FEATURE_SAMBA
  export CONFIG_PACKAGE_samba3
endif
IFX_CFLAGS_y +=-I$(IFX_APIS_DIR)/include
IFX_CFLAGS_y +=-I$(IFX_HTTPD_DIR)
IFX_CFLAGS_y +=-I$(STAGING_DIR)/usr/include

export HTTPD_DIR:=$(IFX_HTTPD_DIR)
export IFX_APIS_DIR:=$(IFX_APIS_DIR)

ifeq ($(CONFIG_PACKAGE_ifx-dsl-cpe-api-danube),y)
	IFX_CFLAGS_y +=-DLINUX \
		-I$(IFX_DSL_CPE_API_INCLUDES) \
		-DDSL_CHANNELS_PER_LINE=1

	ifeq ($(CONFIG_IFX_CONFIG_CPU),"AMAZON_SE")
		IFX_CFLAGS_y  +=-DINCLUDE_DSL_CPE_API_DANUBE -DCONFIG_PACKAGE_IFX_DSL_CPE_API
	endif

	ifeq ($(CONFIG_IFX_CONFIG_CPU),"DANUBE")
		IFX_CFLAGS_y  +=-DINCLUDE_DSL_CPE_API_DANUBE -DCONFIG_PACKAGE_IFX_DSL_CPE_API
	endif

	ifeq ($(CONFIG_IFX_CONFIG_CPU),"AMAZON_S")
		IFX_CFLAGS_y  +=-DINCLUDE_DSL_CPE_API_DANUBE -DCONFIG_PACKAGE_IFX_DSL_CPE_API
	endif

	ifeq ($(CONFIG_IFX_CONFIG_CPU),"XRX3XX")
		IFX_CFLAGS_y  +=-DINCLUDE_DSL_CPE_API_DANUBE -DCONFIG_PACKAGE_IFX_DSL_CPE_API
	endif
else
  ifeq ($(CONFIG_PACKAGE_ifx-dsl-cpe-api-vrx),y)
   	IFX_CFLAGS_y +=-DLINUX \
		-I$(IFX_DSL_CPE_API_INCLUDES) \
		-DDSL_CHANNELS_PER_LINE=1

	IFX_CFLAGS_y +=-DINCLUDE_DSL_CPE_API_VRX -DCONFIG_PACKAGE_IFX_DSL_CPE_API
  endif
  ifeq ($(CONFIG_PACKAGE_ifx-dsl-cpe-api-vrx-bonding),y)
   	IFX_CFLAGS_y +=-DLINUX \
		-I$(IFX_DSL_CPE_API_INCLUDES) \
		-DDSL_CHANNELS_PER_LINE=1

	IFX_CFLAGS_y +=-DINCLUDE_DSL_CPE_API_VRX -DCONFIG_PACKAGE_IFX_DSL_CPE_API
  endif

endif

IFX_LDFLAGS_y +=-L$(STAGING_DIR)/usr/lib
IFX_LDFLAGS_y +=-L$(IFX_APIS_DIR)/lib -Wl,-rpath-link,$(STAGING_DIR)/usr/lib

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
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/cli_fe $(1)/usr/sbin/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/cli_be $(1)/usr/sbin/
endef

define Build/Clean
	if [ -d $(PKG_BUILD_DIR) ]; then $(MAKE) -C $(PKG_BUILD_DIR) clean; \
	$(RM) -r $(PKG_BUILD_DIR)/ipkg-$(BOARD); fi
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
