#
# Openwrt Makefile for lq-wave-300
#
#### Includes ###################################
include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=lq-wave-300-3_2_0
PKG_VERSION:=3.2.0.62
PKG_RELEASE:=1
PKG_BASE_NAME:=lq_wave300_3_2_0
PKG_SOURCE_VERSION:=03.02.00.00.34.11544.full.wls.src
PKG_SOURCE:=lq-wave-300-$(PKG_SOURCE_VERSION).tar.bz2
PKG_SOURCE_DIR:=lq-wave-300-$(PKG_SOURCE_VERSION)
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_BASE_NAME)-$(PKG_VERSION)
PKG_BUILD_DEPENDS:=libnl
PKG_SOURCE_URL:=http://localhost
PKG_DEFAULT_CUSTOM_SOURCE_DIR:=$(TOPDIR)/../../../wlnsw/wireline_sw_wlan/wave_sw
PKG_NO_AUTORECONF:=y
WAVE300_FW_VER:=FW_3.2.0_r3997
WAVE300_PROGMODEL_VER:=11159
WAVE300_RDLIM_VER:=11001.3173
WAVE300_SCRIPTS_VER:=$(shell hg log -l1 -b default > /dev/null 2>&1 && hg log -l1 -b wave_3.2.0 | head -n1 | awk '{print $$2}')

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/package-version-override.mk

define Package/$(PKG_NAME)
	SECTION:=utils
	CATEGORY:=Lantiq
	TITLE:=Wave300 MBSSID support (Experimental)
	DEPENDS:= +kmod-ltqcpe_wlan_wave300_mbssid \
		@+FEATURE_IFX_WIRELESS \
		@+FEATURE_IFX_WIRELESS_WAVE300 \
		@+FEATURE_IFX_WLAN_WMM \
		@+FEATURE_IFX_WLAN_WPS \
		@+PACKAGE_ifx-devm:FEATURE_IFX_TR69_WIFILAN \
		@+PACKAGE_ifx-devm:FEATURE_IFX_TR69_WIFILAN_WPS \
		@+PACKAGE_ifx-devm:FEATURE_IFX_TR69_WIFILAN_WMM \
		@+BUSYBOX_CONFIG_WC \
		@+BUSYBOX_CONFIG_WHICH \
		@+BUSYBOX_CONFIG_AWK \
		@+BUSYBOX_CONFIG_FEATURE_AWK_MATH \
		@+BUSYBOX_CONFIG_FIND \
		@+BUSYBOX_CONFIG_FEATURE_FIND_MTIME \
		@+BUSYBOX_CONFIG_FEATURE_FIND_PERM \
		@+BUSYBOX_CONFIG_FEATURE_FIND_TYPE \
		@+BUSYBOX_CONFIG_FEATURE_FIND_XDEV \
		@+BUSYBOX_CONFIG_FEATURE_FIND_NEWER \
		@+BUSYBOX_CONFIG_FEATURE_FIND_INUM \
		@+BUSYBOX_CONFIG_XARGS \
		@+BUSYBOX_CONFIG_LSMOD \
		@+PACKAGE_libnl \
		@+PACKAGE_libopenssl \
		@+PACKAGE_libuuid \
		@+PACKAGE_uuidgen \
		@+PACKAGE_hostapd_0610 \
                @+PACKAGE_tinytcl
	URL:=http://www.lantiq.com
	MAINTAINER:=Lantiq Technologies
	MENU:=1
endef

define Package/$(PKG_NAME)/description
 This package contains wave300 scripts/binaries
endef

define KernelPackage/ltqcpe_wlan_wave300_mbssid
 SUBMENU:=Lantiq
 TITLE:=LTQ Support for Wave300 WLAN 
 DEPENDS:=@TARGET_ifxcpe_platform_ar9||TARGET_ifxcpe_platform_danube||TARGET_ifxcpe_platform_vr9||TARGET_ltqcpe_platform_ar9||TARGET_ltqcpe_platform_vr9||TARGET_ltqcpe_platform_danube||TARGET_ltqcpe_platform_ar10||TARGET_ltqcpe_platform_hn1
 KCONFIG:= \
         CONFIG_IFX_PCI=y \
         CONFIG_IFX_PCI_HW_SWAP=y \
         CONFIG_FW_LOADER=y \
         CONFIG_HOTPLUG=y \
	 
endef

define KernelPackage/ltqcpe_wlan_wave300_mbssid/description
  Kernel Support for Wave300 WLAN 
endef

#### Exports ###############################
export THISDIR = $(shell pwd)
export TARGET_CROSS

