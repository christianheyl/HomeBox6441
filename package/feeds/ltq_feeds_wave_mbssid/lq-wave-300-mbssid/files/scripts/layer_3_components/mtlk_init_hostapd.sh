#!/bin/sh

if [ ! "$MAPLOADED" ]; then
	if [ -r /tmp/wave300_map_apIndex ]; then
		. /tmp/wave300_map_apIndex 2>/dev/null
		MAPLOADED="1"
	fi
fi

# Defines
if [ ! "$MTLK_INIT_PLATFORM" ]; then			
	. /tmp/mtlk_init_platform.sh
	MTLK_INIT_PLATFORM="1"
fi
print2log DBG "mtlk_init_hostapd.sh: args: $*"
command=$1
apIndex=$2
#get corresponding wlan network interface from mapping file
eval wlan='$'w300_map_idx_${apIndex}


start_mtlk_init_hostapd()
{
	print2log DBG "start mtlk_init_hostapd"
	HOSTAPD_COUN=`ps | grep "\<hostapd_$wlan\>" | grep -vc grep`			
	if [ $HOSTAPD_COUN != 0 ]; then return; fi

    if [ ! -e /tmp/hostapd ]; then ln -s $BINDIR/hostapd /tmp/hostapd; fi
    if [ ! -e /tmp/hostapd.eap_user ]; then ln -s $BINDIR/hostapd.eap_user /tmp/hostapd.eap_user; fi
    if [ ! -e /tmp/hostapd_cli ]; then ln -s $BINDIR/hostapd_cli /tmp/hostapd_cli; fi
    if [ ! -e /tmp/hostapd_$wlan.conf ]; then ln -s $CONFIGS_PATH/hostapd_$wlan.conf /tmp/hostapd_$wlan.conf; fi
	if [ ! -e /tmp/hostapd_$wlan ]; then ln -s $BINDIR/hostapd /tmp/hostapd_$wlan; fi
	/tmp/hostapd_$wlan -d $CONFIGS_PATH/hostapd_$wlan.conf > /dev/null 2>/dev/null &
	print2log DBG "Finish start mtlk_init_hostapd"
}

reload_mtlk_init_init_hostapd()
{
	print2log DBG "reload mtlk_init_hostapd"
	HOSTAPD_COUN=`ps | grep "\<hostapd_$wlan\>" | grep -vc grep`
	if [ $HOSTAPD_COUN != 0 ]
	then
		killall -HUP hostapd_$wlan
	else
		start_mtlk_init_hostapd
	fi
	print2log DBG "Finish reload mtlk_init_hostapd"
}

stop_mtlk_init_hostapd()
{
	print2log DBG "stop mtlk_init_hostapd"
	HOSTAPD_COUN=`ps | grep "\<hostapd_$wlan\>" | grep -vc grep`
	if [ $HOSTAPD_COUN != 0 ]
	then
		killall hostapd_$wlan
		A=`ps | grep "\<hostapd_$wlan\>" | grep -v grep | awk '{print $1}'`
		for a in $A ; do kill -9 $a ; done
	fi
	print2log DBG "Finish stop mtlk_init_hostapd"
}

create_config_mtlk_init_hostapd()
{
	print2log DBG "config mtlk_init_hostapd"
	ESSID=`host_api get $$ $apIndex NonProc_ESSID`
	echo "logger_syslog_level=3"  >  /tmp/temp_hostapd.conf
	echo "ctrl_interface=/var/run/hostapd" >>  /tmp/temp_hostapd.conf
	echo "ctrl_interface_group=0" >>  /tmp/temp_hostapd.conf
	echo "interface=$wlan"  >>  /tmp/temp_hostapd.conf
	echo "driver=mtlk"      >> /tmp/temp_hostapd.conf
	echo "ssid=$ESSID"       >> /tmp/temp_hostapd.conf
	echo "macaddr_acl=0"    >> /tmp/temp_hostapd.conf
	#echo "bridge=br0" >> /tmp/temp_hostapd.conf
	#todo: is this number of wlan cards or number of vaps? do we need both here?
	wlan_count=`host_api get $$ sys wlan_count`
	bridging=`host_api get $$ $apIndex BridgeMode` 
	# in case of dual band, in bridge mode set br0 in both cards. 
	if [ $bridging -gt 0 ] 
	then
		echo "bridge=br0"   >> /tmp/temp_hostapd.conf
	fi	
		
	if [ `host_api get $$ $apIndex IsHTEnabled` = "$ENABLE" ]; then echo "wme_enabled=1" >> /tmp/temp_hostapd.conf; fi
		
	SECURITY_MODE=`host_api get $$ $apIndex NonProcSecurityMode`
	
	case $SECURITY_MODE in
		$open)
			echo "# ------AP is in Plain Text mode------" >> /tmp/temp_hostapd.conf
			;;
		$wep)
			echo "wep_default_key=`host_api get $$ $apIndex WepTxKeyIdx`" >> /tmp/temp_hostapd.conf
			
			for i in 0 1 2 3
            do
