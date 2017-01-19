#!/bin/sh

# This script tests to see if the Target is running from ROM.
# Return values:
#   0  --> Yes, Target is running from ROM
#   !0 --> No,  Target is NOT running from ROM
#
# Example usage:
# support/isrom.sh
# if [ $? -eq 0 ]
# then
#  echo IS running from ROM
# else
#  echo is NOT running from ROM
# fi

NETIF=${NETIF:-eth1}

if [ -z "$WORKAREA" ]
then
	echo "Please set your WORKAREA environment variable."
	exit 1
fi

if [ -z "$ATH_PLATFORM" ]
then
	echo "Please set your ATH_PLATFORM environment variable."
	exit 1
fi

BMILOADER=${BMILOADER:-$WORKAREA/host/.output/$ATH_PLATFORM/image/bmiloader}
if [ ! -x "$BMILOADER" ]; then
	echo "Loader application, '$BMILOADER', not found"
	exit 1
fi

eval export `$BMILOADER -i $NETIF --info | grep TARGET_TYPE`

if [ "$TARGET_TYPE" = "AR6002" ]
then
	# AR6002 always runs from ROM
	exit 0
fi

# Remainder of this script is for AR6001 only

# Cause Target-side DCache line flush
$BMILOADER -i $NETIF --read --address=0x400 --length=1 --file=/dev/null 

# See if we're running from ROM
$BMILOADER -i $NETIF --get --address=0x80000400 | grep 0x81

exit $?
