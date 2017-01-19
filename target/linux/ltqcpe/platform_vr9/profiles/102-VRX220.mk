#
# Copyright (C) 2006-2008 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

define Profile/VRX220
  NAME:=VRX220 Profile
endef

define Profile/VRX220/Description
  VRX220 Profile.
endef

DEFAULT_PACKAGES += kmod-ltqcpe_vrx220_support

$(eval $(call Profile,VRX220))

