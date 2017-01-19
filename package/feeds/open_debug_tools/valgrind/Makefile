#
# Copyright (C) 2006 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
# $Id: $

include $(TOPDIR)/rules.mk

PKG_NAME:=valgrind
PKG_VERSION:=3.8.1
PKG_REVISION:=12672

PKG_SOURCE_PROTO:=svn
PKG_SOURCE_VERSION:=$(PKG_REVISION)
PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)#-r$(PKG_REVISION)
PKG_SOURCE:=$(PKG_SOURCE_SUBDIR).tar.bz2

PKG_SOURCE_URL:=http://valgrind.org/downloads/
PKG_MD5SUM:=288758010b271119a0ffc0183f1d6e38

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_SOURCE_SUBDIR)
PKG_INSTALL_DIR:=$(PKG_BUILD_DIR)/ipkg-install

include $(INCLUDE_DIR)/package.mk

define Package/valgrind
  SECTION:=utils
  CATEGORY:=Utilities
  DEPENDS:=@TARGET_x86||@TARGET_rdc||@TARGET_ps3||@TARGET_magicbox||@TARGET_amcc||@TARGET_uml||@TARGET_ltqcpe
  TITLE:=debugging and profiling Linux programs
  URL:=http://www.valgrind.org
endef

define Package/valgrind/description
	Valgrind is an award-winning suite of tools for debugging and
	profiling Linux programs. With the tools that come with Valgrind,
	you can automatically detect many memory management and threading
	bugs, avoiding hours of frustrating bug-hunting, making your
	programs more stable. You can also perform detailed profiling,
	to speed up and reduce memory use of your programs.
endef

CONFIGURE_ARGS += \
	--disable-tls \
	--enable-only32bit \
	--without-x \
	--without-uiout \
	--disable-valgrindmi \
	--disable-tui \
	--disable-valgrindtk \
	--without-included-gettext

DEFAULT_SUPP += "uclibc.supp"

define Build/Configure
	(cd $(PKG_BUILD_DIR);./autogen.sh) 
	$(call Build/Configure/Default)
endef

define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR) \
		DESTDIR="$(PKG_INSTALL_DIR)" \
		CFLAGS="$(TARGET_CFLAGS)" \
		all install
endef

define Package/valgrind/install	
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/bin/valgrind $(1)/usr/bin/
	#$(INSTALL_BIN) ./files/valgrind.sh $(1)/usr/bin/
	$(INSTALL_DIR) $(1)/usr/lib/valgrind
	#$(CP) ./files/uclibc.supp $(1)/usr/lib/valgrind/
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/valgrind/vgpreload_core-mips32-linux.so $(1)/usr/lib/valgrind/
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/valgrind/vgpreload_memcheck-mips32-linux.so $(1)/usr/lib/valgrind/
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/valgrind/memcheck-mips32-linux $(1)/usr/lib/valgrind/
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/valgrind/default.supp $(1)/usr/lib/valgrind/
	# We copy only memcheck to rootfs. If you need whole valgrind stuff,uncomment below line
	#$(CP) $(PKG_INSTALL_DIR)/usr/lib/valgrind/*-linux $(1)/usr/lib/valgrind/
	mkdir -p $(1)/etc
	$(CP) ./files/swap.sh $(1)/etc/
endef

$(eval $(call BuildPackage,valgrind))
