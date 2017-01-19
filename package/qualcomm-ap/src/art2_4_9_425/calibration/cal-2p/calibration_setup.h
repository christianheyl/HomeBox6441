#define CALIBRATION_SCHEME	"CALIBRATION_SCHEME"
// scheme : 0, use init txgain 1 point calibration
// scheme : 1, use init gainIndex 1 point calibration
// scheme : 2, use init gainIndex and dacgain 2 point calibration
#define POWER_GOAL_MODE		"POWER_GOAL_MODE"
// mode: 0, calculated from nart and pass to calDLL
// mode: 1, use the POWER_GOAL defined here
#define GAIN_INDEX_SCHEME	"GAIN_INDEX_SCHEME"
// scheme : 0 the 2nd calibration point gain_index init with a value
// scheme : 1 the 2nd calibration point gain_index init with a delta plus 
//			the calibrationed 1st calibration point's gain_index

#define GAIN_CHANN_2G		"GAIN_CHANN_2G"
#define GAIN_CHANN_5G		"GAIN_CHANN_5G"
#define GAIN_INDEX_2G_CH0	"GAIN_INDEX_2G_CH0"
#define GAIN_INDEX_2G_CH1	"GAIN_INDEX_2G_CH1"
#define GAIN_INDEX_2G_CH2	"GAIN_INDEX_2G_CH2"
#define GAIN_INDEX2_2G_CH0		"GAIN_INDEX2_2G_CH0"
#define GAIN_INDEX2_2G_CH1		"GAIN_INDEX2_2G_CH1"
#define GAIN_INDEX2_2G_CH2		"GAIN_INDEX2_2G_CH2"
#define GAIN_INDEX2_DELTA_2G "GAIN_INDEX2_DELTA_2G"
#define DAC_GAIN_2G			"DAC_GAIN_2G"
#define DAC_GAIN2_2G		"DAC_GAIN2_2G"
#define GAIN_INDEX_5G_CH0		"GAIN_INDEX_5G_CH0"
#define GAIN_INDEX_5G_CH1		"GAIN_INDEX_5G_CH1"
#define GAIN_INDEX_5G_CH2		"GAIN_INDEX_5G_CH2"
#define GAIN_INDEX2_5G_CH0		"GAIN_INDEX2_5G_CH0"
#define GAIN_INDEX2_5G_CH1		"GAIN_INDEX2_5G_CH1"
#define GAIN_INDEX2_5G_CH2		"GAIN_INDEX2_5G_CH2"
#define DAC_GAIN_5G			"DAC_GAIN_5G"
#define DAC_GAIN2_5G		"DAC_GAIN2_5G"
#define GAIN_INDEX2_DELTA_5G "GAIN_INDEX2_DELTA_5G"
#define POWER_GOAL_2G		"POWER_GOAL_2G"
#define POWER_GOAL_5G		"POWER_GOAL_5G"
#define POWER_GOAL2_2G		"POWER_GOAL2_2G"
#define POWER_GOAL2_5G		"POWER_GOAL2_5G"

#define POWER_DEVIATION		"POWER_DEVIATION"
#define POWER_DEVIATION2	"POWER_DEVIATION2"

#define TXGAIN_SLOPE		"TXGAIN_SLOPE"
#define TXGAIN_SLOPE2		"TXGAIN_SLOPE2"

#define CALIBRATION_ATTEMPT		"CALIBRATION_ATTEMPT"
#define CALIBRATION_ATTEMPT2	"CALIBRATION_ATTEMPT2"
   
#define RESET_UNUSED_PIERS  "RESET_UNUSED_PIERS"

extern int setup_file(char *filename);

enum
{
	CalSetScheme=500,
	CalSetPowerGoalMode,
	CalSetGainIndexScheme,
	CalSetCalibrationAttempt,
	CalSetTxGainSlope,
	CalSetPowerDeviation,
	CalSet2gFreq,
	CalSet2gGainIndex1Ch0,
	CalSet2gGainIndex1Ch1,
	CalSet2gGainIndex1Ch2,
	CalSet2gDacGain1,
	CalSet2gPowerGoal1,
	CalSet2gGainIndex2Ch0,
	CalSet2gGainIndex2Ch1,
	CalSet2gGainIndex2Ch2,
	CalSet2gDacGain2,
	CalSet2gPowerGoal2,
	CalSet5gFreq,
	CalSet5gGainIndex1Ch0,
	CalSet5gGainIndex1Ch1,
	CalSet5gGainIndex1Ch2,
	CalSet5gDacGain1,
	CalSet5gPowerGoal1,
	CalSet5gGainIndex2Ch0,
	CalSet5gGainIndex2Ch1,
	CalSet5gGainIndex2Ch2,
	CalSet5gDacGain2,
	CalSet5gPowerGoal2,
	CalSetResetUnusedCalPiers,
};

