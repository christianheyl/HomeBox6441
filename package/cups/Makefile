#
# Copyright (C) 2006-2010 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=cups
PKG_VERSION:=1.4.8
PKG_RELEASE:=1

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION)-source.tar.bz2
PKG_SOURCE_URL:= \
	http://ftp.rz.tu-bs.de/pub/mirror/ftp.easysw.com/ftp/pub/cups/$(PKG_VERSION) \
	ftp://ftp.easysw.com/pub/cups/$(PKG_VERSION) \
	ftp://ftp3.easysw.com/pub/cups/$(PKG_VERSION)
PKG_MD5SUM:=0ec52d3f3c69bc2ab5ed70c594edbce6
#PKG_MD5SUM:=8776403ad60fea9e85eab9c04d88560d

TARGET_LDFLAGS+=-Wl,-rpath-link=$(STAGING_DIR)/usr/lib

include $(INCLUDE_DIR)/package.mk

define Package/cups
  SECTION:=net
  CATEGORY:=Network
  ### ctc ###
#  DEPENDS:=+zlib +libpthread +libpng +libjpeg +libstdcpp
  DEPENDS:=+zlib +libpthread +libstdcpp
  ###########
  TITLE:=Common UNIX Printing System
  URL:=http://www.cups.org/
endef

define Package/cups/description
	A printer spooling system for devices with USB or LP support.
endef

define Package/cups/conffiles
/etc/cups/classes.conf
/etc/cups/client.conf
/etc/cups/cupsd.conf
/etc/cups/printers.conf
endef

### ctc ###
define Build/Prepare
	$(call Build/Prepare/Default)
	if [ -d ./src ] ; then \
		$(CP) ./src/* $(PKG_BUILD_DIR)/. ; \
	fi
endef
##########

define Build/Configure
	$(call Build/Configure/Default, \
		--libdir=$(STAGING_DIR)/usr/lib \
		--with-printcap="/tmp/printcap" \
		--with-cups-user="nobody" \
		--with-cups-group="nogroup" \
		--without-perl \
		--without-python \
		--without-php \
		--without-arch64flags \
		--enable-shared \
		--disable-image \
		--disable-dbus \
		--disable-dnssd \
		--disable-launchd \
		--disable-ldap \
		--disable-pam \
		--disable-slp \
		--disable-gnutls \
		--disable-openssl \
		--disable-cdsassl \
		--disable-ssl \
		--disable-gssapi \
		--disable-64bit \
		--disable-tiff, \
		UNAME="Linux" \
		LIBS="$(TARGET_LDFLAGS) -lz" \
	)
		#LIBS="$(TARGET_LDFLAGS) -lz -lpng -ljpeg"
		### ctc ###
#		--libdir=$(STAGING_DIR)/usr/lib
#		--with-arch64flags
#		--enable-image
#		--enble-64bit
#		LIBS="$(TARGET_LDFLAGS) -lz"
		###########
endef

define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR) \
		$(TARGET_CONFIGURE_OPTS) \
		DSTROOT="$(PKG_INSTALL_DIR)" \
		STRIP="/bin/true" \
		all install
endef

define Package/cups/install
	rm -rf $(1)/etc/cups
	#Simon@2014/12/25, Prepare cupsd.conf based on lan ip settings before udev
	@echo "Do some dirty hacking..."
	$(CP) ./files/etc/cups/cupsd*.conf  $(PKG_INSTALL_DIR)/etc/cups/
	$(INSTALL_DIR) $(1)/etc/cups
	$(CP) $(PKG_INSTALL_DIR)/etc/cups/* $(1)/etc/cups/
	rm -rf $(1)/etc/cups/certs
	ln -sf /tmp $(1)/etc/cups/certs
	
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/bin/* $(1)/usr/bin/
	rm -f $(1)/usr/bin/cups-config
	
	$(INSTALL_DIR) $(1)/usr/lib
	### ctc ###
	$(CP) $(PKG_BUILD_DIR)/cups/libcups*.so*       $(1)/usr/lib/
	$(CP) $(PKG_BUILD_DIR)/filter/libcups*.so*     $(1)/usr/lib/
	$(CP) $(PKG_BUILD_DIR)/scheduler/libcups*.so*  $(1)/usr/lib/
	###########
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/cups $(1)/usr/lib/

#	$(INSTALL_DIR) $(1)/usr/share/cups/templates
#	$(CP) $(PKG_INSTALL_DIR)/usr/share/cups/templates/*.tmpl $(1)/usr/share/cups/templates/
#	$(INSTALL_DIR) $(1)/usr/share/cups/mime
#	$(CP) $(PKG_INSTALL_DIR)/usr/share/cups/mime/* $(1)/usr/share/cups/mime/


# --- Add by WindChen, for CUPS packages...
	$(INSTALL_DIR) $(1)/usr/share/cups
	$(CP) $(PKG_INSTALL_DIR)/usr/share/cups/* $(1)/usr/share/cups/
	$(INSTALL_DIR) $(1)/usr/share/doc/cups
	$(CP) $(PKG_INSTALL_DIR)/usr/share/doc/cups/* $(1)/usr/share/doc/cups/
# --- Add over ----------

	$(INSTALL_DIR) $(1)/usr/share/doc/cups
	$(CP) $(PKG_INSTALL_DIR)/usr/share/doc/cups/index.html $(1)/usr/share/doc/cups/
	$(CP) $(PKG_INSTALL_DIR)/usr/share/doc/cups/*.css $(1)/usr/share/doc/cups/
	$(CP) $(PKG_INSTALL_DIR)/usr/share/doc/cups/images $(1)/usr/share/doc/cups/
	
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) \
		$(PKG_INSTALL_DIR)/usr/sbin/{cupsaccept,cupsaddsmb,cupsctl,cupsd,cupsfilter,lpadmin,lpc,lpinfo,lpmove} \
		$(1)/usr/sbin/
	(cd $(1)/usr/sbin; ln -sf cupsaccept cupsreject; ln -sf cupsaccept cupsenable; ln -sf cupsaccept cupsdisable;)
	
	# overwrite default config with our own
	$(CP) ./files/etc/cups/* $(1)/etc/cups/
	
	# install initscript with priority 60
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/cupsd.init $(1)/etc/init.d/cupsd

    # 06-Nov-2012, Leo
	$(CP) $(1)/usr/lib/cups/filter/* $(1)/usr/bin

	#2014/05/14, marked by Simon
	#(cd $(1)/etc/cups; tar -zcvf $(1)/etc/cups.conf.tar.gz *;)
	#(cd $(1)/usr/lib/cups; tar -zcvf $(1)/usr/lib/cups.lib.tar.gz *;)
	#rm -rf $(1)/etc/cups
	#rm -rf $(1)/usr/lib/cups
	# Leo End
endef

define Build/InstallDev
	$(INSTALL_DIR) $(2)/bin
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/bin/cups-config $(2)/bin/
	$(INSTALL_DIR) $(1)/usr/include
	$(CP) $(PKG_INSTALL_DIR)/usr/include/cups $(1)/usr/include/
	$(INSTALL_DIR) $(1)/usr/lib
	### ctc ###
#	$(CP) $(PKG_INSTALL_DIR)/usr/lib/libcups*.so* $(1)/usr/lib/
	$(CP) $(PKG_BUILD_DIR)/cups/libcups*.so*    $(1)/usr/lib/
	$(CP) $(PKG_BUILD_DIR)/filter/libcups*.so*  $(1)/usr/lib/
	###########
endef

$(eval $(call BuildPackage,cups))
