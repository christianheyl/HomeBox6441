#!/bin/sh
# This is the script set country.
#SVN id: $Id: mtlk_init_country.sh 2278 2008-02-21 15:40:01Z ediv $

#Defines
if [ ! "$MTLK_INIT_PLATFORM" ]; then			
	. /tmp/mtlk_init_platform.sh
	MTLK_INIT_PLATFORM="1"
	print2log DBG "mtlk_init_platform called in mtlk_init_country.sh"
fi
command=$1

start_mtlk_init_country()
{
	print2log DBG "start mtlk_init_country"
	
	wlan_count=`host_api get $$ sys wlan_count`
	if [ -z $wlan_count ]; then  wlan_count=1; fi
	
	i=0
	while [ "$i" -lt "$wlan_count" ]
	do
		wlan=wlan$i	
	
		COUNTRY_CODE=`iwpriv $wlan gEEPROM | sed -n '/EEPROM country:/{s/EEPROM country:.*\([A-Z][A-Z]\)/\1/p}'`
		
		if [ $COUNTRY_CODE ]
		then	
			host_api set $$ $wlan EEPROMCountryValid $YES
			host_api set $$ $wlan Country $COUNTRY_CODE
		else
			host_api set $$ $wlan EEPROMCountryValid $NO
		fi
		i=`expr $i + 1`
		continue
		#host_api commit $$
		#config_save.sh
	done
	print2log DBG "finish start mtlk_init_country"
}

stop_mtlk_init_country()
{
	return
}

create_config_mtlk_init_country()
{
	return
}

should_run_mtlk_init_country()
{
	true
}

case $command in
	start)
		start_mtlk_init_country
	;;
	stop)
		stop_mtlk_init_country
	;;
	create_config)
		create_config_mtlk_init_country
	;;
	should_run)
		should_run_mtlk_init_country
	;;
esac

