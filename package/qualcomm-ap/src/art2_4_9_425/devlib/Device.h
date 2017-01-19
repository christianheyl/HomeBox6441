#ifndef _DEVICE_H_
#define _DEVICE_H_

#include "ParameterSelect.h"

#ifdef _WINDOWS
#ifdef FIELDDLL
		#define DEVICEDLLSPEC __declspec(dllexport)
	#else
		#define DEVICEDLLSPEC __declspec(dllimport)
	#endif
#else
	#define DEVICEDLLSPEC
#endif


enum _DeviceCalibrationDataType
{
	DeviceCalibrationDataNone = 0,
	DeviceCalibrationDataFlash,
	DeviceCalibrationDataEeprom,
	DeviceCalibrationDataOtp,
	DeviceCalibrationDataDontLoad,
    DeviceCalibrationDataFile,
	DeviceCalibrationDataDram,
};

enum _DeviceEepromSaveSectionType
{
    DeviceEepromSaveSectionAll = 0,
    DeviceEepromSaveSectionId,
    // If more section(s) needed to be added, add anywhere from here
    DeviceEepromSaveSectionMac,
    DeviceEepromSaveSection2GCal,
    DeviceEepromSaveSection5GCal,
    DeviceEepromSaveSection2GCtl,
    DeviceEepromSaveSection5GCtl,
    DeviceEepromSaveSectionConfig,
    DeviceEepromSaveSectionCustomer,
    DeviceEepromSaveSectionUSBID,
	DeviceEepromSaveSectionSDIOID,
	DeviceEepromSaveSectionXTAL,
	DeviceEepromSaveSection5GRxCal,
    DeviceEepromSaveSection2GRxCal,
    // to above
    DeviceEepromSaveSectionMax,
};

//
// The following functions control the device.
//
// unless otherwise noted, all of these functions return 0 on success
//

extern DEVICEDLLSPEC int DeviceChipIdentify(void);

extern DEVICEDLLSPEC char *DeviceName(void);
extern DEVICEDLLSPEC char *DeviceVersion(void);
extern DEVICEDLLSPEC char *DeviceBuildDate(void);

extern DEVICEDLLSPEC int DeviceAttach(int devid, int calmem);
extern DEVICEDLLSPEC int DeviceDetach(void);
extern DEVICEDLLSPEC int DeviceValid(void);
extern DEVICEDLLSPEC int DeviceIdGet(void);


//
// reset the device
//
extern DEVICEDLLSPEC int DeviceReset(int frequency, unsigned char txchain, unsigned char rxchain, int bandwidth);

// 
// set a parameter on the device
//
extern DEVICEDLLSPEC int DeviceSetCommand(int client);
// 
// set a parameter on the device
//
extern DEVICEDLLSPEC int DeviceSetCommandLint(char *input);

//extern struct _ParameterList;

extern DEVICEDLLSPEC int DeviceSetParameterSplice(struct _ParameterList *list);

// 
// get a parameter from the device
//
extern DEVICEDLLSPEC int DeviceGetCommand(int client);

extern DEVICEDLLSPEC int DeviceGetParameterSplice(struct _ParameterList *list);


//
// set the bssid asscoiated with the device
//
extern DEVICEDLLSPEC int DeviceBssIdSet(unsigned char *bssid);

//
// set the station id
//
extern DEVICEDLLSPEC int DeviceStationIdSet(unsigned char *macaddr);

//
// set the receive descriptor pointer on the device
//
extern DEVICEDLLSPEC int DeviceReceiveDescriptorPointer(unsigned int descriptor);

//
// allow/disallow unicast packets
//
extern DEVICEDLLSPEC int DeviceReceiveUnicast(int on);

//
// allow/disallow broadcast packets
//
extern DEVICEDLLSPEC int DeviceReceiveBroadcast(int on);

//
// allow/disallow any type of packets. turning off promiscuous mode does
// not affect any packet types that are explicitly allowed
//
extern DEVICEDLLSPEC int DeviceReceivePromiscuous(int on);

//
// turn on the receiver
//
extern DEVICEDLLSPEC int DeviceReceiveEnable(void);

//
// turn off the receiver
//
extern DEVICEDLLSPEC int DeviceReceiveDisable(void);

//
// put the receiver in deaf mode
//
extern DEVICEDLLSPEC int DeviceReceiveDeafMode(int on);

