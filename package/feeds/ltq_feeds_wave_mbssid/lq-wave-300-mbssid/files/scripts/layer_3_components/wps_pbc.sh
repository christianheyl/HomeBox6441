#!/bin/sh
# A pushbutton daemon for dorango

WPS_PBC_GPIO=`host_api get $$ hw_wlan0 WPS_PB`
if [ "$#" -eq "0" ] || [ "$WPS_PBC_GPIO" = "null" ]; then return; fi
while [ true ]
do
	cat $WPS_PBC_GPIO > /dev/null
	$ETC_PATH/mtlk_wps_cmd.sh $1 $2
	sleep 1
done


