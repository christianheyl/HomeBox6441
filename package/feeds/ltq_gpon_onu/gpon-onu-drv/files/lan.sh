#!/bin/sh /etc/rc.common
# Copyright (C) 2009 OpenWrt.org
# Copyright (C) 2011 lantiq.com

. $IPKG_INSTROOT/lib/falcon.sh

START=91

onu () {
	#echo "onu $*"
	result=`/opt/lantiq/bin/onu $*`
	#echo "result $result"
	status=${result%% *}
	if [ "$status" != "errorcode=0" ]; then
		echo "onu $* failed: $result"
	fi
}

mux_mode="2" # RGMII as default
port_mode_0="1 0 0 0" # GPHY
port_mode_1="1 0 0 0" # GPHY
port_mode_2=""
port_mode_3=""
port_capability_0="1 1 1 1 1 1 1 0"
port_capability_1="1 1 1 1 1 1 1 0"
port_capability_2="1 1 1 1 1 1 1 0"
port_capability_3="1 1 1 1 1 1 1 0"
phy_addr="0 1 4 5" # defaults, TODO: don't set values for ports 2/3, if driver allows it
clk_delay="0 0"

gpon_lan_port_set() {
	[ -n "$2" ] && onu lanpcs $1 0 $2 $clk_delay 1518 1 0 && \
		[ -n "$3" ] && onu lanpccs $1 $3
}

gpon_uni2lan_add() {
    echo "$1 $2" >> /tmp/uni2lan
}

get_gpon_lan_ports() {
	local module
	case $(falcon_board_name) in
	easy98000)
		module=`cat /proc/cpld/xmii`
		case "$module" in
		"RGMII module"*) # GPHY RGMII addon board
			mux_mode="2"
			phy_addr="0 1 4 5"
			port_mode_2="5 0 0 0" # RGMII MAC
			port_mode_3="5 0 0 0" # RGMII MAC
			clk_delay="1 1"
			;;
		"GMII PHY"*) # GPHY GMII addon board
			mux_mode="4"
			#phy_addr="0 1 4 -1"
			port_mode_2="8 1 0 4" # GMII_MAC
			;;
		"MII PHY"*) # GPHY MII addon board
			mux_mode="4"
			#phy_addr="0 1 4 -1"
			port_mode_2="10 0 0 0" # MII_MAC
			;;
		"RMII PHY"*) # GPHY RMII addon board
			mux_mode="2"
			port_mode_2="6 0 0 0" # RMII_MAC
			port_mode_3="6 0 0 0" # RMII_MAC
			;;
		"GMII MAC"*) # Tantos GMII addon board
			mux_mode="4"
			phy_addr="0 1 -1 -1"
			port_mode_2="9 0 0 0" # GMII_PHY
			;;
		"MII MAC"*) # Tantos MII addon board
			mux_mode="4"
			phy_addr="0 1 -1 -1"
			port_mode_2="11 0 0 0" # MII_PHY
			;;
		"TMII MAC"*) # Tantos TMII addon board
			mux_mode="4"
			phy_addr="0 1 -1 -1"
			port_mode_2="13 0 0 0" # TMII_PHY
			;;
		*) # none/unknown
			;;
		esac
		;;
	easy98000_4fe) # four times FE
		mux_mode="1"
		phy_addr="0 1 2 3"
		port_mode_0="2 0 0 0"
		port_mode_1="2 0 0 0"
		port_mode_2="2 0 0 0" 
		port_mode_3="2 0 0 0"
		clk_delay="1 1"
		;;
	easy98020)
		port_mode_2="5 0 0 0" # RGMII MAC
		port_mode_3="5 0 0 0" # RGMII MAC
		phy_addr="0 1 5 6"
		;;
	easy98010)
		port_mode_1=""
		;;
	G5500)
		phy_addr="-1 -1 -1 -1"
		port_mode_0=""
		port_mode_1=""
		port_mode_2="5 1 3 4"
		port_mode_3=""
		clk_delay="2 2"
		;;
	MDU*)
		mux_mode="4"
		phy_addr="0 -1 -1 -1"
		port_mode_0="1 0 0 0" # GPHY
		port_mode_1=""
		port_capability_1=""
		port_mode_2="8 1 4 4" # GMII
		port_capability_2=""
		#port_mode_3="3 1 4 4"  SGMII
		#port_capability_3=""
		clk_delay="2 2"
		gpon_uni2lan_add 1 2
		gpon_uni2lan_add 2 0
		gpon_uni2lan_add 3 1
		gpon_uni2lan_add 4 3
		;;
	esac
}

gpon_lct_setup() {
	local mac=$(sed 's/.*ethaddr=\([0-9a-fA-F]\{2\}:[0-9a-fA-F]\{2\}:[0-9a-fA-F]\{2\}:[0-9a-fA-F]\{2\}:[0-9a-fA-F]\{2\}:[0-9a-fA-F]\{2\}\).*/\1/' < /proc/cmdline)
	[ -z "$mac" ] && mac='ac:9a:96:00:00:00'

	# on reference boards, use ip addr from u-boot
	case $(falcon_board_name) in
	easy98010|easy98020)
		local ip=$(awk 'BEGIN{RS=" ";FS="="} $1 == "ip" {print $2}' /proc/cmdline)
		local ipaddr=$(echo $ip | awk 'BEGIN{FS=":"} {print $1}')
		local gateway=$(echo $ip | awk 'BEGIN{FS=":"} {print $3}')
		local netmask=$(echo $ip | awk 'BEGIN{FS=":"} {print $4}')
		[ -n "$ipaddr" ] && uci set network.lct.ipaddr=$ipaddr
		[ -n "$gateway" ] && uci set network.lct.gateway=$gateway
		[ -n "$netmask" ] && uci set network.lct.netmask=$netmask
		;;
	esac

	onu gpe_sce_mac_set $(echo $mac | sed -e 's/^/0x/' -e 's/:/ 0x/g')

	# start LCT bridge
	ifup lct
}

enable_gpon_lan_ports() {
	local module
	case $(falcon_board_name) in
	MDU*)
		# enable data port towards the Vinax
		# ensure that CLKOC_OEN is enabled
		onu lanpe 2
		;;
	esac
}

start() {
	get_gpon_lan_ports
	onu lani
	onu lancs $mux_mode $phy_addr 0 0 1
	gpon_lan_port_set 0 "$port_mode_0" "$port_capability_0"
	gpon_lan_port_set 1 "$port_mode_1" "$port_capability_1"
	gpon_lan_port_set 2 "$port_mode_2" "$port_capability_2"
	gpon_lan_port_set 3 "$port_mode_3" "$port_capability_3"
	gpon_lct_setup
	enable_gpon_lan_ports
}
