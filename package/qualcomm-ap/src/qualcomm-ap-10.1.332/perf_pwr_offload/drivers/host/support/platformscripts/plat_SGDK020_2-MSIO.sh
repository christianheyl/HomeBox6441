#!/bin/sh
#
# custom platform setup script
#


case $1 in
	loadbus)
	/sbin/insmod $IMAGEPATH/mshost_drv.ko hostsel=1
	;;
	unloadbus)
	/sbin/rmmod -w mshost_drv
	;;
	loadAR6K)
	/sbin/insmod $IMAGEPATH/$AR6K_MODULE_NAME.ko $AR6K_MODULE_ARGS
	;;
	unloadAR6K)
	/sbin/rmmod -w $AR6K_MODULE_NAME.ko
	;;
	*)
		echo "Unknown option : $1"
	
esac
