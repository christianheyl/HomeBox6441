#
# Copyright (C) 2008-2011 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.

include $(TOPDIR)/rules.mk

PKG_NAME:=samba3-generic
PKG_VERSION:=3.0.37
PKG_RELEASE:=3

PKG_MD5SUM:=11ed2bfef4090bd5736b194b43f67289

PKG_SOURCE_URL:=ftp://ftp.samba.org/pub/samba/old-versions/
PKG_SOURCE:=samba-$(PKG_VERSION).tar.gz
PKG_BUILD_DIR:=$(BUILD_DIR)/samba-$(PKG_VERSION)
PKG_BUILD_PARALLEL:=1
#PKG_BUILD_DEPENDS:=openssl arc-middle-framework-open arc-middle-mod-ccfg-open

#Simon@2016/06/07, For GPL Release
PKG_BUILD_DEPENDS:=+libreadline
#PKG_BUILD_DEPENDS:=+libreadline +arcadyan-utility +lib-arc-cfg

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/nls.mk

define Package/$(PKG_NAME)/Default
  SECTION:=net
  CATEGORY:=Network
  SUBMENU:=Filesystem
  URL:=http://www.samba.org/
endef

define Package/$(PKG_NAME)
$(call Package/$(PKG_NAME)/Default)
  TITLE:=SMB server for file and printer sharing
  DEPENDS:=+libpthread +libpopt $(ICONV_DEPENDS) +@SAMBA3-GENERIC_CONFIG_PRINTING:cups
endef

define Package/$(PKG_NAME)-nmbd
$(call Package/$(PKG_NAME)/Default)
  TITLE:=NetBIOS name server
  DEPENDS:=$(PKG_NAME)
endef

define Package/$(PKG_NAME)-mountcifs
$(call Package/$(PKG_NAME)/Default)
  SECTION:=utils
  CATEGORY:=Utilities
  TITLE:=Mount utility for samba shares
endef

define Package/$(PKG_NAME)/description
SMB server for file and printer sharing
Also contains a SMB password utility (smbpasswd)

Made small with patches taken from AVM GPL releases and freetz
endef

define Package/$(PKG_NAME)-nmbd/description
NetBIOS name server

Made small with patches taken from AVM GPL releases and freetz
endef

define Package/cifsmount/description
An user space helper utility for mounting remote CIFS shares.
endef

define Package/$(PKG_NAME)/conffiles
/etc/config/samba
/etc/samba/smb.conf.template
endef

define Package/$(PKG_NAME)/config
	source "$(SOURCE)/Config.in"
endef

TARGET_CFLAGS+=$(if $(CONFIG_SAMBA3-GENERIC_CONFIG_DEBUG),-DSAMBA_DEBUG,-DMAX_DEBUG_LEVEL=10)
TARGET_CFLAGS+=$(if $(CONFIG_SAMBA3-GENERIC_CONFIG_SMALLER),-DAVM_SMALLER)
TARGET_CFLAGS+=$(if $(CONFIG_SAMBA3-GENERIC_CONFIG_PRINTING),,-DAVM_NO_PRINTING)

### jeffery add ###
#TARGET_CFLAGS+=-DARCADYAN_GUEST_PASSWORD_ACCEPT_EXCEPT_NULL_PASSWORD -DARCADYAN_DT_724_SAMBA_EVENT_LOG -DDEBUG_PASSWORD
#TARGET_CFLAGS+=-DARCADYAN_COMPARISON_OF_USERNAME_IS_SENSITIVE -DARCADYAN_PERCENT_U_IN_CONF_FILE_IS_SENSITIVE
#TARGET_CFLAGS+=-UARCADYAN_DNS_SERVER_QUERY_DISABLE -DARCADYAN_RAMDISK_ACCESS_TO_BE_DENIED -DARCADYAN_DT_724_UTF8_TO_UTF16 -USMBPASSWD_DEBUG

### Jared added ###
TARGET_CFLAGS+=-DDENY_SYMLINK_TARGET_OUT_OF_USB_MOUNT_POINT

