#!/bin/sh

# This script sets platform specific settings for the UGW platforms.

export RWFS_PATH=/mnt/jffs2
export ROOT_PATH=/root/mtlk
export ETC_PATH=/etc/rc.d
export IMAGES_PATH=/root/mtlk/images
export DRIVER_PATH=/lib/modules/$(uname -r)/net
export WEB_PATH=/root/mtlk/web
export CONFIGS_PATH=/ramdisk/flash
export BCL_PATH=/root/mtlk/bcl
export BINDIR=/bin

export log_level=1
#log_path="/dev/console"

export AP=2
export VAP=3
export STA=0

#STATIC=1
export DHCPC=0

#VIDEO_BRIDGE=0
#LAN_PARTY=1

export BAND_5G=0
export BAND_24G=1
export BOTH_BAND=2


export ENABLE=1
export DISABLE=0
export YES=1
export NO=0

export MacCloning=3
export L2NAT=2

export open=1
export wep=2
export WPA_Personal=3
export WPA_Enterprise=4

export Enrollee=0
export Wirless_registrar=1
export Unconfigured_AP=0
export Ap_Proxy_registrar=1

# Start the led manager on the dongle
#LEDMAN=0

# Dongle has no pushbutton for WPS
#WPS_PB=1

# Don't use lower memory usage - use default net queues
#LOWER_MEM_USAGE=1


# Don't mangle multicast packets (this is done on STAR to reduce CPU utilization)
#MULTICAST_MANGLING=0

# GPIO pins used for output LEDs
#GPIO_LEDS=0,3
# GPIO pins used for pushbuttons
#GPIO_PUSHBUTTONS=1,13
