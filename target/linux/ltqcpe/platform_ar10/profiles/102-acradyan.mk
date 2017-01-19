#
# Copyright (C) 2006-2008 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

define Profile/MGL7016AW22_VR
	NAME:=MGL7016AW22-VR Profile
endef

define Profile/MGL7016AW22_VR/Description
	MGL7016AW22-VR Verizon LTE IAD profile
endef

$(eval $(call Profile,MGL7016AW22_VR))

define Profile/MGL7016AW_22_ZZ_6L
	NAME:=MGL7016AW-22-ZZ-6L Profile
endef

define Profile/MGL7016AW_22_ZZ_6L/Description
	MGL7016AW-22-ZZ LTE IAD ZZ 6L-PCB profile
endef
$(eval $(call Profile,MGL7016AW_22_ZZ_6L))

define Profile/MGL7016AW_22_ZZ_R0A
	NAME:=MGL7016AW-22-ZZ-R0A Profile
endef

define Profile/Profile/MGL7016AW_22_ZZ_R0A/Description
	MGL7016AW-22-ZZ LTE IAD ZZ 4L-PCB R0A profile
endef
$(eval $(call Profile,MGL7016AW_22_ZZ_R0A))

define Profile/MGL7016AW_22_ZZ_R0B
	NAME:=MGL7016AW-22-ZZ-R0B Profile
endef

define Profile/Profile/MGL7016AW_22_ZZ_R0B/Description
	MGL7016AW-22-ZZ LTE IAD ZZ 4L-PCB R0B profile
endef
$(eval $(call Profile,MGL7016AW_22_ZZ_R0B))

define Profile/VGV951ABWAC23_AB_30a_99
  NAME:=VGV951ABWAC23-AB-30a-99 Profile
endef

define Profile/VGV951ABWAC23_AB_30a_99/Description
  NAME:=VGV951ABWAC23-AB-30a-99 KPN Profile
endef
$(eval $(call Profile,VGV951ABWAC23_AB_30a_99))
