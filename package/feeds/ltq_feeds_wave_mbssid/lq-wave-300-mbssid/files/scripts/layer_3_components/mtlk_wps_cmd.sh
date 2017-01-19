#!/bin/sh

if [ ! "$MAPLOADED" ]; then
	if [ -r /tmp/wave300_map_apIndex ]; then
		. /tmp/wave300_map_apIndex 2>/dev/null
		MAPLOADED="1"
	fi
fi

# Source for common useful functions
if [ ! "$MTLK_INIT_PLATFORM" ]; then			
	. /tmp/mtlk_init_platform.sh
	export MTLK_INIT_PLATFORM="1"
	print2log DBG "mtlk_init_platform called in mtlk_wps_cmd.sh"
fi
print2log DBG "mtlk_wps_cmd.sh: args: $*"
action=$1
ap_index=$2

#get corresponding wlan network interface from mapping file
eval wlan='$'w300_map_idx_${ap_index}


#for now, when working with WPS single-band, the tmp_wps_folder will be /tmp.
# When inserting CDB WPS, we will use /tmp/$wlan
tmp_wps_folder=/tmp

#File names
HOSTAPD_CONF="$tmp_wps_folder/hostapd_$wlan.conf"
SUPPLICANT_CONF="$tmp_wps_folder/config.conf"
HOSTAPD_CLI="$tmp_wps_folder/hostapd_cli"
WPA_CLI="$tmp_wps_folder/wpa_cli"
WPS_CMD_SCRIPT="mtlk_wps_cmd.sh"
WPS_ACTION_STATFILE="$tmp_wps_folder/wps_action"

#Since events are not yet supported in UGW, the following files left as a comment as a preparation for supporting drvhlpr events.
#WPS_START_TIME_STATFILE="$tmp_wps_folder/wps_startup_time"
#This variable should be used by the display layer to figure out what the current WPS status is 
#WPS_CURRENT_STATUS_STATFILE="$tmp_wps_folder/wps_current_status"
#WPS_LAST_CODE_STATFILE="$tmp_wps_folder/wps_last_code"


#Constant variables
STA=0
AP=2
NOT_PBC=0
IS_PBC=1
STA_enrollee=0
AP_enrollee=2
AP_proxy=1

CMD_QUIT=0
CMD_AP_GET_CONF=1
CMD_REG_CONF_AP=2
CMD_STA_GET_CONF=3
CMD_REG_CONF_STA=4
CMD_AP_ABORT=5

ap_select=0
securityOpen=1
securityWEP=2
securityWPAPersonal=3

cmd_length=2
wps_msg_length=256

WPS_Active_Blink=6
WPS_Idle_Blink=0

network_type=`host_api get $$ $ap_index network_type`

#Setting a parameter to know if we are using hostapd_wlanX or supplicant
if [ "$network_type" = "$AP" ]
then
	Hostapd_Supplicant="hostapd_$wlan"
elif [ "$network_type" = "$STA" ]
then
	Hostapd_Supplicant="wpa_supplicant"
fi

# see if this is not the first instance, if another instance is alive, abort the run
# Since we saw some cases where ps detected too many processes, we set that the ps would be printed to the screen in case on the error.
ps_result=`ps`
isWpsCmd=`echo $ps_result | grep "\<$WPS_CMD_SCRIPT\>" | grep -vc grep`
#isWpsCmd=`ps | grep "\<$WPS_CMD_SCRIPT\>" | grep -vc grep`
if [ $isWpsCmd ] && [ $isWpsCmd -gt 2 ]
then
	print2log ERROR "mtlk_wps_cmd: Aborted. $WPS_CMD_SCRIPT is already running"
	echo "ps result: $ps_result" > /dev/console 
	exit
fi
	
