#!/bin/sh

# This script must run before creating drvhlr.conf because drvhlpr uses the RFMgmtEnable parameter.

#Defines
if [ ! "$MTLK_INIT_PLATFORM" ]; then			
	print2log DBG "mtlk_init_platform called in mtlk_init_rf_mgmt.sh"
	. /tmp/mtlk_init_platform.sh
	MTLK_INIT_PLATFORM="1"
fi
command=$1
apIndex=$2
	
start_mtlk_init_rf_mgmt()
{
	print2log DBG "start mtlk_init_rf_mgmt"
	
	RFMgmtEnable=0
	if [ ! "$MAPLOADED" ]; then
		if [ -r /tmp/wave300_map_apIndex ]; then
			print2log DBG "mtlk_init_rf_mgmt.sh: \". /tmp/wave300_map_apIndex\""
			. /tmp/wave300_map_apIndex 2>/dev/null
			MAPLOADED="1"
		fi
	fi
	eval wlan='$'w300_map_idx_${apIndex}
	print2log DBG "mtlk_init_rf_mgmt.sh: wlan=$wlan"
	
	# LBF is no supported for second card 
	if [ "$wlan" = "wlan1" ]
	then			
		host_api set $$ $apIndex RFMgmtEnable 0		
	elif [ "$wlan" = "wlan0" ]
	then
		Ant_selection=`host_api get $$ hw_$wlan Ant_selection`
		Support_3TX=`host_api get $$ hw_$wlan Support_3TX`
		if [ $Ant_selection ] && [ $Ant_selection = 1 ]
		then
			RFMgmtEnable=1
		fi	
		if [ $Support_3TX ] && [ $Support_3TX = 1 ]
		then
			# Select beam forming (explicit or implicit)
			# Explicit beam forming (LBF) == 2
			# Implicit beam forming == 3
			RFMgmtEnable=3
		fi
			
		host_api set $$ $apIndex RFMgmtEnable $RFMgmtEnable
#	else
		#todo: check if RFMgmtEnable must be set for VAPs
#		;
	fi
	
	print2log DBG "Finish mtlk_init_rf_mgmt"
}

stop_mtlk_init_rf_mgmt()
{
    return
}

create_config_mtlk_init_rf_mgmt()
{
    return
}

should_run_mtlk_init_rf_mgmt()
{
   RFMgmtEnable=`host_api get $$ $apIndex RFMgmtEnable` 
	print2log DBG "RFMgmtEnable: $RFMgmtEnable"
	if [ $RFMgmtEnable ]
	then 
		false
	else
		true
	fi
}

case $command in
    start)
    	start_mtlk_init_rf_mgmt
    ;;
    stop)
    	stop_mtlk_init_rf_mgmt
    ;;
    create_config)
    	create_config_mtlk_init_rf_mgmt
    ;;
    should_run)
    	should_run_mtlk_init_rf_mgmt
    ;;
esac
