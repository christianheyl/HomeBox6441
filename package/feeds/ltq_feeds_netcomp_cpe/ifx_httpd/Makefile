#
# Openwrt Makefile for ifx-httpd
#
#### Includes ###################################
include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

#### Package Info ###############################
PKG_NAME:=ifx-httpd
PKG_VERSION:=2.3.15.1
PKG_RELEASE:=1
PKG_MD5SUM:=71bbb51d1b40911d1d314e9e9e0410f6
PKG_BASE_NAME:=ifx_httpd
PKG_SOURCE:=$(PKG_BASE_NAME)-$(PKG_VERSION).tar.gz
PKG_CAT:=zcat
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_BASE_NAME)-$(PKG_VERSION)
PKG_SOURCE_URL:=http://localhost
ifneq ($(CONFIG_FEATURE_IFX_HOST_ENVIR),y)
  PKG_BUILD_DEPENDS:=PACKAGE_ifx-dsl-cpe-api-danube:ifx-dsl-cpe-api-danube PACKAGE_ifx-dsl-cpe-control-danube:ifx-dsl-cpe-control-danube PACKAGE_ifx-dsl-cpe-api-vrx:ifx-dsl-cpe-api-vrx PACKAGE_ifx-dsl-cpe-api-vrx-bonding:ifx-dsl-cpe-api-vrx-bonding PACKAGE_ifx-dsl-cpe-control-vrx:ifx-dsl-cpe-control-vrx PACKAGE_ifx-dsl-cpe-control-vrx-bonding:ifx-dsl-cpe-control-vrx-bonding PACKAGE_ifx-voip:ifx-voip PACKAGE_ltq-hn:ltq-hn
endif
#PKG_FILE_DEPENDS:=$(STAGING_DIR)/usr/include/ifx_config.h
PKG_DEFAULT_CUSTOM_SOURCE_DIR:=$(TOPDIR)/../../../wlnsw/wireline_sw_router/ifx_httpd
PKG_NO_AUTORECONF=y

PKG_CONFIG_DEPENDS_REBUILD=y
PKG_CONFIG_DEPENDS:=\
	$(foreach _mr,$(shell grep -w config config/Config.in|awk '{ print $$2 }'),CONFIG_$(_mr)) \
	$(shell cat config/configs_used.list)

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/package-version-override.mk

define Package/$(PKG_NAME)
	SECTION:=utils
	CATEGORY:=Lantiq
	SUBMENU:=Router Subsystem
	TITLE:=Lantiq httpd
	URL:=http://www.lantiq.com
	MAINTAINER:=Lantiq Technologies AG
	DEPENDS:= +ifx-oam +ifx-IFXAPIs 
	MENU:=1
endef

define Package/$(PKG_NAME)/description
	This package contains Lantiq httpd daemon
endef

#### Directory Paths ############################
IFX_HTTPD_DIR:=$(PKG_BUILD_DIR)
IFX_APIS_DIR:=$(wildcard $(BUILD_DIR)/ifx_IFXAPIs*)
IFX_DSL_CPE_API_INCLUDES:=$(STAGING_DIR)/usr/include/drv_dsl_cpe_api/
VOIP_DIR:=$(wildcard $(BUILD_DIR)/ifx_voip-*)
#BUSYBOX_DIR:=$(wildcard $(BUILD_DIR)/busybox*)

ifeq ($(CONFIG_FEATURE_LTQ_HNX_CONFIG),y)
export CONFIG_FEATURE_LTQ_HNX_CONFIG 
endif 

ifeq ($(CONFIG_FEATURE_HNX_ID),y)
export CONFIG_FEATURE_HNX_ID
endif


### Menuconfig ##################################
define Package/$(PKG_NAME)/config
$(call Package/$(PKG_NAME)/override_version)
$(call Package/$(PKG_NAME)/override_source_path)
	menu "$(PKG_NAME) features"
	depends on PACKAGE_$(PKG_NAME)
	source "$(SOURCE)/config/Config.in"
	endmenu
endef

### Configurations depends on Menuconfig ########
IFX_CFLAGS_y:=
IFX_LDFLAGS_y:=

ifeq ($(CONFIG_FEATURE_IFX_HOST_ENVIR),y)
  IFX_CFLAGS_y +=-g -DHOSTENV -Wall -fno-stack-protector -DBUILD_FROM_LTQ_APPS
