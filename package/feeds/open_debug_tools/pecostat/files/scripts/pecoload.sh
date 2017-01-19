#!/bin/sh
# Comment these lines if pecostat is already in root filesystem and in path
# busybox tftp must be present on target, and pecostat.* must be in tftp server
# home dir. 'serverip' must be exported to right IP
# source this script, not exec from prompt
	cd /tmp
	if [ -z "$SERVERIP" ]
	then
		export SERVERIP=192.168.1.2
	fi
	tftp -g -r pecostat $SERVERIP
	tftp -g -r pecostat.ko $SERVERIP
	tftp -g -r peco.sh $SERVERIP
	insmod pecostat.ko
	chmod a+x pecostat
   chmod a+x peco*.sh
	# Now we are done
	export PATH=$PATH:.
