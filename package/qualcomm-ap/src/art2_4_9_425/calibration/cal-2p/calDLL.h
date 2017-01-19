#ifdef __cplusplus
extern "C" {
#endif // __cplusplus


#ifdef _WINDOWS
	#ifdef CAL2P_EXPORTS
	#define CAL2P_API __declspec(dllexport)
	#else
	#define CAL2P_API __declspec(dllimport)
	#endif
#else
	#define CAL2P_API
#endif

enum CalNextStatus
{
    CALNEXT_ERROR=-2,
    CALNEXT_UNINIT=-1,
    CALNEXT_Done=0,
	CALNEXT_DoneOne,
	CALNEXT_DoneTwo,
    CALNEXT_TxStart,
    CALNEXT_TxStart2,
    CALNEXT_ReMeas,
};

#define CALIBRATION_SCHEME_TXGAIN				0
#define CALIBRATION_SCHEME_GAININDEX			1
#define CALIBRATION_SCHEME_GAININDEX_DACGAIN	2

#define MAX_INITXGAIN_ENTRY		100

// Parse the calibration setup file and get the initial gain settings in setup  file.
// Cart command line init value could overwrite the first calibration point init value in setup file.
extern CAL2P_API int Calibration_Scheme(void);

extern CAL2P_API int Calibration_SetIniTxGain(int *totalGain, int maxGainEntry);

// For 1 point cal, cart init txgain is overwrite calibration_setup value.
// For 2 points cal, txgain is recalculated based on init gainIndex and dacGain
// reset txgain in Cal DLL for calibration calculation
extern CAL2P_API int Calibration_TxGainReset(int *txGain, int *gainIndex, int *dacGain);

// the tx command setting will over write what in calibration setup file
extern CAL2P_API int Calibration_SetTxGainInit(int txgain);
extern CAL2P_API int Calibration_SetTxGainMin(int txgainMin);
extern CAL2P_API int Calibration_SetTxGainMax(int txgainMax);

extern CAL2P_API int Calibration_SetGainIndexInit(int gainIndex);
extern CAL2P_API int Calibration_SetDacGainInit(int dacGain);

// NART inform cal dll the initial calibration value
extern CAL2P_API int Calibration_SetMode(int freq, int rate, int chain);
extern CAL2P_API int Calibration_SetCalibrationPowerGoal(double PowerGoal, int iPoint);
extern CAL2P_API int Calibration_SetCalibrationCalculationValue(double *deltaTxPwrGoal, double *slope, int *iMaxIteration, int iPoint);

extern CAL2P_API int Calibration_GetTxGain(int *txgain, int *gainIndex, int *dacGain, int *calPoint, int ichain);
extern CAL2P_API int Calibration_SetGainInit(int *txGain, int *gainIndex, int *dacGain, int iPoint);

// NART inform cal dll the power measurement of ichain
// cal dll calculate the next gain settings and calibtion status based on the measured power
extern CAL2P_API int Calibration_Calculation(double pwr_dBm, int iChain);

// cal dll send back NART the current calibration status.
extern CAL2P_API int Calibration_Status(int iChain);

// check how many chain left need to run calibration
extern CAL2P_API int Calibration_Chain(int iChain);

extern CAL2P_API int SetCalScheme(int *x);

extern CAL2P_API int Calibration_SetCommand(int client);
extern CAL2P_API int Calibration_GetCommand(int client);
extern CAL2P_API int Calibration_ParameterSplice(struct _ParameterList *list);


#ifdef __cplusplus
}
#endif