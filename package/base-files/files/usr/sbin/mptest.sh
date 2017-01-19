#!/bin/sh
#
# Copyright (C) 2009 Arcadyan
# All Rights Reserved.
#

IN_SEL=
FLASHARGS='setenv bootargs root=$(rootfsmtd) rw rootfstype=squashfs,jffs2'
OP_MODE_CAL="bridge mptest calibrate"
OP_MODE_MPT="bridge mptest"
OP_MODE_EMI="bridge emi"
OP_MODE_BDG_SIP="bridge sip"
OP_MODE_RTD_SIP="sip"
OP_MODE_BDG_NOSIP="bridge"
OP_MODE_RTD_NOSIP=""
VOIP_CH=0
WLAN_TEST_PROG=`which mptest_wlan.sh`
if ! [ -x "$WLAN_TEST_PROG" ] ; then
	WLAN_TEST_PROG=""
fi
TP_TEST_PROG=`which mptest_tp.sh`
if ! [ -x "$TP_TEST_PROG" ] ; then
	TP_TEST_PROG=""
fi
LCD_TEST_PROG=`which mptest_lcd.sh`
if ! [ -x "$LCD_TEST_PROG" ] ; then
	LCD_TEST_PROG=""
fi


display_banner()
{
	echo
	echo
	echo "*****************************************************"
	echo "*                                                   *"
	echo "*                                                   *"
	echo "*               Production Test Utility             *"
	echo "*                                                   *"
	echo "*                                                   *"
	echo "*****************************************************"
	echo
}

display_selection()
{
	echo
	echo "  0. list settings"
	echo "  1. change operation mode"
	echo "  2. change system base MAC address"
	echo "  3. change serial number"
	echo "  4. change hardware version"
	echo "  5. change software version"
	echo "  6. list DSL status"
	echo "  7. LED test"
	echo "  8. VoIP test"
	echo "  9. DECT test"
	echo "  b. Button test"
  if [ -n "$WLAN_TEST_PROG" ] ; then
	echo "  w. WiFi calibration"
  fi
  if [ -n "$TP_TEST_PROG" ] ; then
	echo "  t. Touch pad calibration"
  fi
  if [ -n "$LCD_TEST_PROG" ] ; then
	echo "  l. LCD test"
  fi
	echo "-------------------------------------"
	echo "  r. reboot"
	echo "  x. exit"
	echo
}

# $1 - prompt
input_string()
{
	IN_SEL=
	read -p "$1 --> " IN_SEL
}

# $1 - display field, 0-all, 1-opmode, 2-MAC, 3-serial, 4-hw, 5-sw
display_settings()
{
	echo

	if [ -z "$1" ] || [ "$1" == "0" ] || [ "$1" == "1" ] ; then
		DATA=`uboot_env --get --name flashargs 2>&-`
		DATA=${DATA/$FLASHARGS /}
		if [ "$DATA" == "$OP_MODE_CAL" ] ; then
			echo " current operation mode : WiFi calibration"
		elif [ "$DATA" == "$OP_MODE_MPT" ] ; then
			echo " current operation mode : production test"
		elif [ "$DATA" == "$OP_MODE_EMI" ] ; then
			echo " current operation mode : EMI test"
		elif [ "$DATA" == "$OP_MODE_BDG_SIP" ] ; then
			echo " current operation mode : normal bridged with VoIP"
		elif [ "$DATA" == "$OP_MODE_RTD_SIP" ] ; then
			echo " current operation mode : normal routed with VoIP"
		elif [ "$DATA" == "$OP_MODE_BDG_NOSIP" ] ; then
			echo " current operation mode : normal bridged without VoIP"
		else #elif [ "$DATA" == "$OP_MODE_RTD_NOSIP" ] ; then
			echo " current operation mode : normal routed without VoIP"
		fi
	fi

	if [ -z "$1" ] || [ "$1" == "0" ] || [ "$1" == "2" ] ; then
		echo " system base MAC address : `uboot_env --get_mac --seq_no 0 2>&-`"
	fi

	if [ -z "$1" ] || [ "$1" == "0" ] || [ "$1" == "3" ] ; then
		echo " serial number : `uboot_env --get_manuf --name serial_number 2>&-`"
	fi

	if [ -z "$1" ] || [ "$1" == "0" ] || [ "$1" == "4" ] ; then
		echo " hardware version : `uboot_env --get --name hw_version 2>&-`"
	fi

	if [ -z "$1" ] || [ "$1" == "0" ] || [ "$1" == "5" ] ; then
		ACTIVE_IMG_NO=`image_util --get_active_img --obtain_info 1`
		if [ "$ACTIVE_IMG_NO" == "1" ]; then
			CURRENT_ACTIVE_SW_VER=`image_util --get --file /dev/mtdblock1 --filetype kernel --name ver`	
		else
			CURRENT_ACTIVE_SW_VER=`image_util --get --file /dev/mtdblock3 --filetype kernel --name ver`
		fi		
		echo " software version : $CURRENT_ACTIVE_SW_VER"
	fi

	echo
}

