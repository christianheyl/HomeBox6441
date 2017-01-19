#
# Written by Zenon Fortuna, zenon@mips.com, zenon@fortuna.org
# updated Feb 04, 2011 Lantiq
#
#### Includes ###################################
include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

#### Package Info ###############################
PKG_NAME:=pecostat
PKG_VERSION=0.9.2
PKG_RELEASE:=1

include $(INCLUDE_DIR)/package.mk

define Package/$(PKG_NAME)
	SECTION:=utils
	CATEGORY:=Utilities
	TITLE:=pecostat
	DEPENDS:=+kmod-$(PKG_NAME)
	URL:=http://www.lantiq.com
	MAINTAINER:=Lantiq Communications
endef

define Package/$(PKG_NAME)/description
	This package contains pecostat which is used for performance analysis.
endef

define KernelPackage/$(PKG_NAME)
	SUBMENU:=Other modules
	TITLE:=pecostat kernel Support
	DEPENDS:=@LINUX_2_6
	FILES:=$(PKG_BUILD_DIR)/$(PKG_NAME).$(LINUX_KMOD_SUFFIX)
endef

define KernelPackage/$(PKG_NAME)/description
	Kernel support to pecostat for performance analysis
endef

MAKE_OPTS:= \
	LINUX_DIR="$(LINUX_DIR)" \
	ARCH="$(LINUX_KARCH)" \
	CROSS_COMPILE="$(TARGET_CROSS)" \
	SUBDIRS="$(PKG_BUILD_DIR)" \
	EXTRA_CFLAGS="$(EXTRA_CFLAGS)"

#### Target Rules ###############################

define Build/Prepare
	$(call Build/Prepare/Default)
	cp -rf ./files/src/* $(PKG_BUILD_DIR)/
endef

define Build/Compile
	$(MAKE) $(TARGET_CONFIGURE_OPTS) \
		-C $(PKG_BUILD_DIR) $(MAKE_OPTS) PKG_BUILD_DIR=$(PKG_BUILD_DIR)
endef

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/pecostat $(1)/usr/sbin
	mkdir -p $(1)/etc
	$(CP) ./files/scripts/* $(1)/etc/
endef

define Build/Clean
	if [ -d $(PKG_BUILD_DIR) ]; then \
		$(MAKE) -C $(PKG_BUILD_DIR) clean; \
		$(RM) -r $(PKG_BUILD_DIR)/ipkg-$(BOARD); \
	fi
endef

$(eval $(call KernelPackage,$(PKG_NAME)))
$(eval $(call BuildPackage,$(PKG_NAME)))
