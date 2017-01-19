#!/bin/sh /etc/rc.common
# Copyright (C) 2007 OpenWrt.org
START=20
ENABLE_DEBUG_OUTPUT=0

bindir=/opt/ifx/bin

start() {
	[ -e ${bindir}/debug_level.cfg ] && . ${bindir}/debug_level.cfg

	cd ${bindir}
	[ -e ${bindir}/inst_driver.sh ] && ${bindir}/inst_driver.sh $ENABLE_DEBUG_OUTPUT ifx_switch drv_ethsw
}
