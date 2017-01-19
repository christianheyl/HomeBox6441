include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=ppasessmgmt
PKG_RELEASE:=1

PKG_BUILD_DIR := $(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk
CFLAGS="$(TARGET_CFLAGS) -Wall -I$(STAGING_DIR)/usr/include" \
LDFLAGS="$(TARGET_LDFLAGS) -L$(STAGING_DIR)/usr/lib" \

define Package/$(PKG_NAME)
	SUBMENU:=Utilities
	SECTION:=utils    
  CATEGORY:=Arcadyan
  TITLE:=PPA session mangement
  DEPENDS:=
endef

define Package/$(PKG_NAME)/description
	PPA session mangement
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Build/Configure
endef

define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR)	\
	CC="$(TARGET_CC) -Wp,-dD -save-temps "	\
	CFLAGS1="$(TARGET_CFLAGS) $(EXTRA_CFLAGS) -I$(STAGING_DIR)/usr/include -I$(LINUX_DIR)/include "	\
	LDFLAGS="$(TARGET_LDFLAGS) $(COM_LDFLAGS)"
endef

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/ppasessmgmt $(1)/usr/sbin/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/ppasessmgmtd $(1)/usr/sbin/
endef

define Build/$(PKG_NAME)/clean
	@- [ -e $(PKG_BUILD_DIR)/Makefile ] && $(MAKE) -C $(PKG_BUILD_DIR) clean
	$(call Build/Clean/Default)
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
