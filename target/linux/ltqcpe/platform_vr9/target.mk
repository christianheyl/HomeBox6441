# 
# Copyright (C) 2009 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

ARCH:=mips
BOARDNAME:=VR9 Family
LINUX_VERSION:=2.6.32.42

DEFAULT_PACKAGES += kmod-aprp

define Target/Description
	Build images for Lantiq VR9 CPE device
endef
