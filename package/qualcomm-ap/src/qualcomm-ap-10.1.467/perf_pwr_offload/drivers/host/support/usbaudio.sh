#!/bin/sh
 
BINDIR=$WORKAREA/host/.output/$ATH_PLATFORM/image
PARAMETERS=""
 
$WORKAREA/host/support/loadAR6000.sh enableuartprint bypasswmi bmi
$WORKAREA/host/support/download.ram.sh $WORKAREA/target/AR6002/fpga.6/bin/usbaudioclass.bin
 
    if [ -e "/sys/module/ar6000/debugbmi" ]; then
        PARAMETERS="/sys/module/ar6000"
    fi
 
    if [ -e "/sys/module/ar6000/parameters/debugbmi" ]; then
        PARAMETERS="/sys/module/ar6000/parameters"
    fi
 
    if [ "$PARAMETERS" != "" ]; then
        echo 1 > ${PARAMETERS}/bypass_drv_init
    fi
    
    if [ "$PARAMETERS" == "" ]; then
        echo "No module parameter access"
        exit
    fi
   
 $BINDIR/bmiloader -i $NETIF --done

