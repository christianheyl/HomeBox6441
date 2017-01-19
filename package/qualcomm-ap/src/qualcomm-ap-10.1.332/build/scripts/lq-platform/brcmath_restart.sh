#!/bin/sh
# Lock Semaphore Here
arc_osix_ctrl LockSemaphore WANWIFI



configure_5g() {
    APRESTART=0
    keyischanged=0
    isfirst=0 # first boot up after reseting to default
    WL1_IF=$1
    
    echo "5G Settings"  > /dev/console
    
    opt_value=`ccfg_cli get AP_SSID_2@wireless`
    if [ "`cfg -s | grep AP_SSID:= | awk 'BEGIN{FS="AP_SSID:="}{print $2}'`" != "$opt_value" ] ; then
        cfg -a AP_SSID=$opt_value
        iwconfig $WL1_IF essid $opt_value
        if [ "`ccfg_cli get enableSSN@8021x_1_0`" != 0 ];then
            let APRESTART=1
        fi
    fi
    
    opt_value=`ccfg_cli get 5G_AP_PRIMARY_CH@wireless`
    if [ "`cfg -s | grep AP_PRIMARY_CH:= | awk 'BEGIN{FS="AP_PRIMARY_CH:="}{print $2}'`" != "$opt_value" ] ; then
        cfg -a AP_PRIMARY_CH=$opt_value
        iwconfig $WL1_IF channel $opt_value
	let APRESTART=1
    fi

    cfg -a AP_STARTMODE=standard
    cfg -a AP_RADIO_ID=0
    cfg -a AP_CHMODE=11ACVHT80
    cfg -a AP_BRNAME=br-lan    

    # encryption
    encOpen=`ccfg_cli get enableOS@8021x_1_0`
    encWEP=`ccfg_cli get enableWEP@8021x_1_0`
    encWPA=`ccfg_cli get enableSSN@8021x_1_0`
    pre_encOpen=`ccfg_cli get pre_enableOS@8021x_1_0`
    pre_encWEP=`ccfg_cli get pre_enableWEP@8021x_1_0`
    pre_encWPA=`ccfg_cli get pre_enableSSN@8021x_1_0`
    
    if [ "$pre_encWPA" = "" ]; then
        let isfirst=1
    fi

    wpakey=`ccfg_cli get ssnpskASCIIkey@8021x_1_0`
    if [ "`cfg -s | grep PSK_KEY:= | awk 'BEGIN{FS="PSK_KEY:="}{print $2}'`" != "$wpakey" ] ; then
        let keyischanged=1  
    fi 

    if [ "$pre_encOpen" != "$encOpen" ] ; then
        if [ "$encOpen" = 1 ] ; then
            cfg -a AP_SECMODE=None
            let APRESTART=1
        fi
        ccfg_cli set pre_enableOS@8021x_1_0=$encOpen
    fi
    
    encWEPkey=`ccfg_cli get defaultkey@wep_1_0`
    encWEPkey1=`ccfg_cli get key1@wep_1_0`
    encWEPkey2=`ccfg_cli get key2@wep_1_0`
    encWEPkey3=`ccfg_cli get key3@wep_1_0`
    encWEPkey4=`ccfg_cli get key4@wep_1_0`
    if [ "$pre_encWEP" != "$encWEP" ] ; then   
        if [ "$encWEP" = 1 ] ; then
            cfg -a AP_SECMODE=None
            let APRESTART=1
        fi
        ccfg_cli get pre_enableWEP@8021x_1_0=$encWEP
    fi

    if [ "$pre_encWPA" != "$encWPA" -o "$keyischanged" = 1 ] ; then
        if [ "$encWPA" = 1 ] ; then  
            #WPA 
            cfg -a AP_SECMODE=WPA
            cfg -a AP_SECFILE=PSK
            cfg -a AP_WPA=1
            cfg -a AP_CYPHER=TKIP
            cfg -a PSK_KEY=$wpakey
            let APRESTART=1
        fi
        if [ "$encWPA" = 2 ] ; then   
            #WPA2
            cfg -a AP_SECMODE=WPA
            cfg -a AP_SECFILE=PSK
            cfg -a AP_WPA=2
            cfg -a AP_CYPHER=CCMP
            cfg -a PSK_KEY=$wpakey
            let APRESTART=1
        fi
        if [ "$encWPA" = 3 ] ; then   
            #WPA/WPA2
            cfg -a AP_SECMODE=WPA
            cfg -a AP_SECFILE=PSK
            cfg -a AP_WPA=3
            cfg -a AP_CYPHER=CCMP
            cfg -a PSK_KEY=$wpakey
            let APRESTART=1
        fi
        ccfg_cli set pre_enableSSN@8021x_1_0=$encWPA
    fi
    cfg -c 

    if [ "$APRESTART" = 1 -a "$isfirst" = 0 ] ; then
        echo "5G restarting ... " > /dev/console
        /etc/ath/apdown
        /etc/ath/apup
    fi	


    # Set Country
    # CTRY_UNITED_KINGDOM GB 826
    # CTRY_UNITED_STATES  US 840
    # wirte to boot up
    cur_country=` iwpriv wifi0 getCountryID | awk -F':' '{print $2}'`
    if [ cur_country != 826 ] ; then
        iwpriv wifi0 setCountryID 826
    fi

    opt_value=`ccfg_cli get hide_ssid_2@wireless`
    if [ "`iwpriv $WL1_IF get_hide_ssid | awk -F':' '{print $2}'`" != "$opt_value" ] ; then
            iwpriv $WL1_IF hide_ssid ${opt_value}
            if [ "$opt_value" = 1 ] ; then
                    iwpriv $WL1_IF wps 0
            elif [ "$opt_value" = 0 -a `ccfg_cli get enable_1_0@wps` = 1 ] ; then
                    iwpriv $WL1_IF wps 1
            fi
    fi


} 
#end configure_5g

