#!/bin/sh
# Copyright (C) 2011 lantiq.com

etc_dir=/etc/config/
optic_dir=/etc/optic/

optic() {
	#echo "optic $*"
	result=`/opt/lantiq/bin/optic $*`
	#echo "result $result"
	status=${result%% *}
	key=$(echo $status | cut -d= -f1)
	val=$(echo $status | cut -d= -f2)
	if [ "$key" == "errorcode" -a "$val" != "0" ]; then
		echo "optic $* failed: $result"
	fi
}

. /etc/functions.sh

optic_config_goi() {
	local TempMonitorInterval
	local TempMonitorThreshold_MPDcorrection

	local LaserAgeStoreCycle
	local LaserAgeFile
	local LaserAge

	local Polarity_Rx
	local Polarity_Bias
	local Polarity_Mod

	local Temp_IntCorr_AlarmYellow_Set
	local Temp_IntCorr_AlarmYellow_Clear
	local Temp_IntCorr_AlarmRed_Set
	local Temp_IntCorr_AlarmRed_Clear

	local TxEnableDelay
	local TxDisableDelay
	local TxFifoSize
	local Tref

	# global: configuration values
	config_get TempMonitorInterval global TempMonitorInterval
	config_get TempMonitorThreshold_MPDcorrection global TempMonitorThreshold_MPDcorrection

	config_get LaserAgeStoreCycle global LaserAgeStoreCycle
	config_get LaserAgeFile global LaserAgeFile

	read LaserAge < $optic_dir/$LaserAgeFile

	config_get Polarity_Rx global Polarity_Rx
	config_get Polarity_Bias global Polarity_Bias
	config_get Polarity_Mod global Polarity_Mod

	config_get Temp_IntCorr_AlarmYellow_Set global Temp_IntCorr_AlarmYellow_Set
	config_get Temp_IntCorr_AlarmYellow_Clear global Temp_IntCorr_AlarmYellow_Clear
	config_get Temp_IntCorr_AlarmRed_Set global Temp_IntCorr_AlarmRed_Set
	config_get Temp_IntCorr_AlarmRed_Clear global Temp_IntCorr_AlarmRed_Clear

	# transmitter: configuration values
	config_get TxEnableDelay transmitter TxEnableDelay
	config_get TxDisableDelay transmitter TxDisableDelay
	config_get TxFifoSize transmitter TxFifoSize

	# temperature_tables: configuration values
	config_get Tref temperature_tables Tref


	# or use "$optic goics" for cli interface
	optic config_goi "$TempMonitorInterval $TempMonitorThreshold_MPDcorrection \
$LaserAgeStoreCycle $LaserAge $Polarity_Rx $Polarity_Bias $Polarity_Mod \
$Temp_IntCorr_AlarmYellow_Set $Temp_IntCorr_AlarmYellow_Clear $Temp_IntCorr_AlarmRed_Set $Temp_IntCorr_AlarmRed_Clear \
$TxEnableDelay $TxDisableDelay $TxFifoSize $Tref"
}

