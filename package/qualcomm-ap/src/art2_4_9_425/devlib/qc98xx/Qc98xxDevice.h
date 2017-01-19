#ifndef	_QC98XXDEVICE_H
#define	_QC98XXDEVICE_H

#include "ParameterSelect.h"

#ifdef _WINDOWS
    #ifdef QC98XXDLL
		#define QC98XXDLLSPEC __declspec(dllexport)
	#else
		#define QC98XXDLLSPEC __declspec(dllimport)
	#endif
#else
	#define QC98XXDLLSPEC
#endif


#define MAX_TEMP_HARDLIMIT          40  

#define DO_PAPRD                    0x04000000
#define DISABLE_5G_THERM            0x08000000
#define DO_TX_IQ_CAL                0x10000000
#define DO_SW_NF_CAL                0x20000000
#define DO_PAL_OFFSET_CAL	        0x40000000
#define DO_DEBUG_CAL                0x80000000

#define DEVLIB_BB_2_ANTAB           0x80000000
#define DEVLIB_BB_2_ANTCD           0x40000000

#define MAX_CHAIN_MASK              0x7   // 3 chains


//
// clear all device control function pointers and set to default behavior
//
extern QC98XXDLLSPEC int Qc98xxDeviceSelect();


extern QC98XXDLLSPEC int Qc98xxEepromRead(unsigned int address, unsigned char *buffer, int many);

extern QC98XXDLLSPEC int Qc98xxEepromWrite(unsigned int address, unsigned char *buffer, int many);

extern QC98XXDLLSPEC int Qc98xxOtpRead(unsigned int address, unsigned char *buffer, int many, int is_wifi);

extern QC98XXDLLSPEC int Qc98xxOtpWrite(unsigned int address, unsigned char *buffer, int many,int is_wifi);

extern QC98XXDLLSPEC int Qc98xxRxChainSet(int rxChain);

extern QC98XXDLLSPEC int Qc98xxDeafMode(int deaf); 

extern QC98XXDLLSPEC int Qc98xxAttach(int devid, int ssid);
//extern int Qc98xxAttach(int devid, int calmem);


extern QC98XXDLLSPEC int Qc98xxReset(int frequency, unsigned int txchain, unsigned int rxchain, int bandwidth);

//extern int Qc98xxTransmitDataDut(void *params);

// Qc98xxRfBbTestPoint.c
extern int Qc98xxRfBbTestPoint(int frequency, int ht40, int bandwidth, int antennapair, unsigned char chainnum,
                               int mbgain, int rfgain, int coex, int sharedrx, int switchtable, unsigned char AnaOutEn);


//extern struct _ParameterList;

extern QC98XXDLLSPEC int Qc98xxGetParameterSplice(struct _ParameterList *list);

extern QC98XXDLLSPEC int Qc98xxSetParameterSplice(struct _ParameterList *list);

extern QC98XXDLLSPEC int Qc98xxSetCommand(int client);

extern QC98XXDLLSPEC int Qc98xxSetCommandLine(char *input);

extern QC98XXDLLSPEC int Qc98xxGetCommand(int client);

extern QC98XXDLLSPEC int Qc98xxIsEmbeddedArt(void);
extern QC98XXDLLSPEC int Qc98xxValid(void);

//extern struct ath_hal *AH;

extern QC98XXDLLSPEC int Qc98xxTargetPowerGet(int frequency, int rate, double *power);

// prototype for functions used in qc98xx

extern int Qc98xxReadPciConfigSpace(unsigned int offset, unsigned int *value);
extern int Qc98xxIsVersion1();

#endif //_QC98XXDEVICE_H
