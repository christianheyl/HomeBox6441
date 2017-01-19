#
# Openwrt Makefile for ifx-dect-demo
#

#### Includes ###################################
include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

#### Package Info ###############################
PKG_NAME:=ifx-dect-demo
PKG_VERSION:=3.1.1.3
PKG_RELEASE:=1
#PKG_MD5SUM:=27a19252419b288fd35382397e65c1ed
PKG_BASE_NAME:=ifx_dect_demo
PKG_SOURCE:=$(PKG_BASE_NAME)-$(PKG_VERSION).tar.gz
PKG_CAT:=zcat
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_BASE_NAME)-$(PKG_VERSION)
PKG_SOURCE_URL:=http://localhost
PKG_NO_AUTORECONF:=y
PKG_DEFAULT_CUSTOM_SOURCE_DIR:=$(TOPDIR)/../../../wlnsw/wireline_sw_dect/
PKG_VOIP_DIR:=$(TOPDIR)/../../../wlnsw/wireline_sw_voip/
include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/package-version-override.mk



define Package/$(PKG_NAME)
	SECTION:=utils
	CATEGORY:=Lantiq
	SUBMENU:=VOIP and DECT Subsystem
	TITLE:=Lantiq Dect Demo Package for GW 4.2 onwards
	URL:=http://www.lantiq.com
	MAINTAINER:=Lantiq
  DEPENDS:=+ifx-voice-cpe-tapi +teridian-voice-daa-ter1x66 +ifx-voice-cpe-vmmc @+LTQ_VOICE_CPE_TAPI_DECT @+LTQ_VOICE_CPE_TAPI_KPI @TARGET_ltqcpe_platform_ar9||TARGET_ltqcpe_platform_vr9||TARGET_ltqcpe_platform_danube 
#@!PACKAGE_ifx-voice-cpe-tapidemo
	MENU:=1
endef

define Package/$(PKG_NAME)/description
	This ifx-dect-demo package is an application used to realize DECT functionality on GW 4.2 onwards.
endef

#### Directory Paths ############################
#UBOOT_DIR:=$(KERNEL_BUILD_DIR)/open_uboot/open_uboot
#TAPI_DIR:=$(wildcard $(BUILD_DIR)/drv_tapi*)
#VMMC_DIR:=$(wildcard $(BUILD_DIR)/drv_vmmc*)
#TAPI_DIR:=$(BUILD_DIR)/drv_tapi-3.10.1
#export IFX_PRODUCT_ROOT=$(PKG_BUILD_DIR)/ifx_ft_demo/

IFX_UBOOT_DIR:=$(KERNEL_BUILD_DIR)/open_uboot/open_uboot
IFX_TAPI_DIR:=$(wildcard $(KERNEL_BUILD_DIR)/drv_tapi*)
IFX_VMMC_DIR:=$(wildcard $(KERNEL_BUILD_DIR)/drv_vmmc*)
DSL_API_DIR:=$(wildcard $(BUILD_DIR)/drv_dsl_cpe_api*)
DSL_CONTROL_DIR:=$(wildcard $(BUILD_DIR)/dsl_cpe_control*)
MAPI_DIR:=$(wildcard $(STAGING_DIR)/usr/include/ifx_IFXAPIs)

export UBOOT_DIR=$(IFX_UBOOT_DIR)
export TAPI_DIR=$(IFX_TAPI_DIR)/include
export VMMC_DIR=$(IFX_VMMC_DIR)/include
export IFX_PRODUCT_ROOT=$(PKG_BUILD_DIR)/ifx_ft_demo/
export IFX_MAPI_DIR=$(MAPI_DIR)
#export IFX_DSL_API_DIR=$(DSL_API_DIR)
export IFX_DSL_API_DIR=$(BUILD_DIR)/drv_dsl_cpe_api
export IFX_DSL_CONTROL_DIR=$(DSL_CONTROL_DIR)
export IFX_HTTPD_DIR=$(wildcard $(STAGING_DIR)/usr/include/ifx_httpd)
#export IFX_OAM_DIR=$(BUILD_DIR)/ifx_oam
export IFX_STAGING_DIR=$(STAGING_DIR)



