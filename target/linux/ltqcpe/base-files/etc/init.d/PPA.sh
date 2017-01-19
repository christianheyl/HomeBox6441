#!/bin/sh /etc/rc.common
#START=17
OUT_FILE=/dev/console

if [ ! "$ENVLOADED" ]; then
	if [ -r /etc/rc.conf ]; then
		. /etc/rc.conf 2> /dev/null
		ENVLOADED="1"
	fi
fi
if [ ! "$CONFIGLOADED" ]; then
	if [ -r /etc/rc.d/config.sh ]; then
		. /etc/rc.d/config.sh 2>/dev/null
		CONFIGLOADED="1"
	fi
fi

# $1~$* - wan interfaces
add_ppa_netdev()
{
	arc_lan_if="`mng_cli get ARC_LAN_0_BridgedIfname`"
	arc_br_if="`mng_cli get ARC_LAN_0_Ifname`"
	arc_wlan_if="`mng_cli get ARC_WLAN_24G_SSID_0_Interface`"

	if [ "$arc_lan_if" != "" ]; then
		ifconfig $arc_lan_if down
	#	ifconfig eth0 hw ether `getmacaddr.sh lan 0`
		ifconfig $arc_lan_if up
		ppacmd addlan -i $arc_lan_if    >& $OUT_FILE
	fi

	if [ "$arc_br_if" != "" ]; then
		ifconfig $arc_br_if up
		ppacmd addlan -i $arc_br_if  >& $OUT_FILE
		echo update $arc_br_if > /proc/ppa/api/netif
	fi

	if [ "$arc_wlan_if" != "" ]; then
		#ifconfig $arc_wlan_if up
		ppacmd addlan -i $arc_wlan_if  >& $OUT_FILE
		echo update $arc_wlan_if > /proc/ppa/api/netif
	fi

	if [ -z "$1" ] ; then
		return
	fi

#	ethaddr=`getmacaddr.sh wan 0`
	while [ -n "$1" ] ; do
		ifconfig $1 down
#		ifconfig $1 hw ether $ethaddr
		ifconfig $1 up
		ppacmd addwan -i $1  >& $OUT_FILE
		echo update $1 > /proc/ppa/api/netif
		shift
#		ethaddr=`echo $ethaddr | next_macaddr 0`
	done
}

del_ppa_netdev()
{

	ALLNETIF=`ppacmd getlan 2> /dev/null | awk '{ print $3 }'` # VR9
	for NETIF in $ALLNETIF ; do
		ppacmd dellan -i $NETIF  >& $OUT_FILE
	done

	ALLNETIF=`ppacmd getwan 2> /dev/null | awk '{ print $3 }'` # VR9
	for NETIF in $ALLNETIF ; do
		ppacmd delwan -i $NETIF  >& $OUT_FILE
	done
}

