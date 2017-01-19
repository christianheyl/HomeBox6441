################################################################################
# (This script is for demo only!)                                              #
# In order to reach the maximum throughput, We need to connect two eth port to #
# QTN 11ac AP board. Without port trunk support, this setup will create loop.  #
# This script is to create two saparated VLANs for each eth connection to avoid# 
# the loop                                                                     #
################################################################################  

#!/bin/sh
# Clear switch conf
/etc/init.d/ltq_switch_config.sh undo_switch_config

# Create VLAN 1 and 1024
switch_cli IFX_ETHSW_VLAN_ID_CREATE nVId=1024 nFId=1
switch_cli IFX_ETHSW_VLAN_ID_CREATE nVId=1 nFId=2
switch_cli IFX_ETHSW_DISABLE
switch_cli IFX_ETHSW_CFG_SET eMAC_TableAgeTimer=3 bVLAN_Aware=1 nMaxPacketLen=1536 bPauseMAC_ModeSrc=0 nPauseMAC_Src=00:00:00:00:00:00

# VLAN 1024: port 1, 3, 0
switch_cli IFX_ETHSW_VLAN_PORT_CFG_SET nPortId=0 nPortVId=1024 bVLAN_UnknownDrop=0 bVLAN_ReAssign=0 eVLAN_MemberViolation=3 eAdmitMode=0 bTVM=1
switch_cli IFX_ETHSW_VLAN_PORT_CFG_SET nPortId=1 nPortVId=1024 bVLAN_UnknownDrop=0 bVLAN_ReAssign=0 eVLAN_MemberViolation=3 eAdmitMode=0 bTVM=1
switch_cli IFX_ETHSW_VLAN_PORT_CFG_SET nPortId=3 nPortVId=1024 bVLAN_UnknownDrop=0 bVLAN_ReAssign=0 eVLAN_MemberViolation=3 eAdmitMode=0 bTVM=1
switch_cli IFX_ETHSW_VLAN_PORT_MEMBER_ADD nVId=1024 nPortId=0 bVLAN_TagEgress=0
switch_cli IFX_ETHSW_VLAN_PORT_MEMBER_ADD nVId=1024 nPortId=1 bVLAN_TagEgress=0
switch_cli IFX_ETHSW_VLAN_PORT_MEMBER_ADD nVId=1024 nPortId=3 bVLAN_TagEgress=0
switch_cli IFX_ETHSW_VLAN_PORT_MEMBER_ADD nVId=1024 nPortId=6 bVLAN_TagEgress=1

# VLAN 1: port 2,4
switch_cli IFX_ETHSW_VLAN_PORT_CFG_SET nPortId=2 nPortVId=1 bVLAN_UnknownDrop=0 bVLAN_ReAssign=0 eVLAN_MemberViolation=3 eAdmitMode=0 bTVM=1
switch_cli IFX_ETHSW_VLAN_PORT_CFG_SET nPortId=4 nPortVId=1 bVLAN_UnknownDrop=0 bVLAN_ReAssign=0 eVLAN_MemberViolation=3 eAdmitMode=0 bTVM=1
switch_cli IFX_ETHSW_VLAN_PORT_MEMBER_ADD nVId=1 nPortId=2 bVLAN_TagEgress=0
switch_cli IFX_ETHSW_VLAN_PORT_MEMBER_ADD nVId=1 nPortId=4 bVLAN_TagEgress=0
switch_cli IFX_ETHSW_VLAN_PORT_MEMBER_ADD nVId=1 nPortId=6 bVLAN_TagEgress=1
  
switch_cli IFX_ETHSW_ENABLE

vconfig add eth0 1024
vconfig add eth0 1

ifconfig eth0.1024 up
ifconfig eth0.1 up

brctl addif br0 eth0.1