showUsage()
{
	echo -e "Wave300 WPS control script"
	echo -e "This script is used to control the WPS Session state"
	echo -e "Syntax :"
	echo -e "mtlk_wps_cmd.sh action "
	echo -e "\nactions : "
	echo -e "conf_via_pin <PIN> : Configure a STA via PIN. "
	echo -e "\t PIN : PIN Number"
	echo -e "conf_via_pbc : Configure a STA using PBC"
	echo -e "get_conf_via_pin <MANUAL>: Get configured by an AP using PIN"
	echo -e "\t MANUAL : 1 for manual AP selection, 0 for automatic AP selection"
	echo -e "get_conf_via_pbc : Get configured by an AP using PBC"
	echo -e "ap_selected_from_list <MAC> : Select an AP to connect to."
	echo -e "\t MAC  : MAC Address of AP to connect to"
	echo -e "show_settings : Display current WPS settings"
	echo -e "stop : Stop WPS by disabling it"
	echo -e "start : Start WPS by enabling it"
	echo -e "abort : Currently, same as 'stop'"
	echo -e "save_settings : Save current WPS session settings to configuration files."
}

checkAPconfigured()
{
	print2log DBG "mtlk_wps_cmd: Start checkAPconfigured"
	WPS_ApStatus=`host_api get $$ $ap_index NonProc_WPS_ApStatus`
	
	#If it is AP and it is non-configured.
	if [ "$network_type" = "$AP" ] && [ $WPS_ApStatus ] && [ $WPS_ApStatus = 0 ]
	then
		print2log DBG "mtlk_wps_cmd: checkAPconfigured. AP is non-configured, setting to configured"
		host_api set $$ $ap_index NonProc_WPS_ApStatus 1
		host_api commit $$
		#QUESTION: Is it enough? don't need to restart hostapd?
	else
		print2log DBG "mtlk_wps_cmd: checkAPconfigured. AP is in configured mode"
	fi
	print2log DBG "mtlk_wps_cmd: End checkAPconfigured"
}

get_param()
{
	echo `grep "[^#]\<$1\>" $2 | sed 's/^[^=]*=[ ]*//'`
}

stopWPSPBC()
{
	#PBC functions are not supported at the moment and will be changed when it will be supported by UGW.
	print2log DBG "mtlk_wps_cmd: Start stopPBC"
	killall wps_pbc.sh

	wpsPBCgpio=`host_api get $$ hw_$wlan WPS_PB`
	if [ $wpsPBCgpio ] && [ "$wpsPBCgpio" != "null" ]
	then
		A=`ps | grep "\<cat $wpsPBCgpio\>" | grep -v grep | awk '{print $1}'`
		for a in $A ; do kill -9 $a ; done
	fi
	print2log DBG "mtlk_wps_cmd: End stopPBC"
}

WPSActionStart()
{
	print2log DBG "mtlk_wps_cmd: Start WPSActionStart"
	# erase Wildcard_ESSID if not empty
	wildcard_essid=`iwpriv $wlan gActiveScanSSID | sed 's#.*gActiveScanSSID[^:]*:[ ]*##'`
	if [ "$wildcard_essid" ]
	then
		print2log DBG "mtlk_wps_cmd: WPSActionStart erasing ActiveScanSSID"
		iwpriv $wlan sActiveScanSSID ""
	fi
	
	isHostapdSupplicantAlive=`ps | grep "\<$Hostapd_Supplicant\>" | grep -vc grep`
	#If hostapd/supplicant is not up, reload it.
	if [ $isHostapdSupplicantAlive ] && [ $isHostapdSupplicantAlive = 0 ]
	then
		if [ "$network_type" = "$AP" ]
		then
			print2log DBG "mtlk_wps_cmd: WPSActionStart reloading hostapd"
			$ETC_PATH/mtlk_init_hostapd.sh should_run $ap_index && $ETC_PATH/mtlk_init_hostapd.sh create_config $ap_index  && $ETC_PATH/mtlk_init_hostapd.sh start $ap_index
		elif [ "$network_type" = "$STA" ]
		then
			print2log DBG "mtlk_wps_cmd: WPSActionStart reloading supplicant"
			$ETC_PATH/mtlk_init_wpa_supplicant.sh should_run $ap_index && $ETC_PATH/mtlk_init_wpa_supplicant.sh create_config $ap_index  && $ETC_PATH/mtlk_init_wpa_supplicant.sh start $ap_index
		fi
		
		killall -USR2 drvhlpr
	fi
	
	# Reload the WPS_PBC.sh if it's not there.
	isWpsPbcAlive=`ps | grep wps_pbc | grep -vc grep`
	if [ $isWpsPbcAlive ] && [ $isWpsPbcAlive = 0 ]
	then
		print2log DBG "mtlk_wps_cmd: WPSActionStart reloading WPS_PBC"
		$ETC_PATH/mtlk_wps_pbc.sh should_run $ap_index && $ETC_PATH/mtlk_wps_pbc.sh start $ap_index
	fi
	print2log DBG "mtlk_wps_cmd: End WPSActionStart"
}

