#!/bin/sh
# This is the script set wls links.
#SVN id: $Id: mtlk_init_links.sh 2278 2008-02-21 15:40:01Z ediv $



# Defines
. /tmp/mtlk_init_platform.sh
command=$1
wlan=$2

start_wps_links()
{
	print2log DBG "start wps_links"	
	cd /tmp/wlan$
	cp $CONFIGS_PATH/$wlan/template.conf .
	for i in `ls WPS*`;
	do
		if [ ! -e /tmp/$i ]; then ln -s  /tmp/wlan0/$i  /tmp/$i; fi
	done
	cd - > /dev/null
	print2log DBG "Finish wps_links"
}

stop_wps_links()
{
	return
}

create_config_wps_links()
{
	return
}

should_run_wps_links()
{
	WPS_ON=`host_api get $$ $wlan NonProc_WPS_ActivateWPS`
	if [ "$WPS_ON" = "$YES" ]
	then
		true
	else
		false
	fi
}
case $command in
	start)
		start_wps_links
	;;
	stop)
		stop_wps_links
	;;
	create_config)
		create_config_wps_links
	;;
	should_run)
		should_run_wps_links
	;;
esac
