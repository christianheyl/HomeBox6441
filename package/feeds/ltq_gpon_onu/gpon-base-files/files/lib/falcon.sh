#!/bin/sh
# Copyright (C) 2011 OpenWrt.org
# Copyright (C) 2011 lantiq.com

falcon_board_name() {
	local machine
	local name

	# take board name from cmdline
	name=$(awk 'BEGIN{RS=" ";FS="="} /boardname/ {print $2}' /proc/cmdline)
	[ -z $name ] && {
		# if not define, use cpuinfo
		machine=$(awk 'BEGIN{FS="[ \t]+:[ \t]"} /machine/ {print $2}' /proc/cpuinfo)
		case "$machine" in
		*EASY98000*)
			name="easy98000"
			;;
		*EASY98010*)
			name="easy98010"
			;;
		*EASY98020*)
			name="easy98020"
			;;
		*95C3AM1*)
			name="95C3AM1"
			;;
		*MDU16*)
			name="MDU16"
			;;
		*MDU8*)
			name="MDU8"
			;;
		*MDU*)
			name="MDU1"
			;;
		*)
			name="generic"
			;;
		esac
	}

	echo $name
}
