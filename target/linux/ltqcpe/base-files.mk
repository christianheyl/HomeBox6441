#
# LTQCPE rootfs addon for OpenWRT basefiles
#

ifneq ($(CONFIG_EXTERNAL_TOOLCHAIN),)
  define Package/libc/install_lib
	$(CP) $(filter-out %/libdl_pic.a %/libpthread_pic.a %/libresolv_pic.a %/libnsl_pic.a,$(wildcard $(TOOLCHAIN_ROOT_DIR)/usr/lib/lib*.a) $(wildcard $(TOOLCHAIN_ROOT_DIR)/lib/lib*.a)) $(1)/lib/
	$(if $(wildcard $(TOOLCHAIN_ROOT_DIR)/usr/lib/libc_so.a),$(CP) $(TOOLCHAIN_ROOT_DIR)/usr/lib/libc_so.a $(1)/lib/libc_pic.a)
	$(if $(wildcard $(TOOLCHAIN_ROOT_DIR)/usr/lib/gcc/*/*/libgcc.map), \
		$(CP) $(TOOLCHAIN_ROOT_DIR)/usr/lib/gcc/*/*/libgcc_pic.a $(1)/lib/libgcc_s_pic.a; \
		$(CP) $(TOOLCHAIN_ROOT_DIR)/usr/lib/gcc/*/*/libgcc.map $(1)/lib/libgcc_s_pic.map \
	)
  endef
  define Package/libpthread/install_lib
	$(if $(wildcard $(TOOLCHAIN_ROOT_DIR)/usr/lib/libpthread_so.a),$(CP) $(TOOLCHAIN_ROOT_DIR)/usr/lib/libpthread_so.a $(1)/lib/libpthread_pic.a)
  endef
else
  define Package/libc/install_lib
	$(CP) $(filter-out %/libdl_pic.a %/libpthread_pic.a %/libresolv_pic.a %/libnsl_pic.a,$(wildcard $(TOOLCHAIN_DIR)/usr/lib/lib*.a)) $(1)/lib/
	$(if $(wildcard $(TOOLCHAIN_DIR)/usr/lib/libc_so.a),$(CP) $(TOOLCHAIN_DIR)/usr/lib/libc_so.a $(1)/lib/libc_pic.a)
	$(if $(wildcard $(TOOLCHAIN_DIR)/usr/lib/gcc/*/*/libgcc.map), \
		$(CP) $(TOOLCHAIN_DIR)/usr/lib/gcc/*/*/libgcc_pic.a $(1)/lib/libgcc_s_pic.a; \
		$(CP) $(TOOLCHAIN_DIR)/usr/lib/gcc/*/*/libgcc.map $(1)/lib/libgcc_s_pic.map \
	)
  endef
endif

define Package/base-files/install-target
	rm -rf $(1)/tmp
	rm -f $(1)/etc/fstab
	rm -rf $(1)/etc/init.d

	mkdir -p $(1)/{usr/bin,usr/lib,etc/dsl_api,etc/ppp,etc/init.d,etc/rc.d,proc,root,ramdisk,ramdisk_copy,mnt/overlay}

	if [ -d $(PLATFORM_DIR)/$(PROFILE)/base-files/etc/init.d/. ]; then \
		$(CP) $(PLATFORM_DIR)/$(PROFILE)/base-files/etc/init.d/* $(1)/etc/init.d/; \
	fi
	$(if $(filter-out $(PLATFORM_DIR),$(PLATFORM_SUBDIR)), \
		if [ -d $(PLATFORM_SUBDIR)/base-files/etc/init.d/. ]; then \
			$(CP) $(PLATFORM_SUBDIR)/base-files/etc/init.d/* $(1)/etc/init.d/; \
		fi \
	)
	-sed -i -e '/\/etc\/passwd/d' -e '/\/etc\/hosts/d' $(1)/CONTROL/conffiles

	$(if $(wildcard $(PLATFORM_SUBDIR)/base-files/etc/fstab), \
		cp -f $(PLATFORM_SUBDIR)/base-files/etc/fstab $(1)/etc/; \
	,\
		cp -f $(PLATFORM_DIR)/base-files/etc/fstab $(1)/etc/; \
	)

	$(if $(CONFIG_TARGET_ltqcpe_platform_ar10_vrx318), \
		if [ -d $(PLATFORM_SUBDIR)/base-files-vrx318/ ]; then \
			$(CP) $(PLATFORM_SUBDIR)/base-files-vrx318/* $(1)/; \
		fi \
	)

	$(if $(CONFIG_TARGET_ltqcpe_platform_ar10_GRX390), \
		if [ -d $(PLATFORM_SUBDIR)/base-files-GRX390/ ]; then \
			$(CP) $(PLATFORM_SUBDIR)/base-files-GRX390/* $(1)/; \
		fi \
	)

	$(if $(CONFIG_TARGET_ltqcpe_platform_vr9_VRX220), \
		if [ -d $(PLATFORM_SUBDIR)/base-files-VRX220/ ]; then \
			$(CP) $(PLATFORM_SUBDIR)/base-files-VRX220/* $(1)/; \
		fi \
	)

	$(if $(CONFIG_TARGET_UBI_MTD_SUPPORT), \
		if [ -d $(PLATFORM_DIR)/base-files-ubi/. ]; then \
			$(CP) $(PLATFORM_DIR)/base-files-ubi/* $(1)/; \
			mkdir -p $(1)/mnt/data; \
			$(if $(CONFIG_TARGET_DATAFS_JFFS2), \
				echo jffs2 > $(1)/mnt/data/fs;) \
			$(if $(CONFIG_TARGET_DATAFS_UBIFS), \
				echo ubifs > $(1)/mnt/data/fs;) \
		fi \
	)

	$(if $(CONFIG_LINUX_3_5-rc2), \
		if [ -d $(PLATFORM_DIR)/base-files-3.5/. ]; then \
			$(CP) $(PLATFORM_DIR)/base-files-3.5/* $(1)/; \
		fi \
	)

	cd $(1); \
		mkdir -p lib/firmware/$(LINUX_VERSION); \
		ln -sf lib/firmware/$(LINUX_VERSION) firmware; \
		ln -sf ramdisk/var var; \
		ln -sf ramdisk/flash flash

	mkdir -p $(STAGING_DIR)/usr/include/
	$(if $(CONFIG_PACKAGE_ifx-utilities),cp -f $(PLATFORM_DIR)/base-files/etc/rc.conf $(STAGING_DIR)/usr/include/)

	mkdir -p $(1)/etc
	mkdir -p $(1)/etc/ppp
	mkdir -p $(1)/etc/rc.d
	cd $(1)/etc/rc.d; \
		ln -sf ../init.d init.d; \
		ln -sf ../config.sh config.sh

	$(if $(CONFIG_UBOOT_CONFIG_BOOT_FROM_NAND),mkdir -p $(1)/etc/sysconfig)
endef
