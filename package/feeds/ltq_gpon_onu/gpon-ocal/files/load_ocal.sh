#!/bin/sh /etc/rc.common
# Copyright (C) 2007 OpenWrt.org
START=99

bindir=/opt/lantiq/bin

start() {
   ${bindir}/ocal > /dev/console 2> /dev/console &
}
