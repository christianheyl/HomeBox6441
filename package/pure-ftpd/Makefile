#
# Copyright (C) 2007 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk
include $(TOPDIR)/.config

PKG_NAME:=pure-ftpd
PKG_VERSION:=1.0.31
PKG_RELEASE:=1

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.bz2
PKG_SOURCE_URL:=http://download.pureftpd.org/pub/pure-ftpd/releases
PKG_MD5SUM:=fba0d592a6316f45b5c312a78789ad97
PKG_BUILD_DEPENDS:= libiconv

include $(INCLUDE_DIR)/package.mk

define Package/pure-ftpd
  SUBMENU:=FTP
  SECTION:=net
  CATEGORY:=Network
  DEPENDS:=+libelf
  TITLE:=Secure FTP made easy!
  URL:=http://www.pureftpd.org/project/pure-ftpd
endef

define Package/pure-ftpd/description
 Pure-FTPd is a free (BSD), secure, production-quality and standard-conformant
 FTP server. It doesn't provide useless bells and whistles, but focuses on
 efficiency and ease of use.
 It provides simple answers to common needs, plus unique  useful features for
 personal users as well as hosting providers.
endef

define Build/Prepare
	$(call Build/Prepare/Default)
	$(CP) ./src/* $(PKG_BUILD_DIR)/src/.
endef

define Package/pure-ftpd/conffiles
/etc/pure-ftpd/pure-ftpd.dft
/etc/pure-ftpd/pure-ftpd.pem.dft
endef

define Package/pure-ftpd/config
config PACKAGE_pure-ftpd-tls
   depends PACKAGE_libopenssl
   bool "Allow SSL connection"
   default n

config PACKAGE_pure-ftpd-ANON_CAN_DELETE
   depends PACKAGE_pure-ftpd || PACKAGE_pure-ftpd-tls
   bool "Allow anonymous has delete permission"
   default n

config PACKAGE_pure-ftpd-ANON_CAN_RESUME
   depends PACKAGE_pure-ftpd || PACKAGE_pure-ftpd-tls
   bool "Allow anonymous has resume permission"
   default n

config PACKAGE_pure-ftpd-ANON_CAN_RENAME
   depends PACKAGE_pure-ftpd || PACKAGE_pure-ftpd-tls
   bool "Allow anonymous has resume permission"
   default n
endef

# turn on debug mode
TARGET_CFLAGS+=-DARC_DEBUG

# Jared added:
TARGET_CFLAGS+=-DDENY_SYMLINK_TARGET_OUT_OF_USB_MOUNT_POINT

# pure-pw accept -P option
TARGET_CFLAGS+=-DARC_PW_CLI

# pure-pw accept -A option
TARGET_CFLAGS+=-DARC_PW_PRIVILEGE

# turn on open access mode
TARGET_CFLAGS+=-DARC_OPEN_ACCESS_MODE

# turn on ram read only
TARGET_CFLAGS+=-DARC_RAM_READ_ONLY

# turn on IPv6 address checking.
TARGET_CFLAGS+=-DARC_IPV6_SUBNET

# turn on READ only capability
TARGET_CFLAGS+=-DARC_ACCOUNT_READ_ONLY_SUPPORT
ifeq ($(CONFIG_PACKAGE_pure-ftpd-ANON_CAN_DELETE),y)
	TARGET_CFLAGS += -DANON_CAN_DELETE
endif
ifeq ($(CONFIG_PACKAGE_pure-ftpd-ANON_CAN_RESUME),y)
	TARGET_CFLAGS += -DANON_CAN_RESUME
endif
ifeq ($(CONFIG_PACKAGE_pure-ftpd-ANON_CAN_RENAME),y)
	TARGET_CFLAGS += -DANON_CAN_RENAME
endif


CONFIGURE_ARGS += \
	--with-everything \
	--with-rfc2640 \
	--with-virtualchroot \
	--without-ascii
	
ifeq ($(CONFIG_PACKAGE_pure-ftpd-tls),y)
CONFIGURE_ARGS += \
	--with-tls \
	--with-certfile=/tmp/pure-ftpd/pure-ftpd.pem
endif


define Package/pure-ftpd/install
	$(INSTALL_DIR) $(1)/etc/pure-ftpd $(1)/etc/init.d $(1)/usr/sbin
#	$(INSTALL_CONF) ./files/$(PKG_NAME).config $(1)/etc/pure-ftpd/$(PKG_NAME).dft
	$(INSTALL_CONF) ./files/$(PKG_NAME).pem $(1)/etc/pure-ftpd/$(PKG_NAME).pem.dft
#	$(INSTALL_BIN) ./files/$(PKG_NAME).init $(1)/etc/init.d/$(PKG_NAME)
#	$(INSTALL_BIN) ./files/$(PKG_NAME)2 $(1)/etc/init.d/$(PKG_NAME)2
#	$(INSTALL_BIN) $(PKG_BUILD_DIR)/src/ptracetest $(1)/usr/sbin
#	$(INSTALL_BIN) $(foreach i,ftpd pw ftpwho mrtginfo pwconvert quotacheck statsdecode uploadscript authd, \
#	$(PKG_BUILD_DIR)/src/pure-$(i)) $(1)/usr/sbin
#	$(INSTALL_BIN) ./files/ftp_fw_upgrade.sh $(1)/etc/init.d/ftp_fw_upgrade.sh
	$(INSTALL_BIN) $(foreach i,ftpd pw ftpwho, $(PKG_BUILD_DIR)/src/pure-$(i)) $(1)/usr/sbin
	
endef

$(eval $(call BuildPackage,pure-ftpd))
