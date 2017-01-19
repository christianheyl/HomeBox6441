# 
# Openwrt Makefile for lq-igmpd
#

#### Includes ###################################
include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

#### Package Info ##############################
PKG_NAME:=lq-igmpd
PKG_VERSION:=2.3.15.1
PKG_RELEASE:=1
PKG_BASE_NAME:=lq_igmpd
PKG_SOURCE:=$(PKG_BASE_NAME)-$(PKG_VERSION).tar.gz
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_BASE_NAME)-$(PKG_VERSION)
PKG_SOURCE_URL:=http://localhost
PKG_NO_AUTORECONF=y

PKG_CONFIG_DEPENDS_REBUILD=y
PKG_CONFIG_DEPENDS:=\
	CONFIG_IFX_CONFIG_CPU \
	CONFIG_PACKAGE_IFX_DSL_CPE_API \
	CONFIG_PACKAGE_ifx-dsl-cpe-api-danube \
	CONFIG_PACKAGE_ifx-dsl-cpe-api-vrx \
	CONFIG_PACKAGE_ifx-dsl-cpe-api-vrx-bonding \
	CONFIG_PACKAGE_ifx-voip-common

PKG_BUILD_DEPENDS:=CONFIG_PACKAGE_ifx-voip-common:ifx-voip-common

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/package-version-override.mk

define Package/$(PKG_NAME)
	SECTION:=utils
	CATEGORY:=Lantiq
	SUBMENU:=Router Subsystem
	TITLE:=Lantiq igmp proxy daemon
	URL:=http://www.lantiq.com
	MAINTAINER:=Lantiq Deutschland GmbH
	DEPENDS:=+ifx-oam +ifx-IFXAPIs +ifx-httpd
	MENU:=1
endef

define Package/$(PKG_NAME)/config
$(call Package/$(PKG_NAME)/override_version)
$(call Package/$(PKG_NAME)/override_source_path)
	menu "$(PKG_NAME) features"
	depends on PACKAGE_$(PKG_NAME)
	source "$(SOURCE)/config/Config.in"
	endmenu
endef

define Package/$(PKG_NAME)/description
	This package contain Lantiq IGMP proxy daemon
endef

#### Directory Paths ############################
IFX_APIS_DIR:=$(wildcard $(BUILD_DIR)/ifx_IFXAPIs*)
IFX_HTTPD_DIR:=$(wildcard $(BUILD_DIR)/ifx_httpd*)
IFX_DSL_CPE_API_INCLUDES:=$(STAGING_DIR)/usr/include/drv_dsl_cpe_api/
IFX_DSL_CPE_CTRL_DIR:=$(wildcard $(BUILD_DIR)/dsl_cpe_control*)

### Configurations depends on Menuconfig ########
IFX_CFLAGS_y:=
IFX_LDFLAGS_y:=

IFX_CFLAGS_y +=-I$(KERNEL_BUILD_DIR)/include
IFX_CFLAGS_y +=$(TARGET_CFLAGS) -Wall -Werror
IFX_CFLAGS_y +=-I$(IFX_APIS_DIR)/include
IFX_CFLAGS_y +=-I$(IFX_HTTPD_DIR)
IFX_CFLAGS_y +=-I$(STAGING_DIR)/usr/include -DBUILD_FROM_LTQ_APPS

export HTTPD_DIR:=$(IFX_HTTPD_DIR)
export IFX_APIS_DIR:=$(IFX_APIS_DIR)
export LINUX_DIR:=$(LINUX_DIR)

export CONFIG_PACKAGE_ifx-voip-common
export CONFIG_FEATURE_LTQ_IGMP_STANDARD_FEATURES
export CONFIG_FEATURE_LTQ_IGMP_PREMIUM_FEATURES 
export CONFIG_FEATURE_LTQ_MCAST_FILTER_PORT

ifeq ($(CONFIG_PACKAGE_ifx-dsl-cpe-api-danube),y)
		IFX_CFLAGS_y +=-DLINUX \
				-I$(IFX_DSL_CPE_API_INCLUDES) \
		        	-DDSL_CHANNELS_PER_LINE=1

	ifeq ($(CONFIG_IFX_CONFIG_CPU),"AMAZON_SE")
		IFX_CFLAGS_y +=-DINCLUDE_DSL_CPE_API_DANUBE \
							-DCONFIG_PACKAGE_IFX_DSL_CPE_API
	endif
	
	ifeq ($(CONFIG_IFX_CONFIG_CPU),"DANUBE")
		IFX_CFLAGS_y +=-DINCLUDE_DSL_CPE_API_DANUBE \
							-DCONFIG_PACKAGE_IFX_DSL_CPE_API
	endif
	
	ifeq ($(CONFIG_IFX_CONFIG_CPU),"AMAZON_S")
		IFX_CFLAGS_y +=-DINCLUDE_DSL_CPE_API_DANUBE \
							-DCONFIG_PACKAGE_IFX_DSL_CPE_API
	endif

	ifeq ($(CONFIG_IFX_CONFIG_CPU),"XRX3XX")
		IFX_CFLAGS_y +=-DINCLUDE_DSL_CPE_API_DANUBE \
							-DCONFIG_PACKAGE_IFX_DSL_CPE_API
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

ifeq ($(CONFIG_TARGET_$(BOARD)_platform_danube),y)
	IFX_CFLAGS_y +=-DPLATFORM_DANUBE
endif
ifeq ($(CONFIG_TARGET_$(BOARD)_platform_ar9),y)
	IFX_CFLAGS_y +=-DPLATFORM_AR9
endif

ifeq ($(CONFIG_TARGET_$(BOARD)_platform_ar10),y)
ifeq ($(CONFIG_TARGET_$(BOARD)_platform_ar10_vrx318),y)
        IFX_CFLAGS_y +=-DPLATFORM_AR10_VRX318 -DPLATFORM_VR9
else 
ifeq ($(CONFIG_TARGET_$(BOARD)_platform_ar10_GRX390),y)
 	IFX_CFLAGS_y +=-DPLATFORM_AR10_GRX390 -DPLATFORM_AR10 -DPLATFORM_VR9
else
        IFX_CFLAGS_y +=-DPLATFORM_AR9 -DPLATFORM_AR10
endif
endif
endif

ifeq ($(CONFIG_TARGET_$(BOARD)_platform_amazon_se),y)
	IFX_CFLAGS_y +=-DPLATFORM_AMAZON_SE
endif
ifeq ($(CONFIG_TARGET_$(BOARD)_platform_vr9),y)
	IFX_CFLAGS_y +=-DPLATFORM_VR9
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
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/mcastd $(1)/usr/sbin/
endef

define Build/Clean
	if [ -d $(PKG_BUILD_DIR) ]; then $(MAKE) -C $(PKG_BUILD_DIR) clean; \
	$(RM) -r $(PKG_BUILD_DIR)/ipkg-$(BOARD); fi
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