WPSActionStop ()
{
	print2log DBG "mtlk_wps_cmd: Start WPSActionStop"
	
	stopWPSPBC
	
	rm $WPS_ACTION_STATFILE

	killall -USR1 drvhlpr
	
	# reload hostapd/supplicant
	if [ "$network_type" = "$AP" ]
		then
			print2log DBG "mtlk_wps_cmd: WPSActionStop reloading hostapd"
			$ETC_PATH/mtlk_init_hostapd.sh should_run $ap_index && $ETC_PATH/mtlk_init_hostapd.sh create_config $ap_index  && $ETC_PATH/mtlk_init_hostapd.sh reload $ap_index
		elif [ "$network_type" = "$STA" ]
		then
			print2log DBG "mtlk_wps_cmd: WPSActionStop reloading supplicant"
			$ETC_PATH/mtlk_init_wpa_supplicant.sh should_run $ap_index && $ETC_PATH/mtlk_init_wpa_supplicant.sh create_config $ap_index  && $ETC_PATH/mtlk_init_wpa_supplicant.sh reload $ap_index
	fi
	
	print2log DBG "mtlk_wps_cmd: End WPSActionStop"
}

WPSActionAPAbort()
{
	print2log DBG "mtlk_wps_cmd: Start WPSActionAPAbort"
	WPSActionStop
	print2log DBG "mtlk_wps_cmd: End WPSActionAPAbort"
}

# Since events are not yet supported in UGW, this function is left as a comment as a preparation for supporting drvhlpr events.
# getWPSSessionStat()
# {
#	 print2log DBG "mtlk_wps_cmd: Start getWPSSessionStat"
#	
#	 lastCode=`get_param WPS_LastErrorCode $WPS_LAST_CODE_STATFILE`
#	
#	 isHostapdSupplicantAlive=`ps | grep "\<$Hostapd_Supplicant\>" | grep -vc grep`
#	 if [ $lastCode ] && [ $isHostapdSupplicantAlive -gt 0 ]
#	 then
#		 print2log DBG "mtlk_wps_cmd: getWPSSessionStat last code is $lastCode"
#		 echo $lastCode
#	else
#		 print2log DBG "mtlk_wps_cmd: getWPSSessionStat no last code or $Hostapd_Supplicant is not alive"
#		 echo 0
#	fi
#	
#	print2log DBG "mtlk_wps_cmd: End getWPSSessionStat"
# }

#Since events are not yet supported in UGW, this function is left as a comment as a preparation for supporting drvhlpr events.
#CheckWPSManualSessionTimeout ()
#{
#	print2log DBG "mtlk_wps_cmd: Start CheckWPSManualSessionTimeout"
#	
#	wpsSessionStat=`getWPSSessionStat`
#	if [ $wpsSessionStat = 5 ] || [ $wpsSessionStat = 0 ]
#	then
#		print2log DBG "mtlk_wps_cmd: CheckWPSManualSessionTimeout session is $wpsSessionStat or $Hostapd_Supplicant is not alive"
#		WPSActionStop
#		WPSActionStart
#	fi
#	
#	print2log DBG "mtlk_wps_cmd: End CheckWPSManualSessionTimeout"
#}

