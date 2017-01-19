#!/bin/sh
# Downloads the apps onto target using bmiloader

if [ -z "$WORKAREA" ]
then
	echo "Please set your WORKAREA environment variable."
	exit
fi
if [ -z "$ATH_PLATFORM" ]
then
	echo "Please set your ATH_PLATFORM environment variable."
	exit
fi

eval ATH_PLATFORM="`$WORKAREA/host/support/platform.sh`"

BMILOADER=${BMILOADER:-$WORKAREA/host/.output/$ATH_PLATFORM/image/bmiloader}
if [ ! -x "$BMILOADER" ]; then
	echo "Loader application '$BMILOADER' not found"
	exit
fi

if [ -z "$NETIF" ]
then
    NETIF=eth1
fi

if [ -e "$1" ]; then
        echo "Configuring memory ..."
	$BMILOADER  -i $NETIF -s -a 0xac004010 -p 0xc4000000
	$BMILOADER  -i $NETIF -s -a 0xac004014 -p 0x92020033
	$BMILOADER  -i $NETIF -s -a 0xac004018 -p 0x02770871
	$BMILOADER  -i $NETIF -s -a 0xac00401c -p 0x02770171
	echo
        echo "Downloading file '$1' ..."
	$BMILOADER  -i $NETIF -w -a 0x84000000 -f $1
	echo
        echo "Setting the start address ..."
	$BMILOADER  -i $NETIF -b -a 0x84000000
else
	echo "File '$1' not found"
	echo "Usage: $0 <filename.bin>"
fi
