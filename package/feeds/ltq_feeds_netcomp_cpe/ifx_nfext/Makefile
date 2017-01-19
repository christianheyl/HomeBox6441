# 
# Openwrt Makefile for ifx-nfext
#

#### Includes ###################################
include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

#### Package Info ###############################
PKG_NAME:=ifx-nfext
PKG_VERSION:=2.3.15.0
PKG_RELEASE:=1
PKG_MD5SUM:=7e66fccc2448921219e26ca21c1a6817
PKG_BASE_NAME:=ifx_nfext
PKG_SOURCE:=$(PKG_BASE_NAME)-$(PKG_VERSION).tar.gz
PKG_CAT:=zcat
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_BASE_NAME)-$(PKG_VERSION)
PKG_SOURCE_URL:=http://localhost
#PKG_FILE_DEPENDS:=$(STAGING_DIR)/usr/include/ifx_config.h
PKG_DEFAULT_CUSTOM_SOURCE_DIR:=$(TOPDIR)/../../../wlnsw/wireline_sw_router/ifx_nfext
PKG_NO_AUTORECONF=y

PKG_CONFIG_DEPENDS_REBUILD=y
PKG_CONFIG_DEPENDS:=\
	CONFIG_FULL_PACKAGE \
	CONFIG_IFX_ADDON \
	CONFIG_IFX_NFEXT_AMAZON_SE_SWITCH_PHYPORT \
	CONFIG_IFX_NFEXT_DANUBE_SWITCH_PHYPORT \
	CONFIG_IFXNFEXT \
	CONFIG_IFX_NFEXT_VBRIDGE \
	CONFIG_PPP_IFX_IDLETIME_EXTENSION

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/package-version-override.mk

define KernelPackage/$(PKG_NAME)
	SUBMENU:=Lantiq
	TITLE:=VLAN Support driver
	DEPENDS:=@LINUX_2_6
	FILES:=$(PKG_BUILD_DIR)/*.$(LINUX_KMOD_SUFFIX)
	KCONFIG:=CONFIG_PPP_IFX_IDLETIME_EXTENSION=m
	MENU:=1
endef

define KernelPackage/$(PKG_NAME)/description
	Kernel module to support VLAN
endef

define KernelPackage/$(PKG_NAME)/config
        menu "custom source directory"
               depends on PACKAGE_kmod-$(PKG_NAME)
        config $(PKG_NAME)_USE_CUSTOM_SOURCE_DIR
                depends on PACKAGE_kmod-$(PKG_NAME)
                bool "Use custom source directory"
                default n
        config $(PKG_NAME)_CUSTOM_SOURCE_DIR
                depends on $(PKG_NAME)_USE_CUSTOM_SOURCE_DIR
                string "Custom source directory"
                default "$(PKG_DEFAULT_CUSTOM_SOURCE_DIR)"
        endmenu
endef

### Configurations depends on Menuconfig ########
EXTRA_KCONFIG:= \
	CONFIG_IFXNFEXT=m

EXTRA_CFLAGS:= \
	$(patsubst CONFIG_%, -DCONFIG_%=1, $(patsubst %=m,%,$(filter %=m,$(EXTRA_KCONFIG)))) \
	$(patsubst CONFIG_%, -DCONFIG_%=1, $(patsubst %=y,%,$(filter %=y,$(EXTRA_KCONFIG))))

MAKE_OPTS:= \
	ARCH="$(LINUX_KARCH)" \
	CROSS_COMPILE="$(TARGET_CROSS)" \
	SUBDIRS="$(PKG_BUILD_DIR)" \
	EXTRA_CFLAGS="$(EXTRA_CFLAGS)" \
	LINUXINCLUDE="-I$(LINUX_DIR)/include -include linux/autoconf.h" \
	$(EXTRA_KCONFIG)

IFX_CFLAGS_y:=
IFX_LDFLAGS_y:=

IFX_CFLAGS_y +=$(TARGET_CFLAGS) -Wall -Werror
IFX_CFLAGS_y +=-I$(STAGING_DIR)/usr/include

IFX_CFLAGS:=$(IFX_CFLAGS_y)
IFX_LDFLAGS:=$(IFX_LDFLAGS_y)

#### Export Section for Features & Others #######
export CONFIG_FULL_PACKAGE CONFIG_IFX_ADDON CONFIG_PPP_IFX_IDLETIME_EXTENSION CONFIG_IFX_NFEXT_VBRIDGE
export CONFIG_IFX_NFEXT_AMAZON_SE_SWITCH_PHYPORT CONFIG_IFX_NFEXT_DANUBE_SWITCH_PHYPORT

#### Target Rules ###############################
define Build/Prepare
$(call Build/Prepare/Default)
	$(CP) $(PKG_BUILD_DIR)/Makefile.kernel $(PKG_BUILD_DIR)/Makefile
endef

define Build/Compile
	$(MAKE) \
		$(TARGET_CONFIGURE_OPTS) \
		IFX_CFLAGS="$(IFX_CFLAGS)" \
		IFX_LDFLAGS="$(IFX_LDFLAGS)" \
		-C $(PKG_BUILD_DIR) -f Makefile.user all
	$(MAKE) -C "$(LINUX_DIR)" \
		$(MAKE_OPTS) \
		modules
endef

define KernelPackage/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/vlanctl $(1)/usr/sbin/
	$(call Package/$(PKG_NAME)/install/Default,$(1))
endef

define Build/Clean
	if [ -d $(PKG_BUILD_DIR) ]; then $(MAKE) -C $(PKG_BUILD_DIR) clean; \
	$(RM) -r $(PKG_BUILD_DIR)/ipkg-$(BOARD); fi
endef

$(eval $(call KernelPackage,$(PKG_NAME)))
