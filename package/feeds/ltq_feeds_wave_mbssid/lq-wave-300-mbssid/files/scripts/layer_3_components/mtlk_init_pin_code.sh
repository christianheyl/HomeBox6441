#!/bin/sh

# Defines
if [ ! "$MTLK_INIT_PLATFORM" ]; then			
	. /tmp/mtlk_init_platform.sh
	export MTLK_INIT_PLATFORM="1"
fi
print2log DBG "mtlk_init_pin_code.sh: args: $*"
command=$1
apIndex=$2


start_mtlk_init_pin_code()
{
	print2log DBG "start mtlk_init_pin_code"
	# Get current pin code
	
	DevicePIN=`host_api get $$ $apIndex NonProc_WPS_DevicePIN`
	if [ -z "$DevicePIN" ]
	then 
		return
	fi
	
	DevicePIN=`host_api get $$ $apIndex device_pin_code`
	
	if [ -z "$DevicePIN" ]; then DevicePIN="12345670"; fi
		
	# writing passphrase into wlan0.conf
	host_api set $$ $apIndex NonProc_WPS_DevicePIN $DevicePIN

	#host_api commit $$
	#config_save.sh
	
	print2log DBG "Finish start mtlk_init_pin_code"
}

stop_mtlk_init_pin_code()
{
	return
}

create_config_mtlk_init_pin_code()
{
	return
}

should_run_mtlk_init_pin_code()
{
	true
}

case $command in
	start)
		start_mtlk_init_pin_code
	;;
	stop)
		stop_mtlk_init_pin_code
	;;
	create_config)
		create_config_mtlk_init_pin_code
	;;
	should_run)
		should_run_mtlk_init_pin_code
	;;
esac




