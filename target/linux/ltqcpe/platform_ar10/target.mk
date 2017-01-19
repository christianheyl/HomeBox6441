# 
# Copyright (C) 2009 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

ARCH:=mips
BOARDNAME:=AR-10 Family
LINUX_VERSION:=2.6.32.42
KERNELNAME:="uImage.lzma"

DEFAULT_PACKAGES += kmod-aprp

define Target/Description
	Build images for Lantiq AR-10 CPE device
endef
