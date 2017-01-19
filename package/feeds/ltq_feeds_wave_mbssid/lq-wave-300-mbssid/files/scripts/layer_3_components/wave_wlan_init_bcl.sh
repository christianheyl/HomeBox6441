#!/bin/sh

# This script starts the BCL server, for wireless debug and diagnostics


#Defines
if [ ! "MTLK_INIT_PLATFORM" ]; then			
	. /tmp/mtlk_init_platform.sh
	MTLK_INIT_PLATFORM="1"
fi
command=$1

# BCL contains a built in tftp server, so if inetd is running and contains tftpd, then kill it and restart without that service, before starting bcl
restart_inetd()
{
    if [ `ps | grep -c inetd` -gt 1 ] && [ `grep -c tftpd /etc/inetd.conf` -gt 0 ]
    then
        grep -v tftpd /etc/inetd.conf > /tmp/inetd.conf
        killall inetd
        /usr/sbin/inetd /tmp/inetd.conf
    fi
}

start_init_bcl()
{
	restart_inetd
	BclSockServer &
}

stop_init_bcl()
{
	killall BclSockServer
}

create_config_init_bcl()
{
	return
}

# Never start BCL on init - this is a security hole. Only start manually from the console
should_run_init_bcl()
{
	false
}

case $command in
	start)
		start_init_bcl
	;;
	stop)
		stop_init_bcl
	;;
	create_config)
		create_config_init_bcl
	;;
	should_run)
		should_run_init_bcl
	;;
esac
