#ifndef _DEV_NON_EEPROM_PARAMETER_H_
#define _DEV_NON_EEPROM_PARAMETER_H_


#define DevStbc "stbc"	
#define DevLdpc "ldpc"	
#define DevUsbManufacturerString "usb.manu"
#define DevUsbProductString "usb.proc"
#define DevUsbSerialString "usb.seri"

#define DevRemoteWakeup "usb.remotewakeup"
#define Dev2GPastPower "psat.2g.power"
#define Dev5GPastPower "psat.5g.power"
#define Dev2GDiff_OFDM_CW_Power "psat.2g.diff"
#define Dev5GDiff_OFDM_CW_Power "psat.5g.diff"

#define DevNonCenterFreqAllowed "nonCenterFrequencyAllowed"

enum
{
    DevSetStbc=20000,
    DevSetLdpc,
	DevSetUsbManufacturerString,
	DevSetUsbProductString,
	DevSetUsbSerialString,
    DevGetFirmwareVersion,
	DevSetRemoteWakeup,
	DevSet2GPastPower,
	DevSet5GPastPower,
	DevSet2GDiff_OFDM_CW_Power,
	DevSet5GDiff_OFDM_CW_Power,
    DevSetNonCenterFreqAllowed,
};

// Set Function Prototypes
extern int DevStbcSet(int value);
extern int DevLdpcSet(int value);
extern int DevSvidSet(int value);
extern int DevSsidSet(int value);
extern int DevRemoteWakeupSet(int value);
extern int DevUsbManufacturerStringGet(A_UCHAR *data, int maxlength);
extern int DevUsbProductStringGet(A_UCHAR *data, int maxlength);
extern int DevUsbSerialStringGet(A_UCHAR *data, int maxlength);
extern int DevUsbManufacturerStringSet(A_UCHAR *data, int maxlength);
extern int DevUsbProductStringSet(A_UCHAR *data, int maxlength);
extern int DevUsbSerialStringSet(A_UCHAR *data, int maxlength);
//extern int DevPsatPowerSet(double *value, int ix, int iy, int iz, int num, int iBand);
//extern int DevPsatDiffSet(double *value, int ix, int iy, int iz, int num, int iBand);
extern int DevNonCenterFreqAllowedSet(int value);

// Get Function Prototypes
extern int DevStbcGet();
extern int DevLdpcGet();
extern int DevSvidGet();
extern int DevSsidGet();
extern int DevRemoteWakeupGet();
extern unsigned int DevFirmwareVersionGet(A_UCHAR *data);
//extern int DevPsatPowerGet(double *value, int ix, int iy, int iz, int *num, int iBand);
//extern int DevPsatDiffGet(double *value, int ix, int iy, int iz, int *num, int iBand);
extern int DevNonCenterFreqAllowedGet();

#endif //_DEV_NON_EEPROM_PARAMETER_H_