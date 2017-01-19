#!/bin/sh /etc/rc.common
# Copyright (C) 2012 lantiq.com

. $IPKG_INSTROOT/lib/falcon.sh

START=85

bindir=/opt/lantiq/bin

# dsl api mode settings
fw_image_name=xco3l_fw_image

mei3_cli_dev()
{
   local devnum=$1
   shift
   local cmd=$@
   ${bindir}/control_mei3 -d $devnum -x "$cmd"
}

prepare_firmware()
{
	local fw_image_name

	[ -e /tmp/$fw_image_name.bin ] || \
			tar -xzf /opt/lantiq/firmware/$fw_image_name.tar.gz -C /tmp/
	FW_PATH=/tmp/$fw_image_name.bin
}

start_raw_mode()
{
	case $(falcon_board_name) in
		MDU1)
		# raw mode settings
		wait_states=15
		mei3_baseaddr_dev0=0x14000000
		mei3_irq_num_dev0=99
		mei3_dis_port_mask="0xFFFE 0xFFFF 0xFFFF 0xFFFF"
		mei3_cli_dev 0 "DevInitSet 0 $mei3_baseaddr_dev0 $mei3_irq_num_dev0 $mei3_dis_port_mask $wait_states"
		${bindir}/control_mei3 -d 0 -l 0 -x "lirg 0"
		mei3_cli_dev 0 "DevLowLevelAfeConfigSet 1 0x1 0 0xA 0xFFFF 0xFFFF 0x0B 0x0 0 0 0 0 0 0"
		${bindir}/control_mei3 -d 0 -f -x "${FW_PATH}"
		${bindir}/control_mei3 -d 0 -l 0 -x "LineMessageSend 0x8506 0 0 6 0x20 0 0"
		${bindir}/control_mei3 -d 0 -l 0 -x "LineMessageSend 0x8505 0 0 6 0x17 0xffff 0"
		${bindir}/control_mei3 -d 0 -l 0 -x "LineMessageSend 0x8904 0 0 2 1"
		${bindir}/control_mei3 -d 0 -l 0 -x "LineMessageSend 0x8100 0 0 4 4 0"
		${bindir}/control_mei3 -d 0 -l 0 -x "LineMessageSend 0x200 0 0 2 2"
		;;
		*)
		return -1
		;;
	esac
	return 0
}

start_dsl_api()
{
	local board_id

	case $(falcon_board_name) in
		MDU1)
		board_id=19
		;;
		MDU8)
		board_id=20
		;;
		MDU16)
		board_id=21
		;;
		*)
		return -1
		;;
	esac
	${bindir}/dsl_daemon -b$board_id -f/tmp/$fw_image_name.bin &
}

start() {
	case $(falcon_board_name) in
		MDU*)
		;;
		*)
		return -1
		;;
	esac

	prepare_firmware
	if [ -e ${bindir}/dsl_daemon ]; then
		start_dsl_api
		if [ -e $bindir/dms_default_config.sh ]; then
			sleep 4
			PS=`ps`
			echo $PS | grep -q dsl_daemon && {
				# wait for the DMS pipes to be available
				while [ ! -e /tmp/pipe/dms0_cmd ]; do sleep 1; echo wait...; done
				sleep 4
				# call default configuration for the DMS now
				$bindir/dms_default_config.sh
			}
			echo $PS | grep -q dsl_daemon || {
				echo "dsl_daemon not running, config not possible!!!"
				false
			}
		fi
	else
		start_raw_mode
	fi

	return 0
}