extern DEVICEDLLSPEC int DeviceReceiveFifo(void);
extern DEVICEDLLSPEC int DeviceReceiveDescriptorMaximum(void);
extern DEVICEDLLSPEC int DeviceReceiveEnableFirst(void);

extern DEVICEDLLSPEC int DeviceTransmitFifo(void);
extern DEVICEDLLSPEC int DeviceTransmitDescriptorSplit(void);
extern DEVICEDLLSPEC int DeviceTransmitAggregateStatus(void);
extern DEVICEDLLSPEC int DeviceTransmitEnableFirst(void);

//
// set the transmit descriptor pointer on the device
//
extern DEVICEDLLSPEC int DeviceTransmitDescriptorStatusPointer(unsigned int first, unsigned int last);

//
// set the transmit descriptor pointer on the device
//
extern DEVICEDLLSPEC int DeviceTransmitDescriptorPointer(int queue, unsigned int descriptor);

//
// set the retry limit. retry limit can also be set in the descriptor for particular packets
//
extern DEVICEDLLSPEC int DeviceTransmitRetryLimit(int dcu, int retry);

//
// asscoiate a qcu with a dcu
//
extern DEVICEDLLSPEC int DeviceTransmitQueueSetup(int qcu, int dcu);

//
// use regular data trasnmsission mode. packets, interframe spacing, contention window, etc.
// use DeviceTransmitEnable(void) to start. use DeviceTransmitDisable(void) to stop.
//
extern DEVICEDLLSPEC int DeviceTransmitRegularData(void);	

//
// use tx99 mode. packets, fixed interframe spacing, no contention window, 
// use DeviceTransmitEnable(void) to start. use DeviceTransmitDisable(void) to stop.
//	
extern DEVICEDLLSPEC int DeviceTransmitFrameData(int ifs);

//
// use continuous data mode, one continuous data stream, no interframe spacing, no contention window, just data
// use DeviceTransmitEnable(void) to start. use DeviceReset(void) to stop.
//
extern DEVICEDLLSPEC int DeviceTransmitContinuousData(void);

//
// transmit the carrier frequency. 
// starts itself. use DeviceReset(void) to stop.
//			
extern DEVICEDLLSPEC int DeviceTransmitCarrier(unsigned int txchain);				

//
// start the transmitter
//
extern DEVICEDLLSPEC int DeviceTransmitEnable(unsigned int qmask);

//
// stop the transmitter
//
extern DEVICEDLLSPEC int DeviceTransmitDisable(unsigned int qmask);

//
// set the transmit power. dBm
//
extern DEVICEDLLSPEC int DeviceTransmitPowerSet(int mode, double txp);

extern DEVICEDLLSPEC char *DeviceIniVersion(int devid);

//
// set the pcdac and pdgain values
//
extern DEVICEDLLSPEC int DeviceTransmitGainSet(int mode, int pcdac);

extern DEVICEDLLSPEC int DeviceTransmitGainRead(int entry, unsigned int *rvalue, int *value, int max);

extern DEVICEDLLSPEC int DeviceTransmitINIGainGet(int *total_gain);

extern DEVICEDLLSPEC int DeviceTransmitGainWrite(int entry, int *value, int nvalue);

extern DEVICEDLLSPEC int DeviceTxGainTableRead_AddressGainTable(unsigned int **address, unsigned int *row, unsigned int *col);

extern DEVICEDLLSPEC int DeviceTxGainTableRead_AddressHeader(unsigned int address, char *header, char *subheader, int max);

extern DEVICEDLLSPEC int DeviceTxGainTableRead_AddressValue(unsigned int address, int i, char *rName, char *fName, int *value, int *low, int *high);

extern DEVICEDLLSPEC int Device_get_corr_coeff(int coeff_type, unsigned int **address, unsigned int *row, unsigned int *col);
//
// read/write the eeprom
//
extern DEVICEDLLSPEC int DeviceEepromRead(unsigned int address, unsigned char *value, int count);

extern DEVICEDLLSPEC int DeviceEepromWrite(unsigned int address, unsigned char *value, int count);

//
// read/write the otp
//
extern DEVICEDLLSPEC int DeviceOtpRead(unsigned int address, unsigned char *value, int count, int is_wifi);

extern DEVICEDLLSPEC int DeviceOtpWrite(unsigned int address, unsigned char *value, int count, int is_wifi);