### Simon add ###
TARGET_CFLAGS+=-DARCADYAN_PRINTING_SKIP_WRITE_ON_DISK_FULL
#TARGET_CFLAGS+=-DARCADYAN_PRINTING_SELF_TERMINATION_ON_DISK_FULL

#Simon@2016/06/07, For GPL Release
#Simon@2014/05/12,
#TARGET_CFLAGS+=-DDISABLE_CFG_SECTION_NAME_SUPPORT
#TARGET_CFLAGS+=-DSUPPORT_CCFG_GET_SET_LONG

## for library, statistic ##
ifeq ($(CONFIG_PACKAGE_libstatistic), y)
TARGET_CFLAGS+=-DARCADYAN_STASTISTIC_CFG
TARGET_CPPFLAGS+=-I$(TOPDIR)/package/arcadyan-utility/libstatistic/src/
TARGET_LDFLAGS+=-lstatistic -L$(TOPDIR)/package/arcadyan-utility/
endif

#Simon@2016/06/07, For GPL Release
## for function, osSystem_GetOutput ##
#TARGET_CPPFLAGS+=-I$(TOPDIR)/extern/target-install/lib-arc-com/usr/include
#TARGET_LDFLAGS+=-lArcCom -L$(TOPDIR)/extern/target-install/lib-arc-com/usr/lib

#Simon@2014/04/28, disable for testing
## for mapi_ccfg_XXX() ##
#TARGET_LDFLAGS+=-lcrypto
#TARGET_LDFLAGS+=-lssl
#TARGET_CPPFLAGS+=-I$(TOPDIR)/extern/target-install/arc-middle-framework/usr/include
#TARGET_LDFLAGS+=-lmapitrnx -L$(TOPDIR)/extern/target-install/arc-middle-framework/usr/lib
#TARGET_CPPFLAGS+=-I$(TOPDIR)/extern/target-install/arc-middle-mod-ccfg/usr/include
#TARGET_LDFLAGS+=-lmapi_ccfg -L$(TOPDIR)/extern/target-install/arc-middle-mod-ccfg/usr/lib

#Simon@2016/06/07, For GPL Release
#Simon@2014/04/28, Use new libraday name
#TARGET_LDFLAGS+= -lmidcore


###################
CONFIGURE_PATH:=source

#Enable cups, Leo, 16-Nov-2012
CONFIGURE_ARGS+= \
	$(if $(CONFIG_SAMBA3-GENERIC_CONFIG_DEBUG),--enable-debug) \
	$(if $(CONFIG_SAMBA3-GENERIC_CONFIG_PRINTING),,--enable-iprint=no) \
	--enable-cups \
	--without-krb5 \
	--without-ads \
	--without-ldap \
	--enable-largefile \
	--with-configdir=/etc/samba \
	--with-privatedir=/tmp ### jeffery modified ###

# Make sure we tell the configure script that we support negative enum values and want to use setresuid
CONFIGURE_VARS+= \
	SMB_BUILD_CC_NEGATIVE_ENUM_VALUES= \
	samba_cv_USE_SETEUID=no \
	samba_cv_have_setresuid=yes \
	samba_cv_USE_SETRESUID=yes \
	samba_cv_HAVE_C99_VSNPRINTF=yes \
	samba_cv_have_longlong=yes \
	ac_cv_type_long_long=yes \

MAKE_PATH=source
MAKE_FLAGS += \
	$(if $(CONFIG_SAMBA3-GENERIC_CONFIG_PRINTING),,OWRT_NO_PRINTING=1) \
	$(if $(CONFIG_SAMBA3-GENERIC_CONFIG_SMALLER),OWRT_SMALLER=1)