optic_config_ranges() {
	local TableTemp_ExtCorr_Min
	local TableTemp_ExtCorr_Max
	local TableTemp_ExtNom_Min
	local TableTemp_ExtNom_Max
	local TableTemp_IntCorr_Min
	local TableTemp_IntCorr_Max
	local TableTemp_IntNom_Min
	local TableTemp_IntNom_Max

	local Ibias_Max
	local Imod_Max
	local IbiasImod_Max
	local CInt_Bias_Max
	local CInt_Mod_Max

	local VAPD_Min
	local VAPD_Max
	local VCore_Min
	local VCore_Max
	local VDDR_Min
	local VDDR_Max

	local VAPD_ext_inductivity
	local VAPD_ext_supply
	local VAPD_efficiency
	local VAPD_current_limit
	local VAPD_switching_frequency
	local Overcurrent_Ibias_Thr
	local Overcurrent_Imod_Thr
	local Overcurrent_IbiasImod_Thr

	# ranges: configuration values
	config_get TableTemp_ExtCorr_Min ranges TableTemp_ExtCorr_Min
	config_get TableTemp_ExtCorr_Max ranges TableTemp_ExtCorr_Max
	config_get TableTemp_ExtNom_Min ranges TableTemp_ExtNom_Min
	config_get TableTemp_ExtNom_Max ranges TableTemp_ExtNom_Max
	config_get TableTemp_IntCorr_Min ranges TableTemp_IntCorr_Min
	config_get TableTemp_IntCorr_Max ranges TableTemp_IntCorr_Max
	config_get TableTemp_IntNom_Min ranges TableTemp_IntNom_Min
	config_get TableTemp_IntNom_Max ranges TableTemp_IntNom_Max

	config_get Ibias_Max ranges Ibias_Max
	config_get Imod_Max ranges Imod_Max
	config_get IbiasImod_Max ranges IbiasImod_Max
	config_get CInt_Bias_Max ranges CInt_Bias_Max
	config_get CInt_Mod_Max ranges CInt_Mod_Max

	config_get VAPD_Min ranges VAPD_Min
	config_get VAPD_Max ranges VAPD_Max
	config_get VCore_Min ranges VCore_Min
	config_get VCore_Max ranges VCore_Max
	config_get VDDR_Min ranges VDDR_Min
	config_get VDDR_Max ranges VDDR_Max
	config_get Overcurrent_Ibias_Thr ranges Overcurrent_Ibias_Thr 60
	config_get Overcurrent_Imod_Thr ranges Overcurrent_Imod_Thr 60
	config_get Overcurrent_IbiasImod_Thr ranges Overcurrent_Imod_Thr 100

	# temperature_tables: configuration values
	config_get VAPD_ext_inductivity temperature_tables VAPD_ext_inductivity
	config_get VAPD_ext_supply temperature_tables VAPD_ext_supply
	config_get VAPD_efficiency temperature_tables VAPD_efficiency
	config_get VAPD_current_limit temperature_tables VAPD_current_limit
	config_get VAPD_switching_frequency temperature_tables VAPD_switching_frequency

	# start with this init because application and driver need
	# temperature range for creating temperature tables
	optic config_ranges "$TableTemp_ExtCorr_Min $TableTemp_ExtCorr_Max $TableTemp_ExtNom_Min $TableTemp_ExtNom_Max \
$TableTemp_IntCorr_Min $TableTemp_IntCorr_Max $TableTemp_IntNom_Min $TableTemp_IntNom_Max \
$Ibias_Max $Imod_Max $IbiasImod_Max $CInt_Bias_Max $CInt_Mod_Max $VAPD_Min $VAPD_Max $VCore_Min $VCore_Max $VDDR_Min $VDDR_Max \
$VAPD_ext_inductivity $VAPD_ext_supply $VAPD_efficiency $VAPD_current_limit $VAPD_switching_frequency \
$Overcurrent_Ibias_Thr $Overcurrent_Imod_Thr $Overcurrent_IbiasImod_Thr"
}

optic_config_fcsi() {
	local GVS

	# fcsi: configuration values
	config_get GVS fcsi GVS
	optic config_fcsi "$GVS "
}

