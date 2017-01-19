#
# Copyright (C) 2015 Arcadyan Corporation
# All Rights Reserved.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=libnetfilter_log
PKG_VERSION:=1.0.1
PKG_SNAPSHOT:=20150716

PKG_SOURCE_URL:=http://ftp.netfilter.org/pub/$(PKG_NAME)/snapshot/
PKG_SOURCE:=$(PKG_NAME)-$(PKG_SNAPSHOT).tar.bz2
PKG_CAT:=bzcat

PKG_FIXUP:=autoreconf

PKG_BUILD_DIR := $(BUILD_DIR)/$(PKG_NAME)-$(PKG_SNAPSHOT)

PKG_INSTALL:=1

include $(INCLUDE_DIR)/package.mk

define Package/$(PKG_NAME)
  SECTION:=libs
  CATEGORY:=Libraries
  TITLE:=netfilter log library
  DEPENDS:=+libnfnetlink +kmod-nf-conntrack-netlink
endef

define Package/$(PKG_NAME)/description
 This package contains a library for applications dealing with netfilter log.
endef

TARGET_CFLAGS += $(FPIC)

CONFIGURE_ARGS += \
        --enable-static \
        --enable-shared \

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/include/$(PKG_NAME)
	$(CP) \
		$(PKG_INSTALL_DIR)/usr/include/$(PKG_NAME)/*.h \
		$(1)/usr/include/$(PKG_NAME)/

	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) \
		$(PKG_INSTALL_DIR)/usr/lib/$(PKG_NAME).{so*,a,la} \
		$(1)/usr/lib/

	$(INSTALL_DIR) $(1)/usr/lib/pkgconfig
	$(CP) \
		$(PKG_INSTALL_DIR)/usr/lib/pkgconfig/$(PKG_NAME).pc \
		$(1)/usr/lib/pkgconfig/
endef


define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) \
		$(PKG_INSTALL_DIR)/usr/lib/$(PKG_NAME).so.* \
		$(1)/usr/lib/
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