else
  IFX_CFLAGS_y +=$(TARGET_CFLAGS) -Wall -Werror -DBUILD_FROM_LTQ_APPS
endif
ifeq ($(CONFIG_PACKAGE_samba3),y)
  IFX_CFLAGS_y +=-DCONFIG_FEATURE_SAMBA
  export CONFIG_PACKAGE_samba3
endif
ifeq ($(CONFIG_FEATURE_MEDIA_SERVER),y)
  IFX_CFLAGS_y += -DCONFIG_FEATURE_MEDIA_SERVER
  export CONFIG_FEATURE_MEDIA_SERVER
endif

IFX_CFLAGS_y +=-I$(STAGING_DIR)/usr/include
IFX_CFLAGS_y +=-I$(STAGING_DIR)/usr/include/ifx_httpd
IFX_CFLAGS_y +=-I$(IFX_HTTPD_DIR)
IFX_CFLAGS_y +=-I$(IFX_APIS_DIR)/include
#IFX_CFLAGS_y +=-I$(BUSYBOX_DIR)/networking

ifeq ($(CONFIG_FEATURE_IFX_HTTP_DEBUG),y)
  IFX_CFLAGS_y +=-DIFX_LOG_DEBUG
endif

ifeq ($(CONFIG_PACKAGE_ifx-dsl-cpe-api-danube),y)
	IFX_CFLAGS_y +=-DCONFIG_PACKAGE_IFX_DSL_CPE_API \
		-I$(IFX_DSL_CPE_API_INCLUDES) \
		-DDSL_CHANNELS_PER_LINE=1

  ifeq ($(CONFIG_IFX_CONFIG_CPU),"AMAZON_SE")
	IFX_CFLAGS_y +=-DINCLUDE_DSL_CPE_API_DANUBE
  endif

  ifeq ($(CONFIG_IFX_CONFIG_CPU),"DANUBE")
	IFX_CFLAGS_y +=-DINCLUDE_DSL_CPE_API_DANUBE
  endif

  ifeq ($(CONFIG_IFX_CONFIG_CPU),"AMAZON_S")
	IFX_CFLAGS_y +=-DINCLUDE_DSL_CPE_API_DANUBE
  endif
  ifeq ($(CONFIG_IFX_CONFIG_CPU),"XRX3XX")
	IFX_CFLAGS_y +=-DINCLUDE_DSL_CPE_API_DANUBE
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

IFX_CFLAGS_$(CONFIG_FEATURE_IFX_WIRELESS) +=-DCONFIG_FEATURE_IFX_WIRELESS
IFX_CFLAGS_$(CONFIG_FEATURE_IFX_WIRELESS_RALINK) +=-DCONFIG_FEATURE_IFX_WIRELESS_RALINK
IFX_CFLAGS_$(CONFIG_FEATURE_IFX_WIRELESS_WAVE100) +=-DCONFIG_FEATURE_IFX_WIRELESS_WAVE100
IFX_CFLAGS_$(CONFIG_FEATURE_IFX_WIRELESS_WAVE300) +=-DCONFIG_FEATURE_IFX_WIRELESS_WAVE300
IFX_CFLAGS_$(CONFIG_FEATURE_LTQ_WIRELESS_VB) +=-DCONFIG_FEATURE_LTQ_WIRELESS_VB
IFX_CFLAGS_$(CONFIG_FEATURE_IFX_CONCURRENT_DUAL_WIRELESS) +=-DCONFIG_FEATURE_IFX_CONCURRENT_DUAL_WIRELESS
IFX_CFLAGS_$(CONFIG_FEATURE_SYSTEM_PERFORMANCE_CHARTS) += -DCONFIG_FEATURE_SYSTEM_PERFORMANCE_CHARTS
IFX_CFLAGS_$(CONFIG_FEATURE_LTQ_WIRELESS_STA_SUPPORT) += -DCONFIG_FEATURE_LTQ_WIRELESS_STA_SUPPORT
IFX_CFLAGS_$(CONFIG_FEATURE_CELL_WAN_SUPPORT) += -DCONFIG_FEATURE_CELL_WAN_SUPPORT
IFX_CFLAGS_$(CONFIG_FEATURE_WWAN_LTE_SUPPORT) += -DCONFIG_FEATURE_WWAN_LTE_SUPPORT 
IFX_CFLAGS_$(CONFIG_FEATURE_WEB_REDIRECTOR) += -DCONFIG_FEATURE_WEB_REDIRECTOR
IFX_CFLAGS_$(CONFIG_FEATURE_LTQ_PORT_WAN_BINDING) +=-DCONFIG_FEATURE_LTQ_PORT_WAN_BINDING
IFX_CFLAGS_$(CONFIG_FEATURE_LTQ_VLAN_SWITCH_PORT_ISOLATION) +=-DCONFIG_FEATURE_LTQ_VLAN_SWITCH_PORT_ISOLATION

