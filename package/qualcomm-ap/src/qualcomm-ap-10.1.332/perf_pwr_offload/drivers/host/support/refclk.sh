#!/bin/sh

# This script is used to workaround a bug in the AR6001 2.0 ROM.
# It manually sets the reference clock speed and reinitializes the
# serial port baud rate.
#
# This script should be used ONLY when the OS is running directly
# from AR6001 ROM 2.0.  It should NOT be used when the OS runs from
# AR6001 flash.  It should NOT be used on AR6002.

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

if [ -z "$1" ]
then
    echo Manually set the AR6000 reference clock speed with
    echo $0 "[19.2 | 24 | 26 | 38.4 | 40 | 52]"
    exit 1
fi

case $1 in
    19.2 ) 
	clocking_table_addr=0x81009490
    ;;
    24 ) 
	clocking_table_addr=0x810094a4
    ;;
    26 ) 
	clocking_table_addr=0x810094b8
    ;;
    38.4) 
	clocking_table_addr=0x810094cc
    ;;
    40 ) 
	clocking_table_addr=0x810094e0
    ;;
    52 ) 
	clocking_table_addr=0x810094f4
    ;;
    * ) 
        echo "Unknown reference clock speed"
        exit 1
    ;;
esac

# Set reference clock speed (clock_info)
$BMILOADER --write --address=0x800012fc --param=$clocking_table_addr

# Re-initialize serial port baud rate, using correct uart frequency:
# call cyg_hal_plf_serial_init_channel()
$BMILOADER --execute --address=0x810087bc --param=0

exit 0
