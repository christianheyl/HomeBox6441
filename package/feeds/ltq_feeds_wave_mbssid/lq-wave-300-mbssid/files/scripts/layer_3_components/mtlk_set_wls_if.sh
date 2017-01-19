#!/bin/sh
# This is the script define HW capability.
#SVN id: $Id: mtlk_init_rdlim.sh 2278 2008-02-21 15:40:01Z ediv $

if [ ! "$MAPLOADED" ]; then
	if [ -r /tmp/wave300_map_apIndex ]; then
		. /tmp/wave300_map_apIndex 2>/dev/null
		MAPLOADED="1"
	fi
fi

if [ ! "$MTLK_INIT_PLATFORM" ]; then			
	. /tmp/mtlk_init_platform.sh
	MTLK_INIT_PLATFORM="1"
fi
print2log DBG "mtlk_set_wls_if.sh: args: $*"

if [ -r /etc/rc.conf ]; then
	. /etc/rc.conf 2> /dev/null
fi

command=$1
apIndex=$2

#get corresponding wlan network interface from mapping file
eval wlan='$'w300_map_idx_${apIndex}

start_set_wls_if()
{
	print2log DBG "start set_wls_if"

	ifconfig $wlan 0.0.0.0 down

	board_mac=`/usr/sbin/upgrade mac_get 0` # this is a hack - set and get mac address from lan web page also uses does same
	if [ -z "${board_mac}" ]; then
		board_mac=`/usr/sbin/status_oper GET dev_info mac_addr`
	fi
	print2log DBG "Board MAC = $board_mac"

	board_mac15=`echo $board_mac | cut -d ":" -f 1-5`
	board_mac6=`echo $board_mac | cut -d ":" -f 6`
	print2log DBG "Board MAC = $board_mac15:$board_mac6"

	board_mac61=`echo $board_mac6 | cut -c1`
	if [ $board_mac61 = "a" -o $board_mac61 = "A" ]; then
	board_mac61=10
	elif [ $board_mac61 = "b" -o $board_mac61 = "B" ]; then
	board_mac61=11
	elif [ $board_mac61 = "c" -o $board_mac61 = "C" ]; then
	board_mac61=12
	elif [ $board_mac61 = "d" -o $board_mac61 = "D" ]; then
	board_mac61=13
	elif [ $board_mac61 = "e" -o $board_mac61 = "E" ]; then
	board_mac61=14
	elif [ $board_mac61 = "f" -o $board_mac61 = "F" ]; then
	board_mac61=15
	fi

	board_mac62=`echo $board_mac6 | cut -c2`
	if [ $board_mac62 = "a" -o $board_mac62 = "A" ]; then
	board_mac62=10
	elif [ $board_mac62 = "b" -o $board_mac62 = "B" ]; then
	board_mac62=11
	elif [ $board_mac62 = "c" -o $board_mac62 = "C" ]; then
	board_mac62=12
	elif [ $board_mac62 = "d" -o $board_mac62 = "D" ]; then
	board_mac62=13
	elif [ $board_mac62 = "e" -o $board_mac62 = "E" ]; then
	board_mac62=14
	elif [ $board_mac62 = "f" -o $board_mac62 = "F" ]; then
	board_mac62=15
	fi
	board_mac62=`expr $board_mac62 + 3 + $apIndex`
	print2log DBG "new value = $board_mac62"

	if [ $board_mac62 -gt 15 ]; then
		board_mac61=`expr $board_mac61 + 1`
		board_mac62=`expr $board_mac62 % 16`
	fi

	if [ $board_mac61 -eq 10 ]; then
		board_mac61=A
	elif [ $board_mac61 -eq 11 ]; then
		board_mac61=B
	elif [ $board_mac61 -eq 12 ]; then
		board_mac61=C
	elif [ $board_mac61 -eq 13 ]; then
		board_mac61=D
	elif [ $board_mac61 -eq 14 ]; then
		board_mac61=E
	elif [ $board_mac61 -eq 15 ]; then
		board_mac61=F
	fi

	if [ $board_mac62 -eq 10 ]; then
		board_mac62=A
	elif [ $board_mac62 -eq 11 ]; then
		board_mac62=B
	elif [ $board_mac62 -eq 12 ]; then
		board_mac62=C
	elif [ $board_mac62 -eq 13 ]; then
		board_mac62=D
	elif [ $board_mac62 -eq 14 ]; then
		board_mac62=E
	elif [ $board_mac62 -eq 15 ]; then
		board_mac62=F
	fi
	vap_mac="$board_mac15:$board_mac61$board_mac62"
	print2log DBG "New VAP MAC = $vap_mac"

	/sbin/ifconfig $wlan hw ether $vap_mac

	brctl addif br0 $wlan
	NETWORK_TYPE=`host_api get $$ $apIndex network_type`
	if [ "$NETWORK_TYPE" = "$STA" ]; then ifconfig wlan0 up; fi	
	
	# support PPA. PPA is supported on 2 physical interfaces
	wlan_count=`host_api get $$ sys wlan_count`
	PPAenabled0=`host_api get $$ 0 PPAenabled`
	if [ $wlan_count -gt 1 ] 
	then
		PPAenabled1=`host_api get $$ 1 PPAenabled`		
	fi
	
	print2log DBG "wanphy_phymode = $wanphy_phymode" 
	print2log DBG "CONFIG_IFX_CONFIG_CPU = $CONFIG_IFX_CONFIG_CPU" 
	
	cat /proc/ppa/api/hook | grep "not init" >/dev/null
	if [ "$?" = "1" ]
	then
		if [ "$PPAenabled0" = "1" ]
		then
			print2log DBG "iwpriv wlan0 sIpxPpaEnabled 1"
			iwpriv wlan0 sIpxPpaEnabled 1
			ppacmd addlan -i wlan0
		fi
		if [ "$PPAenabled1" = "1" ]
		then
			print2log DBG "iwpriv wlan1 sIpxPpaEnabled 1"
			iwpriv wlan1 sIpxPpaEnabled 1
			ppacmd addlan -i wlan1
		fi	
	fi	

	print2log DBG "Finsh start set_wls_if"
}

stop_set_wls_if()
{
	ppacmd dellan -i $wlan
	brctl delif br0 $wlan
	ifconfig $wlan down

	return
}

create_config_set_wls_if()
{
	return
}

should_run_set_wls_if()
{
	true	
}


case $command in
	start)
		start_set_wls_if
	;;
	stop)
		stop_set_wls_if
	;;
	create_config)
		create_config_set_wls_if
	;;
	should_run)
		should_run_set_wls_if
	;;
esac
