#!/usr/bin/env bash

BASEDIR="$PWD"
ENVDIR="./config"
INITDIR="../.."
CONFIG_FIND_PATH="../../config_*"
ACTIVE=""
CONFIG_OVERRIDE_USER="/dev/null"
CONFIG_OVERRIDE_RESTORE="/dev/null"
VOIP_PACKAGE_GIT_URL="ssh://gitolite@sw6-builder.arcadyan.com.tw/o2-hb2/arc-voip-ugw389kpn.git"
VOIP_PACKAGE_GIT_BRANCH="develop"
VOIP_PACKAGE_SRC_DIR=$BASEDIR/voip_package/src
VOIP_PACKAGE_BIN_DIR=$BASEDIR/voip_package/bin
VOIP_PACKAGES="libosip2 sip_brn sip_brn-kpn sip_brn-o2 sip_brn-une sip_brn-ta sip_config sipproxy voip_drv arcusb"
ARC_UTILITY_FOLDER=$BASEDIR/package/arcadyan-utility
VOIP_USE_PREBUILT=0

[ -f "$BASEDIR/active_config" ] && {
	ACTIVE=`cat $BASEDIR/active_config`
	echo "active config: $ACTIVE"
}

[ -f "$BASEDIR/other_config_path" ] && {
	CONFIG_FIND_PATH=`cat $BASEDIR/other_config_path`
}

[ -f $BASEDIR/.config.override.user ] && {
	CONFIG_OVERRIDE_USER="$BASEDIR/.config.override.user"
}

[ -f $BASEDIR/.config.override.restore ] && {
	CONFIG_OVERRIDE_RESTORE="$BASEDIR/.config.override.restore"
}

usage() {
	cat <<EOF
Usage: $0 [options] <command> [arguments]
Commands:
	help              This help text
	list              List environments
	clear             Delete all environment and revert to flat config/files
	new <name>        Create a new environment
	switch <name>     Switch to a different environment
	delete <name>     Delete an environment
	rename <newname>  Rename the current environment
	diff              Show differences between current state and environment
	save              Save your changes to the environment
	revert            Revert your changes since last save
	checkin           Checkin to Mercurial

Options:

EOF
	exit ${1:-1}
}

error() {
	echo "$0: ERROR: $*"
	exit 1
}
warn() {
	echo "$0: WARNING: $*"
}

conf() {
	if [ ! -x scripts/config/conf ]; then
		make -s -C scripts/config conf
	fi
	scripts/config/conf $*
}

diffconfig() {
	local cfg=${1:-.config}
	grep \^CONFIG_TARGET_ ${cfg} | head -n3 > tmp/.diffconfig.head
	grep '^CONFIG_ALL=y' ${cfg} >> tmp/.diffconfig.head
	grep '^CONFIG_BROKEN=y' ${cfg} >> tmp/.diffconfig.head
	grep '^CONFIG_DEVEL=y' ${cfg} >> tmp/.diffconfig.head
	grep '^CONFIG_TOOLCHAINOPTS=y' ${cfg} >> tmp/.diffconfig.head
	conf -D tmp/.diffconfig.head -w tmp/.diffconfig.stage1 Config.in &>/dev/null
	./scripts/kconfig.pl '>+' tmp/.diffconfig.stage1 ${cfg} >> tmp/.diffconfig.head
	conf -D tmp/.diffconfig.head -w tmp/.diffconfig.stage2 Config.in &>/dev/null
	./scripts/kconfig.pl '>' tmp/.diffconfig.stage2 ${cfg} >> tmp/.diffconfig.head
	cat tmp/.diffconfig.head
	rm -f tmp/.diffconfig tmp/.diffconfig.*
}

ask_bool() {
	local DEFAULT="$1"; shift
	local def defstr val
	case "$DEFAULT" in
		1) def=0; defstr="Y/n";;
		0) def=1; defstr="y/N";;
		*) def=;  defstr="y/n";;
	esac
	while [ -z "$val" ]; do
		local VAL

		echo -n "$* ($defstr): "
		read VAL
		case "$VAL" in
			y*|Y*) val=0;;
			n*|N*) val=1;;
			*) val="$def";;
		esac
	done
	return "$val"
}