optic_config_measure() {
	local Tscal_ref
	local pnR
	local pnIsource
	local RSSI_1490mode
	local RSSI_1490dark_corr
	local RSSI_1490shunt_res
	local RSSI_1550vref
	local RF_1550vref
	local RSSI_1490scal_ref
	local RSSI_1550scal_ref
	local RF_1550scal_ref
	local RSSI_1490parabolic_ref
	local RSSI_1490dark_ref
	local RSSI_autolevel
	local RSSI_1490threshold_low
	local RSSI_1490threshold_high

	# transmitter: configuration values
	config_get Tscal_ref measurement Tscal_ref
	config_get pnR measurement pnR
	config_get pnIsource measurement pnIsource

	# measurement: configuration values
	config_get RSSI_1490mode measurement RSSI_1490mode
	config_get RSSI_1490dark_corr measurement RSSI_1490dark_corr
	config_get RSSI_1490shunt_res measurement RSSI_1490shunt_res
	config_get RSSI_1550vref measurement RSSI_1550vref
	config_get RF_1550vref measurement RF_1550vref
	config_get RSSI_1490scal_ref measurement RSSI_1490scal_ref "0.12" # [mW/mA]
	config_get RSSI_1550scal_ref measurement RSSI_1550scal_ref
	config_get RF_1550scal_ref measurement RF_1550scal_ref

	config_get RSSI_1490parabolic_ref measurement RSSI_1490parabolic_ref "0.14" # [mW/mA^2]
	config_get RSSI_1490dark_ref measurement RSSI_1490dark_ref 0

	config_get RSSI_autolevel measurement RSSI_autolevel 1
	config_get RSSI_1490threshold_low measurement RSSI_1490threshold_low 40
	config_get RSSI_1490threshold_high measurement RSSI_1490threshold_high 70

	optic config_measure "$Tscal_ref $pnR $pnIsource $RSSI_1490mode \
$RSSI_1490dark_corr $RSSI_1490shunt_res $RSSI_1550vref $RF_1550vref \
$RSSI_1490scal_ref $RSSI_1550scal_ref $RF_1550scal_ref \
$RSSI_1490parabolic_ref $RSSI_1490dark_ref \
$RSSI_autolevel $RSSI_1490threshold_low $RSSI_1490threshold_high"
}

optic_config_mpd() {
	local TiaGain_PL0
	local TiaGain_PL1
	local TiaGain_PL2
	local TiaGain_Global
	local MPD_Calibration_PL0
	local MPD_Calibration_PL1
	local MPD_Calibration_PL2
	local MPD_Calibration_Global

	local ImodScalCorr_PL0
	local ImodScalCorr_PL1
	local ImodScalCorr_PL2
	local Dcalref0_PL0
	local Dcalref1_PL0
	local Dcalref0_PL1
	local Dcalref1_PL1
	local Dcalref0_PL2
	local Dcalref1_PL2
	local Dref0_PL0
	local Dref1_PL0
	local Dref0_PL1
	local Dref1_PL1
	local Dref0_PL2
	local Dref1_PL2

	local CoarseFineRatio
	local PowerSave

	local CID0_Size
	local CID1_Size
	local CID0_MatchAll
	local CID1_MatchAll
	local CID0_Mask
	local CID1_Mask
	local rogue_bias_thr
	local rogue_mod_thr
	local rogue_onu_interburst

	# monitor: configuration values
	config_get TiaGain_PL0 monitor TiaGain_PL0
	config_get TiaGain_PL1 monitor TiaGain_PL1
	config_get TiaGain_PL2 monitor TiaGain_PL2
	config_get TiaGain_Global monitor TiaGain_Global
	config_get MPD_Calibration_PL0 monitor MPD_Calibration_PL0
	config_get MPD_Calibration_PL1 monitor MPD_Calibration_PL1
	config_get MPD_Calibration_PL2 monitor MPD_Calibration_PL2
	config_get MPD_Calibration_Global monitor MPD_Calibration_Global

	config_get ImodScalCorr_PL0 monitor ImodScalCorr_PL0
	config_get ImodScalCorr_PL1 monitor ImodScalCorr_PL1
	config_get ImodScalCorr_PL2 monitor ImodScalCorr_PL2
	config_get Dcalref0_PL0 monitor Dcalref0_PL0
	config_get Dcalref1_PL0 monitor Dcalref1_PL0
	config_get Dcalref0_PL1 monitor Dcalref0_PL1
	config_get Dcalref1_PL1 monitor Dcalref1_PL1
	config_get Dcalref0_PL2 monitor Dcalref0_PL2
	config_get Dcalref1_PL2 monitor Dcalref1_PL2
	config_get Dref0_PL0 monitor Dref0_PL0
	config_get Dref1_PL0 monitor Dref1_PL0
	config_get Dref0_PL1 monitor Dref0_PL1
	config_get Dref1_PL1 monitor Dref1_PL1
	config_get Dref0_PL2 monitor Dref0_PL2
	config_get Dref1_PL2 monitor Dref1_PL2

	config_get CoarseFineRatio monitor CoarseFineRatio
	config_get PowerSave monitor PowerSave

	config_get CID0_Size monitor CID0_Size
	config_get CID1_Size monitor CID1_Size
	config_get CID0_MatchAll monitor CID0_MatchAll
	config_get CID1_MatchAll monitor CID1_MatchAll
	config_get CID0_Mask monitor CID0_Mask
	config_get CID1_Mask monitor CID1_Mask
	config_get rogue_onu_interburst monitor rogue_onu_interburst 0

	optic config_mpd "$TiaGain_PL0 $TiaGain_PL1 $TiaGain_PL2 $TiaGain_Global \
$MPD_Calibration_PL0 $MPD_Calibration_PL1 $MPD_Calibration_PL2 $MPD_Calibration_Global \
$ImodScalCorr_PL0 $ImodScalCorr_PL1 $ImodScalCorr_PL2 \
$Dcalref0_PL0 $Dcalref1_PL0 $Dcalref0_PL1 $Dcalref1_PL1 $Dcalref0_PL2 $Dcalref1_PL2 \
$Dref0_PL0 $Dref1_PL0 $Dref0_PL1 $Dref1_PL1 $Dref0_PL2 $Dref1_PL2 \
$CoarseFineRatio $PowerSave \
$CID0_Size $CID1_Size $CID0_MatchAll $CID1_MatchAll $CID0_Mask $CID1_Mask \
$rogue_onu_interburst"

}

