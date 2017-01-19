//
// unload the dll
//
extern void CalibrationUnload();
//
// loads the device control dll 
//
extern int CalibrationLoad(char *dllname);

extern int CalibrationScheme(void);
extern int CalibrationSetIniTxGain(int *totalGain, int maxGainEntry);
extern int CalibrationTxGainReset(int *txGain, int *gainIndex, int *dacGain);
extern int CalibrationSetTxGainInit(int txGain);
extern int CalibrationSetTxGainMin(int txGainMin);
extern int CalibrationSetTxGainMax(int txGainMax);
extern int CalibrationSetGainIndexInit(int gainIndex);
extern int CalibrationSetDacGainInit(int dacGain);
extern int CalibrationSetMode(int freq, int rate, int chain);
extern int CalibrationSetPowerGoal(double PowerGoal, int iPoint);
extern int CalibrationGetTxGain(int *txGain, int *gainIndex, int *dacGain, int *calPoint, int ichain);
extern int CalibrationCalculation(double pwr_dBm, int iChain);
extern int CalibrationStatus(int iChain);
extern int CalibrationChain(int iChain);

extern int CalibrationSetCommand(int client);
extern int CalibrationGetCommand(int client);
extern int SetCalParameterSplice(struct _ParameterList *list);
extern int CalibrationGetScheme(int *x);
extern int CalibrationGetResetUnusedCalPiers(int *x);