//
// read/write the flash
//
extern DEVICEDLLSPEC int DeviceFlashRead(unsigned int address, unsigned char *value, int count);

extern DEVICEDLLSPEC int DeviceFlashWrite(unsigned int address, unsigned char *value, int count);

//
// read/write the shared memory
//
extern DEVICEDLLSPEC int DeviceMemoryBase(void);

extern DEVICEDLLSPEC unsigned char *DeviceMemoryPtr(unsigned int address);

extern DEVICEDLLSPEC int DeviceMemoryRead(unsigned int address, unsigned int *value, int count);

extern DEVICEDLLSPEC int DeviceMemoryWrite(unsigned int address, unsigned int *value, int count);

extern DEVICEDLLSPEC int DeviceRegisterRead(unsigned int address, unsigned int *value);

extern DEVICEDLLSPEC int DeviceRegisterWrite(unsigned int address, unsigned int value);

extern DEVICEDLLSPEC int DeviceFieldRead(unsigned int address, int low, int high, unsigned int *value);

extern DEVICEDLLSPEC int DeviceFieldWrite(unsigned int address, int low, int high, unsigned int value);

//
// set/save/restore/use calibration and other configuration information
//
extern DEVICEDLLSPEC int DeviceConfigurationSave(void);

extern DEVICEDLLSPEC int DeviceConfigurationRestore(void);

//
// override the normal power control settings with these newly caluclated ones.
// used to test new calibration data, before installing the values in the main structure.
//
extern DEVICEDLLSPEC int DevicePowerControlOverride(int frequency, int *correction, int *voltage, int *temperature);

extern DEVICEDLLSPEC int DeviceCalibrationPierSet(int pier, int frequency, int chain, int correction, int voltage, int temperature);

extern DEVICEDLLSPEC int DeviceCalibrationTxgainCAPSet(int *txgainmax);
//
// set and remember the target powers
//
extern DEVICEDLLSPEC int DeviceTargetPowerSet(void);

//
// looks up the target power for a given frequency and rate
//
extern DEVICEDLLSPEC int DeviceTargetPowerGet(int frequency, int rate, double *power);

//
// apply the target powers
//
extern DEVICEDLLSPEC int DeviceTargetPowerApply(int frequency);

extern DEVICEDLLSPEC int DeviceTemperatureGet(int);

extern DEVICEDLLSPEC int DeviceVoltageGet(void);

extern DEVICEDLLSPEC int DeviceMacAddressSet(unsigned char mac[6]);

extern DEVICEDLLSPEC int DeviceMacAddressGet(unsigned char mac[6]);
extern DEVICEDLLSPEC int DeviceCustomerDataGet(unsigned char *data, int max);
extern DEVICEDLLSPEC int DeviceDoPaprd(int frequency);

extern DEVICEDLLSPEC int DeviceChainMany(void);
extern DEVICEDLLSPEC int DeviceRxChainMany(void);
extern DEVICEDLLSPEC int DeviceRxChainMask(void);
extern DEVICEDLLSPEC int DeviceTxChainMany(void);
extern DEVICEDLLSPEC int DeviceTxChainMask(void);
extern DEVICEDLLSPEC int DeviceRxChainSet(int rxchain);

extern DEVICEDLLSPEC int DeviceEepromTemplatePreference(int value);
extern DEVICEDLLSPEC int DeviceEepromTemplateAllowed(unsigned int *value, unsigned int many);
extern DEVICEDLLSPEC int DeviceEepromCompress(unsigned int value);
extern DEVICEDLLSPEC int DeviceEepromOverwrite(unsigned int value);
extern DEVICEDLLSPEC int DeviceEepromSize(void);
extern DEVICEDLLSPEC int DeviceEepromSaveMemorySet(int memory);
extern DEVICEDLLSPEC int DeviceCalibrationDataAddressSet(int size);
extern DEVICEDLLSPEC int DeviceCalibrationDataAddressGet();
extern DEVICEDLLSPEC int DeviceCalibrationDataSet(int source);
extern DEVICEDLLSPEC int DeviceCalibrationDataGet();
extern DEVICEDLLSPEC int DeviceCalibrationFromEepromFile(void);
extern DEVICEDLLSPEC int DeviceEepromTemplateInstall(int value);
extern DEVICEDLLSPEC int DeviceEepromReport(void (*print)(char *format, ...), int all);