#Since events are not yet supported in UGW, this function is left as a comment as a preparation for supporting drvhlpr events.
#CheckWPSSessionInProgress()
#{
#	print2log DBG "mtlk_wps_cmd: Start CheckWPSSessionInProgress"
#	
#	wpsSessionStat=`getWPSSessionStat`
#	if [ $wpsSessionStat = 6 ]
#	then
#		# TODO: Only exit when in Registering state? What about other WPS in progress states?
#		# How should PBC behave when pressed twice?
#		exit
#	fi
#	
#	print2log DBG "mtlk_wps_cmd: End CheckWPSSessionInProgress"
#}

# This function was used by the UI in VB. Will be used in the future by UGW GUI
#get_current_uptime()
#{
#	print2log DBG "mtlk_wps_cmd: Start get_current_uptime"
#	
#	echo `awk -F "." '{print $1}' /proc/uptime`
#	
#	print2log DBG "mtlk_wps_cmd: End get_current_uptime"
#}

#This function was used by the UI in VB. Will be used in the future by UGW GUI
#resetStartTime()
#{
#	print2log DBG "mtlk_wps_cmd: Start resetStartTime"
#	
#	rm $WPS_START_TIME_STATFILE
#	CURRENT_TIME=`get_current_uptime`
#	echo "WPS_StartTime = $CURRENT_TIME" > $WPS_START_TIME_STATFILE
#	
#	print2log DBG "mtlk_wps_cmd: End resetStartTime"
#}

set_gpio()
{
	print2log DBG "mtlk_wps_cmd: Start set_gpio"
	
	led_gpio=`host_api get $$ hw_$wlan $1`
	value=$2
	
	#In the case where there is no LED, do nothing
	if [ "$led_gpio" != "null" ]
	then
		echo $value > $led_gpio
		print2log DBG "mtlk_wps_cmd: set_gpio $led_gpio with value $value"
	fi
	
	print2log DBG "mtlk_wps_cmd: End set_gpio"
}

startWPSLedBlink()
{
	set_gpio WPS_error_LED $WPS_Idle_Blink
	set_gpio WPS_activity_LED $WPS_Active_Blink
}

stopWPSLedBlink()
{
	set_gpio WPS_activity_LED $WPS_Idle_Blink
	set_gpio WPS_error_LED $WPS_Idle_Blink
}

# Since events are not yet supported in UGW, this function is left as a comment as a preparation for supporting drvhlpr events.
#resetLastWPSCode()
#{
#	print2log DBG "mtlk_wps_cmd: resetLastWPSCode. Deleting $WPS_LAST_CODE_STATFILE"
#	rm $WPS_LAST_CODE_STATFILE
#}

# Since events are not yet supported in UGW, this function is left as a comment as a preparation for supporting drvhlpr events.
#resetCurrentWPSStatus()
#{
#	print2log DBG "mtlk_wps_cmd: resetCurrentWPSStatus. Deleting $WPS_CURRENT_STATUS_STATFILE"
#	rm $WPS_CURRENT_STATUS_STATFILE
#}
	
update_wps_action()
{
	print2log DBG "mtlk_wps_cmd: update_wps_action. WPS action is: $action"
	echo "WPS_action = $action" > $WPS_ACTION_STATFILE
}	

WPSGetConfInit()
{
	print2log DBG "mtlk_wps_cmd: Start WPSGetConfInit"
	
	startWPSLedBlink				
	#CheckWPSManualSessionTimeout
	#CheckWPSSessionInProgress			
	#resetStartTime
	#resetLastWPSCode
	WPSActionStart
	startWPSLedBlink	
	
	print2log DBG "mtlk_wps_cmd: End WPSGetConfInit"
}

