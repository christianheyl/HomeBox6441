#!/bin/sh

/usr/sbin/syscfg_lock /flash/rc.conf '
	/usr/sbin/upgrade /etc/rc.conf.gz sysconfig 0 0
	sync; sleep 2;
	/etc/init.d/rebootsync.sh stop
	reboot -f
'
