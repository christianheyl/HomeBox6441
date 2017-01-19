#!/bin/sh /etc/rc.common
# Copyright (C) 2009 OpenWrt.org
# Copyright (C) 2010 lantiq.com
START=90

onu () {
	#echo "onu $*"
	result=`/opt/lantiq/bin/onu $*`
	#echo "result $result"
	status=${result%% *}
	if [ "$status" != "errorcode=0" ]; then
		echo "onu $* failed: $result"
	fi
}

ploam_config() {
	local nSerial
	nSerial=""
	config_get nSerial "ploam" nSerial
	if [ -z "$nSerial" ]; then
		# create out of lantiq OID and ether mac the serial number
		str="$(cat /proc/cmdline) "
		while [[ -n "$str" ]]; do
			v=${str%% *}
			key=$(echo $v | cut -d= -f1)
			value=$(echo $v | cut -d= -f2)
			if [ "$key" = "ethaddr" ]; then
				ethaddr=$value
			fi
			str=${str#* }
		done
		v=$(echo $ethaddr | cut -d: -f3-6 | sed 's/:/ 0x/g')
		nSerial="0x4c 0x51 0x44 0x45 0x$v"
	fi
	onu ploam_init
	logger -t onu "Using ploam serial number: $nSerial"
	onu gtcsns $nSerial
}

#
# bool bDlosEnable
# bool bDlosInversion
# uint8_t nDlosWindowSize
# uint32_t nDlosTriggerThreshold
# uint8_t nLaserGap
# uint8_t nLaserOffset
# uint8_t nLaserEnEndExt
# uint8_t nLaserEnStartExt
#
# GTC_powerSavingMode_t ePower
#    GPON_POWER_SAVING_MODE_OFF = 0
#    GPON_POWER_SAVING_DEEP_SLEEP = 1
#    GPON_POWER_SAVING_FAST_SLEEP = 2
#    GPON_POWER_SAVING_DOZING = 3
#    GPON_POWER_SAVING_POWER_SHEDDING = 4
#
gtc_config() {
	local bDlosEnable
	local bDlosInversion
	local nDlosWindowSize
	local nDlosTriggerThreshold
	local ePower
	local nLaserGap
	local nLaserOffset
	local nLaserEnEndExt
	local nLaserEnStartExt
	local nPassword
	local nT01
	local nT02
	local nEmergencyStopState

	config_get bDlosEnable "gtc" bDlosEnable
	config_get bDlosInversion "gtc" bDlosInversion
	config_get nDlosWindowSize "gtc" nDlosWindowSize
	config_get nDlosTriggerThreshold "gtc" nDlosTriggerThreshold
	config_get ePower "gtc" ePower
	config_get nLaserGap "gtc" nLaserGap
	config_get nLaserOffset "gtc" nLaserOffset
	config_get nLaserEnEndExt "gtc" nLaserEnEndExt
	config_get nLaserEnStartExt "gtc" nLaserEnStartExt
	config_get nPassword "ploam" nPassword
	config_get nT01 "ploam" nT01
	config_get nT02 "ploam" nT02
	config_get nEmergencyStopState "ploam" nEmergencyStopState

	onu gtccs 3600000 5 9 170 10 0 0 0 $nT01 $nT02 $nEmergencyStopState $nPassword
	onu gtci $bDlosEnable $bDlosInversion $nDlosWindowSize $nDlosTriggerThreshold $nLaserGap $nLaserOffset $nLaserEnEndExt $nLaserEnStartExt
	onu gtcpsms $ePower
}

start() {
	. /lib/falcon.sh
	local cfg="ethernet"
	local bUNI_PortEnable0
	local bUNI_PortEnable1
	local bUNI_PortEnable2
	local bUNI_PortEnable3
	local nPeNumber

	config_load gpon

	config_get bUNI_PortEnable0 "$cfg" bUNI_PortEnable0
	config_get bUNI_PortEnable1 "$cfg" bUNI_PortEnable1
	config_get bUNI_PortEnable2 "$cfg" bUNI_PortEnable2
	config_get bUNI_PortEnable3 "$cfg" bUNI_PortEnable3

	config_get nPeNumber "gpe" nPeNumber

	ploam_config

	# config GTC
	gtc_config

	# download GPHY firmware, if file is available
	[ -f /lib/firmware/phy11g.bin ] && onu langfd "phy11g.bin"

	# init GPE
	onu gpei "falcon_gpe_fw.bin" 1 1 1 $bUNI_PortEnable0 $bUNI_PortEnable1 $bUNI_PortEnable2 $bUNI_PortEnable3 $bUNI_PortEnable0 $bUNI_PortEnable1 $bUNI_PortEnable2 $bUNI_PortEnable3 1 1 0 $nPeNumber
}

stop() {
	# disable line
	onu onules 0
}
