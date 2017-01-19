
include $(TOPDIR)/rules.mk

PKG_NAME:=linux-kbuild
PKG_VERSION:=2.6-2.6.26
PKG_RELEASE:=2.6_2.6.26.orig

PKG_SOURCE:=$(PKG_NAME)-$(PKG_RELEASE).tar.gz
PKG_SOURCE_URL:=http://ftp.debian.org/pool/main/l/linux-kbuild-2.6/
PKG_MD5SUM:=e760747b8c815a1796be4ab31e92174a
PKG_CAT:=zcat

include $(INCLUDE_DIR)/host-build.mk

srctree			= $(HOST_BUILD_DIR)
KBUILD_VERBOSE	= 0
HOSTCC			= gcc
HOSTCXX			= g++
HOSTCFLAGS		= -Wall -Wstrict-prototypes -O2 -fomit-frame-pointer
HOSTCXXFLAGS	= -O2
MAKEFLAGS		+= -rR --no-print-directory
KBUILD_VERBOSE	= 0

CONFIG_SHELL := $(shell if [ -x "$$BASH" ]; then echo $$BASH; \
	  else if [ -x /bin/bash ]; then echo /bin/bash; \
	  else echo sh; fi ; fi)

ifeq ($(KBUILD_VERBOSE),1)
	quiet =
	Q =
else
	quiet=quiet_
	Q = @
endif

export srctree quiet Q KBUILD_VERBOSE CONFIG_SHELL HOSTCC

define Host/Compile
	$(MAKE) -C $(HOST_BUILD_DIR) \
		-f $(srctree)/scripts/Makefile.build \
		obj=scripts/basic
	$(MAKE) -C $(HOST_BUILD_DIR) \
		-f $(srctree)/scripts/Makefile.build \
		obj=scripts/kconfig config
endef

define Host/Install
	$(CP) $(HOST_BUILD_DIR)/scripts/kconfig/conf $(STAGING_DIR_HOST)/bin/
	$(INSTALL_DIR) $(INCLUDE_DIR)/linux
	$(INSTALL_DIR) $(INCLUDE_DIR)/config
endef

define Host/Clean
	rm -f $(STAGING_DIR_HOST)/bin/conf
endef

$(eval $(call HostBuild))
