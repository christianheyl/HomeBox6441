#!/bin/sh /etc/rc.common
# Copyright (C) 2011 OpenWrt.org
# Copyright (C) 2011 lantiq.com
START=93

OMCID_BIN=/opt/lantiq/bin/omcid

status_entry_create() {
	local path=$1

	if [ ! -f "$path" ]; then
		echo >> $path
	fi
	uci set $path.ip_conflicts=status
	uci set $path.dhcp_timeouts=status
	uci set $path.dns_errors=status
}

start() {
	local mib_file
	local uni2lan
	local uni2lan_path
	local tmp

	config_load omci

	mib_file="/etc/mibs/default.ini"
	uni2lan=""
	uni2lan_path="/tmp/uni2lan"

	[ -f "$uni2lan_path" ] && uni2lan="-u $uni2lan_path"

	tmp=`fw_printenv mib_file | cut -f2 -d=`
	if [ -f "$tmp" ]; then
		mib_file="$tmp"
	else
		config_get tmp "default" "mib_file"
		[ -f "$tmp" ] && mib_file="$tmp"
	fi

	config_get tmp "default" "status_file"
	status_entry_create "$tmp"

	logger -t omcid "Use OMCI mib file: $mib_file"
	${OMCID_BIN} -d3 ${uni2lan} -p $mib_file -m0  > /dev/console 2> /dev/console &
}

stop() {
	killall ${OMCID_BIN}
}