extern DEVICEDLLSPEC int DevicePaPredistortionSet(int value);
extern DEVICEDLLSPEC int DevicePaPredistortionGet();

extern DEVICEDLLSPEC int DevicePsatCalibration(int frequency, unsigned int txchain, unsigned int rxchain, int bandwidth);

extern DEVICEDLLSPEC int DeviceRegulatoryDomainGet(void); 
extern DEVICEDLLSPEC int DeviceRegulatoryDomainOverride(unsigned int regdmn);

extern DEVICEDLLSPEC int DeviceNoiseFloorSet(int frequency, int ichain, int nf);
extern DEVICEDLLSPEC int DeviceNoiseFloorGet(int frequency, int ichain);
extern DEVICEDLLSPEC int DeviceNoiseFloorPowerSet(int frequency, int ichain, int nfpower);
extern DEVICEDLLSPEC int DeviceNoiseFloorPowerGet(int frequency, int ichain);
extern DEVICEDLLSPEC int DeviceNoiseFloorTemperatureSet(int frequency, int ichain, int temperature);
extern DEVICEDLLSPEC int DeviceNoiseFloorTemperatureGet(int frequency, int ichain);


extern DEVICEDLLSPEC int DeviceSpectralScanEnable(void);

extern DEVICEDLLSPEC int DeviceSpectralScanProcess(unsigned char *data, int ndata, int *spectrum, int max);

extern DEVICEDLLSPEC int DeviceSpectralScanDisable(void);

#ifdef ATH_SUPPORT_MCI
extern DEVICEDLLSPEC int DeviceMCISetup(void);
extern DEVICEDLLSPEC int DeviceMCIReset(int);
#endif

extern DEVICEDLLSPEC int DeviceTuningCapsSet(int);
extern DEVICEDLLSPEC int DeviceTuningCapsSave(int);

extern DEVICEDLLSPEC int DeviceChannelCalculate(int *frequency, int *option, int maxchannel);

extern DEVICEDLLSPEC int DeviceInitializationCommit(void);
extern DEVICEDLLSPEC int DeviceInitializationUsed(void);
extern DEVICEDLLSPEC int DeviceInitializationSubVendorSet(unsigned int subvendor);
extern DEVICEDLLSPEC int DeviceInitializationSubVendorGet(unsigned int *subvendor);
extern DEVICEDLLSPEC int DeviceInitializationVendorSet(unsigned int vendor);
extern DEVICEDLLSPEC int DeviceInitializationVendorGet(unsigned int *vendor);
extern DEVICEDLLSPEC int DeviceInitializationSsidSet(unsigned int ssid);
extern DEVICEDLLSPEC int DeviceInitializationSsidVendorGet(unsigned int *ssid);
extern DEVICEDLLSPEC int DeviceInitializationDevidSet(unsigned int devid);
extern DEVICEDLLSPEC int DeviceInitializationDevidGet(unsigned int *devid);
extern DEVICEDLLSPEC int DeviceInitializationSet(unsigned int address, unsigned int value, int size);
extern DEVICEDLLSPEC int DeviceInitializationGet(unsigned int address, unsigned int *value);
extern DEVICEDLLSPEC int DeviceInitializationMany(void);
extern DEVICEDLLSPEC int DeviceInitializationGetByIndex(int index, unsigned int *address, unsigned int *value);
extern DEVICEDLLSPEC int DeviceInitializationRemove(unsigned int address, int size);
extern DEVICEDLLSPEC int DeviceInitializationRestore(void);

extern DEVICEDLLSPEC int DeviceNoiseFloorFetch(int *nfc, int *nfe, int nfn);
extern DEVICEDLLSPEC int DeviceNoiseFloorLoad(int *nfc, int *nfe, int nfn);
extern DEVICEDLLSPEC int DeviceNoiseFloorReady(void);
extern DEVICEDLLSPEC int DeviceNoiseFloorEnable(void);

extern DEVICEDLLSPEC int DeviceOpflagsGet(void);
extern DEVICEDLLSPEC int DeviceIs2GHz(void);
extern DEVICEDLLSPEC int DeviceIs5GHz(void);
extern DEVICEDLLSPEC int DeviceIs4p9GHz(void);
extern DEVICEDLLSPEC int DeviceIsHalfRate(void);
extern DEVICEDLLSPEC int DeviceIsQuarterRate(void);

