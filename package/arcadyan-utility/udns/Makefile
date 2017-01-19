# 
# Copyright (C) 2011 Arcadyan
#
#

include $(TOPDIR)/rules.mk

PKG_NAME:=udns
PKG_VERSION:=0.1
PKG_RELEASE:=1

PKG_BUILD_DIR := $(BUILD_DIR)/$(PKG_NAME)
# PKG_BUILD_DEPENDS:=arcadyan-utility lib-arc-cfg lib-arc-com

MKDIR:=mkdir -p
RM:=rm -rf

include $(INCLUDE_DIR)/package.mk

ifeq ($(__target_inc),)
  include $(INCLUDE_DIR)/target.mk
endif

define Package/udns
  SUBMENU:=Libs
  SECTION:=libs
  CATEGORY:=Arcadyan
  TITLE:=udns library
  URL:=http://www.arcadyan.com/
endef

define Package/udns/description
 The udns Library.
endef

define Build/Configure
	- rm -f $(PKG_BUILD_DIR)/.configured*
	- grep 'CONFIG_ARCADYAN_' $(TOPDIR)/.config | sed -e "s,\\(# \)\\?ARCADYAN_\\(.*\\),\\1\\2,g" > $(PKG_BUILD_DIR)/.config
	- grep 'CONFIG_LINUX_2_6' $(TOPDIR)/.config >> $(PKG_BUILD_DIR)/.config
	- grep 'CONFIG_UBOOT_CONFIG_JFFS2_PART' $(TOPDIR)/.config >> $(PKG_BUILD_DIR)/.config
	- grep 'CONFIG_UBOOT_CONFIG_IN_SUPERTASK' $(TOPDIR)/.config >> $(PKG_BUILD_DIR)/.config
	- grep 'CONFIG_UBOOT_CONFIG_NAND_FLASH' $(TOPDIR)/.config >> $(PKG_BUILD_DIR)/.config
	- grep 'CONFIG_FW_XDSL_VDSL_SUPPORT' $(TOPDIR)/.config >> $(PKG_BUILD_DIR)/.config
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(call Build/Prepare/Default)
	if [ -d ./src ] ; then \
		$(CP) ./src/* $(PKG_BUILD_DIR)/. ; \
	fi
endef

define Build/Compile
	### Set 'build/$(SCAN_REPORT_DIR)/$(PKG_NAME)' as the output directory for analyzer reports if CLANG_ANALYZE == yes ###
	if [ "$(CLANG_ANALYZE)" = "yes" ] ; then \
		$(MKDIR) $(SCAN_REPORT_DIR)/$(PKG_NAME); \
		$(CLANG_BUILD) -o $(SCAN_REPORT_DIR)/$(PKG_NAME) $(MAKE) -C $(PKG_BUILD_DIR) CC="$(CLANG_CC)" CCC_CC="$(TARGET_CC)" CFLAGS="$(TARGET_CFLAGS) -Wall -I$(STAGING_DIR)/usr/include" \
			CPPFLAGS="$(TARGET_CPPFLAGS)" LD="$(TARGET_CROSS)ld" LDFLAGS="$(TARGET_LDFLAGS) -lrt" \
			STRIP="$(TARGET_CROSS)strip" \
			DEV_DIR="$(STAGING_DIR)" ; \
	else \
	$(MAKE) -C $(PKG_BUILD_DIR) CC="$(TARGET_CC)" CFLAGS="$(TARGET_CFLAGS) -Wall -I$(STAGING_DIR)/usr/include" \
		CPPFLAGS="$(TARGET_CPPFLAGS)" LD="$(TARGET_CROSS)ld" LDFLAGS="$(TARGET_LDFLAGS) -lrt"  \
		STRIP="$(TARGET_CROSS)strip" \
		DEV_DIR="$(STAGING_DIR)" ; \
	fi

	$(RM) $(PKG_INSTALL_DIR)
	$(MKDIR) $(PKG_INSTALL_DIR)
	$(MAKE) -C $(PKG_BUILD_DIR) INSTALL_DIR="$(INSTALL_DIR)" INSTALL_BIN="$(INSTALL_BIN)" INSTALL_DATA="$(INSTALL_DATA)" \
		CP="$(CP)" CONFIG_PREFIX="$(PKG_INSTALL_DIR)"  DEV_DIR="$(STAGING_DIR)" install;
endef

define Build/InstallDev
endef

define Package/udns/install
	$(CP) $(PKG_INSTALL_DIR)/* $(1)/
endef

define Build/udns/clean
        @- [ -e $(PKG_BUILD_DIR)/Makefile ] && $(MAKE) -C $(PKG_BUILD_DIR) clean
        $(call Build/Clean/Default)
endef

$(eval $(call BuildPackage,udns))
