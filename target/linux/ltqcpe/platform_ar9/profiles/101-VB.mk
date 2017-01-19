#
# Copyright (C) 2006-2008 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

define Profile/VB
  NAME:=VB Profile
endef

define Profile/VB/Description
	VB profile
endef
$(eval $(call Profile,VB))

