#!/bin/sh

PRJ_ROOT=`pwd`
GDBS_PORT=1234

if [ -z ${SSH_CLIENT_IP} ]; then
	echo SSH_CLIENT_IP not set.  Please use 'ssh -X' connection.
	exit
fi

ddd	--debugger ${PRJ_ROOT}/staging_dir/toolchain-mips_r2_gcc-4.3.3+cs_uClibc-0.9.30.1/usr/bin/mips-openwrt-linux-gdb \
	--directory=${PRJ_ROOT}/build_dir \
	--eval-command="target remote ${SSH_CLIENT_IP}:${GDBS_PORT}" \
	$1
