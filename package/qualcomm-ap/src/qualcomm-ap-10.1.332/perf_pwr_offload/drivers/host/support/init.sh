#!/bin/sh

# This script is performs any Host-driven initialization that may be needed.

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
	# No special initialization needed for AR6002.
	exit 0
fi

if [ "$TARGET_TYPE" != "AR6001" ]
then
	echo "Check your TARGET_TYPE environment variable"
	exit 1
fi

# Remainder of this script applies only to AR6001

ISROMSH=${ISROMSH:-$WORKAREA/host/support/isrom.sh}
if [ ! -x "$ISROMSH" ]; then
	echo "Executable ISROMSH script, '$ISROMSH', not found"
	exit 1
fi

$ISROMSH
if [ $? -ne 0 ]
then
	# Not running from ROM -- no special initialization needed.
	exit 0
fi

# Remainder of this script applies to AR6001 running from ROM

# Check for AR6001 revision 2.0 checksum
ar6001rev2_0_cksum=0x10474bbd
ckval=`$BMILOADER -i $NETIF --get --address=0xa100fffc`
if [ $ckval -ne $ar6001rev2_0_cksum ]
then
	# Not Revision 2.0
	exit 0
fi

# Remainder of this script applies to AR6001 Revision 2.0 running from ROM

REFCLKSH=${REFCLKSH:-$WORKAREA/host/support/refclk.sh}
if [ ! -x "$REFCLKSH" ]; then
	echo "Executable REFCLKSH script, '$REFCLKSH', not found"
	exit 1
fi

# By default, assume 40 MHz reference clock
REFCLK=${REFCLK:-40}

# Explicitly set the reference clock speed.
$REFCLKSH $REFCLK

exit 0