#########################################################
# The following functions are the actions available:
# conf_via_pin
#conf_via_pbc
#get_conf_via_pin
#get_conf_via_pbc
#show_settings
#ap_selected_from_list
#restart
#stop
#abort
#start
#save_settings
#########################################################
conf_via_pin()
{
	print2log DBG "mtlk_wps_cmd: Start conf_via_pin"
	
	enrollee_PIN=$1 
	enrollee_type=$2
	
	if [ "$network_type" = "$STA" ] 
	then
		# STA registrar is currently not supported.
		echo "STA registrar is currently not supported !!!" > /dev/console
		print2log DBG "mtlk_wps_cmd: End conf_via_pin"
		return
	fi
	
	startWPSLedBlink
	checkAPconfigured
	#resetStartTime
	#resetLastWPSCode
	update_wps_action
		
	$HOSTAPD_CLI -i $wlan wps_pin any $enrollee_PIN
	
	print2log DBG "mtlk_wps_cmd: End conf_via_pin"		
}

conf_via_pbc()
{
	print2log DBG "mtlk_wps_cmd: Start conf_via_pbc"
	if [ "$network_type" = "$STA" ]
	then
		echo "mtlk_wps_cmd: conf_via_pbc is not supported by STA !!!" > /dev/console
		print2log DBG "mtlk_wps_cmd: End get_conf_via_pbc"
		return
	fi
	
	startWPSLedBlink
	checkAPconfigured

	#resetStartTime
	#resetLastWPSCode
	update_wps_action
	$HOSTAPD_CLI -i $wlan wps_pbc
	print2log DBG "mtlk_wps_cmd: End conf_via_pbc"
}

get_conf_via_pin()
{
	print2log DBG "mtlk_wps_cmd: Start get_conf_via_pin"
	if [ "$network_type" = "$AP" ]
	then
		echo "mtlk_wps_cmd: get_conf_via_pin is not supported by AP !!!" > /dev/console
		print2log DBG "mtlk_wps_cmd: End get_conf_via_pin"
		return
	fi
	manual_AP=$1
	ap_select="any"
	
	WPSGetConfInit	
		
	update_wps_action
	
	if [ $manual_AP ] && [ $manual_AP != "" ]
	then
		ap_select=$manual_AP	
	fi
	
	$WPA_CLI wps_pin $ap_select
	
	print2log DBG "mtlk_wps_cmd: End get_conf_via_pin"
}

get_conf_via_pbc()
{
	print2log DBG "mtlk_wps_cmd: Start get_conf_via_pbc"
	if [ "$network_type" = "$AP" ]
	then
		echo "mtlk_wps_cmd: get_conf_via_pbc is not supported by AP !!!" > /dev/console
		print2log DBG "mtlk_wps_cmd: End get_conf_via_pbc"
		return
	fi
	
	WPSGetConfInit	
	update_wps_action
		
	$WPA_CLI wps_pbc
	print2log DBG "mtlk_wps_cmd: End get_conf_via_pbc"
}

show_settings()
{	
	echo "SSID: `host_api get $$ $ap_index NonProc_ESSID`"
	echo "Security Mode: `host_api get $$ $ap_index NonProcSecurityMode`"
	echo "PSK: `host_api get $$ $ap_index NonProc_WPA_Personal_PSK`"
}