optic_config_omu() {
	local SignalDetectAvailable
	local SignalDetectPort
	local LolThreshold_set
	local LolThreshold_clear
	local LaserSignalSingleEnded

	# omu: configuration values
	config_get SignalDetectAvailable omu SignalDetectAvailable
	config_get SignalDetectPort omu SignalDetectPort
	config_get LolThreshold_set omu LolThreshold_set
	config_get LolThreshold_clear omu LolThreshold_clear
	config_get LaserSignalSingleEnded omu LaserSignalSingleEnded

	# or use "$optic omucs" for cli interface
	optic config_omu "$SignalDetectAvailable $SignalDetectPort $LolThreshold_set $LolThreshold_clear \
$LaserSignalSingleEnded"
}

optic_config_bosa() {
	local LoopMode
	local DeadZoneElimination
	local LolThreshold_set
	local LolThreshold_clear
	local LosThreshold
	local RxOverloadThreshold
	local CInt_Bias_init
	local CInt_Mod_init
	local PICtrl
	local UpdateThreshold_Bias
	local UpdateThreshold_Mod
	local LearningThreshold_Bias
	local LearningThreshold_Mod
	local StableThreshold_Bias
	local StableThreshold_Mod
	local ResetThreshold_Bias
	local ResetThreshold_Mod
	local P0_0
	local P0_1
	local P0_2
	local P1_0
	local P1_1
	local P1_2
	local Pth

	# bosa: configuration values
	config_get LoopMode bosa LoopMode
	config_get DeadZoneElimination bosa DeadZoneElimination
	config_get LolThreshold_set bosa LolThreshold_set
	config_get LolThreshold_clear bosa LolThreshold_clear
	config_get LosThreshold bosa LosThreshold
	config_get RxOverloadThreshold bosa RxOverloadThreshold
	config_get CInt_Bias_init bosa CInt_Bias_init
	config_get CInt_Mod_init bosa CInt_Mod_init
	config_get PICtrl bosa PICtrl
	config_get UpdateThreshold_Bias bosa UpdateThreshold_Bias 1
	config_get UpdateThreshold_Mod bosa UpdateThreshold_Mod 1
	config_get LearningThreshold_Bias bosa LearningThreshold_Bias
	config_get LearningThreshold_Mod bosa LearningThreshold_Mod
	config_get StableThreshold_Bias bosa StableThreshold_Bias
	config_get StableThreshold_Mod bosa StableThreshold_Mod
	config_get ResetThreshold_Bias bosa ResetThreshold_Bias
	config_get ResetThreshold_Mod bosa ResetThreshold_Mod

	# transmit_power: configuration values
	config_get P0_0 transmit_power P0_0
	config_get P0_1 transmit_power P0_1
	config_get P0_2 transmit_power P0_2
	config_get P1_0 transmit_power P1_0
	config_get P1_1 transmit_power P1_1
	config_get P1_2 transmit_power P1_2
	config_get Pth transmit_power Pth

	# or use "$optic bosacs" for cli interface
	optic config_bosa "$LoopMode $DeadZoneElimination $LolThreshold_set $LolThreshold_clear \
$LosThreshold $RxOverloadThreshold $CInt_Bias_init $CInt_Mod_init $PICtrl \
$UpdateThreshold_Bias $UpdateThreshold_Mod $LearningThreshold_Bias $LearningThreshold_Mod \
$StableThreshold_Bias $StableThreshold_Mod $ResetThreshold_Bias $ResetThreshold_Mod \
$P0_0 $P0_1 $P0_2 $P1_0 $P1_1 $P1_2 $Pth"
}