extern DEVICEDLLSPEC int DeviceIsEmbeddedArt(void);
extern DEVICEDLLSPEC int DeviceStickyWrite(int idx,unsigned int address, int low, int  high, unsigned int *value, int numVal, int prepost);
extern DEVICEDLLSPEC int DeviceStickyClear(int idx,unsigned int address, int low, int  high);
extern DEVICEDLLSPEC int DeviceConfigAddrSet(unsigned int address, int low, int  high, unsigned int *value, int numVal, int prepost);

extern DEVICEDLLSPEC int DeviceRfBbTestPoint(int frequency, int ht40, int bandwidth, int antennapair, unsigned char chainnum,
                       int mbgain, int rfgain, int coex, int sharedrx, int switchtable, unsigned char AnaOutEn);

extern DEVICEDLLSPEC int DeviceTransmitDataDut(void *params);
extern DEVICEDLLSPEC int DeviceTransmitStatusReport(void **txStatus, int stop);
extern DEVICEDLLSPEC int DeviceTransmitStop(void **txStatus, int calibrate);
extern DEVICEDLLSPEC int DeviceReceiveDataDut(void *params);
extern DEVICEDLLSPEC int DeviceReceiveStatusReport(void **rxStatus, int stop);
extern DEVICEDLLSPEC int DeviceReceiveStop(void **rxStatus);

extern DEVICEDLLSPEC int DeviceCalInfoInit();
extern DEVICEDLLSPEC int DeviceCalInfoCalibrationPierSet(int pier, int frequency, int chain, int gain, int gainIndex, int dacGain, double power, int correction, int voltage, int temperature, int calPoint);
extern DEVICEDLLSPEC int DeviceCalUnusedPierSet(int iChain, int iBand, int iIndex);

extern DEVICEDLLSPEC int DeviceOtpLoad();
extern DEVICEDLLSPEC int DeviceSetConfigParameterSplice(struct _ParameterList *list);
extern DEVICEDLLSPEC int DeviceSetConfigCommand(void *cmd);

extern DEVICEDLLSPEC int DeviceStbcGet();
extern DEVICEDLLSPEC int DeviceLdpcGet();
extern DEVICEDLLSPEC int DevicePapdGet();

extern DEVICEDLLSPEC int DeviceHeavyClipEnableGet(unsigned int *enabled);
extern DEVICEDLLSPEC int DeviceHeavyClipEnableSet(unsigned int enable);

extern DEVICEDLLSPEC int DeviceEepromSaveSectionSet(unsigned int sectionMask);
extern DEVICEDLLSPEC int DevicePapdIsDone(int txPwr);
extern DEVICEDLLSPEC char *DeviceIniVersion(int devid);
extern DEVICEDLLSPEC int DeviceCalibrationPower(int txgain, double txpower);

extern DEVICEDLLSPEC int DeviceXtalReferencePPMGet(void);
extern DEVICEDLLSPEC double DeviceCmacPowerGet(int chain);
extern DEVICEDLLSPEC int DevicePsatCalibrationResultGet(int frequency, int chain, int *olpc_dalta, int *thermal, double *cmac_power_olpc, double *cmac_power_psat, unsigned int *olcp_pcdac, unsigned int *psat_pcdac);

extern DEVICEDLLSPEC int DeviceDiagData(unsigned char *data, unsigned int ndata, unsigned char *respdata, unsigned int *nrespdata);

extern DEVICEDLLSPEC int DeviceSleepMode(int mode);

extern DEVICEDLLSPEC int DeviceDeviceHandleGet(unsigned int *handle);

extern DEVICEDLLSPEC int DeviceReadPciConfigSpace(unsigned int offset, unsigned int *value);

extern DEVICEDLLSPEC int DeviceWritePciConfigSpace(unsigned int offset, unsigned int value);

extern DEVICEDLLSPEC int DevicetlvCallbackSet(int (*_tlvCallback)(unsigned char*, int, unsigned char*, unsigned long*));
extern DEVICEDLLSPEC int DeviceIs11ACDevice(void);

extern DEVICEDLLSPEC int DeviceNonCenterFreqAllowedGet();
extern DEVICEDLLSPEC int DeviceDiagData(unsigned char *data, unsigned int ndata, unsigned char *respdata, unsigned int *nrespdata);
extern DEVICEDLLSPEC double DeviceGainTableOffset(void);
extern int DEVICEDLLSPEC DeviceCalibrationSetting(void);

