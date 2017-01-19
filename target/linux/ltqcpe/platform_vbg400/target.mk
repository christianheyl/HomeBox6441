# 
# Copyright (C) 2009 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

ARCH:=mipsel
BOARDNAME:=VBG400 Family
LINUX_VERSION:=2.6.32.42
LINUX_VERSION_SUFFIX:=vbg
KERNELNAME:="uImage.lzma"

define Target/Description
	Build images for Lantiq VBG400 CPE device
endef