boot() {
if [ "1$CONFIG_FEATURE_PPA_SUPPORT" = "11" ]; then
	echo "PPA.sh boot" > /dev/console
	mknod /dev/ifx_ppa c 181 0	
	/sbin/ppacmd init -n 30 2> /dev/null
	# enable wan / lan ingress
	/sbin/ppacmd control --enable-lan --enable-wan 2> /dev/null

	# set WAN vlan range 3 to 4095		
	/sbin/ppacmd addvlanrange -s 3 -e 0xfff 2> /dev/null

	# In PPE firmware is A4 or D4 and ppe is loaded as module then reinitialize TURBO MII mode since it is reset during module load.
	if [ "1$CONFIG_PACKAGE_KMOD_LTQCPE_PPA_A4_MOD" = "11" ]; then
	       	echo w 0xbe191808 0000096E > /proc/eth/mem
	fi
	
	if [ "1$CONFIG_PACKAGE_KMOD_LTQCPE_PPA_D4_MOD" = "11" ]; then
	       	echo w 0xbe191808 000003F6 > /proc/eth/mem
	fi

	#For PPA, the ack/sync/fin packet go through the MIPS and normal data packet go through PP32 firmware.
	#The order of packets could be broken due to different processing time.
	#The flag nf_conntrack_tcp_be_liberal gives less restriction on packets and 
	# if the packet is in window it's accepted, do not care about the order

	echo 1 > /proc/sys/net/netfilter/nf_conntrack_tcp_be_liberal
	#if PPA is enabled, enable hardware based QoS to be used later
	#/usr/sbin/status_oper SET "IPQoS_Config" "ppe_ipqos" "1"
		
	echo enable > /proc/ppa/api/hook # after this line, PPA hook adds itself default interfaces
	sleep 1
	ppacmd control --disable-lan --disable-wan  >& $OUT_FILE
	del_ppa_netdev # remove PPA hook default interfaces, e.g. br0 and eth0.5
	ppacmd control --enable-lan --enable-wan  >& $OUT_FILE

# anne@20140806: add_ppa_netdev without arg will only add lan interface. Here we only want to add lan interface.
# Move adding wan interfaces into arc_wan. This is because only there we know exactly what name will be for WAN interfaces.
# in arc_wan or arc_and, only ifname will added. sometimes, it's for with vlan only. So, we hard-code to add interface here.
	arc_wan_type="`arc_cfg_cli get ARC_WAN_Type`"
	if [ "$arc_wan_type" = "0" ]; then #AD
		#arc_wan_ifname="`ccfg_cli get ARC_WAN_000_BASE_Ifname`"
		#arc_wan_ifname_vlan="`ccfg_cli get ARC_WAN_000_VLAN_Id`"
		arc_wan_ifname="nas0"
	elif [ "$arc_wan_type" = "1" ]; then #etherwan
		#arc_wan_ifname="`ccfg_cli get ARC_WAN_100_BASE_Ifname`"
		#arc_wan_ifname_vlan="`ccfg_cli get ARC_WAN_100_VLAN_Id`"
		arc_wan_ifname="eth1"
	elif [ "$arc_wan_type" = "2" ]; then #VD
		#arc_wan_ifname="`ccfg_cli get ARC_WAN_200_BASE_Ifname`"
		#arc_wan_ifname_vlan="`ccfg_cli get ARC_WAN_200_VLAN_Id`"
		arc_wan_ifname="ptm0"
	else
		echo "PPA.sh lte?" > /dev/console
		#arc_wan_ifname="`ccfg_cli get ARC_WAN_100_BASE_Ifname`"
		#arc_wan_ifname_vlan="`ccfg_cli get ARC_WAN_100_VLAN_Id`"
		arc_wan_ifname="eth1"
      	fi

       	add_ppa_netdev $arc_wan_ifname

#	if [ "$arc_wan_ifname_vlan" = "0" ]; then
#        	add_ppa_netdev $arc_wan_ifname
#	else
#      	       	add_ppa_netdev $arc_wan_ifname $arc_wan_ifname.$arc_wan_ifname_vlan
#	fi
fi
echo "@@ ppasessmgmt -i 30 -m 1" > /dev/console
ppasessmgmt -i 30 -m 1
}