IFX_CFLAGS_$(CONFIG_FEATURE_LTQ_MCAST_FILTER_PORT) += -DFEATURE_LTQ_MCAST_FILTER_PORT

ifeq ($(CONFIG_FEATURE_SSH_SERVER),y)
	IFX_CFLAGS_y +=-DCONFIG_FEATURE_IFX_SSH_SERVER
endif

ifeq ($(CONFIG_IFX_CONFIG_FLASH_TYPE),"SPI")
	IFX_CFLAGS_y +=-DSPI_SUPPORT
endif

ifeq ($(CONFIG_FEATURE_IPv6),y)
	IFX_CFLAGS_y += -DCONFIG_FEATURE_IPv6
ifeq ($(CONFIG_PACKAGE_ip6tables),y)
	IFX_CFLAGS_y += -DCONFIG_PACKAGE_ip6tables
endif
endif

ifeq ($(CONFIG_FEATURE_MAC_BASED_WEB_SESSION_HANDLE),y)
	IFX_CFLAGS_y +=-DCONFIG_FEATURE_MAC_BASED_WEB_SESSION_HANDLE
endif

ifeq ($(CONFIG_TARGET_$(BOARD)_platform_danube),y)
	IFX_CFLAGS_y +=-DPLATFORM_DANUBE
endif
ifeq ($(CONFIG_TARGET_$(BOARD)_platform_ar9),y)
	IFX_CFLAGS_y +=-DPLATFORM_AR9
endif
ifeq ($(CONFIG_TARGET_$(BOARD)_platform_vbg400),y)
	IFX_CFLAGS_y +=-DPLATFORM_VBG400
	IFX_CFLAGS_y +=-DPLATFORM_VBX
endif
ifeq ($(CONFIG_TARGET_$(BOARD)_platform_amazon_se),y)
	IFX_CFLAGS_y +=-DPLATFORM_AMAZON_SE
endif
ifeq ($(CONFIG_TARGET_$(BOARD)_platform_vr9_RT288),y)
	IFX_CFLAGS_y +=-DPLATFORM_GR9
endif
ifeq ($(CONFIG_TARGET_$(BOARD)_platform_vr9),y)
	IFX_CFLAGS_y +=-DPLATFORM_VR9
endif

ifeq ($(CONFIG_TARGET_$(BOARD)_platform_ar10),y)
ifeq ($(CONFIG_TARGET_$(BOARD)_platform_ar10_vrx318),y)
        IFX_CFLAGS_y +=-DPLATFORM_AR10_VRX318 -DPLATFORM_VR9
else 
ifeq ($(CONFIG_TARGET_$(BOARD)_platform_ar10_GRX390),y)
 	IFX_CFLAGS_y +=-DPLATFORM_AR10_GRX390 -DPLATFORM_AR9 -DPLATFORM_AR10 -DPLATFORM_VR9
else
        IFX_CFLAGS_y +=-DPLATFORM_AR9 -DPLATFORM_AR10
endif
endif
endif

ifeq ($(CONFIG_TARGET_$(BOARD)_platform_hn1),y)
        IFX_CFLAGS_y +=-DPLATFORM_HN1
endif


ifeq ($(CONFIG_TARGET_$(BOARD)_platform_ar9_VB),y)
	        IFX_CFLAGS_y +=-DPLATFORM_VB300
	IFX_CFLAGS_y +=-DPLATFORM_VBX
endif

ifeq ($(CONFIG_IFX_MODEL_NAME),"AMAZON_SE_RT_HE_BASE_NOR_EXPERIMENTAL")
  IFX_CFLAGS_y += -DAMAZON_SE_RT_HE_BASE_NOR_EXPERIMENTAL
endif

