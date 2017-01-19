#!/bin/sh

# This script read DFS channels disabled from EECaps. if DFS channels disabled = 1 don't show dfs channels in web
#SVN id: $Id: mtlk_init_dfs.sh 2278 2008-02-21 15:40:01Z ediv $

#Defines
if [ ! "MTLK_INIT_PLATFORM" ]; then			
	. /tmp/mtlk_init_platform.sh
	MTLK_INIT_PLATFORM="1"
fi
command=$1


start_mtlk_init_dfs()
{
	print2log DBG "start mtlk_init_dfs"
	
	IS_DFS_ENABLE=`$ETC_PATH/mtpriv wlan0 EECaps | sed -n '/DFS channels disabled:/{s/DFS channels disabled:.*\([0-1]\)/\1/p}'`

	if [ $IS_DFS_ENABLE ]
	then
		host_api set $$ wlan0 is_dfs_enable $IS_DFS_ENABLE
		host_api set $$ wlan1 is_dfs_enable $IS_DFS_ENABLE
		#host_api commit $$
		#config_save.sh
	fi

	print2log DBG "Finish start mtlk_init_dfs"	
}

stop_mtlk_init_dfs()
{
	return
}

create_config_mtlk_init_dfs()
{
	return
}

should_run_mtlk_init_dfs()
{
	true
}

case $command in
	start)
		start_mtlk_init_dfs
	;;
	stop)
		stop_mtlk_init_dfs
	;;
	create_config)
		create_config_mtlk_init_dfs
	;;
	should_run)
		should_run_mtlk_init_dfs
	;;
esac