display_selection_opmode()
{
	echo
	echo "  1. switch to WiFi calibration mode"
	echo "  2. switch to production test  mode"
	echo "  3. switch to EMI test         mode"
	echo "  4. switch to normal bridged   mode with    VoIP"
	echo "  5. switch to normal routed    mode with    VoIP"
	echo "  6. switch to normal bridged   mode without VoIP"
	echo "  7. switch to normal routed    mode without VoIP"
	echo "---------------------------------------------------"
	echo "  x. exit"
	echo
}

change_opmode()
{
	while [ "1" ] ; do
		echo "==================================================="
		display_selection_opmode

		display_settings 1

		input_string "Please select an item"

		SEL=$IN_SEL
		case "${SEL:0:1}" in
			"1") # switch to WiFi calibration mode
				uboot_env --set --name flashargs --value "$FLASHARGS $OP_MODE_CAL" >&-
				;;
			"2") # switch to production test mode
				uboot_env --set --name flashargs --value "$FLASHARGS $OP_MODE_MPT" >&-
				;;
			"3") # switch to EMI test mode
				uboot_env --set --name flashargs --value "$FLASHARGS $OP_MODE_EMI" >&-
				;;
			"4") # switch to normal bridged mode with VoIP
				uboot_env --set --name flashargs --value "$FLASHARGS $OP_MODE_BDG_SIP" >&-
				;;
			"5") # switch to normal routed mode with VoIP
				uboot_env --set --name flashargs --value "$FLASHARGS $OP_MODE_RTD_SIP" >&-
				;;
			"6") # switch to normal bridged mode without VoIP
				uboot_env --set --name flashargs --value "$FLASHARGS $OP_MODE_BDG_NOSIP" >&-
				;;
			"7") # switch to normal routed mode without VoIP
				uboot_env --set --name flashargs --value "$FLASHARGS $OP_MODE_RTD_NOSIP" >&-
				;;
			#---------------------------------------------------------
			"x"|"X") # exit
				break
				;;
			*) # bad choice
				echo "BAD CHOICE !!!"
				;;
		esac
	done
}

display_selection_led_test()
{
	echo
	echo "  1. turn on  all LEDs"
	echo "  2. turn on  all one-color   LEDs"
	echo "  3. turn on  all three-color LEDs with RED    color"
	echo "  4. turn on  all three-color LEDs with GREEN  color"
	echo "  5. turn on  all three-color LEDs with ORANGE color"
	echo "  6. turn off all LEDs"
	echo "  7. turn off all one-color   LEDs"
	echo "  8. turn off all three-color LEDs"
	echo "  9. set bright level of all one-color LEDs"
	echo "------------------------------------------------------"
	echo "  x. exit"
	echo
}

led_test()
{
	while [ "1" ] ; do
		echo "======================================================"
		display_selection_led_test

		input_string "Please select an item"

		SEL=$IN_SEL
		case "${SEL:0:1}" in
			"1") # turn on all LEDs
				arc_led_mptest.sh on all RGB
				;;
			"2") # turn on all one-color LEDs
				arc_led_mptest.sh on one
				;;
			"3") # turn on all three-color LEDs with RED color
				arc_led_mptest.sh on three R
				;;
			"4") # turn on all three-color LEDs with GREEN color
				arc_led_mptest.sh on three G
				;;
			"5") # turn on all three-color LEDs with ORANGE color
				arc_led_mptest.sh on three RG
				;;
			"6") # turn off all LEDs
				arc_led_mptest.sh off all
				;;
			"7") # turn off all one-color LEDs
				arc_led_mptest.sh off one
				;;
			"8") # turn off all three-color LEDs
				arc_led_mptest.sh off three
				;;
			"9") # set bright level of all one-color LEDs
				arc_led_mptest.sh level
				;;
			#---------------------------------------------------------
			"x"|"X") # exit
				break
				;;
			*) # bad choice
				echo "BAD CHOICE !!!"
				;;
		esac
	done
}

