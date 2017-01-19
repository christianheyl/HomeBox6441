#!/bin/sh /etc/rc.common
#
# Install Voice CPE TAPI subsystem

START=30

KERNEL_VERSION=`uname -r`
KERNEL_MAJOR=`uname -r | cut -f1,2 -d. | sed -e 's/\.//'`

# check for Linux 2.6 or higher
if [ $KERNEL_MAJOR -ge 26 ]; then
	MODEXT=.ko
fi

bin_dir=/opt/lantiq/bin
drv_obj_file_name=drv_tapi$MODEXT

start() {
	[ -z `cat /proc/modules | grep drv_ifxos` ] && {
		echo "Ooops - IFXOS isn't loaded, TAPI will do it. Check your basefiles..."
		insmod /lib/modules/${KERNEL_VERSION}/drv_ifxos$MODEXT
	}
	# temporary check for loading the eventlogger driver
	[ -e ${bin_dir}/drv_event_logger$MODEXT ] &&
	[ -z `cat /proc/modules | grep event_logger` ] && {
		insmod ${bin_dir}/drv_event_logger$MODEXT
	}
	[ -e ${bin_dir}/${drv_obj_file_name} ] && {
		insmod ${bin_dir}/${drv_obj_file_name};
	}
}


case "$1" in
	"boot")		start &	;;
	"start")	start &	;;
#	"stop")		stop	;;
	*)
			echo $0 'boot  - init  Voice CPE TAPI subsystem'
			echo $0 'start - start Voice CPE TAPI subsystem'
#			echo $0 'stop  - stop  Voice CPE TAPI subsystem'
esac
