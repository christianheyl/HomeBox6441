#!/bin/sh
# This is the script insmod wls driver.
#SVN id: $Id: mtlk_insmod_wls_driver.sh 2278 2008-02-21 15:40:01Z ediv $

# Defines
if [ ! "$MTLK_INIT_PLATFORM" ]; then			
	. /tmp/mtlk_init_platform.sh
	MTLK_INIT_PLATFORM="1"
	print2log DBG "mtlk_init_platform called in mtlk_insmod_wls_driver.sh"
fi
command=$1

insmod_device()
{
	device=$1
	option="$2"
	cd /tmp	
	insmod $device.ko $option debug=-1
	res=$?
	# verify that insmod was successful
	count=0
	while [ $res != 0 ]
	do
		if [ $count -lt 10 ]
		then
			insmod $device.ko $option debug=-1
			res=$?
			sleep 1
		else
			print2log ASSERT "insmod of $device is failed"
			exit
		fi
		count=`expr $count + 1`
	done
	cd - > /dev/null
}

rmmod_device()
{
	device=$1
	rmmod $device	
	res=$?
	# verify that rmmod was successful
	count=0
	while [ $res != 0 ]
	do
		if [ $count -lt 10 ]
		then
			rmmod $device
			res=$?
			sleep 1
		else
			print2log ALERT "rmmod of $device is failed"
			print2log INFO "Kill drvhlpr and wsccmd processes before"
			break
		fi
		count=`expr $count + 1`
	done		
}

start_insmod_wls_driver()
{	
	
	print2log DBG "start insmod_wls_driver"
	wlan_count=`host_api get $$ sys wlan_count`	
	NETWORK_TYPE=`host_api get $$ 0 network_type`
	if [ "$NETWORK_TYPE" = "$AP" ]
	then
		if [ $wlan_count = 1 ]
		then
			insmod_device mtlk ap=1
		else
			insmod_device mtlk ap=1,1
		fi
	else
		insmod_device mtlk
	fi
	
	print2log DBG "Finish start insmod_wls_driver"
}

stop_insmod_wls_driver()
{
	print2log DBG "stop insmod_wls_driver"
		
	rmmod_device mtlk
	
	print2log DBG "Finish stop insmod_wls_driver"
}

create_config_insmod_wls_driver()
{	
	return	
}

should_run_insmod_wls_driver()
{
	true
}

case $command in
	start)
		start_insmod_wls_driver
	;;
	stop)
		stop_insmod_wls_driver
	;;
	create_config)
		create_config_insmod_wls_driver
	;;
	should_run)
		should_run_insmod_wls_driver
	;;
esac