display_selection_voip_test()
{
	echo
	echo "  1. Production Test On and selecting test Channel"
	echo "  2. Production Test Off"
	echo "  3. Selected Channel Stop Tone"
	echo "  4. Selected Channel Play DTMF *0~9#"
	echo "  5. Selected Channel Play 1K voice file"
	echo "  6. Ringing phone on Selected Channel for FXS SLIC only"
	echo "  7. Stop Ringing phone on Selected Channel for FXS SLIC only"
	echo "  8. DAA offhook on Selected Channel for FXO only"
	echo "  9. DAA onhook on Selected Channel for FXO only"
	echo "  0. Ring FXS with FSK CID"
	echo "------------------------------------------------------"
	echo "  x. exit"
	echo
}

voip_test()
{
	while [ "1" ] ; do
		echo "======================================================"
		display_selection_voip_test
		if ! [ -f /tmp/etc/fxs-stat ] ; then
			#cat /tmp/etc/fxs-stat
			touch /tmp/etc/fxs-stat
			tail -f /tmp/etc/fxs-stat &
		fi

		input_string "Please select an item"

		SEL=$IN_SEL
		case "${SEL:0:1}" in
			"1") # Production Test On and selecting test Channel
				sip_config TEL_Production_Test 1
				voip_mptest ring-off 0
				voip_mptest play-off 0
				voip_mptest ring-off 1
				voip_mptest play-off 1
				# voip_mptest on-hook  2
				while [ "1" ] ; do
					input_string "Please input voice channel # (0-FXS1, 1-FXS2, 2-FXO)"
					SEL=$IN_SEL
					if [ "$SEL" == "0" ] || [ "$SEL" == "1" ] || [ "$SEL" == "2" ] || [ -z "$SEL" ] ; then
						if [ -n "$SEL" ] ; then
							VOIP_CH=$SEL
						fi
						break
					fi
				done
				;;
			"2") # Production Test Off
				voip_mptest ring-off 0
				voip_mptest play-off 0
				voip_mptest ring-off 1
				voip_mptest play-off 1
				voip_mptest on-hook  2
				sip_config TEL_Production_Test 0
				killall tail
				rm -f /tmp/etc/fxs-stat
				;;
			"3") # Selected Channel Stop Tone
				voip_mptest play-off $VOIP_CH
				;;
			"4") # Selected Channel Play DTMF *0~9#
				voip_mptest play-dtmf $VOIP_CH "*0123456789#"
				;;
			"5") # Selected Channel Play 1K voice file
				voip_mptest play-1k   $VOIP_CH "-300"
				;;
			"6") # Ringing phone on Selected Channel for FXS SLIC only
				voip_mptest ring-on   $VOIP_CH
				;;
			"7") # Stop Ringing phone on Selected Channel for FXS SLIC only
				voip_mptest ring-off  $VOIP_CH
				;;
			"8") # DAA offhook on Selected Channel for FXO only
				voip_mptest off-hook  $VOIP_CH
				;;
			"9") # DAA onhook on Selected Channel for FXO only
				voip_mptest on-hook   $VOIP_CH
				;;
			"0") # Ring FXS with FSK CID 1234567890
				voip_mptest ring-cid  $VOIP_CH "1234567890"
				;;
			#---------------------------------------------------------
			"x"|"X") # exit
				break
				;;
			*) # bad choice
				echo "BAD CHOICE !!!"
				;;
		esac
	done
}

display_selection_button_test()
{
	echo
	echo "------------------------------------------------------"
	echo "  Button ID mapping:"
	echo "  0. Wifi"
	echo "  1. WPS"
	echo "  2. Reboot"
	echo "  3. Factory Default"
	echo "  4. Dect Reg"
	echo "  5. Dect Page"
	echo "  6. SIM"
	echo "  7. ECO"
	echo "  8. Self Care"
	echo "------------------------------------------------------"
	echo "  x. exit"
	echo
}

