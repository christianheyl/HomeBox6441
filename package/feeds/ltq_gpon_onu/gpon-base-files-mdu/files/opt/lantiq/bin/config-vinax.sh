#!/bin/sh
# Copyright (C) 2012 lantiq.com

. $IPKG_INSTROOT/lib/falcon.sh

bindir=/opt/lantiq/bin

vlan_base=100
max_lines=0

system_config()
{
	case $(falcon_board_name) in
		MDU1)
		max_lines=1
		;;
		MDU8)
		max_lines=8
		;;
		MDU16)
		max_lines=16
		;;
		*)
		return -1
		;;
	esac
	# set HS-tones
	${bindir}/dsl_pipe LineOptionsConfigSet 0 4 0 0 0
	## nDevice=0, nSysIf0=SGMII(2), nSysIf1=undef(3)
	${bindir}/dsl_pipe DeviceSystemInterfaceConfigSet 0 4 7
	## nDevice=0, nInterface=0, nClassMode=vlan(0), nInterPacketGap=12, nPreambleLength=1(default),
	${bindir}/dsl_pipe GMIInterfaceConfigSet 0 0 0 12 1
	## nDevice=0, nInterface=0, nBaseVlanId=0, bRemoveTag=1
	${bindir}/dsl_pipe VLANClassInterfaceConfigSet 0 0 $vlan_base 1

	return 0
}

line_config()
{
	local line=$1

	${bindir}/dsl_pipe ld $line

	vlan=`expr $line + $vlan_base`

	${bindir}/dsl_pipe lllcs $line 5 4
	${bindir}/dsl_pipe g997xtusecs $line 0 0 0 0 0 0 0 7
	${bindir}/dsl_pipe bpcs $line 132 7 0
	${bindir}/dsl_pipe g997ccs $line 0 0 32000 110000000 1 0 2 0 0 0
	${bindir}/dsl_pipe g997ccs $line 0 1 32000 110000000 1 0 2 0 0 0
	${bindir}/dsl_pipe TrellisEnableConfigSet $line 0 1
	${bindir}/dsl_pipe TrellisEnableConfigSet $line 1 1
	${bindir}/dsl_pipe BitswapEnableConfigSet $line 0 1
	${bindir}/dsl_pipe BitswapEnableConfigSet $line 1 1
	${bindir}/dsl_pipe G997_LineActivateConfigSet $line 0 0 0
	## nLine=0, nChannel=0, nTcLayer=efm(1)
	${bindir}/dsl_pipe TcLayerConfigSet $line 0 1
	## nSystemIF=0
	${bindir}/dsl_pipe SystemInterfaceAssignmentConfigSet $line 0
	${bindir}/dsl_pipe VLANChannelConfigSet $line 0 $vlan
	# change band borders for interoperability in band plan 132
	${bindir}/dsl_pipe bbcs $line 0 4 0 894 1180 1 1996 2780 2 2784 3222 3 5000 5746
	${bindir}/dsl_pipe bbcs $line 1 4 0 34 855 1 1230 1946 2 3272 4950 3 5796 6862
	# disable rate capping in API:
	${bindir}/dsl_pipe mfcs $line 36 1
	${bindir}/dsl_pipe g997upbocs 0 0 0 0 0 0 0 0 0 0

	${bindir}/dsl_pipe la $line

	return 0
}

system_config

line=0
while test $line -lt $max_lines; do
	line_config $line
	line=`expr $line + 1`
done 
