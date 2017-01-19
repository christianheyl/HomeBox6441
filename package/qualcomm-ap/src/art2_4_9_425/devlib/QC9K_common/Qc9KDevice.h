#ifndef	_QC9K_DEVICE_H_
#define	_QC9K_DEVICE_H_

#define MAX_TEMP_HARDLIMIT          40  

#define DO_PAPRD                    0x04000000
#define DISABLE_5G_THERM            0x08000000
#define DO_TX_IQ_CAL                0x10000000
#define DO_SW_NF_CAL                0x20000000
#define DO_PAL_OFFSET_CAL	        0x40000000
#define DO_DEBUG_CAL                0x80000000

#define DEVLIB_BB_2_ANTAB           0x80000000
#define DEVLIB_BB_2_ANTCD           0x40000000

//
// clear all device control function pointers and set to default behavior
//


extern void Qc9KDeviceSelect();

extern int Qc9KChipIdentify(void);

extern int Qc9KCardLoad ();
extern int Qc9KCardRemove();
extern int Qc9KDeviceIdGet(void);
extern int Qc9KDisableDevice(void);

extern void Qc9KUserPrintConsoleSet(int onoff);
extern int Qc9KMemoryRead(unsigned int address, unsigned int *buffer, int many);
extern int Qc9KMemoryWrite(unsigned int address, unsigned int *buffer, int many);
extern int Qc9KRegisterRead(unsigned int address, unsigned int *value);
extern int Qc9KRegisterWrite(unsigned int address, unsigned int value);
extern int Qc9KStickyWrite(int idx, unsigned int address, int low, int  high, unsigned int *value, int numVal, int prepost);
extern int Qc9KStickyClear(int idx, unsigned int address, int low, int  high);

#ifdef _WINDOWS
int Qc9KReEnableDevice(void);
int Qc9KCheckDeviceID(void);
#endif

extern int Qc9KTxGainTableRead_AddressHeader(unsigned int address, char *header, char *subheader, int max);
extern int Qc9KTxGainTableRead_AddressValue(unsigned int address, int idx, char *rName, char *fName, int *value, int *low, int *high);

#endif //_QC9K_DEVICE_H_
