#!/bin/sh

#This script changes debug level for scripting.
log_level=$1
log_path=$2

if [ "$log_level" = "" ]
then
	log_level=3
fi

if [ "$log_path" = "" ]
then
	log_path="/dev/console"
fi

# Change print2log debug level:
rm /tmp/mtlk_init_platform.sh
grep -v "\(log_level\|log_path\)" /etc/rc.d/mtlk_init_platform.sh > /tmp/mtlk_init_platform.sh
echo log_level=$log_level >> /tmp/mtlk_init_platform.sh
echo log_path=$log_path >> /tmp/mtlk_init_platform.sh

