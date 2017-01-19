

//
// save any information required to support calibration
//

extern int CalibrateInformationRecord(int frequency,
                               int txchain,
                               int txgain,
                               double power,
                               int correction,
                               int p1,
                               int p2,
                               int temperature,
                               int voltage);
extern int CalibrateRecord(int client, int frequency, int txchain, int txgain, int gainIndx, int dacGain, double power, int ip);


//
// Save the calibration data in the internal data structure 
//
extern int CalibrateSave(int calPoints);
extern void CalibrateTemperatureSetFromDut();


//
// Clear all of the saved calibration data
//
extern int CalibrateClear();

extern void CalibrateStatsHeader(int client);

