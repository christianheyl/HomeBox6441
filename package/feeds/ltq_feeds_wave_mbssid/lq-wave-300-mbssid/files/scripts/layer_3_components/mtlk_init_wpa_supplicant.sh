#!/bin/sh

# Defines
if [ ! "$MTLK_INIT_PLATFORM" ]; then			
	print2log DBG "mtlk_init_platform called in $SCRIPT"
	. /tmp/mtlk_init_platform.sh
	export MTLK_INIT_PLATFORM="1"
fi


start_mtlk_init_wpa_supplicant()
{
	# Activates the Supplicant if in STA mode.
	NeverConnected=`host_api get $$ wlan0 NeverConnected`
	WPA_SUPPLICANT_COUNT=`ps | grep "\<wpa_supplicant\>" | grep -vc grep`
	
	if [ "$NeverConnected" = "$YES" ] || [ $WPA_SUPPLICANT_COUNT != 0] ; then return; fi
	
	if [ ! -e /tmp/wpa_cli ]; then ln -s /bin/wpa_cli /tmp/wpa_cli; fi
	if [ ! -e /tmp/wpa_passphrase ]; then ln -s /bin/wpa_passphrase /tmp/wpa_passphrase; fi
	if [ ! -e /tmp/wpa_supplicant ]; then ln -s /bin/wpa_supplicant /tmp/wpa_supplicant; fi
	if [ ! -e /tmp/config.conf ]; then ln -s $CONFIGS_PATH/wpa_supplicant0.conf   /tmp/config.conf; fi

	# if the bridge mode is mac_clon
	BRIDGE_MODE=`host_api get $$ sys BridgeMode`
	if [ "$BRIDGE_MODE" = "$MacCloning" ]
	then
		macl=`host_api get $$ sys MacCloningAddr`
		$ETC_PATH/wpa_supplicant -Dwext -bbr0 -iwlan0 -c $CONFIGS_PATH/wpa_supplicant0.conf -p maclone=$macl > /dev/null 2>/dev/null &
	else
		$ETC_PATH/wpa_supplicant -Dwext -bbr0 -iwlan0 -c $CONFIGS_PATH/wpa_supplicant0.conf > /dev/null 2>/dev/null &
	fi
}

reload_mtlk_init_wpa_supplicant()
{
	print2log DBG "reload reload wpa_supplicant"
	WPA_SUPPLICANT_COUNT=`ps | grep "\<wpa_supplicant\>" | grep -vc grep`
	if [ $WPA_SUPPLICANT_COUNT != 0 ]
	then
		killall -HUP wpa_supplicant
	else
		start_mtlk_init_wpa_supplicant
	fi
	print2log DBG "Finish reload reload wpa_supplicant"
}

stop_mtlk_init_wpa_supplicant()
{
	WPA_SUPPLICANT_COUNT=`ps | grep "\<wpa_supplicant\>" | grep -vc grep`
	if [ $WPA_SUPPLICANT_COUNT != 0 ]
	then
		killall wpa_supplicant
		A=`ps | grep "\<wpa_supplicant\>" | grep -v grep | awk '{print $1}'`
		for a in $A ; do kill -9 $a ; done
	fi
}

