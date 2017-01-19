#!/bin/sh /etc/rc.common
START=20
STOP=90
ENABLE_DEBUG_OUTPUT=0

bindir=/opt/lantiq/bin
# install event logger

drv_obj_file_name=drv_event_logger

LEFTSHIFT='<<'
#
# Convert an IP address in dot quad format to an integer
#
decodeaddr() {
    local x
    local temp=0
    local ifs=$IFS
    IFS=.
    for x in $1; do
		temp=$(( $(( $temp $LEFTSHIFT 8 )) | $x ))
    done
    echo $temp
    IFS=$ifs
}

#
# take the server ip for syslog
#
setsyslogip() {
    bootargs=`cat /proc/cmdline`
    for items in $bootargs; do
      key=${items%%=*}
      if [ "${key}" == "ip" ]; then
         value=${items##*=}
         value=${value#*:}
         SYSLOGIP=${value%%:*}
         export SYSLOGIP
         break
      fi
   done
}

start() {
	cd ${bindir}
	[ -e ${bindir}/inst_driver.sh ] && {
      ${bindir}/inst_driver.sh $ENABLE_DEBUG_OUTPUT $drv_obj_file_name $drv_obj_file_name
	   mknod /dev/evlog-misc c 10 100;
   }
	setsyslogip
	# overwrite syslog if needed
	#export SYSLOGIP="10.1.13.1"
	[ -e /proc/driver/el/info ] && {
		echo "$SYSLOGIP" > /proc/driver/el/cfg/ipaddr
		echo "1" > /proc/driver/el/cfg/syslog
		echo "*"  > /proc/driver/el/logs/all
		echo "-"  > /proc/driver/el/logs/reg_rd
		echo "-"  > /proc/driver/el/logs/reg_wr
	}
}

stop() {
   uname -r | grep -q "2.6." && MODEXT=.ko
   rmmod $drv_obj_file_name$MODEXT
}
