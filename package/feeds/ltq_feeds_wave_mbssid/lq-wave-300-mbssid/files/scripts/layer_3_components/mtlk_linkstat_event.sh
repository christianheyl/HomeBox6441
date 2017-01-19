#!/bin/sh

# Defines
. /tmp/mtlk_init_platform.sh


if [ -e /tmp/wls_link_stat ]
then
	LAST_STATUS=`awk -F "=" '/^WLSLinksStatus/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' /tmp/wls_link_stat`
else
	LAST_STATUS=0
fi

NETWORK_TYPE=`host_api get $$ 0 network_type`
STATIC_IP=`host_api get $$ sys ip_config_method`


# Set "connected to AP" param in web
# To prevent race conditions when changing param:
# Get lock, use web HUP mechanism, and wait for web to complete before releasing lock
set_web_connected ()
{
	config_lock.sh $$ /tmp/config_lock 7
	if [  $? -eq 0 ]
	then
		echo "setting connectedToAP web param to $1" > /dev/console
		echo "connectedToAP=$1" > /tmp/updates.ini
		$ETC_PATH/mtlk_init_webs.sh should_run && $ETC_PATH/mtlk_init_webs.sh reload

		update_timeout=0
		while [ -e /tmp/updates.ini ] 
		do
			sleep 1
			update_timeout=`expr $update_timeout + 1`
			if [ $update_timeout -gt 10 ]
			then
				break
			fi
		done

		config_unlock.sh $$ /tmp/config_lock
	fi
}

# Reset the bridge to restore all interfaces to forwarding state,
# in case eth0 was blocked by loop breaking.
# Also ping all known IPs, to make them learn the new path to device.
bridge_reset () 
{
	OLD_IPS=`cat /proc/net/arp | awk '/:/ {print $1}'`
	ifconfig br0 down
	ifconfig br0 up
	
	PING_ARGS=""
	# this arg should be added to a non-star platforms
	# If we use the ping command without arguments on dongle/UGW, then it loops forever.
	# In star, it only pings a few times.

	if [ `hostname` != "Dorango" ] 
	then
		PING_ARGS="-c 1"
	fi
	for IP in $OLD_IPS; do ping $PING_ARGS $IP; done
}

if [ "$1" = "w1" ]
then
	echo mtlk_linkstat_event.sh:Wireless Link is UP >/dev/console
	echo "WLSLinksStatus = 1" > /tmp/wls_link_stat
	
	if [ "$LAST_STATUS" = "1" ] 
	then
		echo mtlk_linkstat_event.sh:Already up >/dev/console					
	else
		if [ "$NETWORK_TYPE" = "$STA" ]
		then
			if [ "$STATIC_IP" = "$DHCPC" ]
			then
				# Force our device to renew its IP
				echo mtlk_linkstat_event.sh:DHCP renew >/dev/console
				$ETC_PATH/mtlk_dhcp_client.sh should_run && $ETC_PATH/mtlk_dhcp_client.sh reload
			fi
			
			# Force devices connected to the eth port to renew their IP, by resetting the eth PHY
			# Currently this feature is disabled, because it is only implemented in the kernel for ICP101 phy (Metalink GPB237 board)
			# and it may reset the phy too fast for PCs to notice this.
			## if [ -e /proc/str9100/phy_reset ]
			## then
			## 	echo 1 > /proc/str9100/phy_reset
			## fi
		fi
	fi
	if [ "$NETWORK_TYPE" = "$STA" ]
	then
		set_web_connected 1
	fi
	exit
fi

if [ "$1" = "w0" ]
then
	echo mtlk_linkstat_event.sh:Wireless Link is DOWN >/dev/console
	echo "WLSLinksStatus = 0" > /tmp/wls_link_stat
	
	if [ "$LAST_STATUS" = "0" ] 
	then
		echo mtlk_linkstat_event.sh:Already down >/dev/console
	else
		# Bring the bridge down and up, to restore all interfaces to forwarding state,
		# in case eth0 was blocked by loop breaking 
		bridge_reset
	fi
	if [ "$NETWORK_TYPE" = "$STA" ]
	then
		set_web_connected 0
	fi
	exit
fi

echo mtlk_linkstat_event.sh:Unknown event $1 >/dev/console


exit


