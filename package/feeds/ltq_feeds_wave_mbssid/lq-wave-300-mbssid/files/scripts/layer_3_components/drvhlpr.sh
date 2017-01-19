#!/bin/sh

if [ ! "$ENVLOADED" ]; then
	if [ -r /etc/rc.conf ]; then
		 . /etc/rc.conf 2> /dev/null
		ENVLOADED="1"
	fi
fi

# Defines
if [ ! "MTLK_INIT_PLATFORM" ]; then			
	. /tmp/mtlk_init_platform.sh
	MTLK_INIT_PLATFORM="1"
fi

wlan=$1

/tmp/drvhlpr_$wlan -p $CONFIGS_PATH/drvhlpr_$wlan.conf </dev/console 1>/dev/console 2>&1

if [ $? = 1 ]
then
  print2log INFO "MAC HANG"
  if [ -e /tmp/mtlk_info_dump.sh ]; then sh -c /tmp/mtlk_info_dump.sh; fi
  #print2log INFO "VERSION WITHOUT REBOOT BY MAC WD - REMOVE IF USING OFFICIAL CV"
  #Comment the Reboot command to disable the MAC WD reboot	
  ##reboot
  #drvhlpr will reboot the system while we have bug in SW Watchdog mechanism
  #(. ./reload_mtlk_driver.sh > /dev/null);

  # Don't reboot the system in UGW - just restart the wls interfaces
  print2log INFO1 "---  drvhlpr.sh:  Restarting wireless interface  ---"
  $ETC_PATH/wave_wlan_stop

	# todo: later wave_wlan_start will not get called for related ap index,
	# but for the moment all AP/VAP must be started (they have been stopped and
	# the driver was rmmod)
	eval wave300ApCount='$'wlan_main_Count
	print2log DBG "wave300ApCount=$wave300ApCountOld"
	for i in 0 1 2 3 4 5 6 7 8 9
	do
		if [ $wave300ApCount -eq $i ]; then
			break
		fi
		print2log DBG "drvhlpr.sh: start: i = $i"
		(. /etc/rc.d/wave_wlan_start $i)
	done

elif [ $? = 2 ]
  then
  echo drvhlpr return rmmod
else
  print2log INFO "drvhlpr return error: $?"
fi