#define	CALIBRATION_SET_SCHEME {CalSetScheme,{"scheme","0","0"},"Set calibration scheme",'u',0,1,1,1,\
	0,0,0,0,0}

#define	CALIBRATION_SET_POWER_GOAL {CalSetPowerGoalMode,{"PowerGoalMode","power_goal_mode",""},"Set calibration power goal",'u',0,1,1,1,\
	0,0,0,0,0}

#define	CALIBRATION_SET_GAIN_INDEX_SCHEME {CalSetGainIndexScheme,{"GainIndexScheme","gain_index_scheme","0"},"Set calibration gain index scheme",'u',0,1,1,1,\
	0,0,0,0,0}

#define	CALIBRATION_SET_CALIBRATION_ATTEMPT {CalSetCalibrationAttempt,{"CalibrationAttempt","calibration_attempt","0"},"Set Maximum Attempt count",'u',0,1,1,1,\
	0,0,0,0,0}

#define	CALIBRATION_SET_TXGAIN_SLOPE {CalSetTxGainSlope,{"TxGainSlope","tx_gain_slope","0"},"Set tx gain slope",'u',0,1,1,1,\
	0,0,0,0,0}

#define	CALIBRATION_SET_POWER_DEVIATION {CalSetPowerDeviation,{"PowerDeviation","power_deviation","0"},"Set power deviation between attempts",'u',0,1,1,1,\
	0,0,0,0,0}

#define	CALIBRATION_SET_2G_FREQ {CalSet2gFreq,{"2gFreq","2gFrequncies","0"},"Set 2G calibration frequencies",'u',0,GAIN_CHANN_MAX_2G,1,1,\
	0,0,0,0,0}

#define	CALIBRATION_SET_2G_GAIN_INDEX1_CH0 {CalSet2gGainIndex1Ch0,{"2gGainIndex1Ch0","0","0"},"Set chain0 gain indicies of first calibration point for 2g band",'u',0,GAIN_CHANN_MAX_2G,1,1,\
	0,0,0,0,0}

#define	CALIBRATION_SET_2G_GAIN_INDEX1_CH1 {CalSet2gGainIndex1Ch1,{"2gGainIndex1Ch1","0","0"},"Set chain1 gain indicies of first calibration point for 2g band",'u',0,GAIN_CHANN_MAX_2G,1,1,\
	0,0,0,0,0}

#define	CALIBRATION_SET_2G_GAIN_INDEX1_CH2 {CalSet2gGainIndex1Ch2,{"2gGainIndex1Ch2","0","0"},"Set chain2 gain indicies of first calibration point for 2g band",'u',0,GAIN_CHANN_MAX_2G,1,1,\
	0,0,0,0,0}

#define	CALIBRATION_SET_2G_DAC_GAIN1 {CalSet2gDacGain1,{"2gDacGain1","0","0"},"Set dac gain of first calibration point for 2g band",'d',0,GAIN_CHANN_MAX_2G,1,1,\
	0,0,0,0,0}

#define	CALIBRATION_SET_2G_POWER_GOAL1 {CalSet2gPowerGoal1,{"2gPowerGoal1","0","0"},"Set power goal of first calibration point for 2g band",'f',"dBm",GAIN_CHANN_MAX_2G,1,1,\
	0,0,0,0,0}

#define	CALIBRATION_SET_2G_GAIN_INDEX2_CH0 {CalSet2gGainIndex2Ch0,{"2gGainIndex2Ch0","0","0"},"Set chain0 gain indicies of 2nd calibration point for 2g band",'u',0,GAIN_CHANN_MAX_2G,1,1,\
	0,0,0,0,0}

#define	CALIBRATION_SET_2G_GAIN_INDEX2_CH1 {CalSet2gGainIndex2Ch1,{"2gGainIndex2Ch1","0","0"},"Set chain1 gain indicies of 2nd calibration point for 2g band",'u',0,GAIN_CHANN_MAX_2G,1,1,\
	0,0,0,0,0}

#define	CALIBRATION_SET_2G_GAIN_INDEX2_CH2 {CalSet2gGainIndex2Ch2,{"2gGainIndex2Ch2","0","0"},"Set chain2 gain indicies of 2nd calibration point for 2g band",'u',0,GAIN_CHANN_MAX_2G,1,1,\
	0,0,0,0,0}

