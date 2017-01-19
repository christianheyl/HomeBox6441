#
# Copyright (C) 2006-2008 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

define Profile/GRX390
  NAME:=GRX390 Profile
endef

define Profile/GRX390/Description
  GRX390 Profile.
endef

$(eval $(call Profile,GRX390))
