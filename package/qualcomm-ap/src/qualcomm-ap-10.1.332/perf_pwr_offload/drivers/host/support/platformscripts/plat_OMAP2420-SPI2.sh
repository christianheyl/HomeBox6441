#!/bin/sh
#
# OMAP2420 platform setup script for SPI operation.
#
#

case $1 in
	loadbus)
	/sbin/insmod $IMAGEPATH/sdio_lib.o
	/sbin/insmod $IMAGEPATH/sdio_busdriver.o debuglevel=7 RequestListSize=128
	/sbin/insmod $IMAGEPATH/athspi_omap2420_hcd.o debuglevel=7 op_clock=12000000
	;;
	
	unloadbus)
	/sbin/rmmod athspi_omap2420_hcd
	/sbin/rmmod sdio_busdriver
	/sbin/rmmod sdio_lib
	;;
	
	loadAR6K)
	/sbin/insmod $IMAGEPATH/$AR6K_MODULE_NAME.o $AR6K_MODULE_ARGS
	;;
	
	unloadAR6K)
	/sbin/rmmod $AR6K_MODULE_NAME
	;;
	*)
		echo "Unknown option : $1"
	
esac
