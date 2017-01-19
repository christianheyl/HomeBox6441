#!/bin/sh /etc/rc.common
# Copyright (C) 2010 OpenWrt.org
# Copyright (C) 2011 lantiq.com
START=50

config=/etc/optic/goi_config.sh

optic() {
	#echo "optic $*"
	result=`/opt/lantiq/bin/optic $*`
	#echo "result $result"
	status=${result%% *}
	key=$(echo $status | cut -d= -f1)
	val=$(echo $status | cut -d= -f2)
	if [ "$key" == "errorcode" -a "$val" != "0" ]; then
		echo "optic $* failed: $result"
	fi
}

gpio_has_pu() {
	local pin=$1
	grep "gpio-${pin}" /sys/kernel/debug/gpio | grep in |grep -q hi
}

omu() {
	local found=no
	gpio_has_pu 107 && gpio_has_pu 108 && which i2cdump >/dev/null && {
		i2cdump -y 0 0x50 2>/dev/null |grep -q FIBERXON && found=yes
	}
	echo $found
}

EXTRA_COMMANDS="omu"
EXTRA_HELP="	omu	Check if OMU is found"

start() {
	local OpticMode
	. /lib/falcon.sh

	while true; do
		[ -e /dev/optic0 ] && break
	done
	case $(falcon_board_name) in
	easy98000)
		config_load goi_config
		config_get OpticMode global OpticMode undefined
		[ "$OpticMode" == "undefined" ] && {
			echo "check for OMU"
			[ "$(omu)" == "yes" ] && OpticMode=OMU
		}
		;;
	*)	# BOSA mode
		OpticMode=BOSA
		;;
	esac
	case $OpticMode in
	[Oo][Mm][Uu]) # OMU mode
		optic optic_mode_set 1
		;;
	*)	# BOSA mode (default)
		optic optic_mode_set 2
		;;
	esac

	. /etc/optic/goi_config.sh

	optic_config_all
	optic_read_tables
	#sleep 1
	#$config init
}
