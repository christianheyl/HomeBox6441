#
# Copyright (C) 2009 OpenWrt.org
# Copyright (C) 2011 Lantiq
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=gpon-base-files
PKG_RELEASE:=3
PKG_BUILD_DEPENDS:=uboot-envtools

include $(INCLUDE_DIR)/package.mk

ifeq ($(DUMP),)
	STAMP_CONFIGURED:=$(strip $(STAMP_CONFIGURED))_$(shell $(SH_FUNC) grep '^CONFIG_GPON_' $(TOPDIR)/.config | md5s)
endif

define Package/$(PKG_NAME)
  SUBMENU:=Base Files
  SECTION:=base
  CATEGORY:=Lantiq
  TITLE:=Common base files for the GPON.
  URL:=http://www.lantiq.com/
  MAINTAINER:=Lantiq
  DEPENDS:=+uboot-envtools +@BUSYBOX_CONFIG_UUDECODE +@BUSYBOX_CONFIG_UUENCODE +@BUSYBOX_CONFIG_TR +@BUSYBOX_CONFIG_TAR
endef

define Package/$(PKG_NAME)/conffiles
/etc/config/gpon
/etc/config/omci
endef

IMAGE_NAME=$(strip $(subst ",,$(CONFIG_GPON_IMAGE_NAME)))
IMAGE_REVISION=$(strip $(subst ",,$(CONFIG_GPON_IMAGE_REVISION)))
OMCI_MIB=$(strip $(subst ",,$(CONFIG_GPON_OMCI_MIB)))

define Package/$(PKG_NAME)/description
 This package contains files and system scripts for OpenWrt.
endef

define Package/$(PKG_NAME)/config
	menu "configuration"
	depends on PACKAGE_gpon-base-files
	config GPON_IMAGE_NAME
		string
		prompt "Image Name"
		default "Lantiq GPON Evaluation System (MDU)"
	config GPON_IMAGE_REVISION
		string
		prompt "Image Revision"
		default "0"
	config GPON_OMCI_MIB
		string
		prompt "Default MIB"
		default "sfu_voice"
	endmenu
endef

define Build/Prepare
endef

define Build/Compile
endef

define Package/$(PKG_NAME)/install
	[ -d ./files ] && $(CP) ./files/* $(1)/
	$(SED) 's,$$$$R,$(REVISION),g' $(1)/etc/banner
	$(SED) 's,$$$$N,$(IMAGE_NAME),g' $(1)/opt/lantiq/image_version
	$(SED) 's,$$$$R,$(IMAGE_REVISION),g' $(1)/opt/lantiq/image_version
	$(SED) 's,$$$$M,$(OMCI_MIB),g' $(1)/etc/config/omci
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