ask_env() {
	local i k name
	printf "[0]\tAbort\n"
	k=1
	for envs in `find $CONFIG_FIND_PATH -type f -name .config -follow | sort -d`
	do
		name=`dirname $envs`
		arch=`cat $envs | grep CONFIG_ARCH= | sed 's/"/ /g' | cut -d' ' -f2`
		gcc=`cat $envs | grep GCC_VERSION= | sed 's/"/ /g' | cut -d' ' -f2`
		bin=`cat $envs | grep CONFIG_BINUTILS_VERSION= | sed 's/"/ /g' | cut -d' ' -f2`
		uclibc=`cat $envs | grep CONFIG_UCLIBC_VERSION= | sed 's/"/ /g' | cut -d' ' -f2`
		uclibcextra=`cat $envs | grep CONFIG_UCLIBC_EXTRA_VERSION= | sed 's/"/ /g' | cut -d' ' -f2`
		printf "[%d]\t%s (%s / %s / %s / %s)\n" $k $name $arch $gcc $bin $uclibc
		let k=$k+1
	done
	let k=$k-1
	printf "choice[0-%d?]: " $k
	read i
	if [ -z $i ] ; then
		echo "aborting... (no selection)"
		exit 1;
	fi
	if [ $i -eq 0 ] ; then
		echo "aborting... (selecting 0)"
		exit 1;
	fi
	if [ $i -gt $k ] ; then
		echo "aborting... (out of range: $i)"
		exit 1;
	fi
	k=1
	for envs in `find $CONFIG_FIND_PATH -type f -name .config -follow | sort -d`
	do
		if [ $i -eq $k ]; then
			name=`dirname $envs`
			env_link_config $name
			return 0
		fi
		let k=$k+1
	done
	return 1
}

clone_voip_packages() {
	# Velmurugan 05272015> clone sip package to have a single code base 
	if [ ! -d $VOIP_PACKAGE_SRC_DIR/arc-voip ] ; then
		echo "Clone $VOIP_PACKAGE_GIT_URL"
		cd $VOIP_PACKAGE_SRC_DIR
		git clone -b $VOIP_PACKAGE_GIT_BRANCH -q $VOIP_PACKAGE_GIT_URL arc-voip 2>/dev/null		
		cd $BASEDIR
	fi
	
	for idx in $VOIP_PACKAGES
	do
		if [ -L $ARC_UTILITY_FOLDER/$idx ] ; then
			rm -f $ARC_UTILITY_FOLDER/$idx
		fi


		if [ -d $VOIP_PACKAGE_SRC_DIR/arc-voip/$idx ] ; then
			ln -sf $VOIP_PACKAGE_SRC_DIR/arc-voip/$idx $ARC_UTILITY_FOLDER/$idx 
		else
			ln -sf $VOIP_PACKAGE_BIN_DIR/$idx $ARC_UTILITY_FOLDER/$idx
			VOIP_USE_PREBUILT=1
		fi
	done
}
env_link_config() {
	local NAME="$1"
	local Model=`echo $NAME | grep -c "VRV9518SWAC"`
	rm -rf $BASEDIR/package/feeds/ltq_feeds_uboot/open_uboot/config
	rm -rf $BASEDIR/package/feeds/ltq_feeds_uboot/open_uboot/patches
	if [ "x$Model" == "x1" ] ; then
	ln -sf $BASEDIR/package/feeds/ltq_feeds_uboot/open_uboot/config_ugw54 $BASEDIR/package/feeds/ltq_feeds_uboot/open_uboot/config
	ln -sf $BASEDIR/package/feeds/ltq_feeds_uboot/open_uboot/patches_ugw54 $BASEDIR/package/feeds/ltq_feeds_uboot/open_uboot/patches
	else
	ln -sf $BASEDIR/package/feeds/ltq_feeds_uboot/open_uboot/config_ori $BASEDIR/package/feeds/ltq_feeds_uboot/open_uboot/config
	ln -sf $BASEDIR/package/feeds/ltq_feeds_uboot/open_uboot/patches_ori $BASEDIR/package/feeds/ltq_feeds_uboot/open_uboot/patches
	fi
	
	clone_voip_packages
	
	[ -f "$NAME/.config" ] || error "$NAME is invalid environment directory"
	rm -f "$BASEDIR/.config"
	rm -Rf "$BASEDIR/files"
	$BASEDIR/scripts/kconfig.pl "+" "$NAME/.config" $CONFIG_OVERRIDE_USER > "$BASEDIR/.config" || error "Failed to copy environment configuration"
	echo "Apply config $NAME"
	make -s defconfig
	[ -d "$NAME/files" ] && {
		cp -Rf "$NAME/files" "$BASEDIR/files" || error "Failed to copy environment files"
		chmod -R u+wr "$BASEDIR/files" || error "Failed to change the protection"
	}
	echo $NAME > "$BASEDIR/active_config"
}

