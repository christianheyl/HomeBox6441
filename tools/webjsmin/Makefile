#
# Copyright (C) 2013 Arcadyan
#
# This is a optimize tool for web pages.
#
include $(TOPDIR)/rules.mk

PKG_NAME := webjsmin

include $(INCLUDE_DIR)/host-build.mk
include $(INCLUDE_DIR)/kernel.mk

define cc
	$(CC) $(HOST_CFLAGS) -o $(HOST_BUILD_DIR)/bin/$(1) src/$(1).c
endef

define Host/Compile
	mkdir -p $(HOST_BUILD_DIR)/bin
	$(call cc,$(PKG_NAME))
	$(CP) src/$(PKG_NAME).sh $(HOST_BUILD_DIR)/bin/
	chmod +x $(HOST_BUILD_DIR)/bin/$(PKG_NAME).sh
endef

define Host/Install
	$(INSTALL_BIN) $(HOST_BUILD_DIR)/bin/* $(STAGING_DIR_HOST)/bin/
endef

define Host/Clean
	rm -f $(STAGING_DIR_HOST)/bin/$(PKG_NAME)
endef

$(eval $(call HostBuild))
