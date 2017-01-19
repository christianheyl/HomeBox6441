#!/bin/sh
# Script to activate of deactivate host mode USB LED's based on disk mount
# This script designed to work only with mountd application

if [ ! "$CONFIGLOADED" ] && [ -r /etc/rc.d/config.sh ]; then
	. /etc/rc.d/config.sh 2>/dev/null                       
	CONFIGLOADED="1"                                         
fi                                                        

[ -n "$CONFIG_TARGET_LTQCPE_PLATFORM_VR9" ] && {
	LED_1_DEV=/sys/class/leds/usb1_link_led/brightness
	LED_2_DEV=/sys/class/leds/usb2_link_led/brightness
}
[ -n "$CONFIG_TARGET_LTQCPE_PLATFORM_DANUBE" ] && {
	LED_1_DEV=/sys/class/leds/usbdev_led/brightness
}
[ -n "$CONFIG_TARGET_LTQCPE_PLATFORM_AR9" ] && {
	LED_1_DEV=/sys/class/leds/ledc_12/brightness
	LED_2_DEV=/sys/class/leds/ledc_15/brightness
}

led_1_f () {
	([ -n "$CONFIG_PACKAGE_KMOD_LTQCPE_USB_HOST" ] || [ -n "$CONFIG_PACKAGE_KMOD_LTQCPE_USB_HOST_PORT1" ]) && {
		[ -f "$LED_1_DEV" ] && echo $1 > "$LED_1_DEV"
	} || {
		[ -n "$CONFIG_PACKAGE_KMOD_LTQCPE_USB_HOST_PORT2" ] && {
			[ -f "$LED_2_DEV" ] && echo $1 > "$LED_2_DEV"
		}
	}
}

led_2_f () {
	([ -n "$CONFIG_PACKAGE_KMOD_LTQCPE_USB_HOST" ] || [ -n "$CONFIG_PACKAGE_KMOD_LTQCPE_USB_HOST_PORT1" ]) && {
			[ -f "$LED_2_DEV" ] && echo $1 > "$LED_2_DEV"
	}
}

[ -d /sys/module/usb_storage/drivers/usb\:usb-storage/1-*/ ] && led_1_f 1 || led_1_f 0
[ -d /sys/module/usb_storage/drivers/usb\:usb-storage/2-*/ ] && led_2_f 1 || led_2_f 0

