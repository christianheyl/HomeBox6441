
//
// Set tx power to a specific value
//
extern int Ar5416TransmitPowerSet(int mode, double txp);


extern int Ar5416TargetPowerApply(int frequency);


//
// set transmit power gain (pcdac) to a specified value
//	
extern int Ar5416TransmitGainSet(int mode, int pcdac);


extern int Ar5416TransmitGainRead(int entry, unsigned int *rvalue, int *value, int max);


extern int Ar5416TransmitGainWrite(int entry, int *value, int nvalue);
