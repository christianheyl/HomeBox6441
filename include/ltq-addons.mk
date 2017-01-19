# 
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

# Parameters: <config file name>
define ifx_expand_paths_in_config
	FILE_NAME=$1; \
	if [ -f $$FILE_NAME ]; then \
	TOT_CNT=`grep -n "*" $$FILE_NAME | grep -w source | wc -l | awk '{ print $$1 }'`; \
	SKIP=1; \
	for ((i=1; i<=$$TOT_CNT; i++)); do \
		KEYLINE=`grep -n "*" $$FILE_NAME | grep -w source | head -$$SKIP | tail -1`; \
		NUM=`echo "$$KEYLINE" | cut -f1 -d:`; \
		TEST_SYN=`echo "$$KEYLINE" | cut -d: -f2- | awk '{ print $$1 }'`; \
		if [ "$$TEST_SYN" == "source" ]; then \
			SHORT_PATH=`echo "$$KEYLINE" | cut -d: -f2- | awk '{ print $$2 }' \
				| sed -e s'/"//' -e s'/"//'`; \
			FULL_PATHS=`find $$SHORT_PATH`; \
			if [ $$? -ne 0 ]; then \
				echo $$FILE_NAME: Parse error line: $$NUM; \
				exit 1; \
			fi; \
			CONFIG_PATHS=`echo "$$FULL_PATHS" | awk '{ print "\""$$0"\"" }'`; \
			sed -i "$$NUM"d $$FILE_NAME; \
			for line in $$CONFIG_PATHS; do \
				sed -i ''$$NUM' i\source '$$line'' $$FILE_NAME; \
			done; \
		else \
			SKIP=`expr $$SKIP + 1`; \
		fi; \
	done; \
	fi;
endef

define ifx_autoconf_generate
	if ! [ -z $(CONFIG_AUTO_CONF_GEN) ]; then \
		CONFIG_FILE=$(TOPDIR)/.config; \
		CONFIG_INPUT_FILE=$(TOPDIR)/Config.in; \
		CONFIG_TARGET_FILE=$(TOPDIR)/tmp/.config-target.in; \
		CONFIG_BINARY_FILE=$(STAGING_DIR_HOST)/bin/conf; \
		INC_CONFIG_DIR=$(INCLUDE_DIR)/config; \
		INC_AUTOCONF_DIR=$(INCLUDE_DIR)/linux; \
		AUTO_CONF_FILE=$$INC_AUTOCONF_DIR/autoconf.h; \
		CONFIG_FEATURE_HEADER=$(TMP_DIR)/ifx_config.h; \
		CONFIG_FEATURE_SCRIPT=$(TMP_DIR)/ifx_config.sh; \
		IFX_CONFIG_DIR=$(STAGING_DIR)/usr/include; \
		if [ $$CONFIG_FILE -nt $$CONFIG_FEATURE_HEADER \
			-o $$CONFIG_FILE -nt $$CONFIG_FEATURE_SCRIPT ]; then \
			if [ -f "$$CONFIG_BINARY_FILE" ]; then \
				cp -f $$CONFIG_INPUT_FILE $$CONFIG_INPUT_FILE.temp; \
	\
				$(call ifx_expand_paths_in_config,$$CONFIG_INPUT_FILE.temp) \
				$(call ifx_expand_paths_in_config,$(TMP_DIR)/.config-package.in) \
	\
				cp -f $$CONFIG_TARGET_FILE $$CONFIG_TARGET_FILE.temp; \
				sed -i -e /"\treset"/s/^/'#'/ -e /"\treset"/s/"[\t|' ']"// $$CONFIG_TARGET_FILE; \
	\
				mkdir -p $$INC_CONFIG_DIR; \
				mkdir -p $$INC_AUTOCONF_DIR; \
	\
				yes "" | $(SCRIPT_DIR)/config/conf -o $$CONFIG_INPUT_FILE.temp; \
				$$CONFIG_BINARY_FILE -s $$CONFIG_INPUT_FILE.temp; \
	\
				echo -en "/*\n * Automatically generated C config. Don't edit.\n */\n" \
					> $$CONFIG_FEATURE_HEADER; \
				cat $$AUTO_CONF_FILE \
					| grep -E '(FEATURE|PACKAGE|IFX|LTQ|UBOOT|BUSYBOX|TARGET_$(BOARD)_platform)' \
					| awk '{ gsub("-","_",$$2);$$2=toupper($$2);print }' \
					>> $$CONFIG_FEATURE_HEADER; \
	\
				echo -en "#\n# Automatically generated config for make. Don't edit.\n#\n" \
					> $$CONFIG_FEATURE_SCRIPT; \
				cat $$CONFIG_FEATURE_HEADER \
					| grep "#define" \
					| awk '{printf "export "$$2"=";for (i=3; i<=NF; i++) printf("%s ", $$i);printf("\n")}' \
					>> $$CONFIG_FEATURE_SCRIPT; \
	\
				mkdir -p $$IFX_CONFIG_DIR; \
				cp -f $$CONFIG_FEATURE_HEADER $$IFX_CONFIG_DIR/ifx_config.h; \
			fi; \
		fi; \
		if [ -f $$CONFIG_FEATURE_HEADER ]; then \
			if [ ! -f $$IFX_CONFIG_DIR/ifx_config.h ]; then \
				mkdir -p $$IFX_CONFIG_DIR; cp -f $$CONFIG_FEATURE_HEADER $$IFX_CONFIG_DIR/ifx_config.h; \
			fi; \
		fi; \
	\
	else \
		echo "autoconf.h generation not enabled..!"; \
	fi;
endef

