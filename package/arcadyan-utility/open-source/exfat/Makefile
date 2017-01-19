# 
# Copyright (C) 2011 Arcadyan
#
#

include $(TOPDIR)/rules.mk

PKG_NAME:=exfat
PKG_REV:=master
PKG_VERSION:=1.0.1
PKG_RELEASE:=1

PKG_SOURCE_VERSION:=$(PKG_REV)
PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)
PKG_MAINTAINER:=jared_huang@arcadyan.com

PKG_BUILD_DIR := $(BUILD_DIR)/$(PKG_NAME)
PKG_BUILD_DEPENDS:=

MKDIR:=mkdir -p
RM:=rm -rf

include $(INCLUDE_DIR)/package.mk


define Package/exfat
  SUBMENU:=Filesystem
  SECTION:=file system
  CATEGORY:=Arcadyan
  TITLE:=A full-featured exFAT file system
  URL:=https://code.google.com/p/exfat/
  DEPENDS:=libfuse libiconv
endef

define Package/exfat/description
 This package aims to provide a full-featured exFAT file system implementation for GNU/Linux and other Unix-like systems as a FUSE module and a set of utilities.
endef

define Package/exfat/config
endef

define Package/exfat/config
	##source "$(SOURCE)/Config.in"
endef

define Build/Configure
	#- rm -f $(PKG_BUILD_DIR)/.configured*
	#- grep 'CONFIG_ARCADYAN_' $(TOPDIR)/.config | sed -e "s,\\(# \)\\?ARCADYAN_\\(.*\\),\\1\\2,g" > $(PKG_BUILD_DIR)/.config
	#- grep 'CONFIG_LINUX_2_6' $(TOPDIR)/.config >> $(PKG_BUILD_DIR)/.config
	#- grep 'CONFIG_UBOOT_CONFIG_JFFS2_PART' $(TOPDIR)/.config >> $(PKG_BUILD_DIR)/.config
	#- grep 'CONFIG_UBOOT_CONFIG_IN_SUPERTASK' $(TOPDIR)/.config >> $(PKG_BUILD_DIR)/.config
	#- grep 'CONFIG_UBOOT_CONFIG_NAND_FLASH' $(TOPDIR)/.config >> $(PKG_BUILD_DIR)/.config
	#- grep 'CONFIG_FW_XDSL_VDSL_SUPPORT' $(TOPDIR)/.config >> $(PKG_BUILD_DIR)/.config
	#- grep 'CONFIG_PACKAGE_arc_and' $(TOPDIR)/.config >> $(PKG_BUILD_DIR)/.config
	#- grep 'CONFIG_PACKAGE_arc-middle-mod-qcsapi' $(TOPDIR)/.config >> $(PKG_BUILD_DIR)/.config
	#- grep 'CONFIG_HTTPD' $(TOPDIR)/.config >> $(PKG_BUILD_DIR)/.config
	#grep 'CONFIG_WEBJSMIN_' $(TOPDIR)/.config | sed -e "s,\\(# \)\\?CONFIG_WEBJSMIN_\\(.*\\),\\1\\2,g" >> $(PKG_BUILD_DIR)/.config
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(call Build/Prepare/Default)
	if [ -d ./src ] ; then \
		$(CP) ./src/* $(PKG_BUILD_DIR)/. ; \
	fi
endef

define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR) \
           CC="$(TARGET_CC)" \
           CFLAGS="$(TARGET_CFLAGS)" \
		   CPPFLAGS="$(TARGET_CPPFLAGS)" \
           LDFLAGS="$(TARGET_LDFLAGS)" \
		   all
	$(MAKE) -C $(PKG_BUILD_DIR) \
		CC="$(TARGET_CC)" \
		CONFIG_PREFIX="$(PKG_INSTALL_DIR)/sbin" \
		install
endef

define Build/InstallDev
endef

define Package/exfat/install
	$(CP) $(PKG_INSTALL_DIR)/* $(1)/
endef

define Build/exfat/clean
	@- [ -e $(PKG_BUILD_DIR)/Makefile ] && $(MAKE) -C $(PKG_BUILD_DIR) clean
	$(call Build/Clean/Default)
endef

$(eval $(call BuildPackage,exfat))
