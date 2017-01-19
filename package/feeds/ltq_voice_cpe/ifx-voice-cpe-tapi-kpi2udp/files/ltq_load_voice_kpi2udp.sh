#!/bin/sh /etc/rc.common
#
# Install Voice CPE TAPI subsystem LL driver for VMMC

START=35

KERNEL_MAJOR=`uname -r | cut -f1,2 -d. | sed -e 's/\.//'`

# check for Linux 2.6 or higher
if [ $KERNEL_MAJOR -ge 26 ]; then
	MODEXT=.ko
fi

bin_dir=/opt/lantiq/bin
drv_obj_file_name=drv_kpi2udp$MODEXT

start() {
	[ -e ${bin_dir}/${drv_obj_file_name} ] && {
		insmod ${bin_dir}/${drv_obj_file_name};
	}
}