# This option is valid only for STA. In order for it to work with supplican of hostapd06, we need to modify this function.
# The wpa_cli needs the index of the MAC of the selected AP and not the MAC itself.
#ap_selected_from_list()
#{
#	print2log DBG "mtlk_wps_cmd: Start ap_selected_from_list"
#	if [ "$network_type" = "$AP" ]
#	then
#		echo "mtlk_wps_cmd: ap_selected_from_list is not supported by AP !!!" > /dev/console
#		print2log DBG "mtlk_wps_cmd: End ap_selected_from_list"
#		return
#	fi
#
#	mac=$1
#	#echo WPS_LastErrorCode = 1 > $WPS_LAST_CODE_STATFILE		
#	#resetStartTime
#		
#	$WPA_CLI select_network $mac
#	print2log DBG "mtlk_wps_cmd: End ap_selected_from_list"	
#}

restart()
{
	print2log DBG "mtlk_wps_cmd: Start 'restart'"
	stop
	start
	print2log DBG "mtlk_wps_cmd: End 'restart'"
}

stop()
{
	print2log DBG "mtlk_wps_cmd: Start 'stop'"
		
	isWPSenabled=`host_api get $$ $ap_index NonProc_WPS_ActivateWPS`
	if [ ! $isWPSenabled ] || [ $isWPSenabled = 0 ]
	then
		echo "WPS is already disabled !!!" > /dev/console 
		print2log DBG "mtlk_wps_cmd: End 'stop'"
		return
	fi
	
	host_api set $$ $ap_index NonProc_WPS_ActivateWPS 0
	host_api commit $$
	WPSActionStop

	print2log DBG "mtlk_wps_cmd: End 'stop'"
}

abort()
{
	print2log DBG "mtlk_wps_cmd: Start 'abort'"
	stop
	print2log DBG "mtlk_wps_cmd: End 'abort'"
}

start()
{
	print2log DBG "mtlk_wps_cmd: Start 'start'"
	isWPSenabled=`host_api get $$ $ap_index NonProc_WPS_ActivateWPS`
	if [ $isWPSenabled = 1 ]
	then
		echo "WPS is already enabled !!!" > /dev/console 
		print2log DBG "mtlk_wps_cmd: End 'start'"
		return
	fi
		
	host_api set $$ $ap_index NonProc_WPS_ActivateWPS 1
	host_api commit $$
	if [ "$network_type" = "$AP" ]
		then
			print2log DBG "mtlk_wps_cmd: start reloading hostapd"
			$ETC_PATH/mtlk_init_hostapd.sh should_run $ap_index && $ETC_PATH/mtlk_init_hostapd.sh create_config $ap_index  && $ETC_PATH/mtlk_init_hostapd.sh reload $ap_index
		elif [ "$network_type" = "$STA" ]
		then
			print2log DBG "mtlk_wps_cmd: start reloading supplicant"
			$ETC_PATH/mtlk_init_wpa_supplicant.sh should_run $ap_index && $ETC_PATH/mtlk_init_wpa_supplicant.sh create_config $ap_index  && $ETC_PATH/mtlk_init_wpa_supplicant.sh reload $ap_index
	fi
	WPSActionStart
	print2log DBG "mtlk_wps_cmd: End 'start'"
}