### Menuconfig ##################################
#define Package/$(PKG_NAME)/config
#	menu "$(PKG_NAME) features"
#	source "$(SOURCE)/config/Config.in"
#	endmenu
#endef


### Configurations depends on Menuconfig ########
IFX_CFLAGS_y:=
IFX_LDFLAGS_y:=

IFX_CFLAGS_y +=$(TARGET_CFLAGS) -Wall

IFX_CFLAGS_y += -DIFX_CONFIG_MEMORY_SIZE=$(CONFIG_IFX_CONFIG_MEMORY_SIZE) -DIFX_CONFIG_FLASH_SIZE=${CONFIG_IFX_CONFIG_FLASH_SIZE} -DCPU_${CONFIG_IFX_CONFIG_CPU} -DCONFIG_AMAZON_S

IFX_CFLAGS:=$(IFX_CFLAGS_y)
IFX_LDFLAGS:=$(IFX_LDFLAGS_y)

EXTRA_CFLAGS :=  -I$(IFX_PRODUCT_ROOT)/ifx_pagebutton_drv
EXTRA_CFLAGS += -I$(KERNEL)/include 

CFLAGS +=  -DLINUX -Wall -DDECT_DEMO -O2 -g -Wunused -Wpointer-arith 

ifdef CONFIG_TARGET_ltqcpe_platform_ar9
CFLAGS += -DCONFIG_AR9
endif

ifdef CONFIG_TARGET_ltqcpe_platform_danube 
CFLAGS += -DCONFIG_DANUBE
endif

ifdef CONFIG_TARGET_ltqcpe_platform_vr9 
CFLAGS += -DCONFIG_VR9
endif

ifdef CONFIG_TARGET_ltqcpe_platform_amazon_se
CFLAGS += -DCONFIG_AMAZON_S
endif

ifeq ($(CONFIG_IFX_MODEL_NAME),"ARX182_GW_EL_FXS_DECT")
FW_PATH=ramdisk_copy/flash/
CFLAGS += -DCOSIC_BMC_FW_ON_RAM
else
FW_PATH=opt/ifx/downloads/
endif
export CONFIGURE_ARGS

#### Target Rules ###############################
define Build/Prepare
$(call Build/Prepare/Default)
	$(INSTALL_DIR) $(IFX_PRODUCT_ROOT)/bin
	$(INSTALL_DIR) $(IFX_PRODUCT_ROOT)/lib
	ln -sf $(PKG_BUILD_DIR)/ifx_stack_toolkit $(PKG_BUILD_DIR)/ifx_ft_demo/ifx_stack_toolkit
	ln -sf $(PKG_BUILD_DIR)/ifx_cosic_driver $(PKG_BUILD_DIR)/ifx_ft_demo/ifx_cosic_driver
	ln -sf $(PKG_BUILD_DIR)/ifx_catiq_stack $(PKG_BUILD_DIR)/ifx_ft_demo/ifx_catiq_stack
	ln -sf $(PKG_BUILD_DIR)/ifx_pagebutton_drv $(PKG_BUILD_DIR)/ifx_ft_demo/ifx_pagebutton_drv
	ln -sf $(PKG_BUILD_DIR)/ifx_timer $(PKG_BUILD_DIR)/ifx_ft_demo/ifx_timer

	if [ ! -d $(IFX_PRODUCT_ROOT)/../ifx_timer_driver ] ; then \
		ln -sf $(PKG_VOIP_DIR)/ifx_timer_driver $(PKG_BUILD_DIR)/ifx_ft_demo/ifx_timer_driver ;\
	fi; 
	if [ -d $(IFX_PRODUCT_ROOT)/../ifx_timer_driver ] ; then \
		ln -sf $(PKG_BUILD_DIR)/ifx_timer_driver $(PKG_BUILD_DIR)/ifx_ft_demo/ifx_timer_driver ;\
	fi;

	if [ ! -d $(IFX_PRODUCT_ROOT)/../ifx_common ] ; then \
		ln -sf $(PKG_VOIP_DIR)/ifx_common $(PKG_BUILD_DIR)/ifx_ft_demo/ifx_common ;\
	fi;
	if [ -d $(IFX_PRODUCT_ROOT)/../ifx_common ] ; then \
		ln -sf $(PKG_BUILD_DIR)/ifx_common $(PKG_BUILD_DIR)/ifx_ft_demo/ifx_common ;\
	fi;

	ln -sf $(PKG_BUILD_DIR)/ifx_dsaa $(PKG_BUILD_DIR)/ifx_ft_demo/ifx_dsaa
	ln -sf $(IFX_TAPI_DIR) $(PKG_BUILD_DIR)/ifx_ft_demo/tapi

	ln -sf $(IFX_TAPI_DIR) $(IFX_TAPI_DIR)/drv_tapi
	ln -sf $(IFX_VMMC_DIR) $(IFX_TAPI_DIR)/drv_vmmc

