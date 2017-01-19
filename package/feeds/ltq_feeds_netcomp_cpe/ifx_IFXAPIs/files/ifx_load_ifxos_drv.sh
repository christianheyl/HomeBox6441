#!/bin/sh /etc/rc.common
# Copyright (C) 2007 OpenWrt.org
START=16
ENABLE_DEBUG_OUTPUT=0

bindir=/opt/lantiq/bin

start() {
	[ -e ${bindir}/debug_level.cfg ] && . ${bindir}/debug_level.cfg

	insmod drv_ifxos debug_level=$ENABLE_DEBUG_OUTPUT
}