extern DEVICEDLLSPEC int DevicetlvCreate(unsigned char);
extern DEVICEDLLSPEC int DevicetlvAddParam(char*, char*);
extern DEVICEDLLSPEC int DevicetlvComplete(void);
extern DEVICEDLLSPEC int DevicetlvGetRspParam(char*, char*);

//
// These are the function pointers to the device dependent control functions.
//
// Please add new functions at the end of the list so that existing DLLs continue to work.
// They will have NULL function pointers for anything that is added, but the original function
// pointers will remain in the right place.
//
struct _DeviceFunction
{
	int (*ChipIdentify)(void);
	char *(*Name)(void);
	char *(*Version)(void);
	char *(*BuildDate)(void);

	int (*Attach)(int devid, int calmem);
	int (*Detach)(void);
	int (*Valid)(void);
	int (*IdGet)(void);

	int (*Reset)(int frequency, unsigned int txchain, unsigned int rxchain, int bandwidth);

    int (*SetCommand)(int client);
    int (*SetParameterSplice)(struct _ParameterList *list);
    int (*GetCommand)(int client);
    int (*GetParameterSplice)(struct _ParameterList *list);

    int (*BssIdSet)(unsigned char *bssid);   
	int (*StationIdSet)(unsigned char *macaddr);

    int (*ReceiveDescriptorPointer)(unsigned int descriptor);
    int (*ReceiveUnicast)(int on);
    int (*ReceiveBroadcast)(int on);
    int (*ReceivePromiscuous)(int on);
    int (*ReceiveEnable)(void);
    int (*ReceiveDisable)(void);
    int (*ReceiveDeafMode)(int deaf);

    int (*ReceiveFifo)(void);
    int (*ReceiveDescriptorMaximum)(void);
    int (*ReceiveEnableFirst)(void);

    int (*TransmitFifo)(void);
    int (*TransmitDescriptorSplit)(void);
    int (*TransmitAggregateStatus)(void);
    int (*TransmitEnableFirst)(void);

    int (*TransmitDescriptorStatusPointer)(unsigned int first, unsigned int last);
    int (*TransmitDescriptorPointer)(int qcu, unsigned int descriptor);
    int (*TransmitRetryLimit)(int dcu, int retry);
    int (*TransmitQueueSetup)(int qcu, int dcu);
    int (*TransmitRegularData)(void);		// normal
    int (*TransmitFrameData)(int ifs);	// tx99
    int (*TransmitContinuousData)(void);		// tx100
    int (*TransmitCarrier)(unsigned int txchain);				// carrier only
    int (*TransmitEnable)(unsigned int qmask);
    int (*TransmitDisable)(unsigned int qmask);

    int (*TransmitPowerSet)(int mode, double txp);
    int (*TransmitGainSet)(int mode, int pcdac);
    int (*TransmitGainRead)(int entry, unsigned int *rvalue, int *value, int max);
    int (*TransmitGainWrite)(int entry, int *value, int nvalue);

    int (*EepromRead)(unsigned int address, unsigned char *value, int count);
    int (*EepromWrite)(unsigned int address, unsigned char *value, int count);

    int (*OtpRead)(unsigned int address, unsigned char *value, int count, int is_wifi);
    int (*OtpWrite)(unsigned int address, unsigned char *value, int count, int is_wifi);

    int (*MemoryBase)(void);
    unsigned char * (*MemoryPtr)(unsigned int address);
    int (*MemoryRead)(unsigned int address, unsigned int *value, int count);
    int (*MemoryWrite)(unsigned int address, unsigned int *value, int count);

    int (*RegisterRead)(unsigned int address, unsigned int *value);
    int (*RegisterWrite)(unsigned int address, unsigned int value);

    int (*FieldRead)(unsigned int address, int low, int high, unsigned int *value);
    int (*FieldWrite)(unsigned int address, int low, int high, unsigned int value);

    int (*ConfigurationRestore)(void);
    int (*ConfigurationSave)(void);
    int (*CalibrationPierSet)(int pier, int frequency, int chain, int correction, int voltage, int temperature);
    int (*PowerControlOverride)(int frequency, int *correction, int *voltage, int *temperature);
    int (*TargetPowerSet)(/* arguments ???? */);
    int (*TargetPowerGet)(int frequency, int rate, double *power);
    int (*TargetPowerApply)(int frequency);

    int (*TemperatureGet)(int);
    int (*VoltageGet)(void);

