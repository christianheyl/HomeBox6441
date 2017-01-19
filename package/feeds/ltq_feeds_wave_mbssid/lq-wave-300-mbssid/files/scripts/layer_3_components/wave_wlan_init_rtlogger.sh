#!/bin/sh

. /tmp/mtlk_init_platform.sh

command=$1

#Variables
TMPDIR="/tmp"
DEVDIR="$TMPDIR/dev"
device="mtlklog"
module="mtlk_log"
device_log="mtlk_log"
logserver="logserver"
logdriver="mtlklog.ko"
device_log_minor="0"
mode="660"
user="root"
group="root"

check_logger_status()
{		
	if [ ! -e $BINDIR/$logserver ] || [ ! -e $DRIVER_PATH/$logdriver ]
	then
		print2log ALERT "wave_wlan_init_rtlogger: RT Logger didn't start: $BINDIR/$logserver or $DRIVER_PATH/$logdriver is missing"
		exit
	fi
}

insmod_device()
{
	cd $TMPDIR
	insmod $device.ko
	res=$?	
	# verify that insmod was successful
	count=0
	while [ $res != 0 ]
	do
		if [ "$count" -lt "10" ]
		then
			insmod $device.ko
			res=$?
			sleep 1
		else
			print2log ALERT "wave_wlan_init_rtlogger: insmod of $device is failed!!!"
			exit
		fi
		count=`expr $count + 1`
	done
	cd - > /dev/null
}

rmmod_device()
{
	rmmod $device.ko
	res=$?
	# verify that rmmod was successful
	count=0
	while [ $res != 0 ]
	do
		if [ "$count" -lt "10" ]
		then
			rmmod $device.ko
			res=$?
			sleep 1
		else
			print2log ALERT "wave_wlan_init_rtlogger: rmmod of $device is failed!!!"
			print2log INFO "verify to rmmod mtlk.ko"
			break
		fi
		count=`expr $count + 1`
	done		
}

logger_up ()
{
	print2log DBG "wave_wlan_init_rtlogger: Start logger_up"
	check_logger_status
	if [ `lsmod | grep -c $device` -gt 0 ]
	then 
		print2log INFO "wave_wlan_init_rtlogger: RT Logger is already up"
		return
	fi
	if [ ! -e $TMPDIR/$logserver ]; then ln -s $BINDIR/$logserver $TMPDIR/$logserver; fi
	if [ ! -e $TMPDIR/$logdriver ]; then ln -s $DRIVER_PATH/$logdriver $TMPDIR/$logdriver; fi
	insmod_device
	mkdir -p ${DEVDIR}
	rm -f ${DEVDIR}/${device}? 
	device_log_major=`cat /proc/devices | awk "\\$2==\"$module\" {print \\$1}"`
	mknod ${DEVDIR}/${device}0 c $device_log_major $device_log_minor
	ln -sf ${device}${device_log_minor} ${DEVDIR}/${device_log}
	# give appropriate group/permissions
	if [ "$LOGNAME" != "root" ]; then chown $user:$group ${DEVDIR}/${device}[0-0]; fi	
	chmod $mode ${DEVDIR}/${device}[0-0]
	# Start logserver
	/tmp/$logserver -f ${DEVDIR}/${device}0 &	
	print2log DBG "wave_wlan_init_rtlogger: End logger_up"
}

logger_down ()
{
	print2log DBG "wave_wlan_init_rtlogger: Start logger_down"
	check_logger_status
	if [ `lsmod | grep -c $device` = 0 ]
	then 
		print2log INFO "wave_wlan_init_rtlogger: RT Logger is already down"
		return
	fi
	killall $logserver	
	# remove nodes
	mkdir -p ${DEVDIR}
	rm -f ${DEVDIR}/${device}? 
	# unload driver
	rmmod_device
	[ -e $TMPDIR/$logserver ] && rm $TMPDIR/$logserver
	[ -e $TMPDIR/$logdriver ] && rm $TMPDIR/$logdriver
	print2log DBG "wave_wlan_init_rtlogger: End logger_down"
}

logger_should_run ()
{
	loggerEnabled=`host_api get $$ sys RTLoggerEnabled`
	print2log DBG "wave_wlan_init_rtlogger.sh: RTLoggerEnabled=$loggerEnabled."
	if [ "$loggerEnabled" = "1" ]
	then
		true
	else
		false
	fi
}

case $command in
	start )
		logger_up
    ;;
	stop )
		logger_down
	;;
	should_run )
		logger_should_run
	;;
	* )
		echo "ERROR: Unknown command=$command"
    ;;
esac