#	rm -f $(IFX_PRODUCT_ROOT)/lib/*
#	rm -f $(IFX_PRODUCT_ROOT)/bin/*
#	find $(IFX_PRODUCT_ROOT)/ -name *.o|xargs rm -f
#	find $(IFX_PRODUCT_ROOT)/ -name *.a|xargs rm -f
#	find $(IFX_PRODUCT_ROOT)/ifx_stack_toolkit/ -name *.o|xargs rm -f
#	find $(IFX_PRODUCT_ROOT)/ifx_stack_toolkit/ -name *.a|xargs rm -f
#	find $(IFX_PRODUCT_ROOT)/ifx_common/ -name *.o|xargs rm -f
#	find $(IFX_PRODUCT_ROOT)/ifx_common/ -name *.a|xargs rm -f

	$(INSTALL_DIR) $(IFX_PRODUCT_ROOT)/bin
	$(INSTALL_DIR) $(IFX_PRODUCT_ROOT)/lib
endef


define Build/Compile

	echo $(CFLAGS)

	$(MAKE) ARCH=$(LINUX_KARCH) $(TARGET_CONFIGURE_OPTS) -C $(LINUX_DIR) M=$(IFX_PRODUCT_ROOT)/ifx_timer_driver modules
	cp -rf $(IFX_PRODUCT_ROOT)/ifx_timer_driver/ifx_voip_timer_driver.ko $(IFX_PRODUCT_ROOT)/bin/voip_timer_driver

	$(MAKE) ARCH=$(LINUX_KARCH) $(TARGET_CONFIGURE_OPTS) -C $(LINUX_DIR) M=$(IFX_PRODUCT_ROOT)/ifx_timer modules
	cp -rf $(IFX_PRODUCT_ROOT)/ifx_timer/drv_timer.ko $(IFX_PRODUCT_ROOT)/bin/

	$(MAKE) ARCH=$(LINUX_KARCH) $(TARGET_CONFIGURE_OPTS) -C $(LINUX_DIR) M=$(IFX_PRODUCT_ROOT)/ifx_pagebutton_drv modules IFX_CFLAGS="$(IFX_CFLAGS)"
	cp -rf $(IFX_PRODUCT_ROOT)/ifx_pagebutton_drv/danube_paging.ko $(IFX_PRODUCT_ROOT)/bin/paging.ko

	$(MAKE) ARCH=$(LINUX_KARCH) $(TARGET_CONFIGURE_OPTS) -C $(LINUX_DIR) M=$(IFX_PRODUCT_ROOT)/ifx_cosic_driver modules IFX_CFLAGS="$(CFLAGS)" IFX_VOIP_COMMON_DIR=$(IFX_PRODUCT_ROOT) TAPI_DIR=$(TAPI_DIR) KBUILD_VERBOSE=1

	cp -rf $(IFX_PRODUCT_ROOT)/ifx_cosic_driver/cosic.ko $(IFX_PRODUCT_ROOT)/bin/drv_dect.ko

	$(MAKE) LINUX_DIR="$(LINUX_DIR)" $(TARGET_CONFIGURE_OPTS) -C $(PKG_BUILD_DIR)/ifx_ft_demo/ all IFX_CFLAGS="$(IFX_CFLAGS)" IFX_LDFLAGS="$(IFX_LDFLAGS)" IFX_VOIP_COMMON_DIR=$(IFX_PRODUCT_ROOT)

	if [ ! -f $(IFX_PRODUCT_ROOT)/ifx_tapidect_demo/Makefile ] ; then \
	$(MAKE) LINUX_DIR="$(LINUX_DIR)" $(TARGET_CONFIGURE_OPTS) -C $(PKG_BUILD_DIR)/ifx_ft_demo/ifx_tapidect_demo -f MakefileTAPI all IFX_CFLAGS="$(IFX_CFLAGS)" IFX_LDFLAGS="$(IFX_LDFLAGS)" IFX_VOIP_COMMON_DIR=$(IFX_PRODUCT_ROOT) ;\
	fi;

	$(MAKE) LINUX_DIR="$(LINUX_DIR)" $(TARGET_CONFIGURE_OPTS) -C $(PKG_BUILD_DIR)/ifx_ft_demo/ifx_tapidect_demo install IFX_CFLAGS="$(IFX_CFLAGS)" IFX_LDFLAGS="$(IFX_LDFLAGS)" IFX_VOIP_COMMON_DIR=$(IFX_PRODUCT_ROOT)
	cp -rf $(PKG_BUILD_DIR)/ifx_ft_demo/ifx_tapidect_demo/src/tapidemo $(PKG_BUILD_DIR)/ifx_ft_demo/bin/tapidemo_dect