save_settings()
{
	#save_settings saves the settings after WPS from the conf file to the rc.conf using host_api
	#In order for save_settings to work, we need drvhlpr to support wps events
	print2log DBG "mtlk_wps_cmd: Start save_settings"
	
	# get current settings.
	ssid=`host_api get $$ $ap_index NonProc_ESSID`
	security=`host_api get $$ $ap_index NonProcSecurityMode`
	psk	=`host_api get $$ $ap_index NonProc_WPA_Personal_PSK`
	
	# if this is an AP, update accordingly
	# Updating AP will occure only on external registrar case, on other cases, settings will not change.
	if [ "$network_type" = "$AP" ]
	then
		if [ ! -e $HOSTAPD_CONF ]
		then
			print2log ERROR "mtlk_wps_cmd: Aborted. $HOSTAPD_CONF file doesn't exist"
			exit
		fi
		g_ssid=`get_param ssid $HOSTAPD_CONF`
		host_api set $$ $ap_index NonProc_ESSID $g_ssid
		
		g_security=`get_param wpa $HOSTAPD_CONF`
		if [ "$g_security" = "$securityWPAPersonal" ]
		then
			host_api set $$ $ap_index NonProcSecurityMode $g_security
								
			g_psk=`get_param wpa_passphrase $HOSTAPD_CONF`
			if [ ! $g_psk ]
			then
				g_psk=`get_param wpa_psk $HOSTAPD_CONF`
			fi
			host_api set $$ $ap_index NonProc_WPA_Personal_PSK $g_psk
		fi
	fi
 	
	# if this is a STA, update accordingly
	if [ "$network_type" = "$STA" ]
	then
		# set NeverConnected in $wlan.conf to 0 so that in reboot, supplicant will be activated 
		NeverConnected=`host_api get $$ $ap_index NeverConnected`
		if [ $NeverConnected = 1 ]
		then
			host_api set $$ $ap_index NeverConnected 0
		fi
		
		if [ ! -e $SUPPLICANT_CONF ]
		then
			print2log ERROR "mtlk_wps_cmd: Aborted. $SUPPLICANT_CONF file doesn't exist"
			exit
		fi
		
		# Since ssid is saved as "SSID", we need to save the ssid as SSID
		g_ssid=`get_param ssid $SUPPLICANT_CONF |  sed 's/\"\([^\"]*\)\"/\1/'`
		host_api set $$ $ap_index NonProc_ESSID $g_ssid
			
		#Set Open security as default
		g_security=$securityOpen
		
		# Since psk is saved as "PSK", we need to save the psk as PSK
		g_psk=`get_param psk $SUPPLICANT_CONF |  sed 's/\"\([^\"]*\)\"/\1/'`
		if [ $g_psk ]
		then
			g_security=$securityWPAPersonal
			host_api set $$ $ap_index NonProc_WPA_Personal_PSK $g_psk
		fi
			
		# Since wep key is saved as "WEP_KEY", we need to save the wep_key as WEP_KEY	
		g_wep_key0=`get_param wep_key0 $SUPPLICANT_CONF |  sed 's/\"\([^\"]*\)\"/\1/'`
		if [ $g_wep_key0 ]
		then
			g_security=$securityWEP
			#In order to determine if it's WEP-64 or WEP-128, we need to check length of the key.
			wep_key0_length=`echo $g_wep_key0 | wc -L`
			if [ $wep_key0_length = 5 ] || [ $wep_key0_length = 10 ]
			then
				WepKeyLength=64
			fi
			if [ $wep_key0_length = 13 ] || [ $wep_key0_length = 26 ]
			then
				WepKeyLength=128
			fi
			host_api set $$ $ap_index NonProc_WepKeyLength $WepKeyLength
			# TODO: check how ascii and how hex are saved
			host_api set $$ $ap_index WepKeys_DefaultKey0 $g_wep_key0
					
			# The NonProc_Authentication also setting WEP-OPEN that value is 1, 2 is shared, 3 is auto. Only 'open' is supported on WPS.
			host_api set $$ $ap_index NonProc_Authentication 1					
		fi
			host_api set $$ $ap_index NonProcSecurityMode $g_security
	fi
	
	host_api commit $$
	print2log DBG "mtlk_wps_cmd: End save_settings"
}

case $action in
	conf_via_pin)
		conf_via_pin $3 $4
	;;
	conf_via_pbc)
		conf_via_pbc
	;;
	get_conf_via_pin)
		get_conf_via_pin $3
	;;
	get_conf_via_pbc)
		get_conf_via_pbc
	;;
	show_settings)
		show_settings
	;;
	ap_selected_from_list)
		ap_selected_from_list $3
	;;
	restart)
		restart
	;;
	stop)
		stop
	;;
	abort)
		abort
	;;
	start)
		start
	;;
	save_settings)
		save_settings
	;;
	*)
		showUsage
	;;
esac
