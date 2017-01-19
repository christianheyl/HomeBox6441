#!/bin/sh
# This is the script set wls links.
#SVN id: $Id: mtlk_init_links.sh 2278 2008-02-21 15:40:01Z ediv $

# Defines
if [ ! "$MTLK_INIT_PLATFORM" ]; then			
	. /tmp/mtlk_init_platform.sh
	MTLK_INIT_PLATFORM="1"
	print2log DBG "mtlk_init_platform called in mtlk_wls_links.sh"
fi
print2log DBG "mtlk_wls_links.sh: args: $*"
command=$1

start_wls_links()
{
	print2log DBG "start wls_links"

	NETWORK_TYPE=`host_api get $$ 0 network_type`

	if [ ! -e /tmp/bootloader.bin ]; then ln -s $IMAGES_PATH/bootloader.bin  /tmp/bootloader.bin; fi
	if [ "$NETWORK_TYPE" = "$AP" ] && [ ! -e /tmp/ap_upper.bin ]; then ln -s $IMAGES_PATH/ap_upper.bin    /tmp/ap_upper.bin; fi
	if [ "$NETWORK_TYPE" = "$STA" ] && [ ! -e /tmp/sta_upper.bin ]; then ln -s $IMAGES_PATH/sta_upper.bin   /tmp/sta_upper.bin; fi
	if [ ! -e /tmp/contr_lm.bin ]; then ln -s $IMAGES_PATH/contr_lm.bin    /tmp/contr_lm.bin; fi
	if [ ! -e /tmp/mtlk.ko ]; then ln -s $DRIVER_PATH/mtlk.ko   /tmp/mtlk.ko; fi
	if [ ! -e /tmp/mtlklog.ko ]; then ln -s $DRIVER_PATH/mtlklog.ko   /tmp/mtlklog.ko; fi

	cd $IMAGES_PATH
	for progmodel in `ls ProgModel*`;
	do
		if [ ! -e /tmp/$progmodel ]; then ln -s  $IMAGES_PATH/$progmodel  /tmp/$progmodel; fi
	done
	cd - > /dev/null
	print2log DBG "Finish wls_links"
}

stop_wls_links()
{
	print2log DBG "stop wls_links"

	cd /tmp
	if [ -e /tmp/bootloader.bin ]; then rm  /tmp/bootloader.bin; fi
	if [ -e /tmp/ap_upper.bin ]; then rm    /tmp/ap_upper.bin; fi
	if [ -e /tmp/sta_upper.bin ]; then rm   /tmp/sta_upper.bin; fi
	if [ -e /tmp/contr_lm.bin ]; then rm    /tmp/contr_lm.bin; fi
	if [ -e /tmp/tmp/mtlk.ko ]; then rm   /tmp/mtlk.ko; fi
	if [ -e /tmp/tmp/mtlklog.ko ]; then rm   /tmp/mtlklog.ko; fi
	if [ -e /tmp/config.conf ]; then rm   /tmp/config.conf; fi
	if [ -e /tmp/ProgModel_BG_nCB.bin ] || [ -e /tmp/ProgModel_A_nCB.bin ]; then rm  /tmp/ProgModel_*; fi
	cd - > /dev/null
	
	print2log DBG "Finish stop wls_links"
}

create_config_wls_links()
{
	return
}

should_run_wls_links()
{
	true
}
case $command in
	start)
		start_wls_links
	;;
	stop)
		stop_wls_links
	;;
	create_config)
		create_config_wls_links
	;;
	should_run)
		should_run_wls_links
	;;
esac
