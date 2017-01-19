#!/bin/sh
# This is the script define HW capability.
#SVN id: $Id: mtlk_init_rdlim.sh 2278 2008-02-21 15:40:01Z ediv $

#if [ ! "$MAPLOADED" ]; then
#	if [ -r /tmp/wave300_map_apIndex ]; then
#		print2log DBG "mtlk_set_driver_params.sh: \". /tmp/wave300_map_apIndex\""
#		. /tmp/wave300_map_apIndex 2>/dev/null
#		MAPLOADED="1"
#	fi
#fi

if [ ! "$MTLK_INIT_PLATFORM" ]; then			
	. /tmp/mtlk_init_platform.sh
	export MTLK_INIT_PLATFORM="1"
fi
print2log DBG "mtlk_set_driver_params.sh: args: $*"
command=$1
apIndex=$2

IWPRIV_SET ()
{
	iwpriv $1 "s$2" $3
}

assignee ()
{
	if [ `expr $1 : $2` != 0 ]
	then
		echo $3
	else
		echo $4
	fi
}

start_mtlk_set_driver_params()
{
	print2log DBG "start mtlk_set_driver_params"
	if [ ! -e /tmp/set_driver_params_${apIndex}.sh ]
	then
		print2log WARNING "/tmp/set_driver_params.sh not found."
		return
	fi
			
	. /tmp/set_driver_params_${apIndex}.sh
	
	#TODO: how control commands order (such as wep). maybe differens queries
	print2log DBG "Finish start mtlk_set_driver_params"
}

stop_mtlk_set_driver_params()
{
	return
}

create_config_mtlk_set_driver_params()
{
	if [ ! -e /tmp/ini.tcl ]; then ln -s $ETC_PATH/ini.tcl /tmp/ini.tcl; fi
	if [ ! -e /tmp/driver_api.ini ]; then ln -s $ETC_PATH/driver_api.ini /tmp/driver_api.ini; fi
	print2log DBG "config mtlk_set_driver_params"
	#get_query | awk -F "/" '
	#{
	#	a="'"'"'";
	#	gsub("NULL","\\N",$0)
	#	print $3, $2, $1, a $4 a
	#}' > /tmp/set_driver_params.sh
	
	BRIDGE_MODE=`host_api get $$ sys BridgeMode`
	if [ "$BRIDGE_MODE" = "$MacCloning" ]
	then
		# Write to the driver
		MacCloningAddr=`host_api get $$ sys MacCloningAddr`
		if [ $MacCloningAddr ]
		then
			host_api set $$ $apIndex MAC $MacCloningAddr
		fi		
	fi

	if [ "$BRIDGE_MODE" = "$L2NAT" ]
	then
		DST_MAC=`ifconfig br0 | awk 'NR<2 {print $5}'`
		host_api set $$ $apIndex L2NAT_LocMAC $DST_MAC
	fi
	
	#host_api commit $$
	#config_save.sh
		
	$ETC_PATH/driver_api.tcl DriverSetAll $apIndex
	
	print2log DBG "Finish config mtlk_set_driver_params"
}

should_run_mtlk_set_driver_params()
{
	true
}


case $command in
	start)
		start_mtlk_set_driver_params
	;;
	stop)
		stop_mtlk_set_driver_params
	;;
	create_config)
		create_config_mtlk_set_driver_params
	;;
	should_run)
		should_run_mtlk_set_driver_params
	;;
esac