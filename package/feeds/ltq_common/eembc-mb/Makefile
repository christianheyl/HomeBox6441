#
# Copyright (C) 2008 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
# $Id: $

include $(TOPDIR)/rules.mk

PKG_NAME:=eembc-mb
PKG_VERSION:=v1
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)
PKG_RELEASE:=1
#PKG_SOURCE_URL:=
#PKG_MD5SUM:=
PKG_SOURCE:=mith_lib_MB$(PKG_VERSION).tgz
PKG_SOURCE2:=mith_wld_MB$(PKG_VERSION).tgz

include $(INCLUDE_DIR)/package.mk

# Re-define PKG_UNPACK to unpack 2 archives instead default one.
PKG_UNPACK:=$(TAR) -C $(PKG_BUILD_DIR) -xzf $(DL_DIR)/$(PKG_SOURCE) && \
            $(TAR) -C $(PKG_BUILD_DIR) -xzf $(DL_DIR)/$(PKG_SOURCE2)

define Package/$(PKG_NAME)
  SECTION:=application
  CATEGORY:=EEMBC
  TITLE:=EEMBC MultiBench
  DEPENDS:=+libpthread
endef

define Package/$(PKG_NAME)/description
  MultiBench is a suite of benchmarks created by EEMBC
  (the Embedded Microprocessor Benchmarking Consortium (R))
  to measure and analyze the performance of platforms
  capable of executing in multiple hardware contexts.
endef

define Build/Configure
endef

define Build/Compile
	$(call Build/Compile/Default,build)
endef

ifeq ($(CONFIG_BIG_ENDIAN),y)
ENDIAN=-DEE_BIG_ENDIAN=1 -DEE_LITTLE_ENDIAN=0
else
ENDIAN=-DEE_BIG_ENDIAN=0 -DEE_LITTLE_ENDIAN=1
endif

MAKE_FLAGS = \
	TOOLS=$(TOOLCHAIN_DIR) \
	TPREF=$(TARGET_CROSS) \
	XCFLAGS="$(TARGET_CFLAGS) $(ENDIAN)" \
	LINKER_FLAGS="$(TARGET_LDFLAGS) -shared-libgcc -lm -lpthread" \
	WARNING_OPTIONS_="-fno-asm -fsigned-char" \
	TARGET=linux \
	TOOLCHAIN=gcc-cross-linux

IFX_BIN_DIR=/opt/ifx/bin

define Package/$(PKG_NAME)/install
	echo INSTALL!!!
	$(INSTALL_DIR) $(1)/usr/bin/
	$(CP) $(PKG_BUILD_DIR)/builds/linux/bin/*.exe $(1)/usr/bin/
	$(INSTALL_DIR) $(1)/usr/data/
	$(CP) $(PKG_BUILD_DIR)/builds/linux/data/* $(1)/usr/data/
endef

$(eval $(call BuildPackage,$(PKG_NAME)))

define RequireSource
  define Require/$(1)
    [ -f $(DL_DIR)/$(1) ]
  endef

  $$(eval $$(call Require,$(1),\
    Please get the file \"$(1)\" and put it in \"$(DL_DIR)\". \
  ))
endef

$(eval $(call RequireSource,$(PKG_SOURCE)))
$(eval $(call RequireSource,$(PKG_SOURCE2)))
