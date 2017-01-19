#!/bin/sh /etc/rc.common
# Copyright (C) 2011 OpenWrt.org
# Copyright (C) 2011 lantiq.com
START=40

TFTPD_DIR=/var/tftpd-hpa
ONU_FW_DIR=/opt/lantiq/firmware
ONU_FW=gpon-onu-firmware.tar.gz

start() {
	[ -f ${ONU_FW_DIR}/${ONU_FW} ] && \
	mkdir -p ${TFTPD_DIR} && \
	tar -xzf ${ONU_FW_DIR}/${ONU_FW} -C ${TFTPD_DIR}
}
