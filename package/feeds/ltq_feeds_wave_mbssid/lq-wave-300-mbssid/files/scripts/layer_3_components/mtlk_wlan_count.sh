#!/bin/sh

if [ ! "$MTLK_INIT_PLATFORM" ]; then			
	. /tmp/mtlk_init_platform.sh
	export MTLK_INIT_PLATFORM="1"
fi
command=$1

start_mtlk_wlan_count()
{
	print2log DBG "start mtlk_wlan_count"
	
	# wlan_count is actual number of wlan cards in system
	wlan_count=`cat /proc/bus/pci/devices | grep 1a30 -c`
	host_api set $$ sys wlan_count $wlan_count
	print2log DBG "wlan_count=$wlan_count"
	#host_api commit $$
	#config_save.sh
	
	print2log DBG "finish mtlk_wlan_count"
}

stop_mtlk_wlan_count()
{
	return
}

create_config_mtlk_wlan_count()
{
	return
}

should_run_mtlk_wlan_count()
{
	true
}

case $command in
	start)
		start_mtlk_wlan_count
	;;
	stop)
		stop_mtlk_wlan_count
	;;
	create_config)
		create_config_mtlk_wlan_count
	;;
	should_run)
		should_run_mtlk_wlan_count
	;;
esac
