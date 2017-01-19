#!/bin/sh

if [ -z "$WORKAREA" ]
then
	echo "Please set your WORKAREA environment variable."
	exit 1
fi

autodetect=0
if [ -z $ATH_PLATFORM ];then
	autodetect=1
else
	test -d $WORKAREA/host/.output/$ATH_PLATFORM/image
	if [ $? -ne 0 ];then
		autodetect=1
	fi
fi

if [ $autodetect -eq 1 ];then
	export KERNEL_RELEASE=`uname -r`
	case $KERNEL_RELEASE in
		2.4.20_mvl31-omap5912_osk)
		ATH_PLATFORM=OMAP5912-SPI
		;;
		2.4.20_mvlcee31-omap2420_gsm_gprs)
		ATH_PLATFORM=OMAP2420-SPI
		;;
		2.6.9)
		ATH_PLATFORM=LOCAL_i686-SDIO
		;;
		*)
		ATH_PLATFORM=LOCAL_i686-SDIO
	esac
fi

echo $ATH_PLATFORM