#                key=`host_api get $$ wlan0 WepKeys_DefaultKey$i`
                key=`host_api get $$ $apIndex WepKeys_DefaultKey$i`
				if [ `echo $key | grep 0x` ]
				then
					echo "wep_key$i=`echo $key | sed 's/^0x//'`" >> /tmp/temp_hostapd.conf
				else
					echo "wep_key$i=\"$key\"" >> /tmp/temp_hostapd.conf
				fi
				
            done
			echo "# ------AP is in WEP mode------"     >> /tmp/temp_hostapd.conf
			;;
		$WPA_Personal)
			WPA_mode=`host_api get $$ $apIndex NonProc_WPA_Personal_Mode`
			if [ "$WPA_mode" = "4" ] 
			then
				echo "wpa=3" >> /tmp/temp_hostapd.conf
			else
				echo "wpa=$WPA_mode" >> /tmp/temp_hostapd.conf
			fi
						
			echo "wpa_key_mgmt=WPA-PSK"   >> /tmp/temp_hostapd.conf
			psk=`host_api get $$ $apIndex NonProc_WPA_Personal_PSK`
			psk_length=`echo $psk | wc -L`
			if [ $psk_length = 64 ]
			then
				echo "wpa_psk=$psk" >> /tmp/temp_hostapd.conf
			else
				echo "wpa_passphrase=$psk" >> /tmp/temp_hostapd.conf
			fi
			
			wpa_group_rekey=`host_api get $$ $apIndex wpa_group_rekey`
			echo "wpa_group_rekey=$wpa_group_rekey" >> /tmp/temp_hostapd.conf
			echo "wpa_gmk_rekey=$wpa_group_rekey"  >> /tmp/temp_hostapd.conf

			if [ $WPA_mode != 4 ] 
			then
				case `host_api get $$ $apIndex NonProc_WPA_Personal_Encapsulation` in
					0) 
						echo "wpa_pairwise=TKIP" >> /tmp/temp_hostapd.conf
						echo "# ------AP is in WPA/WPA2 PSK, TKIP------"     >> /tmp/temp_hostapd.conf					
						;;
					1)
						echo "wpa_pairwise=CCMP" >> /tmp/temp_hostapd.conf
						echo "# ------AP is in WPA/WPA2 PSK, CCMP------"     >> /tmp/temp_hostapd.conf					
						;;
					2)
						echo "wpa_pairwise=TKIP CCMP" >> /tmp/temp_hostapd.conf
						echo "# ------AP is in WPA/WPA2 PSK, TKIP CCMP------"     >> /tmp/temp_hostapd.conf										
						;;
				esac
			else
				echo "wpa_pairwise=TKIP" >> /tmp/temp_hostapd.conf
				echo "rsn_pairwise=CCMP" >> /tmp/temp_hostapd.conf
				echo "# ------AP is in WPA TKIP / WPA2 CCMP------"     >> /tmp/temp_hostapd.conf
			fi	
		;;

		$WPA_Enterprise)
			echo "ieee8021x=1"	>> /tmp/temp_hostapd.conf    	   			      
			echo "auth_server_addr=`host_api get $$ $apIndex NonProc_WPA_Enterprise_Radius_IP`"  >> /tmp/temp_hostapd.conf         
			echo "auth_server_port=`host_api get $$ $apIndex NonProc_WPA_Enterprise_Radius_Port`" >> /tmp/temp_hostapd.conf        
			echo "auth_server_shared_secret=`host_api get $$ $apIndex NonProc_WPA_Enterprise_Radius_Key`" >> /tmp/temp_hostapd.conf
			echo "acct_server_addr=`host_api get $$ $apIndex NonProc_WPA_Enterprise_Radius_IP`" >> /tmp/temp_hostapd.conf          
			echo "acct_server_port=$((`host_api get $$ $apIndex NonProc_WPA_Enterprise_Radius_Port`+1))" >> /tmp/temp_hostapd.conf 			
			echo "acct_server_shared_secret=`host_api get $$ $apIndex NonProc_WPA_Enterprise_Radius_Key`" >> /tmp/temp_hostapd.conf
			echo "wpa_key_mgmt=WPA-EAP" >> /tmp/temp_hostapd.conf  					
			echo "eap_reauth_period=`host_api get $$ $apIndex NonProc_WPA_Enterprise_Radius_ReKey_Interval`" >> /tmp/temp_hostapd.conf
			
			WPA_mode=`host_api get $$ $apIndex NonProc_WPA_Personal_Mode`
			if [ "$WPA_mode" = "4" ] 
			then
				echo "wpa=3" >> /tmp/temp_hostapd.conf  	
			else
				echo "wpa=$WPA_mode" >> /tmp/temp_hostapd.conf  
			fi
			
			# WPA_mode = 4: Restricted Mixed Mode when not connecting WPA-CCMP and WPA2-TKIP
			if [ $WPA_mode != 4 ] 
			then
				case `host_api get $$ $apIndex NonProc_WPA_Enterprise_Encapsulation` in
					0) 
						echo "wpa_pairwise=TKIP" >> /tmp/temp_hostapd.conf
						echo "# ------AP is in WPA/WPA2 Enterprise, TKIP------"     >> /tmp/temp_hostapd.conf					
						;;
					1)
						echo "wpa_pairwise=CCMP" >> /tmp/temp_hostapd.conf
						echo "# ------AP is in WPA/WPA2 Enterprise, CCMP------"     >> /tmp/temp_hostapd.conf					
						;;
					2)
						echo "wpa_pairwise=TKIP CCMP" >> /tmp/temp_hostapd.conf
						echo "# ------AP is in WPA/WPA2 Enterprise, TKIP CCMP------"     >> /tmp/temp_hostapd.conf										
						;;
				esac				
			else
				echo "wpa_pairwise=TKIP" >> /tmp/temp_hostapd.conf
				echo "rsn_pairwise=CCMP" >> /tmp/temp_hostapd.conf
				echo "# ------AP is in WPA TKIP / WPA2 CCMP  Enterprise ------"     >> /tmp/temp_hostapd.conf
			fi
			;;			
	esac
	
	manufacturer=`host_api get $$ env_param manufacturer`
	echo "manufacturer=$manufacturer" >> /tmp/temp_hostapd.conf
	model_name=`host_api get $$ env_param model_name`
	echo "model_name=$model_name" >> /tmp/temp_hostapd.conf
	model_number=`host_api get $$ env_param model_number`
	echo "model_number=$model_number" >> /tmp/temp_hostapd.conf