button_test()
{
	echo 1 > /tmp/b_mptest
	killall arc_gpiod
	arc_gpiod &
	while [ "1" ] ; do
		echo "======================================================"
		display_selection_button_test
		
		input_string "Please push a button"

		SEL=$IN_SEL
		case "${SEL:0:1}" in
			"x"|"X") # exit
				break
				;;
			*) # bad choice
				echo "Please push a button"
				;;
		esac
	done
	rm /tmp/b_mptest
	killall arc_gpiod
	arc_gpiod
}

display_selection_dect_test()
{
	echo
	echo "  0. DECT Phone start/stop paging"
	echo "  1. Start DECT Phone registering"
	echo "  2. NTP_TCORR PWR setting"
	echo "  3. Post Calibration Start"
	echo "  4. Set PIN Code"
	echo "  5. Delete HS"
	echo "  6. Cosic MP Test mode on"
	echo "  7. Reset COSIC"
	echo "  8. TBR6 Mode on"
	echo "  9. Set OSC value adn reset COSIC"
	echo "  a. Send Notify to all HS (?)"
	echo "  b. Send Date and Time to all HS (?)"
	echo "  c. Send MWI (1) to all HS (?)"
	echo "  C. Send MWI (0) to all HS (?)"
	echo "  D. Show DECT buffer used count (?)"
	echo "  e. Change Line Mode to Single Call Mode for all lines (?)"
	echo "  E. Change Line Mode to Multiple Call Mode for all lines (?)"
	echo "  f. Change ALM volume gain for ISDN call quality test! (?)"
	echo "  g. Show Data channel used and FXS mapping table (?)"
	echo "  h. Start to record MAC/SPI layer debug messages (?)"
	echo "  H. Print out MAC/SPI layer debug messages (?)"
	echo "  t. TPC Transmit Power Calibration"
	echo "  w. Write OSC value/PWR value to flash"
	echo "  r. Set RFPI"
	echo "  R. Dump values from flash for verification"
	echo "------------------------------------------------------"
	echo "  x. exit"
	echo
}
dect_test()
{
	while [ "1" ] ; do
		echo "======================================================"
		display_selection_dect_test
		#uboot_env --restore_dect_cal_data --file /tmp/etc/dect_param_cfg
		
		input_string "Please select an item"

		SEL=$IN_SEL
		
		case "${SEL:0:1}" in
			"0") # DECT Phone start/stop paging
				sip_config 'DECT_Page' 6
				;;
			"1") # Start DECT Phone registering
				sip_config 'DECT_Reg'	1
				;;
			"2") # NTP TCORR setting
				input_string "Please Endter 2 digit Hex ucNTP_TCORR:"

				SEL=$IN_SEL
				sip_config 'DECT_NTP_TCORR' "${SEL:0:2}"
				;;
			"3") # Get Tune digit..
				sip_config 'DECT_Post_Cal' 1
				;;
			"4") # Set Base PIN
				input_string "Please input 4~8 digit numbers:"

				SEL=$IN_SEL
				sip_config 'DECT_BASEPIN' "${SEL:0:8}"
				;;
			"5") # UnRegister HS
				input_string "Please input 1 digit number for HS: "

				SEL=$IN_SEL
				sip_config 'DECT_UnReg' "${SEL:0:1}"
				;;
			"6") # Enable/Disable COSIC MP Testing Mode
				sip_config 'DECT_MPTestMode'
				;;
			"7") # Reset COSIC
				sip_config 'DECT_Reset_COSIC'
				;;
			"8") # TBR6 Mode
				sip_config 'DECT_TBR6Mode'
				;;
			"9") # OSC
				input_string "Please Endter 4 digit Hex OSC HIGH,LOW: "

				SEL=$IN_SEL
				sip_config 'DECT_OSCSet' "${SEL:0:4}"
				;;
			"a") # Send Notify
				sip_config 'DECT_SendNotify'
				;;
			"b") # Send Notify
				sip_config 'DECT_SendDateTime'
				;;
			"c") # Send MWI on
				sip_config 'DECT_SendMWI' 1
				;;
			"C") # Send MWI off
				sip_config 'DECT_SendMWI' 0
				;;
			"e") # Single Call Mode on
				sip_config 'DECT_singleCallMode' 1
				;;
			"E") # Single Call Mode off
				sip_config 'DECT_singleCallMode' 0
				;;
			"f") # ALM TX RX
				input_string "Please input ALM TX Gain value :"
				TxSEL=$IN_SEL

				input_string "Please input ALM RX Gain value :"
				RxSEL=$IN_SEL
				sip_config 'DECT_ALMGain' "${TxSEL:0:8}" "${RxSEL:0:8}"
				;;
			"g") # Dump Data Channel
				sip_config 'DECT_DumpDataCh'
				;;
			"l") # DECT_PrintVersion
				sip_config 'DECT_PrintVersion'
				;;
			"t") # TPC Transmit Power Calibration
				input_string "Please Endter 2 digit Hex g_ucTxBias: "
				TxSEL=$IN_SEL
				sip_config 'DECT_TPC_Set' "${TxSEL:0:2}" 
				;;
			"w") # Write Parameter to Flash
				sip_config 'DECT_WriteParam'
				#uboot_env --set_dect_cal_data --file /tmp/etc/dect_param_cfg
				;;
			"r") # Set RFPI
				dect_mptest DECT_UI_GET_RFPI
				input_string "Please input new RFPI"
				DATA=$IN_SEL
				if [ -n "$DATA" ] ; then
					if [ "${DATA:2:1}" == '-' ] ; then
						if [ "${DATA:5:1}" == '-' ] ; then
							if [ "${DATA:8:1}" == '-' ] ; then
								if [ "${DATA:11:1}" == '-' ] ; then
									sip_config 'DECT_SetRFPI' "${DATA:0:15}" 
								fi
							fi
						fi
					fi
				fi
				;;
			"R" ) # Dump Paramters for verification
				vol_mgmt read_cfg_gz dectconfig /tmp/dect_param_cfg.tar.gz; tar xzf /tmp/dect_param_cfg.tar.gz -C /tmp/etc/; rm -f /tmp/dect_param_cfg.tar.gz; hexdump /tmp/etc/dect_param_cfg | awk '{print toupper($2$3$4$5$6$7$8$9)}' | sed ':label;N;s/\n//;b label'
				;;

			#---------------------------------------------------------
			"x"|"X") # exit
				break
				;;
			*) # bad choice
				echo "BAD CHOICE !!!"
				;;
		esac
	done
}