endef

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_DIR) $(1)/usr/drivers
	$(INSTALL_DIR) $(1)/lib/modules
	$(INSTALL_DIR) $(1)/$(FW_PATH)
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/ifx_ft_demo/bin/DectApp $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/ifx_ft_demo/ifx_demoagent/src/DectCli $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/ifx_ft_demo/bin/tapidemo_dect  $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/ifx_ft_demo/dect_demo.sh  $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/ifx_catiq_stack/COSICFw.BIN  $(1)/$(FW_PATH)
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/ifx_catiq_stack/BMCFw.BIN  $(1)/$(FW_PATH)
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/ifx_ft_demo/bin/drv_timer.ko  $(1)/usr/drivers
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/ifx_ft_demo/bin/drv_dect.ko   $(1)/usr/drivers
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/ifx_ft_demo/bin/paging.ko  $(1)/usr/drivers/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/ifx_ft_demo/bin/voip_timer_driver   $(1)/lib/modules/
#	$(INSTALL_DIR) $(1)/etc/init.d
#	$(INSTALL_BIN) ./files/ifx_load_dect_demo.sh $(1)/etc/init.d/
endef

define Build/Clean

	$(MAKE) -C $(LINUX_DIR) M=$(IFX_PRODUCT_ROOT)/ifx_timer_driver  clean

	$(MAKE) -C $(LINUX_DIR) M=$(IFX_PRODUCT_ROOT)/ifx_timer  clean

	$(MAKE) -C $(LINUX_DIR) M=$(IFX_PRODUCT_ROOT)/ifx_pagebutton_drv  IFX_CFLAGS="$(IFX_CFLAGS)" clean

	$(MAKE) -C $(LINUX_DIR) M=$(IFX_PRODUCT_ROOT)/ifx_cosic_driver TAPI_DIR=$(TAPI_DIR) IFX_VOIP_COMMON_DIR=$(IFX_PRODUCT_ROOT) clean

	$(MAKE) LINUX_DIR="$(LINUX_DIR)" $(TARGET_CONFIGURE_OPTS) -C $(PKG_BUILD_DIR)/ifx_ft_demo/ clean IFX_CFLAGS="$(IFX_CFLAGS)" IFX_LDFLAGS="$(IFX_LDFLAGS)" IFX_VOIP_COMMON_DIR=$(IFX_PRODUCT_ROOT)

	$(MAKE) LINUX_DIR="$(LINUX_DIR)" $(TARGET_CONFIGURE_OPTS) -C $(PKG_BUILD_DIR)/ifx_ft_demo/ifx_tapidect_demo distclean IFX_CFLAGS="$(IFX_CFLAGS)" IFX_LDFLAGS="$(IFX_LDFLAGS)" IFX_VOIP_COMMON_DIR=$(IFX_PRODUCT_ROOT)

endef
$(eval $(call BuildPackage,$(PKG_NAME)))
