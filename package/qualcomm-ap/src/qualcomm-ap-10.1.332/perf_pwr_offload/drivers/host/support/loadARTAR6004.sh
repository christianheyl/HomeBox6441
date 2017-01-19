#!/bin/sh

echo "ATH_PLATFORM $ATH_PLATFORM"
echo "WORKAREA $WORKAREA"

BMILOADER=${BMILOADER:-$WORKAREA/host/.output/$ATH_PLATFORM/image/bmiloader}
LOADAR6000=${LOADAR6000:-$WORKAREA/host/support/loadAR6000.sh}

$LOADAR6000 hostonly enableuartprint

export wlanapp=$WORKAREA/target/.output/AR6002/fpga.6/bin/device.bin
$LOADAR6000 targonly enableuartprint noresetok nostart bypasswmi

#export artapp=${artapp:-$WORKAREA/target/.output/AR6002/fpga.6/bin/device.bin}
#echo "$BMILOADER -i $NETIF --write --file=$artapp"
#$BMILOADER -i $NETIF --write --file=$artapp
#echo "$BMILOADER -i $NETIF --write --execute --param=0"
#$BMILOADER -i $NETIF --write --execute --param=0
#$BMILOADER -i $NETIF --done

sleep 1

