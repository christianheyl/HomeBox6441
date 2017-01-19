#!/bin/sh /etc/rc.common
# Copyright (C) 2007 OpenWrt.org
START=18

bindir=/opt/ifx/bin

start() {
	if [ -e ${bindir}/debug_level.cfg ]; then
		# read in the global debug mode
		. ${bindir}/debug_level.cfg
	fi
	# loading CPE API driver -
	cd ${bindir}
	${bindir}/inst_drv_cpe_api.sh $ENABLE_DEBUG_OUTPUT
}
