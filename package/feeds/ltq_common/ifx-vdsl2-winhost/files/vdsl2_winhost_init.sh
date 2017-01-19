#!/bin/sh /etc/rc.common
# Copyright (C) 2007 OpenWrt.org
START=80

bindir=/opt/ifx/bin

start() {
	# start the vinax winhost daemon in the background
	${bindir}/vdsl2_winhost_agent &
}

stop() {
	killall vdsl2_winhost_agent
}