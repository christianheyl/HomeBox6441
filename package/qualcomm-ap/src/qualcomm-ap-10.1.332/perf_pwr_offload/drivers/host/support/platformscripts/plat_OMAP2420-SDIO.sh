#!/bin/sh
#
# OMAP2420 platform setup script (SDIO I/F).
#
#

case $1 in
	loadbus)
	# uncomment this if gpio pin is tied to dat0 line for busy-signal polling */
	#omap_dat0_gpio="sd0_dat0_gpio_pin=16 sd0_dat0_gpio_pad_conf_offset=232 sd0_dat0_gpio_pad_conf_byte=0 sd0_dat0_gpio_pad_mode_value=3 "
    omap_dat0_gpio=""
	/sbin/insmod $IMAGEPATH/sdio_lib.o
	/sbin/insmod $IMAGEPATH/sdio_busdriver.o debuglevel=7 $ATH_SDIO_STACK_PARAMS
	/sbin/insmod $IMAGEPATH/sdio_omap_hcd.o debuglevel=7 builtin_card=1 async_irq=1 $omap_dat0_gpio
	;;
	
	unloadbus)
	/sbin/rmmod sdio_omap_hcd
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
