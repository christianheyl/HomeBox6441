#!/bin/sh -e

# Atheros AR6003 startup script

err() {
	echo "$@" >&2
	logger -t "${0##*/}[$$]" "$@" 2>/dev/null || true
}

ATH6KDIR=/lib/firmware/ath6k/AR6003
ATHLOGFILE=/var/log/ath6klog.txt
ATHSRCDIR=/usr/local/sbin/

export PATH=$PATH:$ATH6KDIR

# For debug, avoid the default 10 second timeout on request_firmware
# by doing this BEFORE LOADING the driver:
#     echo 0 > /sys/class/firmware/timeout

mydev=/sys$DEVPATH/device

# Debug:
err DBG: 0: $0
err DBG: ATH6KDIR=$ATH6KDIR
err DBG: DEVPATH=$DEVPATH
err DBG: FIRMWARE=$FIRMWARE
err DBG: mydev=$mydev
#
# exit 0

# Start capturing the logs
/usr/local/sbin/recEvent --logfile=$ATHLOGFILE --srcdir=$ATHSRCDIR &

if [ ! -e /sys$DEVPATH/loading ]; then
	err "$0 missing file, /sys$DEVPATH/loading"
	exit 1
fi

echo 1 > /sys$DEVPATH/loading

# Disable System Sleep on Target
bmiloader --interface=sysfs --quiet --set --address=0x180c0 --or=8 --device=$mydev
if [ $? -lt 0 ]; then goto err; fi

bmiloader --interface=sysfs --quiet --set --address=0x40c4 --or=1 --device=$mydev
if [ $? -lt 0 ]; then goto err; fi

# Run at 80/88MHz by default.
bmiloader --interface=sysfs --quiet --set --address=0x4020 --param=1 --device=$mydev
if [ $? -lt 0 ]; then goto err; fi

# LPO_CAL_TIME is based on refclk speed: 27500/Actual_MHz_of_refclk
# So for 26MHz (the default), we would use:
# param=$((27500/26))

# LPO_CAL.ENABLE = 1
bmiloader --interface=sysfs --quiet --set --address=0x40e0 --param=0x100000 --device=$mydev

# Add support for using the internal clock TODO

# Transfer Board Data from Target EEPROM to Target RAM.
# Determine where in Target RAM to write Board Data
param=`bmiloader --interface=sysfs --quiet --get --address=0x540654 --device=$mydev`

# Write EEPROM data to Target RAM
bmiloader --interface=sysfs --write --address=$param --file=$ATH6KDIR/hw1.0/fakeBoardData_AR6003_v1_0.bin --device=$mydev

# Record the fact that Board Data IS initialized
bmiloader --interface=sysfs --quiet --set --address=0x540658 --param=1 --device=$mydev

# Transfer One time Programmable data
bmiloader --interface=sysfs --write --address=0x542800 --file=$ATH6KDIR/hw1.0/otp.data --device=$mydev
if [ $? -lt 0 ]; then goto err; fi
bmiloader --interface=sysfs --write --address=0x544c00 --file=$ATH6KDIR/hw1.0/otp.bin --device=$mydev
if [ $? -lt 0 ]; then goto err; fi
bmiloader --interface=sysfs --execute --address=0x944c00 --param=0 --device=$mydev
if [ $? -lt 0 ]; then goto err; fi

# Enable HI_OPTION_DISABLE_DBGLOG
bmiloader --interface=sysfs --quiet --set --address=0x540610 --or=0x40 --device=$mydev
if [ $? -lt 0 ]; then goto err; fi

# Enable HI_OPTION_FW_MODE_BSS_STA (station mode)
#bmiloader --interface=sysfs --set --address=0x540610 --or=8 --device=$mydev
#if [ $? -lt 0 ]; then goto err; fi

# For debugging, enable the Target to print to the Target serial port
# bmiloader --interface=sysfs --set --address=0x540614 --param=1 --device=$mydev
# if [ $? -lt 0 ]; then goto err; fi

# Set MBOX ISR yield limit
#bmiloader --interface=sysfs --set --address=0x540674 --param=99 --device=$mydev
#if [ $? -lt 0 ]; then goto err; fi

# Download Target firmware
bmiloader --interface=sysfs --write --address=0x542800 --file=$ATH6KDIR/hw1.0/AR6003fw.z77 --uncompress --device=$mydev
if [ $? -lt 0 ]; then goto err; fi

# Set starting address for firmware
bmiloader --interface=sysfs --begin --address=0x944c00 --device=$mydev
if [ $? -lt 0 ]; then goto err; fi

# Reserve 5.5K of RAM
bmiloader --interface=sysfs --set --address=0x540668 --param=5632 --device=$mydev
if [ $? -lt 0 ]; then goto err; fi
bmiloader --interface=sysfs --write --address=0x57ea6c --file=$ATH6KDIR/hw1.0/data.patch.hw1_0.bin --device=$mydev
if [ $? -lt 0 ]; then goto err; fi
bmiloader --interface=sysfs --write --address=0x540618 --param=0x57ea6c --device=$mydev
if [ $? -lt 0 ]; then goto err; fi

# Restore System Sleep
#bmiloader --interface=sysfs --set --address=0x40c4 --param=$old_sleep --device=$mydev
#bmiloader --interface=sysfs --set --address=0x180c0 --param=$old_options --device=$mydev

# Configure GPIO AR6003 UART
bmiloader --interface=sysfs --set --address=0x540680 --param=8 --device=$mydev
if [ $? -lt 0 ]; then goto err; fi

# Tell Target to execute loaded firmware
bmiloader --interface=sysfs --done --device=$mydev
if [ $? -lt 0 ]; then goto err; fi

# The current implementation of request_firmware considers 0-length data
# to be a failure, so give it a little something to keep it happy.
echo 0 > /sys$DEVPATH/data

# Firmware loaded and initialized...SUCCESS
echo 0 > /sys$DEVPATH/loading

exit 0

err:
	err "$0 failed to load firmware"
	echo -1 > /sys$DEVPATH/loading
	exit 1