#### Target Rules ###############################
define Build/Prepare
	$(call Build/Prepare/Default)
	mv $(BUILD_DIR)/$(PKG_SOURCE_DIR) $(PKG_BUILD_DIR)/
	rm $(PKG_BUILD_DIR)/$(PKG_SOURCE_DIR)/tools/kconfig/*.o || true
	rm $(PKG_BUILD_DIR)/$(PKG_SOURCE_DIR)/tools/kconfig/*.d || true
	rm $(PKG_BUILD_DIR)/$(PKG_SOURCE_DIR)/tools/kconfig/.config || true
	cp files/config/ugw-linux26.env $(PKG_BUILD_DIR)/$(PKG_SOURCE_DIR)/support/
	cp files/config/ugw.config $(PKG_BUILD_DIR)/$(PKG_SOURCE_DIR)/
	cp files/config/MTLKConfig $(PKG_BUILD_DIR)/$(PKG_SOURCE_DIR)/
	sh -c "cd $(PKG_BUILD_DIR)/$(PKG_SOURCE_DIR); patch -p0 < $(THISDIR)/files/config/relocation_overflow_fix.patch; $(MAKE) defconfig PWD=$(PKG_BUILD_DIR)/$(PKG_SOURCE_DIR)/ MINICONFIG=ugw.config APP_TOOLCHAIN_DIR=$(TOPDIR) KERNEL_DIR=$(LINUX_DIR) CPPFLAGS=\"-I$(STAGING_DIR)/usr/include\" LDFLAGS=\"-L$(STAGING_DIR)/usr/lib -lnl\""
endef

define Build/Compile
	sh -c "cd $(PKG_BUILD_DIR)/$(PKG_SOURCE_DIR) && $(MAKE) APP_TOOLCHAIN_DIR=$(TOPDIR) KERNEL_DIR=$(LINUX_DIR)"
endef

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/ramdisk_copy/flash/
	$(INSTALL_DIR) $(1)/lib/modules/$(LINUX_VERSION)/net/
	$(INSTALL_DIR) $(1)/etc/rc.d/
	$(INSTALL_DIR) $(1)/bin/
	$(INSTALL_DIR) $(1)/sbin/
	$(INSTALL_DIR) $(1)/root/mtlk/images/
	$(INSTALL_DIR) $(1)/etc/hotplug/
	$(INSTALL_DIR) $(1)/etc/hotplug.d/
	$(CP) ./files/root/mtlk/images/* $(1)/root/mtlk/images/
	$(CP) ./files/ramdisk_copy/* $(1)/ramdisk_copy/flash/
	$(CP) $(PKG_BUILD_DIR)/$(PKG_SOURCE_DIR)/builds/ugw-linux26/binaries/wls/driver/mtlk.ko $(1)/lib/modules/$(LINUX_VERSION)/net/
	chmod 555 $(1)/lib/modules/$(LINUX_VERSION)/net/mtlk.ko
	$(CP) ./files/scripts/layer_2_actions/* $(1)/etc/rc.d/
	$(CP) ./files/scripts/layer_3_components/*  $(1)/etc/rc.d/
	$(CP) ./files/scripts/host_api/*.csv $(1)/bin/
	$(CP) ./files/scripts/host_api/host_api $(1)/bin/
	$(CP) ./files/scripts/host_api/print2log $(1)/bin/
	$(CP) $(PKG_BUILD_DIR)/$(PKG_SOURCE_DIR)/builds/ugw-linux26/tools/mttools/drvhlpr/drvhlpr $(1)/bin/	|| echo "Skipping drvhlpr install"
	$(CP) $(PKG_BUILD_DIR)/$(PKG_SOURCE_DIR)/builds/ugw-linux26/tools/mttools/mtdump/mtdump $(1)/bin/ || echo "Skipping mtdump install"
	$(CP) $(PKG_BUILD_DIR)/$(PKG_SOURCE_DIR)/builds/ugw-linux26/tools/mttools/mtlk_cli/mtlk_cli $(1)/bin/ || echo "Skipping mtlk_cli install"
	$(CP) $(PKG_BUILD_DIR)/$(PKG_SOURCE_DIR)/builds/ugw-linux26/binaries/wls/mttools/BclSockServer $(1)/bin || echo "Skipping BclSockServer install"
	chmod 555 $(1)/bin/drvhlpr || true
	chmod 555 $(1)/bin/mtdump || true
	chmod 555 $(1)/bin/mtlk_cli || true
	chmod 555 $(1)/bin/BclSockServer || true
	$(CP) ./files/binaries/sbin/* $(1)/sbin/
	$(CP) ./files/etc/* $(1)/etc/
	echo -e "wave_release_minor=\"$(PKG_VERSION)\"\nwave_mac_ver=\"$(WAVE300_FW_VER)\"\nwave_progmodel_ver=\"$(WAVE300_PROGMODEL_VER)\"\nwave_rdlim_ver=\"$(WAVE300_RDLIM_VER)\"\nwave_driver_ver=\"$(PKG_SOURCE_VERSION)\"" > $(1)/etc/wave_components.ver
	[ ! -z $WAVE300_SCRIPTS_VER ] && echo -e "wave_scripts_ver=\"$(WAVE300_SCRIPTS_VER)\"" >> $(1)/etc/wave_components.ver
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
$(eval $(call KernelPackage,ltqcpe_wlan_wave300_mbssid))