display_selection_dect_mptest()
{
	echo
	echo "  0. TBR6 Mode on/off (fuction_flag==0)"
	echo "  1. Start MP Test mode (cosic_mp_enable==1)"
	echo "  2. Stop MP Test mode (fuction_flag==1)"
	echo "  3. rfmode_set (fuction_flag==6)"
	echo "  4. reset Cosic (fuction_flag==7)"
	echo "  5. diag_bmc_set (fuction_flag==8)"
	echo "  6. xram_set Cosic (fuction_flag==9)"
	echo "  7. freq_off_set Cosic (fuction_flag==10)"
	echo "  8. TestAppInitMac (fuction_flag==15)"
	echo "  9. PWR Set (fuction_flag==18)"
	echo "  a. xram_get (fuction_flag==11)"
	echo "  b. rfmode_get Cosic (fuction_flag==12) use dect_mptest"
	echo "  d. bmc_get (fuction_flag==14) use dect_mptest"
	echo "  e. tpc_get (fuction_flag==21) use dect_mptest"

	echo "  g. need to set MP RFPI (fuction_flag==3)"
	echo "  i. Clear to default in flash (fuction_flag==20)"

	echo "------------------------------------------------------"
	echo "  x. exit"
	echo
}
dect_MPtest()
{
	while [ "1" ] ; do
		echo "======================================================"
		display_selection_dect_mptest

		input_string "Please select an item"

		SEL=$IN_SEL
		
		case "${SEL:0:1}" in
			"0") # TBR6 Mode on/off (fuction_flag==0)
				sip_config 'DECT_TBR6Mode'
				;;
			"1") # Start MP Test Mode
				sip_config 'DECT_MPTestMode'	1
				;;
			"2") # Start MP Test Mode
				sip_config 'DECT_MPTestMode'	0
				;;
			"3") # RF Mode setting
				input_string "Please Endter 2 digit Hex ucRfTestMode:"
				ucRfTestMode=$IN_SEL
				input_string "Please Endter 2 digit Hex ucChannelNo:"
				ucChannelNo=$IN_SEL
				input_string "Please Endter 2 digit Hex ucSlotNo:"
				ucSlotNo=$IN_SEL

				sip_config 'DECT_RfTestModeSet' "${ucRfTestMode:0:2}" "${ucChannelNo:0:2}" "${ucSlotNo:0:2}"
				;;
			"4") # Reset Cosic..
				sip_config 'DECT_Reset_COSIC'
				;;
			"5") # BMC Set
				input_string "Please input filename:"

				sip_config 'DECT_BMCSet' "$IN_SEL"
				;;
			"6") # DECT_XRAMSet
				input_string "Please input filename:"
				SEL=$IN_SEL
				input_string "Please input Byes:"
				sip_config 'DECT_XRAMSet' "$SEL" "$IN_SEL"
				;;
			"7") # IFX_DECT_DIAG_FreqOffSet(PARA[0],PARA[1],PARA[2]);

				input_string "Please Endter 2 digit Hex ucRfTestMode:"
				PARA0=$IN_SEL
				input_string "Please Endter 2 digit Hex ucChannelNo:"
				PARA1=$IN_SEL
				input_string "Please Endter 2 digit Hex ucSlotNo:"
				PARA2=$IN_SEL

				sip_config 'DECT_FreqOffSet' "${PARA0:0:2}" "${PARA1:0:2}" "${PARA2:0:2}"
				;;
			"8") # DECT_COSIC_reinit TestAppInitMac
				sip_config 'DECT_COSIC_reinit'
				;;
			"9") # PWR Set
				input_string "Please input filename:"
				SEL=$IN_SEL
				input_string "Please input Byes:"
				sip_config 'DECT_PWR_Set' "$SEL" "$IN_SEL"
				;;

			#---------------------------------------------------------
			"x"|"X") # exit
				break
				;;
			*) # bad choice
				echo "BAD CHOICE !!!"
				;;
		esac
	done
}