optic_config_dcdc_apd() {
	local Rdiv_low
	local Rdiv_high
	local VAPD_ext_supply

	# dcdc_apd: configuration values
	config_get Rdiv_low dcdc_apd Rdiv_low
	config_get Rdiv_high dcdc_apd Rdiv_high
	config_get VAPD_ext_supply temperature_tables VAPD_ext_supply

	optic config_dcdc_apd "$Rdiv_low $Rdiv_high $VAPD_ext_supply"
}

optic_config_dcdc_core() {
	local R_min
	local R_max
	local I_min
	local I_max
	local V_tolerance_input
	local V_tolerance_target
	local pmos_on_delay	
	local nmos_on_delay

	# dcdc_core: configuration values
	config_get R_min dcdc_core R_min
	config_get R_max dcdc_core R_max
	config_get I_min dcdc_core I_min
	config_get I_max dcdc_core I_max
	config_get V_tolerance_input dcdc_core V_tolerance_input
	config_get V_tolerance_target dcdc_core V_tolerance_target
	config_get pmos_on_delay dcdc_core pmos_on_delay 4
	config_get nmos_on_delay dcdc_core nmos_on_delay 7

	optic config_dcdc_core "$R_min $R_max $I_min $I_max $V_tolerance_input $V_tolerance_target $pmos_on_delay $nmos_on_delay"
}

optic_config_dcdc_ddr() {
	local R_min
	local R_max
	local I_min
	local I_max
	local V_tolerance_input
	local V_tolerance_target
	local pmos_on_delay
	local nmos_on_delay
	
	# dcdc_ddr: configuration values
	config_get R_min dcdc_ddr R_min
	config_get R_max dcdc_ddr R_max
	config_get I_min dcdc_ddr I_min
	config_get I_max dcdc_ddr I_max
	config_get V_tolerance_input dcdc_ddr V_tolerance_input
	config_get V_tolerance_target dcdc_ddr V_tolerance_target
	config_get pmos_on_delay dcdc_ddr pmos_on_delay 4
	config_get nmos_on_delay dcdc_ddr nmos_on_delay	7	

	optic config_dcdc_ddr "$R_min $R_max $I_min $I_max $V_tolerance_input $V_tolerance_target $pmos_on_delay $nmos_on_delay"
}

