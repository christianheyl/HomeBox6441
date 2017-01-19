#!/bin/sh
# This is the script define HW capability.
#SVN id: $Id: mtlk_init_rdlim.sh 2278 2008-02-21 15:40:01Z ediv $

if [ ! "$MAPLOADED" ]; then
	if [ -r /tmp/wave300_map_apIndex ]; then
		. /tmp/wave300_map_apIndex 2>/dev/null
		MAPLOADED="1"
	fi
fi

if [ ! "MTLK_INIT_PLATFORM" ]; then			
	. /tmp/mtlk_init_platform.sh
	MTLK_INIT_PLATFORM="1"
fi
# . /tmp/mtlk_init_platform.sh
command=$1
#wlan=$2
apIndex=$2

#get corresponding wlan network interface from mapping file
eval wlan='$'w300_map_idx_${apIndex}

start_mtlk_init_mbss()
{
	# vap needs to be added to phy AP
	phyAP=`echo $wlan | cut -f1 -d "."`
	print2log DBG "phyAP = $phyAP"
	iwpriv $phyAP sAddVap

	print2log INFO "BSS $wlan created."
}

stop_mtlk_init_mbss()
{
	# vap is removed from phy AP
	phyAP=`echo $wlan | cut -f1 -d "."`
	# vap needs to be removed from phy AP -> index for wave300 driver
	vapIdx=`echo $wlan | cut -f2 -d "."`
	let vapIdx=$vapIdx+1
	print2log INFO "vapIdx $vapIdx"
	iwpriv $phyAP sDelVap $vapIdx

	print2log INFO "BSS $wlan removed"
}

create_config_mtlk_init_mbss()
{
	return
}

should_run_mtlk_init_mbss()
{
#	mbss=`host_api get $$ $apIndex MBSSEnabled`
#	print2log DBG "mbss = $mbss"
#	if [ $mbss ]
#	then
#		true
#	else
#		print2log DBG "MBSS not enabled"
#		false
#	fi		
	return
}


case $command in
	start)
		start_mtlk_init_mbss
	;;
	stop)
		stop_mtlk_init_mbss
	;;
	create_config)
		create_config_mtlk_init_mbss
	;;
	should_run)
		should_run_mtlk_init_mbss
	;;
esac