#!/bin/sh /etc/rc.common
START=15
start() {
	if [ "$CONFIG_FEATURE_ATHEROS_WLAN_TYPE_USB" != "1" ]; then
		if [ "$CONFIG_FEATURE_IFX_WIRELESS" = "1" ]; then
      /etc/rc.d/wlan_discover
			if [ "$CONFIG_TARGET_LTQCPE_PLATFORM_AR10" = "1" ]; then
				/etc/rc.d/rc.bringup_wlan init
			fi
		fi
	fi
}

stop() {
	if [ "$CONFIG_FEATURE_ATHEROS_WLAN_TYPE_USB" != "1" ]; then
	    if [ "$CONFIG_FEATURE_IFX_WIRELESS" = "1" ]; then
			if [ "$CONFIG_TARGET_LTQCPE_PLATFORM_AR10" = "1" ]; then
				/etc/rc.d/rc.bringup_wlan uninit
			fi
		fi
	fi
}