# ?		serial_number=`host_api get $$ env_param serial_number`
	serial_number="000"
	echo "serial_number=$serial_number" >> /tmp/temp_hostapd.conf
	config_methods="label display push_button keypad"
	echo "config_methods=$config_methods" >> /tmp/temp_hostapd.conf
	
	echo "device_type=6-0050F204-1" >> /tmp/temp_hostapd.conf
	echo "os_version=01020300" >> /tmp/temp_hostapd.conf
	echo "ignore_broadcast_ssid=0" >> /tmp/temp_hostapd.conf
	
	auth_algs=`host_api get $$ $apIndex Authentication`
	echo "auth_algs=$auth_algs" >> /tmp/temp_hostapd.conf
	echo "eapol_key_index_workaround=0" >> /tmp/temp_hostapd.conf
	echo "eap_server=1" >> /tmp/temp_hostapd.conf

	WPS_ON=`host_api get $$ $apIndex NonProc_WPS_ActivateWPS`
	if [ "$WPS_ON" = "$YES" ]
	then
		AP_CONFIGURED=`host_api get $$ $apIndex NonProc_WPS_ApStatus`
		if [ "$AP_CONFIGURED" = "$NO" ]
		then
			echo "wps_state=1" >> /tmp/temp_hostapd.conf
			echo "upnp_iface=br0" >> /tmp/temp_hostapd.conf
			echo "ap_setup_locked=0" >> /tmp/temp_hostapd.conf
		else
			echo "wps_state=2" >> /tmp/temp_hostapd.conf
		fi
		AP_PIN=`host_api get $$ $apIndex NonProc_WPS_DevicePIN`
		echo "ap_pin=$AP_PIN" >> /tmp/temp_hostapd.conf
	else
		echo "wps_state=0" >> /tmp/temp_hostapd.conf
	fi

	mv /tmp/temp_hostapd.conf $CONFIGS_PATH/hostapd_$wlan.conf
		
	#write to flash
	#host_api commit $$
	print2log DBG "Finish config mtlk_init_hostapd"
}

should_run_mtlk_init_hostapd()
{
	NETWORK_TYPE=`host_api get $$ $apIndex network_type`
	if [ "$NETWORK_TYPE" = "$AP" -o "$NETWORK_TYPE" = "$VAP" ]
	then
		true
	else
		false
	fi
}

case $command in
	start)
		start_mtlk_init_hostapd 
	;;
	reload)
		reload_mtlk_init_init_hostapd
	;;
	stop)
		stop_mtlk_init_hostapd
	;;
	create_config)
		create_config_mtlk_init_hostapd
	;;
	should_run)
		should_run_mtlk_init_hostapd
	;;
esac


