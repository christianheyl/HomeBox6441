#!/bin/sh

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


case "$1" in
	start)
		shift
       if [ "$CONFIG_PACKAGE_DOT1AG_UTILS" = "1" ]; then
          eval wanConnName='$'default_wan_conn_connName
          eval wanDefIf='$'default_wan_conn_iface

          # Get wan index and wan type
          wan_idx=${wanConnName##WANIP}
          if [ ! -z $wan_idx ] && [ -z "${wan_idx//[0-9]/}" ]; then
             wan_type="ip"  
          else
             wan_idx=${wanConnName##WANPPP}
             wan_type="ppp"
          fi

          echo "wan type: $wan_type  index: $wan_idx"
          #WAN_CONN_TAG="Wan${wan_type}${wan_idx}_IF_Info"
          WAN_CONN_TAG="wan_${wan_type}"
          WAN_L2IF_PARAM="wan${wan_type}_${wan_idx}_l2ifName"
          l2if="`/usr/sbin/status_oper -f /flash/rc.conf GET "$WAN_CONN_TAG" "$WAN_L2IF_PARAM" `"
          l2if_base=`echo $l2if | cut -d \. -f 1`
          echo "l2if $l2if l2if_base $l2if_base"
          insmod /lib/modules/2.6.32.42/ltq_eth_oam_handler.ko ethwan="$l2if_base" 
          ifconfig eoam up 
          dot1agd -i eoam &
       fi
		;;
	stop)
		shift
       if [ "$CONFIG_PACKAGE_DOT1AG_UTILS" = "1" ]; then
          ifconfig eoam down
          rmmod ltq_eth_oam_handler
       fi
		;;
	restart)
		shift
		;;
	*)
		echo $"Usage $0 {start|stop|restart}"
		#exit 1
esac
