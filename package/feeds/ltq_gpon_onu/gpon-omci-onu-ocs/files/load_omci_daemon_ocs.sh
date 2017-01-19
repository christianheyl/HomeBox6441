#!/bin/sh /etc/rc.common
# Copyright (C) 2009 OpenWrt.org
# Copyright (C) 2009 lantiq.com
START=99

bindir=/opt/lantiq/bin

start() {
	cd ${bindir}
	${bindir}/onu_omci_daemon -o 1,1,192.168.0.8:10000 > /dev/console &
} 

