#
# Main Makefile for building for Atheros Linux-based targets
#
# To use this:
# -- You need to work on a Linux development system with some specific
#  software tools installed (e.g. certain versions of gcc).
# -- You will need to set up the appropriate perforce client view for
#  your Atheros target. Note that client views are NOT under version
#  control (although build/clients was one attempt to do this)...
#  syncronize with your co-workers.
# -- Select an Atheros target.
#  An Atheros target is usually a particular board in combination
#  with a particular use of the board.
#  For a list of existing targets, see the content of
#  the build/scripts directory ... there is a subdirectory
#  thereof per target.
#  (To add a new target, not only must you add a subdirectory
#  to build/scripts, but additional per-target configuration
#  files will be needed in quite a few other places.)
# -- cd to the build subdirectory (this subdirectory) of your
#  perforce client root.
# -- Run make preceded by environmental variables, at LEAST including
#  BOARD_TYPE, e.g.
#  	BOARD_TYPE=pb44-router make
#   or alternately:
#  	export BOARD_TYPE=pb44-router
#  	make
#   ... which can easily be put into a script.
#
#
# Overview of what is made:
# Simply running this Makefile causes the $(INSTALL_ROOT) directory
# to be created... various files are then installed beneath $(INSTALL_ROOT).
# Eventually, $(INSTALL_ROOT) is (almost) the final target linux
# filesystem content; however, it will contain some files which will be
# deleted and some files which will be rewritten to optimize them;
# and other files (e.g. device files) may not be in their final form either.
# The directory $(IMAGE_ROOT) is created based on $(INSTALL_ROOT) doing
# fixups including deleting some files, doing library optimization etc.
# Based on $(IMAGE_ROOT), loadable filesystem images are created.
# In addition, outputs that will not be in the file system (e.g. linux kernel)
# are created.
# All of the final output files go into the directory $(IMAGEPATH)
# and some of these are also copied into $(TFTPPATH)...
# note that $(TFTPPATH) does NOT by default live beneath the client view
# directory, but instead is /tftpboot/<username> ... you can change this
# by defining TFTPPATH=<yourpath> in the make command line.
#
#
# Adding make instructions:
# Usually board/scripts/<target> is the correct place to do this.
# IMPORTANT: all "make targets" which add to $(INSTALL_ROOT)
# (i.e. to be added to the target linux file system)
# should be # added to the INSTALLS variable, e.g.:
# 	INSTALLS += widget_build
# Make sure you use "+=" in the above.
#

ifndef BOARD_TYPE
$(error "You have to define Board Type to use this Makefile")
endif

ifndef BUILD_TYPE
export BUILD_TYPE=jffs2
endif

ifeq ($(BOOT_FROM_NAND),1) #{
export NAND=-nand
else #}{
ifeq ($(ATH_DUAL_FLASH),1) #{
export NAND=-dual-flash
else #}{
export NAND=
endif #}
endif #}

export CFG_BOARD_TYPE := $(BOARD_TYPE)

ifeq ($(strip $(CONFIG_BASIC)),1)
export CONFIG_EXT=-basic
else
export CONFIG_EXT=
endif

ifneq (,$(findstring $(BOARD_TYPE),scoemu tb614 tb627 tb627_slave ap136 ap132 ap135 aph131 ap135_wapi))
override BOARD_TYPE = board955x
$(info ******************************)
$(info * Mapping $(CFG_BOARD_TYPE) to $(BOARD_TYPE) *)
$(info ******************************)
export DEFCONFIG=$(BOARD_TYPE)$(CONFIG_EXT)$(BUILD_CONFIG)_defconfig
else
export DEFCONFIG=$(BOARD_TYPE)$(CONFIG_EXT)$(BUILD_CONFIG)$(NAND)_defconfig
endif
#
# Include the specific configuration files from the config.boardtype file
# here.  This removes the need to set environmental variables through a
# script before building
#

include scripts/$(BOARD_TYPE)/config.$(BOARD_TYPE)

#
# Put in safety checks here to ensure all required variables are defined in
# the configuration file
#

#ifndef TOOLPREFIX
#$(error "Must specify TOOLPREFIX value")
#endif

ifndef TOOLCHAIN
$(error "Must specify TOOLCHAIN value")
endif

ifndef TOOLARCH
$(error "Must specify TOOLARCH value")
endif

ifndef KERNEL
$(error "Must specify KERNEL value")
endif

ifndef KERNELVER
$(error "Must specify KERNELVER value")
endif

ifndef KERNELTARGET
$(error "Must specify KERNELTARGET value")
endif

ifndef KERNELARCH
$(error "Must specify KERNELARCH value")
endif

ifndef BUSYBOX
$(error "Must specify BUSYBOX value")
endif


ifndef TFTPPATH
export TFTPPATH=/tftpboot/$(shell whoami)
endif

ifndef  BUILD_OPTIMIZED
export STRIP=$(TOOLPREFIX)strip
else
ifeq ($(BUILD_OPTIMIZED), y)
export OLDSTRIP=$(TOOLPREFIX)strip
export STRIP=$(TOOLPREFIX)sstrip
endif
endif

ifndef COMPRESSED_UBOOT
export COMPRESSED_UBOOT=0
endif

ifndef ATH_CONFIG_NVRAM
export ATH_CONFIG_NVRAM=0
endif

ifndef ATH_SINGLE_CFG
export ATH_SINGLE_CFG=0
endif

ifndef INSTALL_BLACKLIST
export INSTALL_BLACKLIST="None"
endif

