#
# Copyright (C) 2006-2008 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

define Profile/vrx318
  NAME:=vrx318 Profile
endef

define Profile/vrx318/Description
  VRX318 Profile.
endef

$(eval $(call Profile,vrx318))
