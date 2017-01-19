#!/bin/sh
BMILOADER="sudo $WORKAREA/host/.output/$ATH_PLATFORM/image/bmiloader"
RM_PROG="sudo rm"
NETIF=${NETIF:-eth1}
OD="sudo od"
PATCH_ADDR=0x80013c00
echo "PATCH_ADDR:$PATCH_ADDR"
$BMILOADER -i $NETIF --read --address=0xa207fe0e --length=1 --file=subsysID.$$ 2>&1 >/dev/null
SUBSYSID=`$OD -x --read-bytes=1 subsysID.$$ | head -1 | cut -b 11-`
$RM_PROG -f subsysID.$$    
if [ "A$SUBSYSID" ==  "A88" ]; then
$BMILOADER -i $NETIF --dsetpatch --address=$PATCH_ADDR
else
echo Not CUS88
fi

