#
# Copyright (C) 2009 Arcadyan Corporation
# All Rights Reserved.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=ulogd
PKG_RELEASE:=1

PKG_BUILD_DIR := $(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk

define Package/ulogd
  SECTION:=utils
  CATEGORY:=Utilities
  TITLE:=ulog daemon for WAN traffic trace
  DEPENDS:=
endef

define Package/ulogd/description
 This package contains a ulog daemon program for capturing WAN port traffic.
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Build/Configure
endef

define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR) \
					CC="$(TARGET_CC)" LD="$(TARGET_CROSS)ld" \
					CFLAGS="$(TARGET_CPPFLAGS) -I$(PKG_BUILD_DIR)/include -I$(PKG_BUILD_DIR)/include/ulogd -I$(PKG_BUILD_DIR)/libipulog/include -DULOGD_LOGFILE_MAXSZ_DEFAULT=0x40000" \
					LDFLAGS="$(TARGET_LDFLAGS)"
endef

define Package/ulogd/install
	$(MAKE) -C $(PKG_BUILD_DIR) install INSTALLDIR=$(1)
endef

$(eval $(call BuildPackage,ulogd))
