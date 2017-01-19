include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=ltq-pmon
PKG_BASE_NAME:=pmon
PKG_RELEASE:=1
PKG_VERSION:=2.0.1

PKG_BUILD_DIR := $(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/kernel.mk
 
ifeq ($(CONFIG_TARGET_$(BOARD)_platform_vr9),y)
        platform=VR9
endif
ifeq ($(CONFIG_TARGET_$(BOARD)_platform_hn1),y)
        platform=HN1
endif
ifeq ($(CONFIG_TARGET_$(BOARD)_platform_ar10),y)
        platform=AR10
endif

define Package/$(PKG_NAME)
	SECTION:=utils
	CATEGORY:=Lantiq
	TITLE:=Utility for configuring performance counters(LTQ SoCs) 
	URL:=http://www.lantiq.com
	MAINTAINER:=Lantiq Technologies AG
	MENU:=1
endef

define Package/$(PKG_NAME)/description
	This tool can be used to configure performance counters (external to CPU and 
	specific to Lantiq SoCs). After configuration these performance counters can be 
	read using pecostat.
endef

define Build/Prepare
	$(call Build/Prepare/Default)
	ln -fs `pwd`/src/* $(PKG_BUILD_DIR)/
	echo PKG_BUILD_DIR=${PKG_BUILD_DIR}
endef

define Build/Compile
	$(MAKE) -C "$(PKG_BUILD_DIR)" \
		KERNEL_INCLUDE_PATH="$(LINUX_DIR)/include" PLATFORM="$(platform)"\
		CC="$(TARGET_CROSS)gcc"
endef

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/pmon $(1)/sbin
endef

define Build/Clean
	if [ -d $(PKG_BUILD_DIR) ]; then $(MAKE) -C $(PKG_BUILD_DIR) clean; \
	$(RM) -r $(PKG_BUILD_DIR)/ipkg-$(BOARD); fi
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