create_config_mtlk_init_wpa_supplicant()
{	
	ESSID=`host_api get $$ wlan0 NonProc_ESSID`
	PSK=`host_api get $$ wlan0 NonProc_WPA_Personal_PSK`
	if [ -z $PSK ]; then PSK="WPADummyKey"; fi
	
	echo "ctrl_interface=/var/run/wpa_supplicant" > /tmp/temp_supplicant.conf
	echo "ctrl_interface_group=0" >> /tmp/temp_supplicant.conf
	$ETC_PATH/wpa_passphrase $ESSID $PSK >> /tmp/temp_supplicant.conf
	PSK_Lenght=`echo $PSK | wc -L`
	if [ "$PSK_Lenght" = 64 ]
	then
		cat /tmp/temp_supplicant.conf | sed "s/psk=.*/psk=$PSK/" > /tmp/tmp_supplicant.conf
		mv /tmp/tmp_supplicant.conf /tmp/temp_supplicant.conf
	fi
	cat /tmp/temp_supplicant.conf | sed 's/^}//' > /tmp/tmp_supplicant.conf
	mv /tmp/tmp_supplicant.conf /tmp/temp_supplicant.conf
	
	SECURITY_MODE=`host_api get $$ wlan0 NonProcSecurityMode`
	
	case $SECURITY_MODE in
		$open)
			echo -e "\t key_mgmt=NONE" >> /tmp/temp_supplicant.conf
			echo -e "\t auth_alg=OPEN" >> /tmp/temp_supplicant.conf
			;;
		$wep)
			echo -e "\t key_mgmt=NONE" >> /tmp/temp_supplicant.conf	
			#setting wep keyes
			for i in 0 1 2 3
			do
				echo -e "\t wep_key$i=`host_api get $$ wlan0 WepKeys_DefaultKey$i | sed 's/^0x//'`" >> /tmp/temp_supplicant.conf
			done			
			echo -e "\t wep_tx_keyidx=`host_api get $$ wlan0 WepTxKeyIdx`" >> /tmp/temp_supplicant.conf
			echo -e "\t group=WEP40 WEP104" >> /tmp/temp_supplicant.conf
			#setting  Authentication			
			case `host_api get $$ wlan0 NonProc_Authentication` in
				1) echo -e "\t auth_alg=OPEN" >> /tmp/temp_supplicant.conf ;;
				2) echo -e "\t auth_alg=SHARED" >> /tmp/temp_supplicant.conf ;;
				3) echo -e "\t auth_alg=OPEN SHARED" >> /tmp/temp_supplicant.conf ;;
				*) echo "supported modes are OPEN=1 SHARED=2 OPEN SHARED=3"
			esac
			;;
		$WPA_Personal)
			case `host_api get $$ wlan0 NonProc_WPA_Personal_Mode` in
				1) echo -e "\t proto=WPA" >> /tmp/temp_supplicant.conf ;;
				2) echo -e "\t proto=RSN" >> /tmp/temp_supplicant.conf ;;
			esac
			case `host_api get $$ wlan0 NonProc_WPA_Personal_Encapsulation` in
				0) 
					echo -e "\t pairwise=TKIP" >> /tmp/temp_supplicant.conf
					echo -e "\t group=TKIP" >> /tmp/temp_supplicant.conf					
					;;
				1)
					echo -e "\t pairwise=CCMP" >> /tmp/temp_supplicant.conf
					echo -e "\t group=CCMP" >> /tmp/temp_supplicant.conf					
					;;
				2)
					if [ `host_api get $$ wlan0 NonProc_WPA_Personal_Mode` != 3 ]
					then
						echo -e "\t pairwise=CCMP TKIP" >> /tmp/temp_supplicant.conf
						echo -e "\t group=CCMP TKIP" >> /tmp/temp_supplicant.conf
					fi
					;;
			esac
			;;
		$WPA_Enterprise)
			echo -e "\t key_mgmt=WPA-EAP" >> /tmp/temp_supplicant.conf
			echo -e "\t eap=TTLS PEAP TLS" >> /tmp/temp_supplicant.conf
			echo -e "\t phase2=\"auth=MSCHAPV2 auth=GTC\"" >> /tmp/temp_supplicant.conf
			if [ `host_api get $$ wlan0 NonProc_WPA_Enterprise_Certificate` = 1 ]
			then
				certificate_file=$ROOT_PATH/saved_configs/certificate.pem
				if [ ! -e $certificate_file]; then event_log "WARNING: $certificate_file hasn't found"; fi
				echo -e "\t client_cert=\"$certificate_file\"" >> /tmp/temp_supplicant.conf
			fi
			username=`host_api get $$ wlan0 NonProc_WPA_Enterprise_Radius_Username`
			user_password=`host_api get $$ wlan0 NonProc_WPA_Enterprise_Radius_Password`
			echo -e "\t identity=\"$username\"" >> /tmp/temp_supplicant.conf
			echo -e "\t password=\"$user_password\"" >> /tmp/temp_supplicant.conf
			echo -e "\t auth_alg=OPEN" >> /tmp/temp_supplicant.conf
			;;			
	esac
	
	echo -e "\t scan_ssid=1" >> /tmp/temp_supplicant.conf
	echo "}" >> /tmp/temp_supplicant.conf
	
	#hostapd06:	

	#TODO- support also wps_state=1
	WPS_ON=`host_api get $$ $wlan NonProc_WPS_ActivateWPS`
	if [ "$WPS_ON" = "$YES" ]
	then
		# Vendor specific information
		manufacturer=`host_api get $$ env_param manufacturer`
		echo -e "\t manufacturer=\"$manufacturer\"" >> /tmp/temp_supplicant.conf
		model_name=`host_api get $$ env_param model_name`
		echo -e "\t model_name=$model_name\" >> /tmp/temp_supplicant.conf
		model_number=`host_api get $$ env_param model_number`
		echo -e "\t model_number=$model_number\" >> /tmp/temp_supplicant.conf
		device_name=`host_api get $$ env_param device_name`
		echo -e "\t device_name=\"$device_name\"" >> /tmp/temp_supplicant.conf
# ?		serial_number=`host_api get $$ env_param serial_number`
		serial_number="000"
		echo -e "\t serial_number=$serial_number\" >> /tmp/temp_supplicant.conf

		# Use defined UUID, otherwise use a default
		#TBD, use: cat /proc/sys/kernel/random/uuid ?????	
		uuid=`host_api get $$ sys UUID_WPS`
		if [ -z "$uuid" ]
		then
			uuid=`uuidgen`
			host_api set $$ sys UUID_WPS $uuid
		fi
		echo -e "\t uuid=$uuid\" >> /tmp/temp_supplicant.conf
		echo -e "\t update_config=1\" >> /tmp/temp_supplicant.conf
		device_type="1-0050F204-1"
		echo -e "\t device_type=$device_type\" >> /tmp/temp_supplicant.conf
		os_version=01020300
		echo -e "\t os_version=$os_version\" >> /tmp/temp_supplicant.conf
	fi


	mv /tmp/temp_supplicant.conf $CONFIGS_PATH/wpa_supplicant0.conf
		
	#write to flash
	#host_api commit $$
}

should_run_mtlk_init_wpa_supplicant()
{
	NETWORK_TYPE=`host_api get $$ 0 network_type`
	if [ "$NETWORK_TYPE" = "$STA" ]
	then
		true
	else
		false
	fi
}

case $1 in
	start)
		start_mtlk_init_wpa_supplicant 
	;;
	reload)
		reload_mtlk_init_wpa_supplicant
	;;
	stop)
		stop_mtlk_init_wpa_supplicant
	;;
	create_config)
		create_config_mtlk_init_wpa_supplicant
	;;
	should_run)
		should_run_mtlk_init_wpa_supplicant
	;;
esac