ifndef EXTRAVERSION
EXTRAVERSION=$(shell if test -e $(KERNELPATH)/ath_version.mk ; then cat $(KERNELPATH)/ath_version.mk | sed s/EXTRAVERSION=//g; fi)
endif

export NANDJFFS2FILE=$(CFG_BOARD_TYPE)$(BUILD_CONFIG)$(BUILD_EXT)-nand-jffs2
export YAFFS2FILE=$(CFG_BOARD_TYPE)$(BUILD_CONFIG)$(BUILD_EXT)-yaffs2
export JFFS2FILE=$(CFG_BOARD_TYPE)$(BUILD_CONFIG)$(BUILD_EXT)-jffs2
export IMAGEPATH=$(TOPDIR)/images/$(CFG_BOARD_TYPE)$(BUILD_CONFIG)$(BUILD_EXT)
export MODCPPATH=$(TOPDIR)/modules/$(CFG_BOARD_TYPE)$(BUILD_CONFIG)$(BUILD_EXT)

export IMAGEPATH=$(TOPDIR)/images/$(CFG_BOARD_TYPE)$(BUILD_CONFIG)$(BUILD_EXT)$(NAND)$(CONFIG_EXT)

export INFOFILE=vmlinux$(BUILD_CONFIG)$(BUILD_EXT).info
export KTFILE=$(KERNELTARGET:.bin=$(BUILD_CONFIG)$(BUILD_EXT).bin)

ifneq ($(COMPRESSED_UBOOT),1)
export UBOOTFILE=u-boot$(BUILD_CONFIG)$(BUILD_EXT).bin
export UBOOT_BINARY=u-boot.bin
else
export UBOOTFILE=tuboot$(BUILD_CONFIG)$(BUILD_EXT).bin
export UBOOT_BINARY=tuboot.bin
endif
#
# Other environmental variables that are configured as per the configuration file
# specified above.  These contain all platform specific configuration items.
#

export TOPDIR=$(PKG_BUILD_DIR)
export INSTALL_ROOT=$(TOPDIR)/rootfs-$(CFG_BOARD_TYPE)$(BUILD_CONFIG)$(BUILD_EXT)$(NAND).build
$(shell mkdir -p $(INSTALL_ROOT))
$(shell mkdir -p $(INSTALL_ROOT)/sbin)
$(shell mkdir -p $(INSTALL_ROOT)/usr/sbin)
$(shell mkdir -p $(INSTALL_ROOT)/lib)
$(shell mkdir -p $(INSTALL_ROOT)/etc)
export IMAGE_ROOT=$(TOPDIR)/rootfs-$(CFG_BOARD_TYPE)$(BUILD_CONFIG)$(BUILD_EXT)$(NAND).optbuild
temp_BOARD_TYPE = $(strip $(subst fus, , $(CFG_BOARD_TYPE)))

ifeq ($(strip $(ATH_CARR_DIR)),)
export KERNELPATH=$(TOPDIR)/linux/kernels/$(KERNEL)
export MAKEARCH=$(MAKE) ARCH=$(KERNELARCH) CROSS_COMPILE=$(TOOLPREFIX) EXTRAVERSION=-$(EXTRAVERSION)

export TOOLPATH=$(TOPDIR)/build/$(TOOLCHAIN)/$(TOOLARCH)/
export BOOTLOADERDIR=$(TOPDIR)/boot/redboot

export UBOOTDIR=$(TOPDIR)/boot/u-boot
endif

# Save PATH for later use for compiling host-only tools etc.
export ORIGINAL_PATH:=$(PATH)
# Set PATH so we find target compiler when say "gcc", etc.
# as well as other tools we expect to find.
ifeq ($(BOARD_TYPE),onu1)
TMPPATH=$(TOPDIR)/build/$(TOOLCHAIN)/opt/eldk/usr/bin:$(TOPDIR)/build/$(TOOLCHAIN)/opt/openwrt/kamikaze_7.09/staging_dir_mips/bin
export PATH:=${TMPPATH}:${PATH}
export KERNELPATH=$(TOPDIR)/linux/kernel_v3.1.1/$(KERNEL)
export APPPATH=$(TOPDIR)/linux/app_v3.1.1
ifeq ($(VOIP_BUILD),web)
export PATH:=$(TOPDIR)/build/onu-toolchain/opt/eldk/usr/bin:$(TOPDIR)/build/onu-toolchain/opt/openwrt/kamikaze_7.09/staging_dir_mips/bin:${PATH}
endif    
else
ifeq ($(BOARD_TYPE),onu1e)
TMPPATH=$(TOPDIR)/build/$(TOOLCHAIN)/opt/eldk/usr/bin:$(TOPDIR)/build/$(TOOLCHAIN)/opt/openwrt/kamikaze_7.09/staging_dir_mips/bin
export PATH:=${TMPPATH}:${PATH}
export KERNELPATH=$(TOPDIR)/linux/kernel_v3.1.1/$(KERNEL)
export APPPATH=$(TOPDIR)/linux/app_v3.1.1
ifeq ($(VOIP_BUILD),web)
export PATH:=$(TOPDIR)/build/onu-toolchain/opt/eldk/usr/bin:$(TOPDIR)/build/onu-toolchain/opt/openwrt/kamikaze_7.09/staging_dir_mips/bin:${PATH}
endif 
else
export PATH:=$(TOPDIR)/build/util:$(TOOLPATH)/bin:$(TOPDIR)/linux:$(TOPDIR)/build:$(BOOTLOADERDIR)/ecos/tools/bin:`pwd`:${PATH}
endif
endif

# madwifi
export HAL=$(TOPDIR)/wlan/madwifi/hal/main
export ATH_PHYERR=$(TOPDIR)/wlan/madwifi/dfs
export ATH_RATE=$(TOPDIR)/wlan/madwifi/ratectrl11n/
export MODULEPATH=$(INSTALL_ROOT)/lib/modules/$(KERNELVER)/net

# This is to allow the target file system size to be specified on the command
# line, if desired
ifndef TARGETFSSIZE
export TARGETFSSIZE=2621440
endif

ifeq ($(NAND),) #{
# This allows the target flash erase block size to specified... it MUST be specified
# correctly or else e.g. jffs2 will break.
ifndef ERASEBLOCKSIZE
export ERASEBLOCKSIZE=0x10000
endif
else #}{
ifndef ERASEBLOCKSIZE
export ERASEBLOCKSIZE=0x20000
else
temp := $(NANDJFFS2FILE)
NANDJFFS2FILE = $(temp)-$(ERASEBLOCKSIZE)
endif

ifndef NAND_PAGE_SIZE
export NAND_PAGE_SIZE=0x800
else
temp := $(NANDJFFS2FILE)
NANDJFFS2FILE = $(temp)-$(NAND_PAGE_SIZE)
endif
endif #}


ENTRY=`readelf -a vmlinux|grep "Entry"|cut -d":" -f 2`
LDADR=`readelf -a vmlinux|grep "\[ 1\]"|cut -d" " -f 26`
RDADR=`strings vmlinux | grep rd_start= | sed 's/^.*rd_start=//;s/ .*//'`

#
# Include the board specific make file
#

include scripts/$(BOARD_TYPE)/Makefile.$(BOARD_TYPE)

ifndef WIRELESSTOOLNAMES
$(warning "Should specify WIRELESSTOOLNAMES value")
## Note: WIRELESSTOOLNAMES can contain more files that we actually have...
## e.g. WIRELESSTOOLNAMES := athstats athstatsclr athdebug 80211stats 80211debug \
		athkey athampdutrc athcwm atrc pktlogconf pktlogdump radartool
WIRELESSTOOLNAMES :=
endif


#
# Common targts
#

ifndef AP_TYPE
# The INSTALL_ROOT is similar but not exactly what appears on the
# target filesystem; it is copied and converted into IMAGE_ROOT
# which is space-optimized.
rootfs_prep:	# this is prep of the INSTALL_ROOT, not the final fs directory
	@echo Begin rootfs_prep $(INSTALL_ROOT)
	rm -rf $(IMAGE_ROOT)
	rm -rf $(INSTALL_ROOT); mkdir $(INSTALL_ROOT)
	cp -R ../rootfs/common/* $(INSTALL_ROOT)
	cp -Rf ../rootfs/$(BOARD_TYPE)$(BUILD_CONFIG)/* $(INSTALL_ROOT)
	chmod 755 $(INSTALL_ROOT)/etc/rc.d/*
	chmod 755 $(INSTALL_ROOT)/etc/ath/*
	chmod 755 $(INSTALL_ROOT)/etc/ath/default/*
	@echo End rootfs_prep $(INSTALL_ROOT)

else
#
# For the retail AP designs, a new common area is used that contains the
# web interface and supporting files.  Also provides a common fusion based
# filesystem.
#

rootfs_prep:
	@echo Begin rootfs_prep $(INSTALL_ROOT)
	rm -rf $(IMAGE_ROOT)
	rm -rf $(INSTALL_ROOT); mkdir $(INSTALL_ROOT)
#	cp -R ../rootfs/cgiCommon/* $(INSTALL_ROOT)
#	cp -rf ../rootfs/cgiCommon/etc/ath.$(AP_TYPE)/* $(INSTALL_ROOT)/etc/ath
	rm -rf $(INSTALL_ROOT)/etc/ath.single $(INSTALL_ROOT)/etc/ath.dual
#	cp -rf ../rootfs/cgiCommon/usr/www.$(AP_TYPE)/* $(INSTALL_ROOT)/usr/www
#	rm -rf $(INSTALL_ROOT)/usr/www.single
#	rm -rf $(INSTALL_ROOT)/usr/www.dual
#	rm -rf $(INSTALL_ROOT)/etc/ath/hostapd0.7.0_conf
ifeq ($(KERNELVER),2.6.31)
	rm -rf $(INSTALL_ROOT)/lib/modules/2.6.15
endif
	if test -d "../rootfs/$(BOARD_TYPE)$(BUILD_CONFIG)" ; then  \
 	    cp -Rf ../rootfs/$(BOARD_TYPE)$(BUILD_CONFIG)/* $(INSTALL_ROOT) ; \
        else \
 	    cp -Rf ../rootfs/$(BOARD_TYPE)/* $(INSTALL_ROOT) ; \
	fi;
	chmod 755 $(INSTALL_ROOT)/etc/rc.d/*
	chmod 755 $(INSTALL_ROOT)/etc/ath/*
#	mv -f $(INSTALL_ROOT)/etc/ath/apoob $(INSTALL_ROOT)/sbin
#	mv -f $(INSTALL_ROOT)/etc/ath/wpscli_ap $(INSTALL_ROOT)/sbin
#	mv -f $(INSTALL_ROOT)/etc/ath/wpscli_sta $(INSTALL_ROOT)/sbin
	cp -f scripts/$(BOARD_TYPE)/brcmath_restart.sh $(INSTALL_ROOT)/sbin
endif

ifeq ($(BUILD_UCLIBC_DEBUG),y)
BUILD_UCLIBC_DEBUG_FILTER = sed -e '/^DODEBUG=n/s/=n/=y/'
else
BUILD_UCLIBC_DEBUG_FILTER = "cat"
endif
toolchain_build: rootfs_prep
	echo "***** Checking toolchain status .."
	if [ "$(TOOLCHAIN)" = "gcc-4.3.3" -o "$(TOOLCHAIN)" = "gcc-4.5.1" ]; then \
	if test -f "$(TOOLCHAIN)/$(TOOLARCH)/.tcbuilt"; then echo "***** Toolchain already built.... ******"; \
	else \
	cd $(TOOLCHAIN) && \
	cp -f $(BOARD_TYPE).config .config && \
	rm -f toolchain/uClibc/uClibc.config && \
	cat toolchain/uClibc/$(BOARD_TYPE).config | $(BUILD_UCLIBC_DEBUG_FILTER) >toolchain/uClibc/uClibc.config && \
	$(MAKE) && touch $(TOOLARCH)/.tcbuilt; \
	fi;     \
	else \
	cd $(TOOLCHAIN) && \
	cp -f $(BOARD_TYPE).config .config && \
	rm -f toolchain/uClibc/uClibc.config && \
	cat toolchain/uClibc/$(BOARD_TYPE).config | $(BUILD_UCLIBC_DEBUG_FILTER) >toolchain/uClibc/uClibc.config && \
	$(MAKE); \
	fi;
	mkdir -p $(INSTALL_ROOT)/lib && \
	cd $(TOOLCHAIN) && $(MAKE) PREFIX=$(INSTALL_ROOT) install;
	# gdbserver to support debugging (if it has been created)
	if [ -f $(GDB_INSTALL_ROOT)/gdbserver ] ; then \
	    mkdir -p $(INSTALL_ROOT)/usr/bin ; \
	    cp -f $(GDB_INSTALL_ROOT)/gdbserver $(INSTALL_ROOT)/usr/bin ; \
	fi
	@echo End Making toolchain_build at `date`

toolchain_clean:
	@echo Cleaning toolchain ....
	cd $(TOOLCHAIN) &&  $(MAKE) clean && rm $(TOOLCHAIN)/$(TOOLARCH)/.tcbuilt

check_tftp: image_prep
	mkdir -p $(IMAGEPATH)
	if test -d $(TFTPPATH); then echo $(TFTPPATH) exists; else mkdir $(TFTPPATH); fi;
image_prep:
	if test -d $(TOPDIR)/images; then echo $(TOPDIR)/images exists; \
	else \
	mkdir $(TOPDIR)/images; \
	fi;
	if test -d $(IMAGEPATH); then echo $(IMAGEPATH) exists; \
	else \
	mkdir -p $(IMAGEPATH); \
	fi;
	if test -d $(TOPDIR)/modules; then echo $(TOPDIR)/modules exists; \
	else \
	mkdir $(TOPDIR)/modules; \
	fi;
	if test -d $(MODCPPATH); then echo $(MODCPPATH) exists; \
	else \
	mkdir -p $(MODCPPATH); \
	fi;


kernel_clean:
	cd $(KERNELPATH) &&  $(MAKEARCH) mrproper

kernel_info:
	cd $(KERNELPATH) && rm -f $(INFOFILE)
	cd $(KERNELPATH) && echo "entry:"${ENTRY} >> $(INFOFILE)
	cd $(KERNELPATH) && echo "link: 0x"${LDADR} >> $(INFOFILE)
	cp $(KERNELPATH)/$(INFOFILE) $(TFTPPATH)
	cp $(KERNELPATH)/$(INFOFILE) $(IMAGEPATH)
	@( \
		cd $(KERNELPATH) && \
		echo re && \
		echo re && \
		echo load ${RDADR} `echo $(TFTPPATH) | cut -f3- -d/`/$(BOARD_TYPE)-ramdisk.gz bin && \
		echo load 0x${LDADR} `echo $(TFTPPATH) | cut -f3- -d/`/$(KERNELTARGET) bin && echo go ${ENTRY} \
	) > $(TFTPPATH)/readme

#
# If configured in kernel config, an initramfs trampoline is used.
# Should not hurt to build it anyway.
#
initramfs_prep:
	cd $(TOPDIR)/boot/initramfs && $(MAKE) clean && \
            $(MAKE) CC=$(TOOLPREFIX)gcc AR=$(TOOLPREFIX)ar LD=$(TOOLPREFIX)ld NM=$(TOOLPREFIX)nm all

#
# Use different kernel_build rules depending on the type of target
#

ifeq ($(KERNELTARGET), zImage)
kernel_build: image_prep initramfs_prep
	@echo " Making Kernel Build Version $(EXTRAVERSION)" at `date`
	exit
	cd $(KERNELPATH) && $(MAKEARCH) $(BOARD_TYPE)$(CONFIG_EXT)$(BUILD_CONFIG)_defconfig
	cd $(KERNELPATH) && $(MAKEARCH)
	cd $(KERNELPATH)/arch/$(KERNELARCH)/boot && \
	cp $(KERNELTARGET) $(TFTPPATH) && cp $(KERNELTARGET) $(IMAGEPATH)
else
ifeq ($(BOARD_TYPE),onu1)
kernel_build: image_prep initramfs_prep
	@if [ -e $(KERNELPATH)/root/modules ]; then cd $(KERNELPATH)/root/modules/ && rm *.ko -rf; fi
	@echo "clean opconn KOs"
ifeq ($(OPCONN),en)
	@echo " Making APP Build Version $(EXTRAVERSION)" at `date`
	cd $(APPPATH) && make clean f33 && sh ./f33build
else
endif
	@echo " Making Kernel Build Version $(EXTRAVERSION)" at `date`
	rm -f $(KERNELPATH)/root/sbin/masterd
	cd $(KERNELPATH) && make onu_f23p_voip_defconfig && sh ./mk.sh
else
ifeq ($(BOARD_TYPE),onu1e)
kernel_build: image_prep initramfs_prep
	@if [ -e $(KERNELPATH)/root/modules ]; then cd $(KERNELPATH)/root/modules/ && rm *.ko -rf;fi
	@echo "clean opconn KOs"
ifeq ($(OPCONN),en)
	@echo " Making APP Build Version $(EXTRAVERSION)" at `date`
	cd $(APPPATH) && make clean c13v1e && sh ./c13-v1e-build
else
	@echo " Kernel path $(KERNELPATH)"
endif

	@echo " Making Kernel Build Version $(EXTRAVERSION)" at `date`
	cd $(KERNELPATH) && make onu_respin_c13_spi_defconfig && sh ./mk.sh
else
kernel_build: image_prep initramfs_prep
	@echo " Making Kernel Build Version $(EXTRAVERSION)" at `date`
	cd $(KERNELPATH) && $(MAKEARCH) $(DEFCONFIG)
	cd $(KERNELPATH) && $(MAKEARCH)
	cd $(KERNELPATH) && $(MAKEARCH) $(KERNELTARGET)
	cd $(KERNELPATH)/arch/$(KERNELARCH)/boot && \
	cp $(KERNELTARGET) $(TFTPPATH)/$(KTFILE) && \
	cp $(KERNELTARGET) $(IMAGEPATH)/$(KTFILE) && \
	gzip -9c $(KERNELTARGET) > $(KERNELTARGET).gz && \
	cp $(KERNELTARGET).gz $(IMAGEPATH)/$(KTFILE).gz;
endif
endif
endif

redboot_build:
	@echo Making redboot at `date`
	cd $(BOOTLOADERDIR) && make $(BOARD_TYPE)_rom
	cp $(BOOTLOADERDIR)/rom_bld/install/bin/redboot.rom $(IMAGEPATH)/redboot.$(BOARD_TYPE).rom
	cd $(BOOTLOADERDIR) && make $(BOARD_TYPE)_ram
	cp $(BOOTLOADERDIR)/ram_bld/install/bin/redboot.bin $(IMAGEPATH)/redboot.$(BOARD_TYPE).bin
	cp $(BOOTLOADERDIR)/ram_bld/install/bin/redboot.srec $(IMAGEPATH)/redboot.$(BOARD_TYPE).srec
	@echo End Making redboot at `date`

uboot:
	@echo Making uboot at `date`
	cd $(UBOOTDIR) && $(MAKEARCH) mrproper
ifneq ($(BOARD_TYPE), $(temp_BOARD_TYPE))
	echo ====Using $(temp_BOARD_TYPE) config for $(BOARD_TYPE) ===
	cd $(UBOOTDIR) && $(MAKEARCH) $(temp_BOARD_TYPE)_config
else
	cd $(UBOOTDIR) && $(MAKEARCH) $(CFG_BOARD_TYPE)_config
endif
	@echo ========= build dir: $(TOPDIR)/build ============
	cd $(UBOOTDIR) && $(MAKEARCH) all BUILD_DIR=$(TOPDIR)/build
	cp -f $(UBOOTDIR)/${UBOOT_BINARY} ${IMAGEPATH}/${UBOOTFILE}
	cp -f $(UBOOTDIR)/${UBOOT_BINARY} $(TFTPPATH)/${UBOOTFILE}
	@echo End Making uboot at `date`

busybox_clean:
	@echo Cleaning busybox
	cd ../apps/$(BUSYBOX) && make clean;

voip_clean:
	@echo Cleaning VoIP module
	cd ../apps/gateway/services/phone && make clean;

utelnetd:
	@echo make utelnetd
	cd ../apps/utelnetd-0.1.9 && make clean install

busybox_build:
	@echo Making busybox at `date`
	if test -f  "../apps/$(BUSYBOX)/defconfig-$(BOARD_TYPE)$(BUILD_CONFIG)$(BUILD_DEBUG)"; then \
	cd ../apps/$(BUSYBOX) && make clean && \
	cp -f defconfig-$(BOARD_TYPE)$(BUILD_CONFIG)$(BUILD_DEBUG) .config && \
	    $(MAKE) && $(MAKE) PREFIX=$(INSTALL_ROOT) install; \
	else \
	    cd ../apps/$(BUSYBOX) && make clean && \
	    cp -f  defconfig-$(BOARD_TYPE)$(BUILD_DEBUG) .config && \
	    $(MAKE) && $(MAKE) PREFIX=$(INSTALL_ROOT) install; \
	fi;
	@echo End Making busybox at `date`

# wastemem is a very small but very useful development tool
# If you really don't want it in the build,
# add it to the optimzation blacklist file!
wastemem_build:
	@echo Making wastemem
	cd ../apps/wastemem && make clean && make install
wastemem_clean:
	cd ../apps/wastemem && make clean
# Ugly hack so you get wastemem if you get busybox...
## use /proc/sys/vm/min_free_kbytes instead:   busybox_build: wastemem_build

spectral_app_clean:
ifeq ($(ATH_SUPPORT_SPECTRAL),1)
	@echo Cleaning spectral
	cd ../apps/spectral && make clean;
endif

spectral_app_build: spectral_app_clean
ifeq ($(ATH_SUPPORT_SPECTRAL),1)
	@echo making Spectral tools
	cd ../apps/spectral && $(MAKE) CC=$(TOOLPREFIX)gcc AR=$(TOOLPREFIX)ar LD=$(TOOLPREFIX)ld -f Makefile
	cp ../apps/spectral/ssd $(INSTALL_ROOT)/sbin/
	cp ../apps/spectral/athssd $(INSTALL_ROOT)/sbin/
else
	@echo Spectral feature is not enabled, skipping Spectral applications build
endif

diag_build:
	@echo diag_build
	cd $(HAL)/diag && make

hal_build:
	@echo making hal
	if test -n "$(MADWIFITARGET)"; then \
	cd $(HAL)/linux && make TARGET=$(MADWIFITARGET) clean &&  \
	make TARGET=$(MADWIFITARGET) && make TARGET=$(MADWIFITARGET) release; \
	fi

wpc_build:
ifeq ($(ATH_SUPPORT_WIFIPOS),1)
	@echo Making Wifi Positioning Controller
	if (test -e ../apps/positioning/wpc) then \
	cd ../apps/positioning/wpc; make clean; make; \
	cp wpc $(INSTALL_ROOT)/sbin; \
	fi
else
	@echo Wifipositioning feature is not enabled
endif

ifeq ($(BUILD_WPA2),y)  ####################################
ifeq ($(BUILD_WPA2_ATHR),y)#########
#apps/athr-hostap provide a third generation (compared with apps/wpa and apps/wpa2)
#       of authentication (including WPS) programs:
#       hostapd, wpa_supplicant, etc.
#   It installs via $(INSTALL_ROOT).
#   It depends only on header files from the driver, and linux driver
#   (madwifi) header files specified by $(MADWIFIPATH)
wpa2:rootfs_prep
	@echo Make athr-hostap ath `date`
	rm -f ../apps/athr-hostap/hostapd/.config ../apps/athr-hostap/wpa_supplicant/.config
ifneq ($(BUILD_WPA2_NO_HOSTAPD),y)
	@echo UPDATING hostapd
	cp scripts/$(BOARD_TYPE)/athr_hostapd.conf ../apps/athr-hostap/hostapd/.config
	cd ../apps/athr-hostap/hostapd/ && \
         $(MAKE) clean && $(MAKE) CC=$(TOOLPREFIX)gcc AR=$(TOOLPREFIX)ar LD=$(TOOLPREFIX)ld hostapd hostapd_cli && \
         cp hostapd $(INSTALL_ROOT)/sbin && \
	cp hostapd_cli $(INSTALL_ROOT)/sbin
endif
ifneq ($(BUILD_WPA2_NO_WPA_SUPPLICANT),y)
	@echo UPDATING wpa_supplicant
	cp scripts/$(BOARD_TYPE)/athr_supplicant.conf ../apps/athr-hostap/wpa_supplicant/.config
	cd ../apps/athr-hostap/wpa_supplicant && make clean && \
        $(MAKE)  CC=$(TOOLPREFIX)gcc AR=$(TOOLPREFIX)ar LD=$(TOOLPREFIX)ld wpa_supplicant wpa_cli && \
        cp wpa_supplicant $(INSTALL_ROOT)/sbin && \
	cp wpa_cli $(INSTALL_ROOT)/sbin

endif
	@echo End Making athr-hostap

wpa2_clean:
	@rm -f ../apps/athr-hostap/hostapd/.config ../apps/athr-hostap/wpa_supplicant/.config
ifneq ($(BUILD_WPA2_NO_HOSTAPD),y)
	@cp scripts/$(BOARD_TYPE)/athr_hostapd.conf ../apps/athr-hostap/hostapd/.config
	@$(MAKE) -C $(TOPDIR)/apps/athr-hostap/hostapd clean
endif
ifneq ($(BUILD_WPA2_NO_WPA_SUPPLICANT),y)
	@cp scripts/$(BOARD_TYPE)/athr_supplicant.conf ../apps/athr-hostap/wpa_supplicant/.config
	@$(MAKE) -C $(TOPDIR)/apps/athr-hostap/wpa_supplicant clean
endif
else #########
# apps/wpa2 provides a second generation (as compared with apps/wpa)
#       of authentication (including WPS) programs:
#       hostapd, wpa_supplicant, etc.
# 	It installs via $(INSTALL_ROOT).
# 	It depends only on header files from the driver, and linux driver
# 	(madwifi) header files specified by $(MADWIFIPATH)

wpa2: wpa2_clean rootfs_prep
	@echo Making wpa2 at `date`
	cd ../apps/wpa2 && $(MAKE) 	\
		CC=$(TOOLPREFIX)gcc AR=$(TOOLPREFIX)ar LD=$(TOOLPREFIX)ld
	@echo End Making wpa2 at `date`

wpa2_clean:
	cd ../apps/wpa2 && $(MAKE) clean
clean: wpa2_clean
endif
else  ##################################################
# apps/wpa provides older generation of hostapd, wpa_supplicant, etc.

hostapd: openssl
	if ( test -e ../apps/wpa/hostapd-0.4.8 ) then \
	cd ../apps/wpa/hostapd-0.4.8; cp -f defconfig .config; make clean; make; \
	make PREFIX=$(INSTALL_ROOT)/sbin CONFIG_PATH=$(INSTALL_ROOT)/etc/ath DEFAULT_CFG=$(INSTALL_ROOT)/etc/ath/default install; \
	fi

openssl:
	if (test -e ../apps/wpa/wsc/lib/openssl-0.9.8a ) then \
	chmod -R 755 ../apps/wpa/wsc/lib/openssl-0.9.8a; \
	cd ../apps/wpa/wsc/lib/openssl-0.9.8a; make clean; make; fi

wsc: hostapd
	if (test -e ../apps/wpa/wsc/src/linux ) then \
	chmod -R 755 ../apps/wpa/wsc/src/lunux; \
	cd ../apps/wpa/wsc/src/linux; make clean; \
	make PREFIX=$(INSTALL_ROOT)/sbin CONFIG_PATH=$(INSTALL_ROOT)/etc/ath DEFAULT_CFG=$(INSTALL_ROOT)/etc/ath/default install; \
	fi

wpa_supplicant:
	if (test -e ../apps/wpa/wpa_supplicant-0.5.5 ) then \
	cd ../apps/wpa/wpa_supplicant-0.5.5; cp -f defconfig .config; make clean; \
    make; make PREFIX=$(INSTALL_ROOT)/sbin install; \
	fi

wps_enrollee:
	if (test -e ../apps/wpa/wps_enrollee) then \
	cd ../apps/wpa/wps_enrollee; make clean; make; \
    make PREFIX=$(INSTALL_ROOT)/sbin install; \
	fi
endif ##################################################

sar:
	@echo making sar
	cd ../apps/sysstat-6.0.1/ && rm -f sysstat.cron.daily && rm -f sysstat.cron.hourly && $(MAKE) CC=$(TOOLPREFIX)gcc
	cd ../apps/sysstat-6.0.1/ && cp sar $(INSTALL_ROOT)/sbin
	cd ../apps/sysstat-6.0.1/ && cp sadc $(INSTALL_ROOT)/sbin/

profiling: kernel_build
	@echo making oprofile
	cd $(OPROFILE_UTILS) && ./build_utils.sh $(OPROFILE_UTILS) $(KERNELPATH) $(TOOLPREFIX);
	cd $(OPROFILE_ROOT)/oprofile/; \
	./configure --host=mips-linux --target=mips-linux --with-linux=$(KERNELPATH) --with-kernel-support --with-extra-libs=$(OPROFILE_LIBS) --with-extra-includes=$(OPROFILE_INCLUDES); \
	make clean; make CC=$(TOOLPREFIX)gcc AR=$(TOOLPREFIX)ar LN=$(TOOLPREFIX)ln; make DESTDIR=$(INSTALL_ROOT) install;
	cd $(OPROFILE_ROOT)/scripts && ./mkpackage.sh $(OPROFILE_ROOT) $(INSTALL_ROOT) $(KERNELPATH);
	cp $(OPROFILE_ROOT)/package/prof.tar.gz $(TFTPPATH)/;
	cp $(KERNELPATH)/arch/mips/oprofile/oprofile.ko $(MODULEPATH)/

flash_mac:
	@echo making flash_mac
	cd ../apps/flash_mac/ && make CC=$(TOOLPREFIX)gcc STRIP=$(TOOLPREFIX)strip && cp read_cfg $(INSTALL_ROOT)/usr/bin/


# Optional library optimization (upon IMAGE_ROOT), enable with
#            BUILD_LIBRARYOPT := y
#
# Library optimization removes unused code from shared libraries.
# Read libraryopt/README-libraryopt.txt for details.
LIBOPTTEMP=$(TOPDIR)/build/libopt.temp
#
# Second stage of library optimization is done on the copy of $(INSTALL_ROOT)
# which is $(IMAGE_ROOT) .
# NOTE: $(INSTALL_ROOT) executables must NOT be stripped!
# that would interfere with debugging and perhaps with library optimization.
#
# The main problems with using library optimizer are:
# -- The build procedure is fragile and can break with any tools
#    or c library upgrade.
# -- Executables not visible at build time but added later may fail
#    unless they have been staticly linked.
#
ifeq ($(BUILD_LIBRARYOPT),y)

LIBOPTINFOSRC=$(TOPDIR)/build/libraryopt/optinfo
LIBOPTSCRIPTSRC=$(TOPDIR)/build/libraryopt/libraryopt-1.0.1

# libopt requires target/usr/lib/optinfo below LIBOPTTEMP
LIBOPTTEMPINFO=$(LIBOPTTEMP)/target/usr/lib/optinfo
LIBOPTTOOLSRC=$(TOPDIR)/build/$(TOOLCHAIN)/$(TOOLARCH)
LIBOPTTOOLSRC2=$(TOPDIR)/build/$(TOOLCHAIN)/toolchain_$(TOOLARCH)/uClibc
endif

# The IMAGE_ROOT is created from the INSTALL_ROOT by making
# a copy and optimizing the amount of file system space consumed
# through a variety of methods.
# This will be the file system actually installed on target.
# NOTE! It is important that $(INSTALLS) contains a list of all make targets to
# be done in order that $(INSTALL_ROOT) is complete !!!!!!!!!!!!!
image_root: $(INSTALLS)
	@echo Making image root $(IMAGE_ROOT) at `date`
	rm -rf $(IMAGE_ROOT)
	rm -rf $(LIBOPTTEMP)
	cp -a $(INSTALL_ROOT) $(IMAGE_ROOT)
	# Remove unwanted files
	@for file in "$(INSTALL_BLACKLIST)" ; \
		do echo "Removing $(IMAGE_ROOT)/$$file"; rm -rf $(IMAGE_ROOT)/$$file ;done
	rm -rf $(IMAGE_ROOT)/include
	rm -rf $(IMAGE_ROOT)/man
	rm -rf $(IMAGE_ROOT)/usr/man
	rm -rf $(IMAGE_ROOT)/lib/*.a
	rm -rf $(IMAGE_ROOT)/usr/lib/*.a
	# could be:  rm -f $(IMAGE_ROOT)/usr/bin/gdbserver
	# Apply library optimizer (optional)
ifeq ($(BUILD_LIBRARYOPT),y)
	rm -rf $(LIBOPTTEMP)
	echo Preparing for library optimizer at `date`
	mkdir -p $(LIBOPTTEMP)
	# libopt expects tools in bin, all with same prefix,
	# including libopt and libindex scripts themselves.
	mkdir -p $(LIBOPTTEMP)/bin
	ln -s $(LIBOPTTOOLSRC)/bin/* $(LIBOPTTEMP)/bin/.
	ln -s $(LIBOPTSCRIPTSRC)/src/libopt $(LIBOPTTEMP)/bin/$(TOOLPREFIX)libopt
	ln -s $(LIBOPTSCRIPTSRC)/src/libindex $(LIBOPTTEMP)/bin/$(TOOLPREFIX)libindex
	# and for our own convenience we build a lib directory with all
	# of the various files we will need
	mkdir -p $(LIBOPTTEMP)/lib
	ln -s $(LIBOPTTOOLSRC)/lib/* $(LIBOPTTEMP)/lib/.
	# libgcc.a is hard to find. E.g. it can be found at:
	#  build/gcc-3.4.4-2.16.1/build_mips/lib/gcc/mips-linux-uclibc/3.4.4/libgcc.a
	ln -s $(LIBOPTTOOLSRC)/lib/gcc/*/*/libgcc.a $(LIBOPTTEMP)/lib/.
	ln -s $(LIBOPTTOOLSRC2)/*/*_so.a $(LIBOPTTEMP)/lib/.
	ln -s $(LIBOPTTOOLSRC2)/*/*/*_so.a $(LIBOPTTEMP)/lib/.
	ln -s $(LIBOPTTOOLSRC2)/lib/interp.os $(LIBOPTTEMP)/lib/.
	mkdir -p $(LIBOPTTEMPINFO)
	set -e ; \
        app_sofiles=`find $(INSTALL_ROOT) -name '*.so' -print` ; \
	for app_sofile in $$app_sofiles ; do \
          sobject=`basename $$app_sofile` ; \
          soname=`basename $$sobject .so` ; \
          aobject=`dirname $$app_sofile`/$$soname.a ; \
	  if [ -L $$app_sofile ] ; then true; else \
	    echo Looking at $$app_sofile ... ; \
	    if [ -d $(LIBOPTINFOSRC)/$$sobject ] ; then \
	      echo Creating libopt temp info for special shared object file $$sobject ; \
	      cp -a $(LIBOPTINFOSRC)/$$sobject $(LIBOPTTEMPINFO)/. ; \
	      cp -a $$app_sofile $(LIBOPTTEMPINFO)/$$sobject/.;  \
              ln -s $(LIBOPTTEMP)/lib $(LIBOPTTEMPINFO)/$$sobject/required; \
	      (cd $(LIBOPTTEMPINFO)/$$sobject && ./prebuild $(LIBOPTTEMP)/bin/$(TOOLPREFIX)libindex $(LIBOPTTOOLSRC)/lib) ; \
	    elif [ -f $$aobject ] ; then \
	      echo Creating libopt temp info for application shared object file $$sobject ; \
	      cp -a $(LIBOPTINFOSRC)/generic $(LIBOPTTEMPINFO)/$$sobject ; \
	      cp -a $$app_sofile $(LIBOPTTEMPINFO)/$$sobject/.;  \
              ln -s $(LIBOPTTEMP)/lib $(LIBOPTTEMPINFO)/$$sobject/required; \
              mkdir $(LIBOPTTEMPINFO)/$$sobject/apps; \
              for other_so_file in $$app_sofiles ; do \
                other_so=`basename $$other_so_file`  ;   \
                if [ $$other_so != $$sobject ] ; then \
                  ln -s $$other_so_file $(LIBOPTTEMPINFO)/$$sobject/apps/. ; \
                fi ; \
              done ; \
              ln -s $$aobject $(LIBOPTTEMPINFO)/$$sobject/. ; \
	      (cd $(LIBOPTTEMPINFO)/$$sobject && ./prebuild $(LIBOPTTEMP)/bin/$(TOOLPREFIX)libindex $$sobject) ; \
	      (cd $(LIBOPTTEMPINFO)/$$sobject && $(LIBOPTTEMP)/bin/$(TOOLPREFIX)objdump -p $$sobject | awk '/^ *NEEDED/{print $$2}' >needed ) ; \
            else echo Skipping $$sobject ; \
	    fi; \
	  fi; \
        done
	echo Running library optimizer at `date`
	$(LIBOPTTEMP)/bin/$(TOOLPREFIX)libopt $(IMAGE_ROOT)
	echo Done with library optimizer at `date`
endif  # BUILD_LIBRARYOPT
	# Now we can strip executables (also strip libraries if needed)
	cd $(IMAGE_ROOT)/lib && $(STRIP) *.so
	# find $(IMAGE_ROOT)/sbin -type f -perm -u+x -exec $(STRIP) '{}' ';'
	# find $(IMAGE_ROOT)/bin -type f -perm -u+x -exec $(STRIP) '{}' ';'
	# find $(IMAGE_ROOT)/usr/bin -type f -perm -u+x -exec $(STRIP) '{}' ';'
	find $(IMAGE_ROOT) -type f -perm -u+x -exec $(STRIP) '{}' ';'
ifeq ($(BUILD_OPTIMIZED),y)
	# Refer to kernel/module.c:load_module() in linux sources
	# for the sections that can be removed without affecting insmod
	find $(IMAGE_ROOT)/lib/modules/$(KERNELVER) -name "*.ko" -type f \
		-exec $(OLDSTRIP) \
			--strip-unneeded \
			--remove-section=__kcrctab \
			--remove-section=__kcrctab_gpl \
			--remove-section=__ex_table \
			--remove-section=__obsparm \
			--remove-section=__versions \
			--remove-section=.pdr \
			--remove-section=.mdebug.abi32 \
			--remove-section=.comment \
			--remove-section=__ksymtab_gpl_future \
			--remove-section=__kcrctab_gpl_future \
			--remove-section=__ksymtab_unused \
			--remove-section=__kcrctab_unused \
			--remove-section=__ksymtab_unused_gpl \
			--remove-section=__kcrctab_unused_gpl \
			--remove-section=.ctors \
			--remove-section=__markers \
			--remove-section=__tracepoints \
			--remove-section=_ftrace_events \
			--remove-section=__mcount_loc \
			-x '{}' ';'
endif
	# Some additional space savings is gained by using tar/gzip compression
	# on wireless tools, which get unpacked by rcS script into /tmp
	# ram disk... perhaps a waste of ram however.
	# The amount of flash space thus saved is not large.
	@echo Warnings from tar about missing files are normal for some targets.
	if [ -n "$(WIRELESSTOOLNAMES)" -a "$(BUILD_TYPE)" = jffs2 ] ; then cd $(IMAGE_ROOT)/sbin && \
	    tar --ignore-failed-read -czf debug.tgz $(WIRELESSTOOLNAMES) && \
	    rm -f $(WIRELESSTOOLNAMES) && \
	    for tool in $(WIRELESSTOOLNAMES) ; do ln -s /tmp/tools/$$tool .; done; \
	    fi
	@echo DONE BUILDING image_root at `date`

ram_build: image_root
	@echo Making ramfs at `date`
	cd util && \
        genext2fs -b 8192 -i 512 -D ../scripts/$(BOARD_TYPE)/dev.txt -d $(IMAGE_ROOT) $(CFG_BOARD_TYPE)-ramdisk
	cd util && gzip --best -f $(CFG_BOARD_TYPE)-ramdisk
	cd util && cp $(CFG_BOARD_TYPE)-ramdisk.gz $(TFTPPATH)
	cd util && cp $(CFG_BOARD_TYPE)-ramdisk.gz $(IMAGEPATH)
	@echo Done Making ramfs at `date`

uimage:	kernel_build
	@echo Making uImage at `date`
	cd util && mkuImage.sh $(UBOOTDIR)/tools $(KERNELPATH) "$(BUILD_CONFIG)$(BUILD_EXT)"
	@echo Done Making uImage at `date`

mkyaffs2image: $(KERNELPATH)/fs/yaffs2/utils/mkyaffs2image.c
	cd $(KERNELPATH)/fs/yaffs2/utils/ && make clean && make
	cp $(KERNELPATH)/fs/yaffs2/utils/mkyaffs2image util

# mkyaffs2image: image building tool for YAFFS2
# usage: mkyaffs2image dir image_file [convert]
# 	dir		the directory tree to be converted
# 	image_file	the output file to hold the image
# 	convert		produce a big-endian image from a little-endian machine
#	bs		block size (to pad till end of block)

yaffs2_build: image_root mkyaffs2image
	@echo making $@
	cd $(IMAGEPATH) && \
	$(TOPDIR)/build/util/mkyaffs2image \
		$(IMAGE_ROOT) \
		$(YAFFS2FILE) \
		convert \
		bs 2112 \
		dev $(TOPDIR)/build/scripts/$(BOARD_TYPE)/dev.txt > /dev/null
	cp $(IMAGEPATH)/$(YAFFS2FILE) $(TFTPPATH)

# mkfs.jffs2: Usage: mkfs.jffs2 [OPTIONS]
# Make a JFFS2 file system image from an existing directory tree
#
# Options:
#   -p, --pad[=SIZE]       Pad output to SIZE bytes with 0xFF. If SIZE is
#                          not specified, the output is padded to the end of
#                          the final erase block
#   -r, -d, --root=DIR     Build file system from directory DIR (default: cwd)
#   -s, --pagesize=SIZE    Use page size (max data node size) SIZE (default: 4KiB)
#   -e, --eraseblock=SIZE  Use erase block size SIZE (default: 64KiB)
#   -c, --cleanmarker=SIZE Size of cleanmarker (default 12)
#   -n, --no-cleanmarkers  Don't add a cleanmarker to every eraseblock
#   -o, --output=FILE      Output to FILE (default: stdout)
#   -l, --little-endian    Create a little-endian filesystem
#   -b, --big-endian       Create a big-endian filesystem
#   -D, --devtable=FILE    Use the named FILE as a device table file
#   -f, --faketime         Change all file times to '0' for regression testing
#   -q, --squash           Squash permissions and owners making all files be owned by root
#   -U, --squash-uids      Squash owners making all files be owned by root
#   -P, --squash-perms     Squash permissions on all files
#   -h, --help             Display this help text
#   -v, --verbose          Verbose operation
#   -V, --version          Display version information

nandjffs2_build: image_root
	@echo making $@ for pagesize=$(NAND_PAGE_SIZE) eraseblock=$(ERASEBLOCKSIZE)
	cd $(IMAGEPATH) && \
	$(TOPDIR)/build/util/mkfs.jffs2 \
		--root=$(IMAGE_ROOT) \
		--no-cleanmarkers \
		--pagesize=$(NAND_PAGE_SIZE) \
		--eraseblock=$(ERASEBLOCKSIZE) \
		--big-endian \
		--devtable=$(TOPDIR)/build/scripts/$(BOARD_TYPE)/dev.txt \
		--squash \
		--output=$(NANDJFFS2FILE) \
		--pad=$(TARGETFSSIZE)
	cp $(IMAGEPATH)/$(NANDJFFS2FILE) $(TFTPPATH)


jffs2_build: image_root
	@echo Making jffs2 at `date`
	cd $(IMAGEPATH) && \
	$(TOPDIR)/build/util/mkfs.jffs2 --root=$(IMAGE_ROOT) --eraseblock=$(ERASEBLOCKSIZE) -b -D $(TOPDIR)/build/scripts/$(BOARD_TYPE)/dev.txt --squash -o $(JFFS2FILE) --pad=$(TARGETFSSIZE)
	cp $(IMAGEPATH)/$(JFFS2FILE) $(TFTPPATH)
	@echo Done Making jffs2 at `date`

# NOTE: initramfs_build does NOT necessarily build the primary initramfs image,
#       which may be done by the kernel... instead, it may build a secondary
#       initramfs image, which is loaded from flash by inittrampoline.
#       It is possible however to use this generated as the primary
#       initramfs image, but in this case you will need to insure that
#       the kernel build depends upon initramfs_build, and have an appropriate
#       kernel configuration of CONFIG_INITRAMFS_SOURCE.
initramfs_build : initramfs_prep image_root
	rm -f $(IMAGEPATH)/$(CFG_BOARD_TYPE)$(BUILD_CONFIG)$(BUILD_EXT).cpio*
	$(TOPDIR)/boot/initramfs/geninitramfs -device_table $(TOPDIR)/build/scripts/$(BOARD_TYPE)/dev.txt -copy $(IMAGE_ROOT) /  >$(IMAGEPATH)/$(CFG_BOARD_TYPE)$(BUILD_CONFIG)$(BUILD_EXT).cpio
	lzma $(IMAGEPATH)/$(CFG_BOARD_TYPE)$(BUILD_CONFIG)$(BUILD_EXT).cpio

# An empty jffs2 filesystem may be used in conjunction with squashfs or initramfs to hold
# 	non-volatile storage.
# 	NOTE: We need an empty directory for mkfs.jffs2 to copy, so we temporarily create one.
empty_jffs2_build:
	cd $(IMAGEPATH) && rm -rf empty empty-jffs2 && mkdir empty && \
	$(TOPDIR)/build/util/mkfs.jffs2 --root=$(IMAGEPATH)/empty --eraseblock=$(ERASEBLOCKSIZE) -b --squash -o empty-jffs2 --pad=$(TARGETFSSIZE) && \
	rm -rf empty
	cp $(IMAGEPATH)/empty-jffs2 $(TFTPPATH)

ifeq ($(VOIP_BUILD), web)
web_clean:
	@echo remove the bin and lib of onu web uImage
	cd $(KERNELPATH)/root/ && rm -fr sbin/masterd sbin/shttpd usr/bin/web_ssi usr/bin/xmib etc/rc.d/rc.web webs && \
	cd lib/ && rm -fr libath.a libath.so libbrouting.a libcares.a libcares.so libcgic.a libdevinfo.a libgwmib.a liblan.a \
		libnvs.a libpassword.a libpolarssl.a libpolarssl.so libwcm.a libzlib.a
	@echo remove the bin and lib of onu web uImage 
endif

ifeq ($(VOIP_BUILD),M)
clean: kernel_clean busybox_clean voip_clean
	@echo CAUTION THIS WILL NOT CLEAN EVERYTHING
	rm -rf $(TOOLPATH)
	rm -rf $(TOPDIR)/build/$(TOOLCHAIN)/toolchain_$(TOOLARCH)/
	rm -rf $(TOPDIR)/images
	rm -rf $(IMAGE_ROOT)
	rm -rf $(INSTALL_ROOT)
	rm -rf $(LIBOPTTEMP)
	@echo CAUTION THIS WILL NOT CLEAN EVERYTHING
else
ifeq ($(VOIP_BUILD), web)
clean: kernel_clean busybox_clean web_clean
	@echo CAUTION THIS WILL NOT CLEAN EVERYTHING
	rm -rf $(TOOLPATH)
	rm -rf $(TOPDIR)/build/$(TOOLCHAIN)/toolchain_$(TOOLARCH)/
	rm -rf $(TOPDIR)/images
	rm -rf $(IMAGE_ROOT)
	rm -rf $(INSTALL_ROOT)
	rm -rf $(LIBOPTTEMP)
	@echo CAUTION THIS WILL NOT CLEAN EVERYTHING
endif
clean: kernel_clean busybox_clean
	@echo CAUTION THIS WILL NOT CLEAN EVERYTHING
	rm -rf $(TOOLPATH)
	rm -rf $(TOPDIR)/build/$(TOOLCHAIN)/toolchain_$(TOOLARCH)/
	rm -rf $(TOPDIR)/images
	rm -rf $(IMAGE_ROOT)
	rm -rf $(INSTALL_ROOT)
	rm -rf $(LIBOPTTEMP)
	@echo CAUTION THIS WILL NOT CLEAN EVERYTHING
endif