start() {
if [ "1$CONFIG_FEATURE_PPA_SUPPORT" = "11" ]; then
	echo "PPA.sh start " > /dev/console
	/sbin/ppacmd init -n 30 2> /dev/null
	# enable wan / lan ingress
	/sbin/ppacmd control --enable-lan --enable-wan 2> /dev/null

	# set WAN vlan range 3 to 4095		
	/sbin/ppacmd addvlanrange -s 3 -e 0xfff 2> /dev/null

	# In PPE firmware is A4 or D4 and ppe is loaded as module then reinitialize TURBO MII mode since it is reset during module load.
	if [ "1$CONFIG_PACKAGE_KMOD_LTQCPE_PPA_A4_MOD" = "11" ]; then
        	echo w 0xbe191808 0000096E > /proc/eth/mem
	fi
	
    	if [ "1$CONFIG_PACKAGE_KMOD_LTQCPE_PPA_D4_MOD" = "11" ]; then
        	echo w 0xbe191808 000003F6 > /proc/eth/mem
	fi

	#For PPA, the ack/sync/fin packet go through the MIPS and normal data packet go through PP32 firmware.
	#The order of packets could be broken due to different processing time.
	#The flag nf_conntrack_tcp_be_liberal gives less restriction on packets and 
	# if the packet is in window it's accepted, do not care about the order

	echo 1 > /proc/sys/net/netfilter/nf_conntrack_tcp_be_liberal
	#if PPA is enabled, enable hardware based QoS to be used later
	#/usr/sbin/status_oper SET "IPQoS_Config" "ppe_ipqos" "1"
		
	echo enable > /proc/ppa/api/hook # after this line, PPA hook adds itself default interfaces
		
	sleep 1
		
	ppacmd control --disable-lan --disable-wan  >& $OUT_FILE
		
	del_ppa_netdev # remove PPA hook default interfaces, e.g. br0 and eth0.5
		
	ppacmd control --enable-lan --enable-wan  >& $OUT_FILE
		
# anne@20140806: add_ppa_netdev without arg will only add lan interface. Here we only want to add lan interfaces.
# Move adding wan interfaces into arc_wan. This is because only there we know exactly what name will be for WAN interfaces.
# in arc_wan or arc_and, only ifname will added. sometimes, it's for with vlan only. So, we hard-code to add interface here.
	arc_wan_ifname=""
#	arc_wan_ifname_vlan="0"
	if [ -r /proc/eth/vrx318/ver ]; then
		wanphy_tc="`mng_cli get wanphy_tc@wan_phy_cfg`"
		if [ $wanphy_tc == "0" ]; then
			#AD
			arc_wan_ifname="nas0"
#			arc_wan_ifname="`ccfg_cli get ARC_WAN_000_BASE_Ifname`"
#	                arc_wan_ifname_vlan="`ccfg_cli get ARC_WAN_000_VLAN_Id`"
		elif [ $wanphy_tc == "1" ]; then
			#VD
			arc_wan_ifname="ptm0"
#			arc_wan_ifname="`ccfg_cli get ARC_WAN_200_BASE_Ifname`"
#	                arc_wan_ifname_vlan="`ccfg_cli get ARC_WAN_200_VLAN_Id`"
		else
			echo "PPA.sh error: wanphy_tc can't be identified!!" > /dev/console
		fi
	else
		#etherwan
		arc_wan_ifname="eth1"
#		arc_wan_ifname="`ccfg_cli get ARC_WAN_100_BASE_Ifname`"
#	        arc_wan_ifname_vlan="`ccfg_cli get ARC_WAN_100_VLAN_Id`"
	fi

      	add_ppa_netdev $arc_wan_ifname

#       if [ "$arc_wan_ifname_vlan" = "0" ]; then
#              	add_ppa_netdev $arc_wan_ifname
#       else
#      	        add_ppa_netdev $arc_wan_ifname $arc_wan_ifname.$arc_wan_ifname_vlan
#       fi
fi
echo "@@ ppasessmgmt -i 30 -m 1" > /dev/console
ppasessmgmt -i 30 -m 1
}

stop() {
if [ "1$CONFIG_FEATURE_PPA_SUPPORT" = "11" ]; then
	echo "PPA.sh stop" > /dev/console
	/sbin/ppacmd exit 2> /dev/null

	/sbin/ppacmd control --disable-lan --disable-wan 2> /dev/null

	# reset to defaul WAN vlan range 0 to 16		
	/sbin/ppacmd addvlanrange -s 0 -e 10 2> /dev/null

	#if PPA is stopped, disable hardware based QoS to be used later
	#/usr/sbin/status_oper SET "IPQoS_Config" "ppe_ipqos" "0"
fi
}
