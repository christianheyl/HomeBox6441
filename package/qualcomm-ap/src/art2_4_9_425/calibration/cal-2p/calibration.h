#define MCHAIN 3

#define GAIN_CHANN_MAX_2G	3
#define GAIN_CHANN_MAX_5G	8

struct _Cal
{
	int freq;
	int rate;
	int tMask;
	int tryNum;
	int scheme;
	// scheme : 0, use init txgain 1 point calibration
	// scheme : 1, use init gainIndex 1 point calibration
	// scheme : 2, use init gainIndex and dacgain 2 point calibration
	int PowerGoalMode;	// 0: calculated from nart and pass to calDLL, 
						// 1: use the POWER_GOAL defined here
	int gainIndexScheme;
	// scheme : 0 the 2nd calibration point gain_index init with a value
	// scheme : 1 the 2nd calibration point gain_index init with a delta plus 
	//			the calibrationed 1st calibration point's gain_index

	int gainChann_2g[GAIN_CHANN_MAX_2G], gainChann_5g[GAIN_CHANN_MAX_5G];
	int gainIndex_2g_ch0[GAIN_CHANN_MAX_2G], gainIndex_2g_ch1[GAIN_CHANN_MAX_2G], gainIndex_2g_ch2[GAIN_CHANN_MAX_2G];
	int gainIndex2_2g_ch0[GAIN_CHANN_MAX_2G], gainIndex2_2g_ch1[GAIN_CHANN_MAX_2G], gainIndex2_2g_ch2[GAIN_CHANN_MAX_2G];
	int gainIndex_5g_ch0[GAIN_CHANN_MAX_5G], gainIndex_5g_ch1[GAIN_CHANN_MAX_5G], gainIndex_5g_ch2[GAIN_CHANN_MAX_5G];
	int gainIndex2_5g_ch0[GAIN_CHANN_MAX_5G], gainIndex2_5g_ch1[GAIN_CHANN_MAX_5G], gainIndex2_5g_ch2[GAIN_CHANN_MAX_5G];
	int dacGain_2g[GAIN_CHANN_MAX_2G], dacGain_5g[GAIN_CHANN_MAX_5G];
	int dacGain2_2g[GAIN_CHANN_MAX_2G], dacGain2_5g[GAIN_CHANN_MAX_5G];
	int gainIndex2Delta_2g[GAIN_CHANN_MAX_2G], gainIndex2Delta_5g[GAIN_CHANN_MAX_5G];
	double PowerGoal_2g[GAIN_CHANN_MAX_2G], PowerGoal_5g[GAIN_CHANN_MAX_5G];
	double PowerGoal2_2g[GAIN_CHANN_MAX_2G], PowerGoal2_5g[GAIN_CHANN_MAX_5G];

	double PowerGoal;	// 1st. point calibrated power
	double PowerGoal2;	// 2nd. point calibrated power
	int txgain, txgainMin, txgainMax;
	int dacGain, dacGain2;
	int gainIndex, gainIndex2;
	int gainIndexDelta;

	double powerDeviation;
	double powerDeviation2;
	double txgainSlope;
	double txgainSlope2;
	int attempt;
	int attempt2;

	int GainTbl_totalGain[100];
	int maxIniTxGainEntry;
	int resetUnusedCalPiers;
};

struct _txGain
{
	int txgain;
	int gainIndex;
	int dacGain;
};

struct _Cal cal;
struct _txGain currentGain;

extern int calibration_reset(int freq);
extern int getStatus(int ich);
extern int getCalibrationGain(int iChain);
extern int getChainMask(int iChain);
extern int calibration_one(double power, int ichain);
extern int calibration_next(double power, int ichain);
extern int updateGain(int *calPoint, int ichain);
extern int updateGainM(int *calPoint, int ichain);

