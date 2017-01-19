#!/bin/sh /etc/rc.common
# Copyright (C) 2007 OpenWrt.org
START=20

bindir=/opt/lantiq/bin

start() {
	# read in the global debug mode
	. ${bindir}/debug_level.cfg

	# loading SPI driver -
	cd ${bindir}
	${bindir}/inst_driver.sh $ENABLE_DEBUG_OUTPUT spi drv_spi
}