IFX_CFLAGS_$(CONFIG_TARGET_UBI_MTD_SUPPORT) += -DCONFIG_TARGET_UBI_MTD_SUPPORT

IFX_CFLAGS_$(CONFIG_FEATURE_VDSL_VECTORING) += -DCONFIG_FEATURE_VDSL_VECTORING

IFX_CFLAGS_y += -DCONFIG_IFX_CONFIG_FLASH_SIZE=$(CONFIG_IFX_CONFIG_FLASH_SIZE)

IFX_CFLAGS_y +=-Os
IFX_CFLAGS_y +=-DCONFIG_PACKAGE_NTPCLIENT

IFX_LDFLAGS_y +=-L$(IFX_APIS_DIR)/lib
IFX_LDFLAGS_y +=-L$(STAGING_DIR)/usr/lib -Wl,-rpath-link,$(STAGING_DIR)/usr/lib

IFX_CFLAGS:=$(IFX_CFLAGS_y)
IFX_LDFLAGS:=$(IFX_LDFLAGS_y)

#### Export Section for Features & Others #######
export CONFIG_FEATURE_IFX_VOIP CONFIG_FEATURE_IFX_WIRELESS CONFIG_PACKAGE_IFX_DSL_CPE_API CONFIG_PACKAGE_ifx-voip
export CONFIG_FEATURE_IFX_VDSL2 CONFIG_PACKAGE_ifx-oam CONFIG_IFX_CONFIG_FLASH_SIZE CONFIG_PACKAGE_kmod-fs-nfs-root
export CONFIG_FEATURE_IFX_CONCURRENT_DUAL_WIRELESS CONFIG_FEATURE_CELL_WAN_SUPPORT CONFIG_FEATURE_WEB_REDIRECTOR

ifeq ($(CONFIG_FEATURE_WWAN_LTE_SUPPORT),y)
  export CONFIG_FEATURE_WWAN_LTE_SUPPORT
  export CONFIG_FEATURE_CELL_WAN_SUPPORT=y
endif

export IFX_DECT_SS_DIR=$(VOIP_DIR)
export HTTPD_COMMON_INCLUDES=$(STAGING_DIR)/usr/include/ifx_httpd/
export CONFIG_FEATURE_LTQ_PORT_WAN_BINDING CONFIG_FEATURE_LTQ_VLAN_SWITCH_PORT_ISOLATION
export CONFIG_FEATURE_LTQ_MCAST_FILTER_PORT

ifeq ($(CONFIG_FEATURE_IFX_HOST_ENVIR),y)
  export CONFIG_FEATURE_IFX_HOST_ENVIR
endif
ifeq ($(CONFIG_FEATURE_LTQ_WIRELESS_VB),y)
  export CONFIG_FEATURE_LTQ_WIRELESS_VB
endif
ifeq ($(CONFIG_FEATURE_LTQ_WIRELESS_STA_SUPPORT),y)
  export CONFIG_FEATURE_LTQ_WIRELESS_STA_SUPPORT
endif

