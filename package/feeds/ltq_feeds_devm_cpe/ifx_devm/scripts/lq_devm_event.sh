#!/bin/sh /etc/rc.common

START=99

start() {
	#posting event to devm when system init completes.
	/usr/sbin/ifx_event_util "TR69_SYS_INIT" "UP"
}