optic_read_table_pth() {
	local TableTemp_ExtCorr_Min
	local TableTemp_ExtCorr_Max
	local Table_Pth_corr

	# ranges: configuration values
	config_get TableTemp_ExtCorr_Min ranges TableTemp_ExtCorr_Min
	config_get TableTemp_ExtCorr_Max ranges TableTemp_ExtCorr_Max

	# temperature_tables: configuration values
	config_get Table_Pth_corr temperature_tables Table_Pth_corr


	# send Pth-Table to application
	optic read_table_pth "$TableTemp_ExtCorr_Min $TableTemp_ExtCorr_Max $Table_Pth_corr"
}

optic_read_table_laserref() {
	local TableTemp_ExtCorr_Min
	local TableTemp_ExtCorr_Max
	local Tci_Ith_low
	local Tci_Ith_high
	local Tci_SE_low
	local Tci_SE_high
	local Tcd_Ith_low
	local Tcd_Ith_high
	local Tcd_SE_low
	local Tcd_SE_high

	local Table_LaserRef

	# ranges: configuration values
	config_get TableTemp_ExtCorr_Min ranges TableTemp_ExtCorr_Min
	config_get TableTemp_ExtCorr_Max ranges TableTemp_ExtCorr_Max

	# temperature_tables: configuration values
	config_get Tci_Ith_low temperature_tables Tci_Ith_low
	config_get Tci_Ith_high temperature_tables Tci_Ith_high
	config_get Tci_SE_low temperature_tables Tci_SE_low
	config_get Tci_SE_high temperature_tables Tci_SE_high
	config_get Tcd_Ith_low temperature_tables Tcd_Ith_low
	config_get Tcd_Ith_high temperature_tables Tcd_Ith_high
	config_get Tcd_SE_low temperature_tables Tcd_SE_low
	config_get Tcd_SE_high temperature_tables Tcd_SE_high

	config_get Table_LaserRef temperature_tables Table_LaserRef


	# send LaserRef-Table to application
	optic read_table_laserref "$TableTemp_ExtCorr_Min $TableTemp_ExtCorr_Max \
$Tci_Ith_low $Tci_Ith_high $Tci_SE_low $Tci_SE_high $Tcd_Ith_low $Tcd_Ith_high $Tcd_SE_low $Tcd_SE_high \
$Table_LaserRef"
}

optic_write_table_laserref() {
	local TableTemp_ExtCorr_Min
	local TableTemp_ExtCorr_Max
	local Tci_Ith_low
	local Tci_Ith_high
	local Tci_SE_low
	local Tci_SE_high
	local Tcd_Ith_low
	local Tcd_Ith_high
	local Tcd_SE_low
	local Tcd_SE_high

	local Table_LaserRef
	local Table_LaserRef_base

	# ranges: configuration values
	config_get TableTemp_ExtCorr_Min ranges TableTemp_ExtCorr_Min
	config_get TableTemp_ExtCorr_Max ranges TableTemp_ExtCorr_Max

	# temperature_tables: configuration values
	config_get Tci_Ith_low temperature_tables Tci_Ith_low
	config_get Tci_Ith_high temperature_tables Tci_Ith_high
	config_get Tci_SE_low temperature_tables Tci_SE_low
	config_get Tci_SE_high temperature_tables Tci_SE_high
	config_get Tcd_Ith_low temperature_tables Tcd_Ith_low
	config_get Tcd_Ith_high temperature_tables Tcd_Ith_high
	config_get Tcd_SE_low temperature_tables Tcd_SE_low
	config_get Tcd_SE_high temperature_tables Tcd_SE_high

	# temperature_tables: configuration values
	config_get Table_LaserRef temperature_tables Table_LaserRef
	config_get Table_LaserRef_base temperature_tables Table_LaserRef_base


	# send LaserRef-Table to application
	optic write_table_laserref "$TableTemp_ExtCorr_Min $TableTemp_ExtCorr_Max \
$Tci_Ith_low $Tci_Ith_high $Tci_SE_low $Tci_SE_high $Tcd_Ith_low $Tcd_Ith_high $Tcd_SE_low $Tcd_SE_high \
$Table_LaserRef $Table_LaserRef_base"
}

