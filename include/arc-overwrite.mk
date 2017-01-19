LINUX_BASENAME=$(shell basename $(LINUX_DIR))
CONFIG_DIR=$(shell cat $(TOPDIR)/active_config)
LINUX_DESTDIR=$(shell dirname $(LINUX_DIR))
OVERWRITE_DIR=$(TOPDIR)/$(CONFIG_DIR)/overwrite

# linux kernel config for assigned project
ARC_LINUX_CONFIG=$(shell if [ -f $(OVERWRITE_DIR)/$(LINUX_BASENAME)/kernel_config ]; then echo "$(OVERWRITE_DIR)/$(LINUX_BASENAME)/kernel_config"; fi;)

define Kernel/Overwrite/Default
	echo "arc_linux_config="$(ARC_LINUX_CONFIG)
	if [ -d $(OVERWRITE_DIR) ]; then \
		if [ -d $(OVERWRITE_DIR)/$(LINUX_BASENAME) ]; then \
			$(CP) $(OVERWRITE_DIR)/$(LINUX_BASENAME) $(LINUX_DESTDIR); \
		fi \
	fi
endef