### Define feature wise rootfs scripts to install in 'INSTALL_FILES_y' macro
INSTALL_FILES_y:=
INSTALL_FILES_y += ./all/common/*
INSTALL_FILES_y += ./all/css/*
INSTALL_FILES_y += ./all/js/*
INSTALL_FILES_$(CONFIG_FEATURE_SYSTEM_PERFORMANCE_CHARTS) += ./all/js_misc/*
INSTALL_FILES_y += ./dsl/* # have wild card check on dsl api/dsl cpe control package
INSTALL_FILES_y += ./firewall/common/*
INSTALL_FILES_$(IFX_DOS_ENABLE) += ./firewall/dos/*
INSTALL_FILES_$(CONFIG_PACKAGE_lq-igmpd) += ./igmp/*
INSTALL_FILES_$(CONFIG_FEATURE_IFX_IPSEC_TERMINATION) += ./ipsec/*
INSTALL_FILES_$(CONFIG_FEATURE_IPv6) += ./ipv6/*
INSTALL_FILES_y += ./lan/common/*
ifeq ($(shell [[ "A$(CONFIG_FEATURE_DHCP_SERVER)" == "Ay" || "A$(CONFIG_FEATURE_DHCP_RELAY_)" == "Ay" ]] && echo true),true)
	INSTALL_FILES_y += ./lan/dhcp/*
endif
INSTALL_FILES_y += ./misc/*
INSTALL_FILES_y += ./nat/common/*
INSTALL_FILES_y += ./nat/port_trigger/*
INSTALL_FILES_$(CONFIG_FEATURE_LTQ_PORT_WAN_BINDING) += ./nat/pwb/*
INSTALL_FILES_$(CONFIG_FEATURE_IFX_IPQOS) += ./qos/*
INSTALL_FILES_y += ./route/common/*
INSTALL_FILES_$(CONFIG_FEATURE_RIP) += ./route/rip/*
INSTALL_FILES_$(POLICY_ROUTING) += ./route/policy_route/*
INSTALL_FILES_y += ./system/common/*
INSTALL_FILES_y += ./system/snmp/* # SNMP wild card check
INSTALL_FILES_$(CONFIG_PACKAGE_ifx-devm) += ./tr69/*
INSTALL_FILES_$(CONFIG_FEATURE_IFX_VOIP) += ./voip/*
ifeq ($(shell [[ "A$(CONFIG_FEATURE_ADSL_WAN_SUPPORT)" == "Ay" || "A$(CONFIG_FEATURE_PTM_WAN_SUPPORT)" == "Ay" || \
"A$(CONFIG_FEATURE_ETH_WAN_SUPPORT)" == "Ay" ]] && echo true ),true)
#ifeq ($(CONFIG_FEATURE_ADSL_WAN_SUPPORT),y) || ($(CONFIG_FEATURE_PTM_WAN_SUPPORT),y) || ($(CONFIG_FEATURE_ETH_WAN_SUPPORT),y)
	INSTALL_FILES_y += ./wan/common/*
endif
ifeq ($(CONFIG_FEATURE_ADSL_WAN_SUPPORT),y)
	INSTALL_FILES_y += ./wan/atm_wan/*
endif
ifeq ($(CONFIG_FEATURE_PTM_WAN_SUPPORT),y)
	INSTALL_FILES_y += ./wan/ptm_wan/*
endif
ifeq ($(CONFIG_FEATURE_ETH_WAN_SUPPORT),y)
	INSTALL_FILES_y += ./wan/eth_wan/*
endif
#ifeq ($(CONFIG_FEATURE_WWAN_LTE_SUPPORT),y) || ($(CONFIG_FEATURE_CELL_WAN_SUPPORT),y)
ifeq ($(shell [[ "A$(CONFIG_FEATURE_WWAN_LTE_SUPPORT)" == "Ay" || "A$(CONFIG_FEATURE_CELL_WAN_SUPPORT)" == "Ay" ]] && echo true),true)
	INSTALL_FILES_y += ./wan/wwan/*
endif
INSTALL_FILES_$(CONFIG_FEATURE_ANY_WAN_SUPPORT) += ./all/anywan_demo/*
INSTALL_FILES_$(CONFIG_FEATURE_IFX_WIRELESS) += ./wlan/*


#### Target Rules ###############################
define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR) \
	$(if $(CONFIG_FEATURE_IFX_HOST_ENVIR),,\
		$(TARGET_CONFIGURE_OPTS)) \
	IFX_CFLAGS="$(IFX_CFLAGS)" IFX_LDFLAGS="$(IFX_LDFLAGS)"
endef

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/lib
	$(INSTALL_DIR) $(1)/usr
	$(INSTALL_DIR) $(STAGING_DIR)/usr/lib/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/libifx_httpd.so $(1)/lib/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/libifx_httpd.so $(STAGING_DIR)/usr/lib/.
	$(INSTALL_DIR) $(1)/www
	$(foreach w_file, $(INSTALL_FILES_y), \
		$(CP) $(PKG_BUILD_DIR)/web/$(w_file) $(1)/www/.; \
	)
	$(CP) $(PKG_BUILD_DIR)/web/all/images $(1)/www/.
	$(if $(CONFIG_PACKAGE_ifx-cli),$(CP) -af $(PKG_BUILD_DIR)/cli $(1)/usr/)
	ln -sf  ../tmp/rc.conf.gz $(1)/www/rc.conf.gz
endef

define Build/Clean
	if [ -d $(PKG_BUILD_DIR) ]; then $(MAKE) -C $(PKG_BUILD_DIR) clean; \
	$(RM) -rf $(PKG_BUILD_DIR)/ipkg-$(BOARD); fi
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
