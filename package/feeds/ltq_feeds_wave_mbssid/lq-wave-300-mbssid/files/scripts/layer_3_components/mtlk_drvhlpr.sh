#!/bin/sh

if [ ! "$CONFIGLOADED" ]; then
	if [ -r /etc/rc.d/config.sh ]; then
		. /etc/rc.d/config.sh 2>/dev/null
		CONFIGLOADED="1"
	fi
fi

if [ ! "$MAPLOADED" ]; then
	if [ -r /tmp/wave300_map_apIndex ]; then
		. /tmp/wave300_map_apIndex 2>/dev/null
		MAPLOADED="1"
	fi
fi

# Defines
if [ ! "$MTLK_INIT_PLATFORM" ]; then			
	. /tmp/mtlk_init_platform.sh
	MTLK_INIT_PLATFORM="1"
fi
print2log DBG "mtlk_drvhlpr.sh: args: $*"
command=$1
apIndex=$2
#get corresponding wlan network interface from mapping file
eval wlan='$'w300_map_idx_${apIndex}

start_mtlk_drvhlpr()
{	
	print2log DBG "start mtlk_drvhlpr"
	if [ ! -e /tmp/drvhlpr_$wlan ]; then ln -s /bin/drvhlpr /tmp/drvhlpr_$wlan; fi
	$ETC_PATH/drvhlpr.sh $wlan &
	print2log DBG "Finish start mtlk_drvhlpr"
}

stop_mtlk_drvhlpr()
{
	print2log DBG "stop mtlk_drvhlpr"
	killall drvhlpr_$wlan
	A=`ps | grep drvhlpr_$wlan | grep -v grep | awk '{print $1}'`
	for a in $A ; do kill -9 $a ; done
	print2log DBG "Finish stop mtlk_drvhlpr"
}

create_config_mtlk_drvhlpr()
{
	print2log DBG "config mtlk_drvhlpr"
	drvhlprConfFile=$CONFIGS_PATH/drvhlpr_$wlan.conf
	if [ "$wlan" = "wlan0" ]
	then
		wps_script_path=$ETC_PATH/mtlk_wps_event.sh
	else
		wps_script_path=""
	fi
	wls_link_script_path=$ETC_PATH/mtlk_linkstat_event.sh
	data_link_script_path=$ETC_PATH/mtlk_data_link_status.sh
	leds_conf_file=$ETC_PATH/leds.conf
	
	# if dual concurrent wlan, then first two indices are reserved for physical AP
	if [ "$CONFIG_FEATURE_IFX_CONCURRENT_DUAL_WIRELESS" = "1" ]
	then
		phyApIdxLim=2
	else
		phyApIdxLim=1
	fi

	# some parameters are only for physical AP
	if [ $apIndex -lt $phyApIdxLim ]
	then
		NETWORK_TYPE=`host_api get $$ $apIndex network_type`
		# RFMgmt params
		RFMgmtForced=`host_api get $$ $apIndex RFMgmtForced`
		Ant_HW_Support=`host_api get $$ hw_$wlan Ant_selection`
		Support_3TX=`host_api get $$ hw_$wlan Support_3TX`
		if [ "$wlan" = "wlan1" ]
		then			
			RFMgmtEnable=0		
		elif [ "$RFMgmtForced" = "0" ] && [ "$Ant_HW_Support" = "0" ] && [ "$Support_3TX" = "0" ] 
		then
			RFMgmtEnable=0
		else
			RFMgmtEnable=`host_api get $$ $apIndex RFMgmtEnable`
		fi	
		RFMgmtRefreshTime=`host_api get $$ $apIndex RFMgmtRefreshTime`  		 
		RFMgmtKeepAliveTimeout=`host_api get $$ $apIndex RFMgmtKeepAliveTimeout` 	 
		RFMgmtAveragingAlpha=`host_api get $$ $apIndex RFMgmtAveragingAlpha`  	 
		RFMgmtMetMarginThreshold=`host_api get $$ $apIndex RFMgmtMetMarginThreshold`
	fi

	SW_WD_Enable=`host_api get $$ $apIndex Debug_SoftwareWatchdogEnable`
	SECURITY_MODE=`host_api get $$ $apIndex NonProcSecurityMode`
	#The driver NonProcSecurityMode value is decreased by one from the web value
	SECURITY_MODE=`expr $SECURITY_MODE - 1`
		
	cp $leds_conf_file $drvhlprConfFile
	
	# some parameters are only for physical AP
	if [ $apIndex -lt $phyApIdxLim ]
	then
	    echo "network_type = $NETWORK_TYPE"                        >>  $drvhlprConfFile
	    echo "wps_script_path = $wps_script_path"                   >>  $drvhlprConfFile
	    echo "RFMgmtEnable = $RFMgmtEnable"                   	     >>  $drvhlprConfFile
	    echo "RFMgmtRefreshTime = $RFMgmtRefreshTime"               >>  $drvhlprConfFile    
	    echo "RFMgmtKeepAliveTimeout = $RFMgmtKeepAliveTimeout"   	 >>  $drvhlprConfFile   
		echo "RFMgmtAveragingAlpha = $RFMgmtAveragingAlpha"         >>  $drvhlprConfFile   
		echo "RFMgmtMetMarginThreshold = $RFMgmtMetMarginThreshold" >>  $drvhlprConfFile   
	fi

	echo "Debug_SoftwareWatchdogEnable = $SW_WD_Enable"   >>   $drvhlprConfFile
	echo "NonProcSecurityMode = $SECURITY_MODE"         >>  $drvhlprConfFile
    echo "interface = $wlan"                        >>  $drvhlprConfFile
    echo "arp_iface0 = eth0"                                  >>  $drvhlprConfFile
	arp_eth1=`grep eth1 /proc/net/dev`
	if [ "$arp_eth1" ]; then echo "arp_iface1 = eth1"  >>  $drvhlprConfFile; fi

    echo "led_resolution = 1"                                   >>  $drvhlprConfFile
    echo "wls_link_script_path = $wls_link_script_path"         >>  $drvhlprConfFile
	echo "wls_link_status_script_path = $data_link_script_path" >>  $drvhlprConfFile	

	#host_api commit $$
	print2log DBG "Finish config mtlk_drvhlpr"
}

should_run_mtlk_drvhlpr()
{
	driverInst=`lsmod | grep -c mtlk`
	print2log DBG "driverInst: $driverInst"
	if [ $driverInst -lt 1 ]
	then
		false
	else	
		DRVHLPR_COUNT=`ps | grep "\<drvhlpr_$wlan\>" | grep -vc grep`	
		print2log DBG "DRVHLPR_COUNT: $DRVHLPR_COUNT"
		if [ $DRVHLPR_COUNT = 0 ]
		then
			true
		else 
			false
		fi
	fi
}

case $command in
	start)
		start_mtlk_drvhlpr 
	;;
	stop)
		stop_mtlk_drvhlpr
	;;
	create_config)
		create_config_mtlk_drvhlpr
	;;
	should_run)
		should_run_mtlk_drvhlpr
	;;
esac
