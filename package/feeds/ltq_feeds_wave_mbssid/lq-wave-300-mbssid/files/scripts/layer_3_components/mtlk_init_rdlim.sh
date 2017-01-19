#!/bin/sh
# This is the script define HW capability.
#SVN id: $Id: mtlk_init_rdlim.sh 2278 2008-02-21 15:40:01Z ediv $

if [ ! "$MAPLOADED" ]; then
	if [ -r /tmp/wave300_map_apIndex ]; then
		. /tmp/wave300_map_apIndex # 2>/dev/null
		MAPLOADED="1"
	fi
fi

if [ ! "$MTLK_INIT_PLATFORM" ]; then			
	print2log DBG "mtlk_init_platform called in mtlk_init_rdlim.sh"
	. /tmp/mtlk_init_platform.sh
	MTLK_INIT_PLATFORM="1"
fi
command=$1
apIndex=$2

start_mtlk_init_rdlim()
{
	print2log DBG "start mtlk_init_rdlim"
	
	if [ -e $CONFIGS_PATH/hw_$wlan.ini ]; then rm $CONFIGS_PATH/hw_$wlan.ini; fi
	if [ ! -e /tmp/ini.tcl ]; then ln -s $ETC_PATH/ini.tcl /tmp/ini.tcl; fi
	if [ ! -e /tmp/rdlim.ini ]; then ln -s $ETC_PATH/rdlim.ini /tmp/rdlim.ini; fi
	eval wlan='$'w300_map_idx_${apIndex}
	print2log DBG "mtlk_init_rdlim.sh: $ETC_PATH/rdlim.tcl $wlan"
	$ETC_PATH/rdlim.tcl $wlan
	
	if [ -e $CONFIGS_PATH/hw_wlan0.ini ] && [ ! -e /tmp/HW.ini ]
	then
		#Workaround: create link in the tmp untill fix drvhlpr and more for dual concard"
		ln -s $CONFIGS_PATH/hw_wlan0.ini  /tmp/HW.ini
	fi
	
	#config_save.sh
	
	print2log DBG "Finsh start mtlk_init_rdlim"
}

stop_mtlk_init_rdlim()
{
	return
}

create_config_mtlk_init_rdlim()
{
	return
}

should_run_mtlk_init_rdlim()
{
	#todo: improve by checking AP index dependency on wlan_count and CONFIG_FEATURE (as defined in config.sh)
	if [ $apIndex -gt 1 ]
	then
		false
	else
		true
	fi
}


case $command in
	start)
		start_mtlk_init_rdlim
	;;
	stop)
		stop_mtlk_init_rdlim
	;;
	create_config)
		create_config_mtlk_init_rdlim
	;;
	should_run)
		should_run_mtlk_init_rdlim
	;;
esac