################### main ######################
display_banner

while [ "1" ] ; do
	echo "====================================="
	display_selection
	rm -f /tmp/etc/fxs-stat

	input_string "Please select an item"

	SEL=$IN_SEL
	case "${SEL:0:1}" in
		"0") # list settings
			display_settings
			;;
		"1") # change operation mode
			change_opmode
			;;
		"2") # change system base MAC address
			display_settings 2
			input_string "Please input new MAC address"
			DATA=$IN_SEL
			if [ -n "$DATA" ] ; then
				uboot_env --set --name ethaddr --value $DATA >&-
			fi
			;;
		"3") # change serial number
			display_settings 3
			input_string "Please input new serial number"
			DATA=$IN_SEL
			if [ -n "$DATA" ] ; then
				uboot_env --set --name serial --value $DATA >&-
			fi
			;;
		"4") # change hardware version
			display_settings 4
			input_string "Please input new hardware version"
			DATA=$IN_SEL
			if [ -n "$DATA" ] ; then
				uboot_env --set --name hw_version --value $DATA >&-
			fi
			;;
		"5") # change software version
			display_settings 5
			input_string "Please input new software version"
			DATA=$IN_SEL
			if [ -n "$DATA" ] ; then
				uboot_env --set --name sw_version --value $DATA >&-
			fi
			;;
		"6") # list DSL status
			libmapi_dsl_cli
			;;
		"7") # LED test
			led_test
			;;
		"8") # VoIP test
			voip_test
			;;
		"9") # DECT test
			dect_test
			;;
		"b") # Button test
			button_test
			;;
		"d") # DECT MPtest
			dect_MPtest
			;;
		"w") # WiFi calibration
			if [ -n "$WLAN_TEST_PROG" ] ; then
				$WLAN_TEST_PROG
			fi
			;;
		"t") # Touch pad calibration
			if [ -n "$TP_TEST_PROG" ] ; then
				$TP_TEST_PROG
			fi
			;;
		"l") # LCD test
			if [ -n "$LCD_TEST_PROG" ] ; then
				$LCD_TEST_PROG
			fi
			;;
		#---------------------------------------------------------
		"r"|"R") # reboot
			reboot -f
			break
			;;
		"x"|"X") # exit
			break
			;;
		*) # bad choice
			echo "BAD CHOICE !!!"
			;;
	esac
done

