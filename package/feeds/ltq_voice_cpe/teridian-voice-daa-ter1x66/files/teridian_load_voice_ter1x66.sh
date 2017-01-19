#!/bin/sh /etc/rc.common
#
# Install Teridian DAA driver with TAPI-like interface

START=35

# check for linux 2.6.x
uname -r | grep -q 2.6.
if [ $? -eq 0 ]; then
    MODEXT=.ko
fi

bin_dir=/opt/lantiq/bin
drv_dev_base_name=ter1x66
drv_obj_file_name=drv_ter1x66$MODEXT

start() {
	[ -e ${bin_dir}/${drv_obj_file_name} ] && {
		insmod ${bin_dir}/${drv_obj_file_name};
		#create device nodes for...
		ter_major=`grep 73m1966_TAPI /proc/devices |cut -d' ' -f1`
		ter_minors="10 11"
		for ter_minor in $ter_minors ; do \
		   test ! -e /dev/ter$ter_minor && mknod /dev/ter$ter_minor c $ter_major $ter_minor;
		done
	}
	[ `cat /proc/cpuinfo | grep system | cut -f 3 -d ' '` =  "Danube" ] && {
		# configure reset pin on Danube V3 reference board
		echo DECT_ISDN_FXS_RESET > /sys/class/leds/dect_isdn_fxs_reset/trigger
        #
		# --> disable reset via startup script
		# (DECT reset will be moved to a separte GPIO)
		# echo 0 > /sys/class/leds/dect_isdn_fxs_reset/brightness
		# sleep 1
		# echo 1 > /sys/class/leds/dect_isdn_fxs_reset/brightness
	}
}