env_save() {
	[ -z "$ACTIVE" ] && error "No active environment found."
	[ -d "$ACTIVE/" ] || error "Can't save environment, directory $ACTIVE does not exist."
	diffconfig > .config.diff
	$BASEDIR/scripts/kconfig.pl "-" ".config.diff" $CONFIG_OVERRIDE_USER > "$ACTIVE/.config" || error "Failed to save the active environment"
	[ -d "$BASEDIR/files" ] && {
		cp -Rf "$BASEDIR/files" "$ACTIVE/"  || error "Failed to copy environment files"
	}
}

env_revert() {
	[ -z "$ACTIVE" ] && error "No active environment found."
	[ -d "$ACTIVE" ] && {
		(cd "$ACTIVE"; hg revert . --no-backup)
	}
}

env_checkin() {
	[ -z "$ACTIVE" ] && error "No active environment found."
	[ -f "$ACTIVE/.config" ] || error "Can't checkin configuration, ( $ACTIVE/.config does not exist)."
	cd "$ACTIVE"
	hg ci -m"config update" .config || error "Failed to checkin the active configuration"
	cd "$BASEDIR"
}

env_list() {
	for envs in `find $CONFIG_FIND_PATH -type f -name .config -follow | sort -d`
	do
		name=`dirname $envs`
		if [ "$ACTIVE" != "" -a "$ACTIVE" = "$name" ]; then
			printf " * "
		else
			printf "   "
		fi
		printf "%s\n" "$name"
	done
}

env_new() {
	local NAME="$1"
	[ -z "$NAME" ] && usage
	[ -f "$NAME/.config" ] && error "The configuration $NAME already exists."
	mkdir -p "$NAME/files"
	if [ -f "$BASEDIR/.config" ]; then
		if ask_bool 0 "Do you want to clone the current environment?"; then
			diffconfig > .config.diff
			$BASEDIR/scripts/kconfig.pl "-" ".config.diff" $CONFIG_OVERRIDE_USER > "$NAME/.config"
			[ -d "$BASEDIR/files" ] && cp -Rf "$BASEDIR/files/" "$NAME/files/"
		fi
	fi
	[ -f "$NAME/.config" ] || touch "$ENVDIR/$NAME/.config"
	rm -f "$BASEDIR/.config" "$BASEDIR/files"
	env_link_config $NAME
}

env_switch() {
	local NAME="$1"
	[ -z "$NAME" ] && {
		ask_env
	} || {
		env_link_config $NAME
	}
}

env_delete() {
	local NAME="$1"
	[ -z "$NAME" ] && usage
	[ -f "$BASEDIR/.config" ] &&
		[ "$ACTIVE" = "$NAME" ] && {
			if ask_bool 0 "Do you want delete the active anvironment?"; then
				rm -f "$BASEDIR/.config" "$BASEDIR/files"
				rm -Rf "$NAME/"
			fi
		} || {
			rm -Rf "$NAME/"
		}
}

env_rename() {
	local NAME="$1"
	[ -z "$NAME" ] && usage
	[ -z "$ACTIVE" ] && usage
	[ "$ACTIVE" = "$NAME" ] && error "Previous and new name are equal"
	rm -f "$BASEDIR/.config"
	rm -f "$BASEDIR/files"
	mv "$ACTIVE" "$NAME"
	env_link_config $NAME
}

env_diff() {
	[ -d "$BASEDIR/.config" ] && error "Can't find $BASEDIR/.config file"
	[ -z "$ACTIVE" ] && usage
	diffconfig > .config.diff
	diff "$ACTIVE/.config" <($BASEDIR/scripts/kconfig.pl "-" ".config.diff" $CONFIG_OVERRIDE_USER)
}

env_clear() {
	for idx in $VOIP_PACKAGES
	do
		if [ -L $ARC_UTILITY_FOLDER/$idx ] ; then
			rm -f $ARC_UTILITY_FOLDER/$idx
		fi
	done

	rm -f "$BASEDIR/active_config"
	rm -f "$BASEDIR/.config"
}

COMMAND="$1"; shift
case "$COMMAND" in
	help) usage 0;;
	new) env_new "$@";;
	list) env_list "$@";;
	clear) env_clear "$@";;
	switch) env_switch "$@";;
	delete) env_delete "$@";;
	rename) env_rename "$@";;
	diff) env_diff "$@";;
	save) env_save "$@";;
	revert) env_revert "$@";;
	checkin) env_checkin "$@";;
	*) usage;;
esac
