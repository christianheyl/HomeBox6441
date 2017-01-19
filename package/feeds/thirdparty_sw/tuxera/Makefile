#
# Openwrt feed for tuxera-ntfs
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=tuxera-ntfs
PKG_VERSION:=3013.2.5-arx388
PKG_RELEASE:=1

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=http://localhost
PKG_MD5SUM:=af5fc3223a6ca063383159adaa1c2c86

TNTFS_TOOLS:=mkntfs ntfsck ntfsdebug ntfsinfo ntfslabel

include $(INCLUDE_DIR)/package.mk

ifeq ($(DUMP),)
  STAMP_CONFIGURED:=$(strip $(STAMP_CONFIGURED))_$(shell grep '^CONFIG_PACKAGE_$(PKG_NAME)_TOOL_' $(TOPDIR)/.config | md5s)
endif

define Package/$(PKG_NAME)
  SECTION:=utils
  CATEGORY:=Utilities
  SUBMENU:=Filesystem
  TITLE:=Tuxera NTFS for Lantiq ARX3XX platforms
  URL:=http://www.tuxera.com
  DEPENDS:=+kmod-nls-utf8
  MENU:=1
endef

define Package/$(PKG_NAME)/description
  This package contains pre-compiled binaries of Tuxera NTFS driver
  and tools tuned for Lantiq ARX3XX platforms.
  Supported tools: $(subst $(space),$(strip ,) ,$(TNTFS_TOOLS))
endef

define config_gen
  config PACKAGE_$(PKG_NAME)_TOOL_$(shell echo $(1)|tr a-z A-Z)
    depends on PACKAGE_$(PKG_NAME)
    bool '$(1)'

endef

define Package/$(PKG_NAME)/config
  $(foreach file,$(TNTFS_TOOLS),$(call config_gen,$(file)))
endef

define Build/Compile
	echo -n;
endef

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/lib/modules/$(LINUX_VERSION)
	$(if $(CONFIG_PACKAGE_kmod-smvp), \
		$(INSTALL_BIN) $(PKG_BUILD_DIR)/ntfs/kernel-module/smp/tntfs.ko, \
		$(INSTALL_BIN) $(PKG_BUILD_DIR)/ntfs/kernel-module/nosmp/tntfs.ko) \
	$(1)/lib/modules/$(LINUX_VERSION)/

	$(INSTALL_DIR) $(1)/usr/sbin
	$(foreach tbin, $(TNTFS_TOOLS), \
		$(if $(CONFIG_PACKAGE_$(PKG_NAME)_TOOL_$(shell echo $(tbin)|tr a-z A-Z)), \
			$(INSTALL_BIN) $(PKG_BUILD_DIR)/ntfs/tools/$(tbin) $(1)/usr/sbin/;) \
	)
endef

$(eval $(call BuildPackage,$(PKG_NAME)))