#define	CALIBRATION_SET_2G_DAC_GAIN2 {CalSet2gDacGain2,{"2gDacGain2","0","0"},"Set dac gain of 2nd calibration point for 2g band",'d',0,GAIN_CHANN_MAX_2G,1,1,\
	0,0,0,0,0}

#define	CALIBRATION_SET_2G_POWER_GOAL2 {CalSet2gPowerGoal2,{"2gPowerGoal2","0","0"},"Set power goal of 2nd calibration point for 2g band",'f',"dBm",GAIN_CHANN_MAX_2G,1,1,\
	0,0,0,0,0}

#define	CALIBRATION_SET_5G_FREQ {CalSet5gFreq,{"5gFreq","5gFrequncies","0"},"Set 5g calibration frequencies",'u',0,GAIN_CHANN_MAX_5G,1,1,\
	0,0,0,0,0}

#define	CALIBRATION_SET_5G_GAIN_INDEX1_CH0 {CalSet5gGainIndex1Ch0,{"5gGainIndex1Ch0","0","0"},"Set chain0 gain indicies of first calibration point for 5g band",'u',0,GAIN_CHANN_MAX_5G,1,1,\
	0,0,0,0,0}

#define	CALIBRATION_SET_5G_GAIN_INDEX1_CH1 {CalSet5gGainIndex1Ch1,{"5gGainIndex1Ch1","0","0"},"Set chain1 gain indicies of first calibration point for 5g band",'u',0,GAIN_CHANN_MAX_5G,1,1,\
	0,0,0,0,0}

#define	CALIBRATION_SET_5G_GAIN_INDEX1_CH2 {CalSet5gGainIndex1Ch2,{"5gGainIndex1Ch2","0","0"},"Set chain2 gain indicies of first calibration point for 5g band",'u',0,GAIN_CHANN_MAX_5G,1,1,\
	0,0,0,0,0}

#define	CALIBRATION_SET_5G_DAC_GAIN1 {CalSet5gDacGain1,{"5gDacGain1","0","0"},"Set dac gain of first calibration point for 5g band",'d',0,GAIN_CHANN_MAX_5G,1,1,\
	0,0,0,0,0}

#define	CALIBRATION_SET_5G_POWER_GOAL1 {CalSet5gPowerGoal1,{"5gPowerGoal1","0","0"},"Set power goal of first calibration point for 5g band",'f',"dBm",GAIN_CHANN_MAX_5G,1,1,\
	0,0,0,0,0}

#define	CALIBRATION_SET_5G_GAIN_INDEX2_CH0 {CalSet5gGainIndex2Ch0,{"5gGainIndex2Ch0","0","0"},"Set chain0 gain indicies of 2nd calibration point for 5g band",'u',0,GAIN_CHANN_MAX_5G,1,1,\
	0,0,0,0,0}

#define	CALIBRATION_SET_5G_GAIN_INDEX2_CH1 {CalSet5gGainIndex2Ch1,{"5gGainIndex2Ch1","0","0"},"Set chain1 gain indicies of 2nd calibration point for 5g band",'u',0,GAIN_CHANN_MAX_5G,1,1,\
	0,0,0,0,0}

#define	CALIBRATION_SET_5G_GAIN_INDEX2_CH2 {CalSet5gGainIndex2Ch2,{"5gGainIndex2Ch2","0","0"},"Set chain2 gain indicies of 2nd calibration point for 5g band",'u',0,GAIN_CHANN_MAX_5G,1,1,\
	0,0,0,0,0}

#define	CALIBRATION_SET_5G_DAC_GAIN2 {CalSet5gDacGain2,{"5gDacGain2","0","0"},"Set dac gain of 2nd calibration point for 5g band",'d',0,GAIN_CHANN_MAX_5G,1,1,\
	0,0,0,0,0}

#define	CALIBRATION_SET_5G_POWER_GOAL2 {CalSet5gPowerGoal2,{"5gPowerGoal2","0","0"},"Set power goal of 2nd calibration point for 5g band",'f',"dBm",GAIN_CHANN_MAX_5G,1,1,\
	0,0,0,0,0}

#define	CALIBRATION_SET_RESET_UNUSED_CAL_PIERS {CalSetResetUnusedCalPiers,{"resetUnusedPiers","0","0"},"Reset unused calibration piers",'d',0,1,1,1,\
	0,0,0,0,0}