optic_read_table_ibiasimod() {
	local TableTemp_ExtCorr_Min
	local TableTemp_ExtCorr_Max
	local P0_0
	local P0_1
	local P0_2
	local P1_0
	local P1_1
	local P1_2
	local Pth
	local Tci_Ith_low
	local Tci_Ith_high
	local Tci_SE_low
	local Tci_SE_high
	local Tcd_Ith_low
	local Tcd_Ith_high
	local Tcd_SE_low
	local Tcd_SE_high
	local Ibias_Max
	local Imod_Max
	local IbiasImod_Max

	local Table_Pth
	local Table_LaserRef

	# ranges: configuration values
	config_get TableTemp_ExtCorr_Min ranges TableTemp_ExtCorr_Min
	config_get TableTemp_ExtCorr_Max ranges TableTemp_ExtCorr_Max

	config_get Ibias_Max ranges Ibias_Max
	config_get Imod_Max ranges Imod_Max
	config_get IbiasImod_Max ranges IbiasImod_Max

	# temperature_tables: configuration values
	config_get P0_0 temperature_tables P0_0
	config_get P0_1 temperature_tables P0_1
	config_get P0_2 temperature_tables P0_2
	config_get P1_0 temperature_tables P1_0
	config_get P1_1 temperature_tables P1_1
	config_get P1_2 temperature_tables P1_2
	config_get Pth temperature_tables Pth

	config_get Tci_Ith_low temperature_tables Tci_Ith_low
	config_get Tci_Ith_high temperature_tables Tci_Ith_high
	config_get Tci_SE_low temperature_tables Tci_SE_low
	config_get Tci_SE_high temperature_tables Tci_SE_high
	config_get Tcd_Ith_low temperature_tables Tcd_Ith_low
	config_get Tcd_Ith_high temperature_tables Tcd_Ith_high
	config_get Tcd_SE_low temperature_tables Tcd_SE_low
	config_get Tcd_SE_high temperature_tables Tcd_SE_high

	config_get Table_Pth temperature_tables Table_Pth
	config_get Table_LaserRef temperature_tables Table_LaserRef


		# send Pth- and LaserRef-Table + P0/1 and PthRef constants to
		# application for calculation of Ibias/Imod values
	optic read_table_ibiasimod "$TableTemp_ExtCorr_Min $TableTemp_ExtCorr_Max \
$P0_0 $P0_1 $P0_2 $P1_0 $P1_1 $P1_2 $Pth \
$Tci_Ith_low $Tci_Ith_high $Tci_SE_low $Tci_SE_high $Tcd_Ith_low $Tcd_Ith_high $Tcd_SE_low $Tcd_SE_high \
$Ibias_Max $Imod_Max $IbiasImod_Max $Table_Pth $Table_LaserRef"
}

optic_read_table_vapd() {
	local TableTemp_ExtCorr_Min
	local TableTemp_ExtCorr_Max

	local Tref
	local VAPD_bd_ref
	local VAPD_offset
	local VAPD_scal_ref
	local VAPD_Min
	local VAPD_Max

	local VAPD_ext_inductivity
	local VAPD_ext_supply
	local VAPD_efficiency
	local VAPD_current_limit
	local VAPD_switching_frequency

	# ranges: configuration values
	config_get TableTemp_ExtCorr_Min ranges TableTemp_ExtCorr_Min
	config_get TableTemp_ExtCorr_Max ranges TableTemp_ExtCorr_Max
	config_get VAPD_Min ranges VAPD_Min
	config_get VAPD_Max ranges VAPD_Max

	# temperature_tables: configuration values
	config_get Tref temperature_tables Tref
	config_get VAPD_bd_ref temperature_tables VAPD_bd_ref
	config_get VAPD_offset temperature_tables VAPD_offset
	config_get VAPD_scal_ref temperature_tables VAPD_scal_ref

	config_get VAPD_ext_inductivity temperature_tables VAPD_ext_inductivity
	config_get VAPD_ext_supply temperature_tables VAPD_ext_supply
	config_get VAPD_efficiency temperature_tables VAPD_efficiency
	config_get VAPD_current_limit temperature_tables VAPD_current_limit
	config_get VAPD_switching_frequency temperature_tables VAPD_switching_frequency

		# send parameters to application for calculation of VAPD values
	optic read_table_vapd "$TableTemp_ExtCorr_Min $TableTemp_ExtCorr_Max \
$Tref $VAPD_bd_ref $VAPD_offset $VAPD_scal_ref $VAPD_Min $VAPD_Max \
$VAPD_ext_inductivity $VAPD_ext_supply $VAPD_efficiency $VAPD_current_limit $VAPD_switching_frequency"
}