    int (*MacAddressGet)(unsigned char mac[6]);
    int (*CustomerDataGet)(unsigned char *data, int max);

	int (*TxChainMany)(void);
	int (*TxChainMask)(void);
	int (*RxChainMany)(void);
	int (*RxChainMask)(void);
    int (*RxChainSet)(int rxchain);

    int (*EepromTemplatePreference)(int value);
    int (*EepromTemplateAllowed)(unsigned int *value, unsigned int many);
    int (*EepromCompress)(unsigned int value);
    int (*EepromOverwrite)(unsigned int value);
    int (*EepromSize)(void);
    int (*EepromSaveMemorySet)(int memmory);
    int (*EepromReport)(void (*print)(char *format, ...), int all);

    int (*CalibrationDataAddressSet)(int size);
    int (*CalibrationDataAddressGet)(void);

    int (*CalibrationDataSet)(int source);
    int (*CalibrationDataGet)(void);

    int (*CalibrationFromEepromFile)(void);
    int (*EepromTemplateInstall)(int value);

    int (*PaPredistortionSet)(int value);
    int (*PaPredistortionGet)(void);

    int (*RegulatoryDomainOverride)(unsigned int regdmn);
    int (*RegulatoryDomainGet)(void);

    int (*NoiseFloorSet)(int frequency, int ichain, int nf);
    int (*NoiseFloorGet)(int frequency, int ichain);

    int (*NoiseFloorPowerSet)(int frequency, int ichain, int nfpower);
    int (*NoiseFloorPowerGet)(int frequency, int ichain);

	int (*SpectralScanEnable)(void);
	int (*SpectralScanProcess)(unsigned char *data, int ndata, int *spectrum, int max);
	int (*SpectralScanDisable)(void);

    int (*NoiseFloorTemperatureSet)(int frequency, int ichain, int temperature);
    int (*NoiseFloorTemperatureGet)(int frequency, int ichain);

//#ifdef ATH_SUPPORT_MCI
	int (*MCISetup)(void);
	int (*MCIReset)(int);
//#endif

	int (*TuningCapsSet)(int caps);
	int (*TuningCapsSave)(int caps);

	int (*ChannelCalculate)(int *frequency, int *option, int maxchannel);

	int (*InitializationCommit)(void);
	int (*InitializationUsed)(void);
	int (*InitializationSubVendorSet)(unsigned int subvendor);
	int (*InitializationSubVendorGet)(unsigned int *subvendor);
	int (*InitializationVendorSet)(unsigned int vendor);
	int (*InitializationVendorGet)(unsigned int *vendor);
	int (*InitializationSsidSet)(unsigned int ssid);
	int (*InitializationSsidVendorGet)(unsigned int *ssid);
	int (*InitializationDevidSet)(unsigned int devid);
	int (*InitializationDevidGet)(unsigned int *devid);
	int (*InitializationSet)(unsigned int address, unsigned int value, int size);
	int (*InitializationGet)(unsigned int address, unsigned int *value);
	int (*InitializationMany)(void);
	int (*InitializationGetByIndex)(int index, unsigned int *address, unsigned int *value);
	int (*InitializationRemove)(unsigned int address, int size);
	int (*InitializationRestore)(void);

	int (*NoiseFloorFetch)(int *nfc, int *nfe, int nfn);
	int (*NoiseFloorLoad)(int *nfc, int *nfe, int nfn);
	int (*NoiseFloorReady)(void);
	int (*NoiseFloorEnable)(void);

	int (*OpflagsGet)(void);
	int (*Is2GHz)(void);
	int (*Is5GHz)(void);
	int (*Is4p9GHz)(void);
	int (*IsHalfRate)(void);
	int (*IsQuarterRate)(void);
	
    int (*FlashRead)(unsigned int address, unsigned char *value, int count);
    int (*FlashWrite)(unsigned int address, unsigned char *value, int count);

    int (*IsEmbeddedArt)(void);
    int (*StickyWrite)(int idx,unsigned int address, int low, int  high, unsigned int *value, int numVal, int prepost);
    int (*StickyClear)(int idx,unsigned int address, int low, int  high);
    int (*ConfigAddrSet)(unsigned int address, int low, int  high, unsigned int *value, int numVal, int prepost);
    int (*RfBbTestPoint)(int frequency, int ht40, int bandwidth, int antennapair, unsigned char chainnum,
                       int mbgain, int rfgain, int coex, int sharedrx, int switchtable, unsigned char AnaOutEn);

