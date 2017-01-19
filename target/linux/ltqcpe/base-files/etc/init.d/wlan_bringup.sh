#!/bin/sh /etc/rc.common
START=45

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

start() {
	if [ "$CONFIG_FEATURE_ATHEROS_WLAN_TYPE_USB" != "1" ]; then
		if [ "$CONFIG_FEATURE_IFX_WIRELESS" = "1" ]; then
			if [ "$CONFIG_FEATURE_IFX_WIRELESS_WAVE300" = "1" ]; then
				if [ ! "$MTLK_INIT_PLATFORM" ]; then
					. /etc/rc.d/mtlk_init_platform.sh
				fi
			fi
			if [ "$CONFIG_TARGET_LTQCPE_PLATFORM_AR10" != "1" ]; then
				/etc/rc.d/rc.bringup_wlan init
			fi
			/etc/rc.d/rc.bringup_wlan start
			if [ "$CONFIG_FEATURE_IFX_WIRELESS_WAVE300" = "1" ]; then
				if [ ! -e $wave_init_failure ] && [ ! -e $wave_start_failure ]; then
					echo "S45: wlan bringup success"  >> $wave_init_success
				fi
				if [ -e $wave_start_failure ] && [ `cat $wave_start_failure | grep -v "non-existing physical wlan1" | grep -v "wlan1 failed:" -c` -eq 0 ]; then
					echo "S45: wlan bringup success"  >> $wave_init_success
					echo "S45: Platform is dual concurrent, but wlan1 isn't connected" >> $wave_init_success
				fi

			fi
		fi
	fi
	if  [ "$CONFIG_FEATURE_RIP" = "1" ]; then
		if ps ax | grep -v grep | grep ripd  > /dev/null
		then
			echo "ripd service running"                                 
		else
			if [ "$route_dynamic_fEnable" = "1" ]; then
				if [ -r /etc/ripd.conf ]; then
					echo "hostname $hostname" > /etc/zebra.conf
                                        . /etc/rc.d/init.d/ripd start
				fi
			fi
		fi
	fi

}

stop() {
	if [ "$CONFIG_FEATURE_ATHEROS_WLAN_TYPE_USB" != "1" ]; then
		if [ "$CONFIG_FEATURE_IFX_WIRELESS" = "1" ]; then
			/etc/rc.d/rc.bringup_wlan stop
			if [ "$CONFIG_TARGET_LTQCPE_PLATFORM_AR10" != "1" ]; then
				/etc/rc.d/rc.bringup_wlan uninit
			fi
		fi
	fi
}