optic_read_table_mpdresp() {
	local TableTemp_ExtCorr_Min
	local TableTemp_ExtCorr_Max

	local Table_MPDresp_corr

	# ranges: configuration values
	config_get TableTemp_ExtCorr_Min ranges TableTemp_ExtCorr_Min
	config_get TableTemp_ExtCorr_Max ranges TableTemp_ExtCorr_Max

	# temperature_tables: configuration values
	config_get Table_MPDresp_corr temperature_tables Table_MPDresp_corr

	# send MPD resp corection table to application for transfering to driver
	optic read_table_mpdresp "$TableTemp_ExtCorr_Min $TableTemp_ExtCorr_Max $Table_MPDresp_corr"
}

optic_read_table_temptrans() {
	local TableTemp_ExtNom_Min
	local TableTemp_ExtNom_Max

	local Table_TempTrans_ext

	# ranges: configuration values
	config_get TableTemp_ExtNom_Min ranges TableTemp_ExtNom_Min
	config_get TableTemp_ExtNom_Max ranges TableTemp_ExtNom_Max

	# temperature_tables: configuration values

	config_get Table_TempTrans_ext temperature_tables Table_TempTrans_ext


	# send MPD resp corection table to application for transfering to driver
	optic read_table_temptrans "$TableTemp_ExtNom_Min $TableTemp_ExtNom_Max $Table_TempTrans_ext"
}

optic_read_table_power() {
	local TableTemp_ExtCorr_Min
	local TableTemp_ExtCorr_Max
	local Table_RSSI1490_corr
	local Table_RSSI1550_corr
	local Table_RF1550_corr

	# ranges: configuration values
	config_get TableTemp_ExtCorr_Min ranges TableTemp_ExtCorr_Min
	config_get TableTemp_ExtCorr_Max ranges TableTemp_ExtCorr_Max

	# temperature_tables: configuration values
	config_get Table_RSSI1490_corr temperature_tables Table_RSSI1490_corr
	config_get Table_RSSI1550_corr temperature_tables Table_RSSI1550_corr
	config_get Table_RF1550_corr temperature_tables Table_RF1550_corr

	# send power-Table to application
	optic read_table_power "$TableTemp_ExtCorr_Min $TableTemp_ExtCorr_Max \
$Table_RSSI1490_corr $Table_RSSI1550_corr $Table_RF1550_corr"
}

optic_init() {
	optic goi_init
}

optic_timestamp() {
	local LaserAgeFile

	# global: configuration values
	config_get LaserAgeFile global LaserAgeFile

	echo -n $1 > $optic_dir/$LaserAgeFile
}

optic_config_all() {
	optic_config_goi
	optic_config_ranges
	optic_config_fcsi
	optic_config_measure
	optic_config_mpd
	optic_config_omu
	optic_config_bosa
	optic_config_dcdc_apd
	optic_config_dcdc_core
	optic_config_dcdc_ddr
}

optic_read_tables() {
	optic_read_table_pth
	optic_read_table_laserref
	optic_read_table_vapd
	optic_read_table_mpdresp
	optic_read_table_power
	optic_read_table_temptrans
}

optic_write_tables() {
	optic_write_table_laserref
}

config_load goi_config

[ "$0" = "/etc/optic/goi_config.sh" ] && optic_${1} ${2}
