#
# Copyright (C) 2011 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=drv_tapi
PKG_VERSION:=4.7.3.4
PKG_RELEASE:=4

PKG_SOURCE:=drv_tapi-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=http://mirror2.openwrt.org/sources
PKG_MD5SUM:=196891177fa24ae7cce236c39e4f4af1

include $(INCLUDE_DIR)/package.mk

define KernelPackage/ltq-tapi
  SUBMENU:=Voice over IP
  TITLE:=Lantiq TAPI subsystem
  URL:=http://www.lantiq.com/
  DEPENDS:=+kmod-ifxos @TARGET_lantiq @!TARGET_lantiq_ase
  FILES:=$(PKG_BUILD_DIR)/src/drv_tapi.ko
  AUTOLOAD:=$(call AutoLoad,20,drv_tapi)
  MAINTAINER:=John Crispin <blogic@openwrt.org>
endef

ifeq ($(CONFIG_VOICE_CPE_TAPI_LINUX_HOTPLUG),y)
GPIO_TAPI_HOOK_STATUS0=$(strip $(subst ",,$(CONFIG_VOICE_CPE_TAPI_GPIO_HOOK_STATUS0)))
GPIO_TAPI_HOOK_STATUS1=$(strip $(subst ",,$(CONFIG_VOICE_CPE_TAPI_GPIO_HOOK_STATUS1)))
endif

define KernelPackage/ltq-tapi/description
	Voice Subsystem Telephony API High Level Driver
endef

define KernelPackage/ltq-tapi/config
	source "$(SOURCE)/Config.in"
endef

CONFIGURE_ARGS += \
	ARCH=$(LINUX_KARCH) \
	--enable-linux-26 --enable-tapi3 \
	--enable-kernelbuild="$(LINUX_DIR)" \
	--enable-kernelincl="$(LINUX_DIR)/include" \
	--with-ifxos-incl=$(STAGING_DIR)/usr/include/ifxos \
	--with-ifxos-lib=$(STAGING_DIR)/usr/lib \
	$(call autoconf_bool,CONFIG_IFX_DRV_TAPI_EVENT_LOGGER,el-debug) \
	$(call autoconf_bool,CONFIG_VOICE_CPE_TAPI_FAX,fax t38) \
	$(call autoconf_bool,CONFIG_VOICE_CPE_TAPI_CID,cid) \
	$(call autoconf_bool,CONFIG_VOICE_CPE_TAPI_DECT,dect) \
	$(call autoconf_bool,CONFIG_VOICE_CPE_TAPI_KPI,kpi) \
	$(call autoconf_bool,CONFIG_VOICE_CPE_TAPI_KPI,kpi-tasklet) \
	$(call autoconf_bool,CONFIG_VOICE_CPE_TAPI_QOS,qos) \
	$(call autoconf_bool,CONFIG_VOICE_CPE_TAPI_LT_GR909,lt) \
	$(call autoconf_bool,CONFIG_VOICE_CPE_TAPI_STATISTICS,statistics) \
	$(call autoconf_bool,CONFIG_VOICE_CPE_TAPI_METERING,metering) \
	$(call autoconf_bool,CONFIG_VOICE_CPE_TAPI_HDLC,hdlc) \
	$(call autoconf_bool,CONFIG_VOICE_CPE_TAPI_TRACES,trace) \
	$(call autoconf_bool,CONFIG_VOICE_CPE_TAPI_LINUX_HOTPLUG,hotplug)

define Build/Configure
	(cd $(PKG_BUILD_DIR); aclocal -I m4 && autoconf && automake)
	$(call Build/Configure/Default)
endef

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/include/drv_tapi
	$(CP) --dereference $(PKG_BUILD_DIR)/include/* $(1)/usr/include/drv_tapi
	(cd $(1)/usr/include/drv_tapi && ln -s ../ifxos/ifx_types.h .)
endef

define KernelPackage/ltq-tapi/install
	$(INSTALL_DIR) $(1)/etc/config
ifeq ($(CONFIG_VOICE_CPE_TAPI_LINUX_HOTPLUG),y)
	$(INSTALL_DATA) ./files/etc/config/tapi $(1)/etc/config/tapi
	$(SED) 's,@GPIO_TAPI_HOOK_STATUS0@,$(GPIO_TAPI_HOOK_STATUS0),g' $(1)/etc/config/tapi
	$(SED) 's,@GPIO_TAPI_HOOK_STATUS1@,$(GPIO_TAPI_HOOK_STATUS1),g' $(1)/etc/config/tapi
	mkdir -p $(1)/etc/hotplug.d/tapi
	$(INSTALL_DATA) ./files/10-tapi $(1)/etc/hotplug.d/tapi/10-tapi
endif
endef

$(eval $(call KernelPackage,ltq-tapi))