    int (*TransmitDataDut)(void *params);
    int (*TransmitStatusReport)(void **txStatus, int stop);
    int (*TransmitStop)(void **txStatus, int calibrate);
    int (*ReceiveDataDut)(void *params);
    int (*ReceiveStatusReport)(void **rxStatus, int stop);
    int (*ReceiveStop)(void **rxStatus);
    int (*CalInfoInit)();
    int (*CalInfoCalibrationPierSet)(int pier, int frequency, int chain, int gain, int gainIndex, int dacGain, double power, int correction, int voltage, int temperature, int calPoint);
    int (*CalUnusedPierSet)(int iChain, int iBand, int iIndex);
    int (*OtpLoad)();
    int (*SetConfigParameterSplice)(struct _ParameterList *list);
    int (*SetConfigCommand)(void *cmd);
    int (*StbcGet)();
    int (*LdpcGet)();
    int (*PapdGet)();
    int (*EepromSaveSectionSet)(unsigned int sectionMask);
    int (*PapdIsDone)(int txPwr);
	
	int (*CalibrationPower)(int txgain, double txpower);
	int (*CalibrationTxgainCAPSet)(int *txgainmax);
	char *(*IniVersion)(int devid);

	int (*TxGainTableRead_AddressGainTable)(unsigned int **address, unsigned int *row, unsigned int *col);
	int (*TxGainTableRead_AddressHeader)(unsigned int address, char *header, char *subheader, int max);
	int (*TxGainTableRead_AddressValue)(unsigned int address, int i, char *rName, char *fName, int *value, int *low, int *high);
	int (*Get_corr_coeff)(int coeff_type, unsigned int **coeff_array, unsigned int *row, unsigned int *col);
	int (*TransmitINIGainGet)(int *total_gain);
	int (*SleepMode)(int mode);
	int (*DeviceHandleGet)(unsigned int *handle);
    int (*XtalReferencePPMGet)(void);
    double (*CmacPowerGet)(int chain);
    int (*PsatCalibrationResultGet)(int frequency, int chain, int *olpc_dalta, int *thermal, double *cmac_power_olpc, double *cmac_power_psat, unsigned int *olcp_pcdac, unsigned int *psat_pcdac);
	// For Qdart
	int (*DiagData)(unsigned char *data, unsigned int ndata, unsigned char *respdata, unsigned int *nrespdata);

	double (*GainTableOffset)(void);
	int (*CalibrationSetting)(void);
	int (*pll_screen)(void);
    int (*ReadPciConfigSpace)(unsigned int offset, unsigned int *value);
    int (*WritePciConfigSpace)(unsigned int offset, unsigned int value);
    int (*Is11ACDevice)(void);
	int (*SetCommandLine)(char*);
    int (*NonCenterFreqAllowedGet)();

	int (*HeavyClipEnableSet)(unsigned int enable);
	int (*HeavyClipEnableGet)(unsigned int *enable);

	int (*MacAddressSet)(unsigned char mac[6]);
	int (*PsatCalibration)(int frequency, unsigned int txchain, unsigned int rxchain, int bandwidth);
};


//
// The following functions setup the device control functions for a specific device type.
//

//
// clear all device control function pointers and set to default behavior
//
extern DEVICEDLLSPEC void DeviceFunctionReset(void);

//
// clear all device control function pointers and set to default behavior
//
extern DEVICEDLLSPEC int DeviceFunctionSelect(struct _DeviceFunction *device);


struct _TlvDeviceFunction
{
	int (*tlvCallbackSet)(int (*_tlvCallback)(unsigned char*, int, unsigned char*, unsigned long*));
    int (*tlvCreate)(unsigned char);
    int (*tlvAddParam)(char*, char*);
    int (*tlvComplete)(void);
    int (*tlvGetRspParam)(char*, char*);
    int (*tlvCalibrationInit)(int);
    int (*tlvCalibration)(double);
};
extern DEVICEDLLSPEC void TlvDeviceFunctionReset(void);
extern DEVICEDLLSPEC int TlvDeviceFunctionSelect(struct _TlvDeviceFunction *device);
typedef int (*_tlvCallback)(unsigned char*, int, unsigned char*, unsigned long*);

#endif /* _DEVICE_H_ */
