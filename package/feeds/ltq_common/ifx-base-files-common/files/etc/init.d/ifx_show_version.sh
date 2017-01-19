#!/bin/sh /etc/rc.common
# Copyright (C) 2007 OpenWrt.org
# Copyright (C) 2007 infineon.com

START=99

bindir=/opt/ifx/bin

start() {
   # start the dsl daemon
   [ -e ${bindir}/show_version.sh ] && ${bindir}/show_version.sh
	[ -e /opt/ifx/image_version ] && cat /opt/ifx/image_version
}