if [ -z "`nvram get brcm_restart`" ] ; then
	nvram set brcm_restart=1
	ccfg_cli set WIRELESS_RUNNING@wireless=1

	enabled_24=`ccfg_cli get 2.4G_WIRELESS_ENABLE@wireless`
	enabled_5=`ccfg_cli get 5G_WIRELESS_ENABLE@wireless`
	enabled_wps_0=`ccfg_cli get enable_0_0@wps`
	enabled_wps_1=`ccfg_cli get enable_1_0@wps`
	wl0_if=`nvram get wl0_ifname`
	wl1_if="ath0"
	export wl0_if
	export wl1_if
	wireless_restart=0
	export wireless_restart
	export restart_24g=0
	export restart_5g=0

	wl0_isup=`wl -i $wl0_if isup`
	wl1_isup=`if [ -z "$( ifconfig $wlan_if | grep "UP" )" ] ; then echo "0"; else echo "1" ; fi`
	if [ "$wl0_isup" != "$enabled_24" ] ; then
		let wireless_restart=3
	fi
    if [ "$wl1_isup" != "$enabled_5" ] ; then
        let wireless_restart=4
    fi
	if [ "$wireless_restart" = 0 ] ; then
		if [ -n "$wl0_if" ] ; then
		. /usr/sbin/set1x.sh 0 0
		. /usr/sbin/brcm_1x.sh 0
		fi
		if [ -n "$wl1_if" ] ; then
            configure_5g $wl1_if
    		if [ "$wl1_isup" = 1 ] ; then
    				if [ -n `if [ -z "$( ppacmd getlan | grep "$wl1_if" )" ] ; then echo "0"; else echo "1" ; fi` ] ; then
    					ppacmd addlan -i $wl1_if
    					ppacmd control --enable-lan --enable-wan
    				fi
                    iwpriv $wl1_if ppa 1
    				ifconfig $wl1_if down
    				ifconfig $wl1_if up
    	    fi
    		if [ "$wl1_isup" = 0 ] ; then
    				if [ -z `if [ -z "$( ppacmd getlan | grep "$wl1_if" )" ] ; then echo "0"; else echo "1" ; fi` ] ; then
    					ppacmd dellan -i $wl1_if
    				fi
    				iwpriv $wl1_if ppa 0
    				ifconfig $wl1_if up
    				ifconfig $wl1_if down
    	    fi
		fi
		

		. /usr/sbin/wlan_acl_config.sh all all
	fi

	case $wireless_restart in
		1)	echo "WLAN restart" > /dev/console	
			killall wps_monitor
			killall nas
			killall eapd
			nvram set wps_proc_status=0
			nvram unset acs_ifnames
			ccfg_cli set WIRELESS_ENABLE@wireless=0

			if [ "$enabled_24" = 1 ] || [ "$enabled_5" = 1 ]  ; then
				/usr/sbin/arc_led.sh wlan on
			else
				/usr/sbin/arc_led.sh wlan off
			fi

			if [ -n "$wl0_if" ] && [ "$restart_24g" = 1 -o "$restart_5g" = 0 ] ; then
				# set FON initial state
				. /usr/sbin/brcm_fon.sh 0
				/sbin/wlconf $wl0_if down
				# Remove wlan0 from ppa and add back later
				if [ -n `if [ -z "$( ppacmd getlan | grep "$wl1_if" )" ] ; then echo "0"; else echo "1" ; fi` ] ; then
					ppacmd dellan -i $wl0_if
				fi
			fi
			if [ -n "$wl1_if" ] && [ "$restart_5g" = 1 -o "$restart_24g" = 0 ] ; then
				# set FON initial state
				. /usr/sbin/brcm_fon.sh 1
				ifconfig $wl1_if down
				# Remove wlan1 from ppa and add back later
				if [ -n `if [ -z "$( ppacmd getlan | grep "$wl1_if" )" ] ; then echo "0"; else echo "1" ; fi` ] ; then
					ppacmd dellan -i $wl1_if
				fi
				if [ "`iwpriv $wl1_if get_ppa | awk -F':' '{print $2}'`" = 1 ] ; then 
					iwpriv $wl1_if ppa 0
				fi
			fi

			if [ -n "$wl0_if" -a "$enabled_24" = 1 ] ; then
				if [ "$restart_24g" = 1 -o "$restart_5g" = 0 ] ; then
					if [ -z `brctl show | grep ${wl0_if} ` ] ; then
						brctl addif br-lan $wl0_if
					fi
					/sbin/wlconf $wl0_if up
					/sbin/wlconf $wl0_if start
					wl -i $wl0_if rxchain_pwrsave_pps 10
					wl -i $wl0_if rxchain_pwrsave_quiet_time 60
					wl -i $wl0_if rxchain_pwrsave_stas_assoc_check 1
					wl -i $wl0_if rxchain_pwrsave_enable 1
					wl -i $wl0_if ppa 1
					wl -i $wl0_if ampdu_rx_density 7
					ppacmd addlan -i $wl0_if
				fi
			else
				nvram set wl0_wps_mode=disabled
			fi

			if [ -n "$wl1_if" -a "$enabled_5" = 1 ] ; then
				if [ "$restart_5g" = 1 -o "$restart_24g" = 0 ] ; then
					if [ -z `brctl show | grep ${wl1_if} ` ] ; then
						brctl addif br-lan $wl1_if
					fi
					if [ -z `if [ -z "$( ppacmd getlan | grep "$wl1_if" )" ] ; then echo "0"; else echo "1" ; fi` ] ; then
						ppacmd addlan -i $wl1_if
						ppacmd control --enable-lan --enable-wan
					fi
					if [ "`iwpriv $wl1_if get_ppa | awk -F':' '{print $2}'`" = 0 ] ; then
						iwpriv $wl1_if ppa 1
					fi
					ifconfig $wl1_if down
					ifconfig $wl1_if up
				fi
			else
				nvram set wl1_wps_mode=disabled
			fi

			if [ "$enabled_24" = 1 ] || [ "$enabled_5" = 1 ]  ; then
				wl0_cs=`nvram get wl0_chanspec`
				wl1_cs=`nvram get wl1_chanspec`
				/sbin/acsd
				if [ "$wl0_cs" = 0 -o "$wl1_cs" = 0 ] && [ "$restart_24g" = 1 -o "$restart_5g" = 0 ] ; then
					if [ "$enabled_24" = 1 -a "$wl0_cs" = 0 ] ; then
						/sbin/acs_cli -i $wl0_if autochannel
					fi
					#if [ "$enabled_5" = 1 -a "$wl1_timer" = 1 -a "$wl1_cs" = 0 ] ; then
					#	/sbin/acs_cli -i $wl1_if autochannel
					#fi
				fi
				if [ "$enabled_5" = 1 ] && [ "$restart_5g" = 1 -o "$restart_24g" = 0 ] ; then
					wl -i $wl1_if down
					wl -i $wl1_if up
					wl1_chanspec=`wl -i $wl1_if chanspec | cut -d ' ' -f 1`
					wl1_channel=`echo $wl1_chanspec | cut -d '/' -f 1`
					if [ "$wl1_channel" = 116 ] || [ "$wl1_channel" = 120 ] || [ "$wl1_channel" = 124 ] || [ "$wl1_channel" = 128 ] || [ "$wl1_channel" = 140 ]; then
						wl -i $wl1_if down
						bw=`echo $wl1_chanspec | cut -d '/' -f 2`
						wl -i $wl1_if chanspec 36/$bw
						wl -i $wl1_if up
					fi
					wl -i $wl1_if ppa 1
				fi
				/sbin/eapd
				/sbin/nas
				if [ "$enabled_wps_0" = 1 -o "$enabled_wps_1" = 1 ] ; then
					if [ "`nvram get wl0_wps_mode`" = enabled  -o  "`nvram get wl1_wps_mode`" = enabled ] ; then
						/sbin/wps_monitor &
					fi
				fi
				killall acsd
				ccfg_cli set WIRELESS_ENABLE@wireless=1
			fi

			# update FON status
			/usr/sbin/fon_update_status.sh
		;;
		2)	echo "WLAN fast sync" > /dev/console
			killall wps_monitor
			killall nas
			killall eapd
			sleep 3
			if [ "$enabled_24" = 1 ] || [ "$enabled_5" = 1 ]  ; then
				/sbin/eapd
				/sbin/nas
				if [ "$enabled_wps_0" = 1 -o "$enabled_wps_1" = 1 ] ; then
					if [ "`nvram get wl0_wps_mode`" = enabled  -o  "`nvram get wl1_wps_mode`" = enabled ] ; then
						/sbin/wps_monitor &
					fi
				fi
			fi
		;;
		3)	echo "2.4G ${enabled_24}" > /dev/console
			# set FON initial state
			. /usr/sbin/brcm_fon.sh 0

			killall wps_monitor
			killall nas
			killall eapd
			nvram set wps_proc_status=0
			nvram unset acs_ifnames

			if [ -n "$wl0_if" -a $wl0_isup = 1 ] ; then
				/sbin/wlconf $wl0_if down
				ppacmd dellan -i $wl0_if
			fi

			ccfg_cli set WIRELESS_ENABLE@wireless=0

			if [ "$enabled_24" = 1 ] || [ "$enabled_5" = 1 ]  ; then
				/usr/sbin/arc_led.sh wlan on
				ccfg_cli set WIRELESS_ENABLE_2@wireless=1
			else
				/usr/sbin/arc_led.sh wlan off
				umng_syslog_cli addEventCode W007
				ccfg_cli set WIRELESS_ENABLE_2@wireless=0
			fi

			if [ -n "$wl0_if" -a "$enabled_24" = 1 ] ; then
				if [ -z `brctl show | grep ${wl0_if} ` ] ; then
					brctl addif br-lan $wl0_if
				fi
				. /usr/sbin/set1x.sh 0 0
				. /usr/sbin/brcm_1x.sh 0
				/sbin/wlconf $wl0_if up
				/sbin/wlconf $wl0_if start
				wl -i $wl0_if rxchain_pwrsave_pps 10
				wl -i $wl0_if rxchain_pwrsave_quiet_time 60
				wl -i $wl0_if rxchain_pwrsave_stas_assoc_check 1
				wl -i $wl0_if rxchain_pwrsave_enable 1
				wl -i $wl0_if ppa 1
				wl -i $wl0_if ampdu_rx_density 7
				ppacmd addlan -i $wl0_if
			else
				nvram set wl0_wps_mode=disabled
			fi

			if [ "$enabled_24" = 1 ] || [ "$enabled_5" = 1 ]  ; then
				if [ "$enabled_24" = 1 -a "$enabled_5" = 0 ] ; then
					umng_syslog_cli addEventCode W006
				fi
				wl0_cs=`nvram get wl0_chanspec`
				/sbin/acsd
				if [ "$enabled_24" = 1 -a "$wl0_cs" = 0 ] ; then
					/sbin/acs_cli -i $wl0_if autochannel
				fi
				/sbin/eapd
				/sbin/nas
				if [ "$enabled_wps_0" = 1 -o "$enabled_wps_1" = 1 ] ; then
					if [ "`nvram get wl0_wps_mode`" = enabled  -o  "`nvram get wl1_wps_mode`" = enabled ] ; then
						/sbin/wps_monitor &
					fi
				fi
				killall acsd
				ccfg_cli set WIRELESS_ENABLE@wireless=1
			fi

			# update FON status
			/usr/sbin/fon_update_status.sh
		;;
		4) echo "5G ${enabled_5}" > /dev/console
			# set FON initial state
			. /usr/sbin/brcm_fon.sh 1

			if [ -n "$wl1_if" -a $wl1_isup ] ; then
				ifconfig $wl1_if down
				if [ -n `if [ -z "$( ppacmd getlan | grep "$wl1_if" )" ] ; then echo "0"; else echo "1" ; fi` ] ; then
					ppacmd dellan -i $wl1_if
				fi
				iwpriv $wl1_if ppa 0
			fi

			ccfg_cli set WIRELESS_ENABLE@wireless=0

			if [ "$enabled_24" = 1 ] || [ "$enabled_5" = 1 ]  ; then
				/usr/sbin/arc_led.sh wlan on
				ccfg_cli set WIRELESS_ENABLE_2@wireless=1
			else
				/usr/sbin/arc_led.sh wlan off
				umng_syslog_cli addEventCode W007
				ccfg_cli set WIRELESS_ENABLE_2@wireless=0
			fi

			if [ -n "$wl1_if" -a "$enabled_5" = 1 ] ; then
				if [ -z `brctl show | grep ${wl1_if} ` ] ; then
					brctl addif br-lan $wl1_if
				fi
				if [ -z `if [ -z "$( ppacmd getlan | grep "$wl1_if" )" ] ; then echo "0"; else echo "1" ; fi` ] ; then
					ppacmd addlan -i $wl1_if
					ppacmd control --enable-lan --enable-wan
				fi
				if [ "`iwpriv $wl1_if get_ppa | awk -F':' '{print $2}'`" = 0 ] ; then
					iwpriv $wl1_if ppa 1
				fi
				ifconfig $wl1_if down
				ifconfig $wl1_if up
				configure_5g $wl1_if
        		if [ "$wl1_isup" = 1 ] ; then
        				if [ -n `if [ -z "$( ppacmd getlan | grep "$wl1_if" )" ] ; then echo "0"; else echo "1" ; fi` ] ; then
        					ppacmd addlan -i $wl1_if
        					ppacmd control --enable-lan --enable-wan
        				fi
        				iwpriv $wl1_if ppa 1
        				ifconfig $wl1_if down
        				ifconfig $wl1_if up
        	    fi
			else
				nvram set wl1_wps_mode=disabled
			fi

			if [ "$enabled_24" = 1 ] || [ "$enabled_5" = 1 ]  ; then
				if [ "$enabled_24" = 0 -a "$enabled_5" = 1 ] ; then
					umng_syslog_cli addEventCode W006
				fi
				wl1_cs=`nvram get wl1_chanspec`
				/sbin/acsd
				#if [ "$enabled_5" = 1 -a "$wl1_cs" = 0 ] ; then
				#	/sbin/acs_cli -i $wl1_if autochannel
				#fi
				if [ "$enabled_5" = 1 ] ; then
					wl -i $wl1_if down
					wl -i $wl1_if up
					wl1_chanspec=`wl -i $wl1_if chanspec | cut -d ' ' -f 1`
					wl1_channel=`echo $wl1_chanspec | cut -d '/' -f 1`
					if [ "$wl1_channel" = 116 ] || [ "$wl1_channel" = 120 ] || [ "$wl1_channel" = 124 ] || [ "$wl1_channel" = 128 ] || [ "$wl1_channel" = 140 ]; then
						wl -i $wl1_if down
						bw=`echo $wl1_chanspec | cut -d '/' -f 2`
						wl -i $wl1_if chanspec 36/$bw
						wl -i $wl1_if up
					fi
					wl -i $wl1_if ppa 1
				fi
				/sbin/eapd
				/sbin/nas
				if [ "$enabled_wps_0" = 1 -o "$enabled_wps_1" = 1 ] ; then
					if [ "`nvram get wl0_wps_mode`" = enabled  -o  "`nvram get wl1_wps_mode`" = enabled ] ; then
						/sbin/wps_monitor &
					fi
				fi
				killall acsd
				ccfg_cli set WIRELESS_ENABLE@wireless=1
			fi

			# update FON status
			/usr/sbin/fon_update_status.sh
		;;
	esac
	ccfg_cli set WIRELESS_RUNNING@wireless=0
	nvram unset brcm_restart
fi

# UnLock Semaphore Here
arc_osix_ctrl UnLockSemaphore WANWIFI
  

