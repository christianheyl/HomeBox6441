#
# Copyright (C) 2012 OpenWrt.org
# Copyright (C) 2012 Lantiq
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=base-files-nfs
PKG_RELEASE:=1

include $(INCLUDE_DIR)/package.mk

define Package/$(PKG_NAME)
  SUBMENU:=Base Files
  SECTION:=base
  CATEGORY:=Lantiq
  TITLE:=Base files and kernel settings for booting from NFS.
  URL:=http://www.lantiq.com/
  MAINTAINER:=Lantiq
  KCONFIG:= \
            CONFIG_CRYPTO_AEAD2=y \
            CONFIG_CRYPTO_ALGAPI2=y \
            CONFIG_CRYPTO_ALGAPI=y \
            CONFIG_CRYPTO_BLKCIPHER2=y \
            CONFIG_CRYPTO_BLKCIPHER=y \
            CONFIG_CRYPTO_CBC=y \
            CONFIG_CRYPTO_DES=y \
            CONFIG_CRYPTO_HASH2=y \
            CONFIG_CRYPTO_MANAGER2=y \
            CONFIG_CRYPTO_MANAGER=y \
            CONFIG_CRYPTO_MD5=y \
            CONFIG_CRYPTO_RNG2=y \
            CONFIG_IP_PNP=y \
            CONFIG_IP_PNP_DHCP=y \
            CONFIG_IP_PNP_BOOTP=y\
            CONFIG_IP_PNP_RARP=y\
            CONFIG_LOCKD=y \
            CONFIG_NFS_FS=y \
            CONFIG_ROOT_NFS=y \
            CONFIG_RPCSEC_GSS_KRB5=y \
            CONFIG_SUNRPC_GSS=y \
            CONFIG_SUNRPC=y
endef

define Package/$(PKG_NAME)/description
 This package contains files and system scripts to use a NFS root.
endef

define Build/Prepare
endef

define Build/Compile
endef

define Package/$(PKG_NAME)/install
	[ -d ./files ] && $(CP) ./files/* $(1)/
endef

$(eval $(call BuildPackage,$(PKG_NAME)))

