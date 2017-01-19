#!/bin/sh /etc/rc.common
# Copyright (C) 2012 OpenWrt.org
# Copyright (C) 2012 lantiq.com

START=49
sysfs_attr="/sys/class/falcon_timer/clkdis"

boot() {
	# only activate the automatic clock disabling
	# if the voice driver is not loaded
	lsmod | grep -q "^drv_vmmc" && exit 1

	[ -e $sysfs_attr ] && echo 1 > $sysfs_attr
}