define Build/Compile
	# Ugly fix for parallel building (without this some generated files will be missing upon clean build)
	### jeffery add ###
		$(CP) ./src/source/include/config.h $(PKG_BUILD_DIR)/source/include/.
		$(CP) ./src/source/libsmb/ntlm_check.c $(PKG_BUILD_DIR)/source/libsmb/.
		$(CP) ./src/source/utils/owrt_smbpasswd.c $(PKG_BUILD_DIR)/source/utils/.
		$(CP) ./src/source/smbd/*	$(PKG_BUILD_DIR)/source/smbd/.
		$(CP) ./src/source/passdb/pdb_smbpasswd.c $(PKG_BUILD_DIR)/source/passdb/.
		$(CP) ./src/source/lib/substitute.c $(PKG_BUILD_DIR)/source/lib/.
		$(CP) ./src/source/param/loadparm.c $(PKG_BUILD_DIR)/source/param/.
		$(CP) ./src/source/lib/system.c $(PKG_BUILD_DIR)/source/lib/.
		$(CP) ./src/source/lib/util_sock.c $(PKG_BUILD_DIR)/source/lib/.
		$(CP) ./src/source/include/local.h $(PKG_BUILD_DIR)/source/include/.
		$(CP) ./src/source/printing/printing_db.c $(PKG_BUILD_DIR)/source/printing/.
		$(CP) ./src/source/include/smb.h  $(PKG_BUILD_DIR)/source/include/.
		### Simon add ###
		$(CP) ./src/source/printing/* $(PKG_BUILD_DIR)/source/printing/.
		$(CP) ./src/source/rpc_server/* $(PKG_BUILD_DIR)/source/rpc_server/.
		$(call Build/Compile/Default, proto_exists)
		$(call Build/Compile/Default, \
		$(if $(CONFIG_PACKAGE_samba3-generic),bin/smbd bin/smbpasswd bin/nmbd) \
		### jeffery marked ###
		#$(if $(CONFIG_PACKAGE_samba3-generic-nmbd),bin/nmbd) \
		#$(if $(CONFIG_PACKAGE_samba3-generic-mountcifs),bin/mount.cifs bin/umount.cifs) \
	)
endef

define Package/$(PKG_NAME)/install
	@echo "Do samba3-geniric install to $(1)"
	$(INSTALL_DIR) $(1)/bin
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_DIR) $(1)/etc/{samba,init.d,config}
	#Simon@2014/05/06, may not need this
	#$(INSTALL_DIR) $(1)/usr/share/config/etc/config/samba
	#$(INSTALL_BIN) ./files/sambacfg.sh $(1)/usr/sbin/sambacfg.sh
	$(INSTALL_BIN) ./files/samba.init $(1)/etc/init.d/samba
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/source/bin/smbpasswd $(1)/bin/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/source/bin/smbd $(1)/bin/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/source/bin/nmbd $(1)/bin/
	#Simon@2014/05/06, may not need this
	#$(INSTALL_DATA) ./files/samba.config $(1)/usr/share/config/etc/config/samba
	$(INSTALL_DATA) ./files/smb.conf.template $(1)/etc/samba/
	$(INSTALL_DATA) ./files/smb.conf.debug $(1)/etc/samba/
	touch $(1)/etc/samba/smbpasswd
	$(INSTALL_DIR) $(1)/usr/local/bin
	$(INSTALL_BIN) ./files/dfree $(1)/usr/local/bin/dfree
	@echo "Finish samba3-geniric install"
endef

define Package/$(PKG_NAME)-nmbd/install
	@echo "Do nmbd install"
	### jeffery marked ###
	#$(INSTALL_DIR) $(1)/etc/init.d
	#$(INSTALL_BIN) ./files/samba-nmbd.init $(1)/etc/init.d/samba-nmbd
	#$(INSTALL_DIR) $(1)/usr/sbin
	#$(INSTALL_BIN) $(PKG_BUILD_DIR)/source/bin/nmbd $(1)/usr/sbin/
	@echo "Finish nmbd install"
endef

define Package/$(PKG_NAME)-mountcifs/install
	@echo "Do mountcifs install"
	### jeffery marked ###
	#$(INSTALL_DIR) $(1)/usr/sbin
	#$(INSTALL_BIN) $(PKG_BUILD_DIR)/source/bin/mount.cifs $(1)/usr/sbin/
	#$(INSTALL_BIN) $(PKG_BUILD_DIR)/source/bin/umount.cifs $(1)/usr/sbin/
	@echo "Finish mountcifs install"
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
$(eval $(call BuildPackage,$(PKG_NAME)-nmbd))
$(eval $(call BuildPackage,$(PKG_NAME)-mountcifs))
