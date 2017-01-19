#!/bin/sh
#
# SANDGATEII (PXA270) platform setup script
#

case $1 in
	loadbus)
	/sbin/insmod $IMAGEPATH/sdio_lib.ko
	/sbin/insmod $IMAGEPATH/sdio_busdriver.ko debuglevel=7 $ATH_SDIO_STACK_PARAMS
	/sbin/insmod $IMAGEPATH/sdio_pxa270hcd.ko debuglevel=7
	;;
	unloadbus)
	/sbin/rmmod sdio_pxa270hcd.ko
	/sbin/rmmod sdio_busdriver.ko
	/sbin/rmmod sdio_lib.ko
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
