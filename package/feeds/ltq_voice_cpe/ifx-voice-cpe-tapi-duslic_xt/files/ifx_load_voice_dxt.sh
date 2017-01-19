#!/bin/sh /etc/rc.common
#
# Install Voice CPE TAPI subsystem LL driver for DUSLIC-xT

START=32

KERNEL_MAJOR=`uname -r | cut -f1,2 -d. | sed -e 's/\.//'`

# check for Linux 2.6 or higher
if [ $KERNEL_MAJOR -ge 26 ]; then
	MODEXT=.ko
fi

bin_dir=/opt/lantiq/bin
drv_dev_base_name=dxt
drv_obj_file_name=drv_dxt$MODEXT

start() {
	[ -e ${bin_dir}/${drv_obj_file_name} ] && {
		insmod ${bin_dir}/${drv_obj_file_name};
		#create device nodes for...
		dxt_major=`grep $drv_dev_base_name /proc/devices |cut -d' ' -f1`
		dxt_minors="10 11 12"
		for dxt_minor in $dxt_minors ; do \
		   [ ! -e /dev/dxt$dxt_minor ] && mknod /dev/dxt$dxt_minor c $dxt_major $dxt_minor;
		done
	}
}
