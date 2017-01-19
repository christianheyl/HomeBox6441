#!/bin/sh /etc/rc.common
#
# Install DECT DEMO subsystem

START=36

# check for linux 2.6.x
#uname -r | grep -q 2.6.
#if [ $? -eq 0 ]; then
#    MODEXT=.ko
#fi

#bin_dir=/opt/ifx/bin
#drv_dev_base_name=tapi
#drv_obj_file_name=drv_tapi$MODEXT

start() {
#	[ -e ${bin_dir}/${drv_obj_file_name} ] && {
#		insmod ${bin_dir}/${drv_obj_file_name};
#		#create device nodes for...
#		#major_no=`grep $drv_dev_base_name /proc/devices |cut -d' ' -f1`
#	}
	/bin/mknod /dev/dect_drv c 213 0
	/bin/mknod /dev/timer_drv c 212 0
	/bin/mknod /dev/pb c 150 0
	/bin/mknod /dev/amazon_s-port c 240 0

	insmod /usr/drivers/drv_timer.ko
	insmod /usr/drivers/drv_dect.ko
	insmod /lib/modules/voip_timer_driver
	insmod /usr/drivers/paging.ko


	/usr/sbin/tapidemo_dect -d 1 -l /opt/ifx/downloads/ &
	/usr/sbin/DectApp &


}
