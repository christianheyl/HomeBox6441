#!/bin/sh

# Source for common useful functions
if [ ! "$MTLK_INIT_PLATFORM" ]; then			
	. /tmp/mtlk_init_platform.sh
	export MTLK_INIT_PLATFORM="1"
	print2log DBG "mtlk_init_platform called in mtlk_wps_pbc.sh"
fi
print2log DBG "mtlk_wps_pbc.sh: args: $*"
command=$1
ap_index=$2

#get corresponding wlan network interface from mapping file
eval wlan='$'w300_map_idx_${ap_index}

start_mtlk_wps_pbc()
{
	print2log DBG "start mtlk_wps_pbc"
	NETWORK_TYPE=`host_api get $$ $ap_index network_type`
	if [ ! -e /tmp/wps_pbc.sh ]; then ln -s $ETC_PATH/wps_pbc.sh /tmp/wps_pbc.sh; fi
	action_type='get_conf_via_pbc'
	if [ "$NETWORK_TYPE" = "$AP" ]; then action_type='conf_via_pbc'; fi		
	$ETC_PATH/wps_pbc.sh $action_type  $ap_index &
	print2log DBG "Finish start mtlk_wps_pbc"
}

stop_mtlk_wps_pbc()
{
	killall wps_pbc.sh
}

create_config_mtlk_wps_pbc()
{
	return
}

should_run_mtlk_wps_pbc()
{
	WPS_PBC_GPIO=`host_api get $$ hw_$wlan WPS_PB`
	WPS_ON=`host_api get $$ $apIndex NonProc_WPS_ActivateWPS`
	
	if [ "$WPS_ON" = "$YES" ] && [ "$WPS_PBC_GPIO" != "null" ]
	then
		true
	else 
		false
	fi
}

case $1 in
	start)
		start_mtlk_wps_pbc 
	;;
	stop)
		stop_mtlk_wps_pbc
	;;
	create_config)
		create_config_mtlk_wps_pbc
	;;
	should_run)
		should_run_mtlk_wps_pbc
	;;
esac