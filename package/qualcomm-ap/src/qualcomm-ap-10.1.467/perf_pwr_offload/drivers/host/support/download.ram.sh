#!/bin/sh
# Downloads an application to target RAM

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

export IMAGEPATH=${IMAGEPATH:-$WORKAREA/host/.output/$ATH_PLATFORM/image}
BMILOADER=$IMAGEPATH/bmiloader
if [ ! -x "$BMILOADER" ]; then
	echo "Loader application '$BMILOADER' not found"
	exit
fi

if [ -z "$NETIF" ]
then
	NETIF=eth1
fi

if [ -e "$1" ]; then
	echo "Downloading file '$1' ..."
else
	echo "File '$1' not found"
	echo "Usage: $0 <filename.bin>"
fi

eval export `$BMILOADER -i $NETIF --info | grep TARGET_VERSION`
AR6003_VERSION_REV2=0x30000384

if [ "$TARGET_VERSION" = "$AR6003_VERSION_REV2" ]; then
	LOADADDR=0x543180
	BEGINADDR=0x945400
	$BMILOADER -i $NETIF -w -a $LOADADDR -f $1
	echo "Setting the start address ..."
	$BMILOADER -i $NETIF -b -a $BEGINADDR
else
	# segmented binary
	$BMILOADER -i $NETIF -w -f $1
fi
