
//
// This file contains the functions that control the radio chip set and make it do the correct thing.
//
// These are the only functions that should be used by higher level software. These functions, in turn, call
// the appropriate device dependent functions based on the currently selected chip set.
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>



#include "smatch.h"
#include "ErrorPrint.h"

#include "DeviceError.h"
#include "Device.h"
#include "DeviceLoad.h"


static struct _DeviceFunction _Device;

DEVICEDLLSPEC int DeviceChipIdentify(void)
{
	if(_Device.ChipIdentify!=0)
	{
		return _Device.ChipIdentify();
	}
	ErrorPrint(DeviceNoFunction,"DeviceChipIdentify");
	return 0;
}


DEVICEDLLSPEC char *DeviceName(void)
{
	if(_Device.Name!=0)
	{
		return _Device.Name();
	}
	ErrorPrint(DeviceNoFunction,"DeviceName");
	return 0;
}

DEVICEDLLSPEC char *DeviceVersion(void)
{
	if(_Device.Version!=0)
	{
		return _Device.Version();
	}
	ErrorPrint(DeviceNoFunction,"DeviceVersion");
	return 0;
}


DEVICEDLLSPEC char *DeviceBuildDate(void)
{
	if(_Device.BuildDate!=0)
	{
		return _Device.BuildDate();
	}
	ErrorPrint(DeviceNoFunction,"DeviceBuildDate");
	return 0;
}


int DEVICEDLLSPEC DeviceAttach(int devid, int calmem)
{
	if(_Device.Attach!=0)
	{
		return _Device.Attach(devid, calmem);
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceAttach");
}


int DEVICEDLLSPEC DeviceDetach(void)
{
	if(_Device.Detach!=0)
	{
		return _Device.Detach();
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceDetach");
}


int DEVICEDLLSPEC DeviceValid(void)
{
	if(_Device.Valid!=0)
	{
		return _Device.Valid();
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceValid");
}


int DEVICEDLLSPEC DeviceIdGet(void)
{
	if(_Device.IdGet!=0)
	{
		return _Device.IdGet();
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceIdGet");
}


int DEVICEDLLSPEC DeviceReset(int frequency, unsigned char txchain, unsigned char rxchain, int bandwidth)
{
	if(_Device.Reset!=0)
	{
		return _Device.Reset(frequency,txchain,rxchain,bandwidth);
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceReset");
}

int DEVICEDLLSPEC DevicePllScreen(void)
{
	if(_Device.pll_screen!=0)
	{
		return _Device.pll_screen();
	}
	return -ErrorPrint(DeviceNoFunction,"DevicePllScreen");
}

int DEVICEDLLSPEC DeviceSetCommand(int client)
{
	if(_Device.SetCommand!=0)
	{
		return _Device.SetCommand(client);
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceSetCommand");
}

int DEVICEDLLSPEC DeviceSetCommandLine(char *input)
{
	if(_Device.SetCommand!=0)
	{
		return _Device.SetCommandLine(input);
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceSetCommandLine");
}

int DEVICEDLLSPEC DeviceSetParameterSplice(struct _ParameterList *list)
{
	if(_Device.SetParameterSplice!=0)
	{
		return _Device.SetParameterSplice(list);
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceSetParameterSplice");
}


int DEVICEDLLSPEC DeviceGetCommand(int client)
{
	if(_Device.GetCommand!=0)
	{
		return _Device.GetCommand(client);
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceGetCommand");
}


int DEVICEDLLSPEC DeviceGetParameterSplice(struct _ParameterList *list)
{
	if(_Device.GetParameterSplice!=0)
	{
		return _Device.GetParameterSplice(list);
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceGetParameterSplice");
}


int DEVICEDLLSPEC DeviceBssIdSet(unsigned char *bssid)
{
	if(_Device.BssIdSet!=0)
	{
		return _Device.BssIdSet(bssid);
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceBssIdSet");
}


int DEVICEDLLSPEC DeviceStationIdSet(unsigned char *macaddr)
{
	if(_Device.StationIdSet!=0)
	{
		return _Device.StationIdSet(macaddr);
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceStationIdSet");
}


int DEVICEDLLSPEC DeviceReceiveDescriptorPointer(unsigned int descriptor)
{
	if(_Device.ReceiveDescriptorPointer!=0)
	{
		return _Device.ReceiveDescriptorPointer(descriptor);
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceReceiveDescriptorPointer");
}


int DEVICEDLLSPEC DeviceReceiveUnicast(int on)
{
	if(_Device.ReceiveUnicast!=0)
	{
		return _Device.ReceiveUnicast(on);
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceReceiveUnicast");
}


int DEVICEDLLSPEC DeviceReceiveBroadcast(int on)
{
	if(_Device.ReceiveBroadcast!=0)
	{
		return _Device.ReceiveBroadcast(on);
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceReceiveBroadcast");
}


int DEVICEDLLSPEC DeviceReceivePromiscuous(int on)
{
	if(_Device.ReceivePromiscuous!=0)
	{
		return _Device.ReceivePromiscuous(on);
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceReceivePromiscuous");
}


int DEVICEDLLSPEC DeviceReceiveEnable(void)
{
	if(_Device.ReceiveEnable!=0)
	{
		return _Device.ReceiveEnable();
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceReceiveEnable");
}


int DEVICEDLLSPEC DeviceReceiveDisable(void)
{
	if(_Device.ReceiveDisable!=0)
	{
		return _Device.ReceiveDisable();
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceReceiveDisable");
}

int DEVICEDLLSPEC DeviceReceiveFifo(void)
{
	if(_Device.ReceiveFifo!=0)
	{
		return _Device.ReceiveFifo();
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceReceiveFifo");
}

int DEVICEDLLSPEC DeviceReceiveDescriptorMaximum(void)
{
	if(_Device.ReceiveDescriptorMaximum!=0)
	{
		return _Device.ReceiveDescriptorMaximum();
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceReceiveDescriptorMaximum");
}

int DEVICEDLLSPEC DeviceReceiveEnableFirst(void)
{
	if(_Device.ReceiveEnableFirst!=0)
	{
		return _Device.ReceiveEnableFirst();
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceReceiveEnableFirst");
}

int DEVICEDLLSPEC DeviceTransmitFifo(void)
{
	if(_Device.TransmitFifo!=0)
	{
		return _Device.TransmitFifo();
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceTransmitFifo");
}

int DEVICEDLLSPEC DeviceTransmitDescriptorSplit(void)
{
	if(_Device.TransmitDescriptorSplit!=0)
	{
		return _Device.TransmitDescriptorSplit();
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceTransmitDescriptorSplit");
}

int DEVICEDLLSPEC DeviceTransmitAggregateStatus(void)
{
	if(_Device.TransmitAggregateStatus!=0)
	{
		return _Device.TransmitAggregateStatus();
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceTransmitAggregateStatus");
}

int DEVICEDLLSPEC DeviceTransmitEnableFirst(void)
{
	if(_Device.TransmitEnableFirst!=0)
	{
		return _Device.TransmitEnableFirst();
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceTransmitEnableFirst");
}

int DEVICEDLLSPEC DeviceTransmitDescriptorStatusPointer(unsigned int first, unsigned int last)
{
	if(_Device.TransmitDescriptorStatusPointer!=0)
	{
		return _Device.TransmitDescriptorStatusPointer(first,last);
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceTransmitDescriptorStatusPointer");
}


int DEVICEDLLSPEC DeviceTransmitDescriptorPointer(int queue, unsigned int descriptor)
{
	if(_Device.TransmitDescriptorPointer!=0)
	{
		return _Device.TransmitDescriptorPointer(queue,descriptor);
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceTransmitDescriptorPointer");
}


int DEVICEDLLSPEC DeviceTransmitRetryLimit(int dcu, int retry)
{
	if(_Device.TransmitRetryLimit!=0)
	{
		return _Device.TransmitRetryLimit(dcu,retry);
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceTransmitRetryLimit");
}


int DEVICEDLLSPEC DeviceTransmitQueueSetup(int qcu, int dcu)
{
	if(_Device.TransmitQueueSetup!=0)
	{
		return _Device.TransmitQueueSetup(qcu,dcu);
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceTransmitQueueSetup");
}


int DEVICEDLLSPEC DeviceReceiveDeafMode(int deaf)
{
	if(_Device.ReceiveDeafMode!=0)
	{
		return _Device.ReceiveDeafMode(deaf);
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceReceiveDeafMode");
}


int DEVICEDLLSPEC DeviceTransmitRegularData(void)			// normal
{
	if(_Device.TransmitRegularData!=0)
	{
		return _Device.TransmitRegularData();
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceTransmitRegularData");
}


int DEVICEDLLSPEC DeviceTransmitFrameData(int ifs)	// tx99
{
	if(_Device.TransmitFrameData!=0)
	{
		return _Device.TransmitFrameData(ifs);
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceTransmitFrameData");
}


int DEVICEDLLSPEC DeviceTransmitContinuousData(void)		// tx100
{
	if(_Device.TransmitContinuousData!=0)
	{
		return _Device.TransmitContinuousData();
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceTransmitContinuousData");
}


int DEVICEDLLSPEC DeviceTransmitCarrier(unsigned int txchain)					// carrier only
{
	if(_Device.TransmitCarrier!=0)
	{
		return _Device.TransmitCarrier(txchain);
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceTransmitCarrier");
}


int DEVICEDLLSPEC DeviceTransmitEnable(unsigned int qmask)
{
	if(_Device.TransmitEnable!=0)
	{
		return _Device.TransmitEnable(qmask);
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceTransmitEnable");
}


int DEVICEDLLSPEC DeviceTransmitDisable(unsigned int qmask)
{
	if(_Device.TransmitDisable!=0)
	{
		return _Device.TransmitDisable(qmask);
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceTransmitDisable");
}

int DEVICEDLLSPEC DeviceTransmitPowerSet(int mode, double txp)
{
	if(_Device.TransmitPowerSet!=0)
	{
		return _Device.TransmitPowerSet(mode, txp);
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceTransmitPowerSet");
}

int DEVICEDLLSPEC DeviceTransmitGainRead(int entry, unsigned int *rvalue, int *value, int max)
{
	if(_Device.TransmitGainRead!=0)
	{
		return _Device.TransmitGainRead(entry,rvalue,value,max);
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceTransmitGainRead");
}

int DEVICEDLLSPEC DeviceTransmitINIGainGet(int *total_gain)
{
	if(_Device.TransmitINIGainGet!=0)
	{
		return _Device.TransmitINIGainGet(total_gain);
	}
	return -ErrorPrint(DeviceNoFunction,"TransmitINIGainGet");
}

int DEVICEDLLSPEC DeviceTxGainTableRead_AddressGainTable(unsigned int **address, unsigned int *row, unsigned int *col)
{
	if(_Device.TxGainTableRead_AddressGainTable!=0)
	{
		return _Device.TxGainTableRead_AddressGainTable(address, row, col);
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceTxGainTableRead_AddressGainTable");
}

int DEVICEDLLSPEC DeviceTxGainTableRead_AddressHeader(unsigned int address, char *header, char *subheader, int max)
{
	if(_Device.TxGainTableRead_AddressHeader!=0)
	{
		return _Device.TxGainTableRead_AddressHeader(address, header, subheader, max);
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceTxGainTableRead_AddressHeader");
}

int DEVICEDLLSPEC Device_get_corr_coeff(int coeff_type, unsigned int **coeff_array, unsigned int *row, unsigned int *col)
{
	if(_Device.Get_corr_coeff!=0)
	{
		return _Device.Get_corr_coeff(coeff_type, coeff_array, row, col);
	}
	return -ErrorPrint(DeviceNoFunction,"Device_get_corr_coeff");
}

int DEVICEDLLSPEC DeviceTxGainTableRead_AddressValue(unsigned int address, int idx, char *rName, char *fName, int *value, int *low, int *high)
{
	if(_Device.TxGainTableRead_AddressValue!=0)
	{
		return _Device.TxGainTableRead_AddressValue(address, idx, rName, fName, value, low, high);
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceTxGainTableRead_AddressValue");
}

int DEVICEDLLSPEC DeviceTransmitGainWrite(int entry, int *value, int nvalue)
{
	if(_Device.TransmitGainWrite!=0)
	{
		return _Device.TransmitGainWrite(entry,value,nvalue);
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceTransmitGainWrite");
}

int DEVICEDLLSPEC DeviceTransmitGainSet(int mode, int pcdac)
{
	if(_Device.TransmitGainSet!=0)
	{
		return _Device.TransmitGainSet(mode,pcdac);
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceTransmitGainSet");
}


int DEVICEDLLSPEC DeviceEepromRead(unsigned int address, unsigned char *value, int count)
{
	if(_Device.EepromRead!=0)
	{
		return _Device.EepromRead(address, value, count);
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceEepromRead");
}

int DEVICEDLLSPEC DeviceEepromWrite(unsigned int address, unsigned char *value, int count)
{
	if(_Device.EepromWrite!=0)
	{
		return _Device.EepromWrite(address, value, count);
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceEepromWrite");
}

int DEVICEDLLSPEC DeviceFlashRead(unsigned int address, unsigned char *value, int count)
{
	if(_Device.FlashRead!=0)
	{
		return _Device.FlashRead(address, value, count);
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceFlashRead");
}

int DEVICEDLLSPEC DeviceFlashWrite(unsigned int address, unsigned char *value, int count)
{
	if(_Device.FlashWrite!=0)
	{
		return _Device.FlashWrite(address, value, count);
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceFlashWrite");
}

int DEVICEDLLSPEC DeviceConfigurationRestore(void)
{
	if(_Device.ConfigurationRestore!=0)
	{
		return _Device.ConfigurationRestore();
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceConfigurationRestore");
}

int DEVICEDLLSPEC DeviceConfigurationSave(void)
{
	if(_Device.ConfigurationSave!=0)
	{
		return _Device.ConfigurationSave();
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceConfigurationSave");
}

int DEVICEDLLSPEC DeviceCalibrationPierSet(int pier, int frequency, int chain, int correction, int voltage, int temperature)
{
	if(_Device.CalibrationPierSet!=0)
	{
		return _Device.CalibrationPierSet(pier,frequency,chain,correction,voltage,temperature);
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceCalibrationPierSet");
}

int DEVICEDLLSPEC DeviceCalibrationPower(int txgain, double txpower)
{
	if(_Device.CalibrationPower!=0)
	{
		return _Device.CalibrationPower(txgain,txpower);
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceCalibrationPower");
}

int DEVICEDLLSPEC DeviceCalibrationTxgainCAPSet(int *txgainmax)
{
	if(_Device.CalibrationTxgainCAPSet!=0)
	{
		return _Device.CalibrationTxgainCAPSet(txgainmax);
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceCalibrationTxgainCAPSet");
}

int DEVICEDLLSPEC DevicePowerControlOverride(int frequency, int *correction, int *voltage, int *temperature)
{
	if(_Device.PowerControlOverride!=0)
	{
		return _Device.PowerControlOverride(frequency, correction,voltage,temperature);
	}
	return -ErrorPrint(DeviceNoFunction,"DevicePowerControlOverride");
}

int DEVICEDLLSPEC DeviceTargetPowerGet(int frequency, int rate, double *power)
{
	if(_Device.TargetPowerGet!=0)
	{
		return _Device.TargetPowerGet(frequency, rate, power);
	}
	*power=0;
	return -ErrorPrint(DeviceNoFunction,"DeviceTargetPowerGet");
}

int DEVICEDLLSPEC DeviceTargetPowerApply(int frequency)
{
	if(_Device.TargetPowerApply!=0)
	{
		return _Device.TargetPowerApply(frequency);
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceTargetPowerApply");
}

int DEVICEDLLSPEC DeviceOtpRead(unsigned int address, unsigned char *value, int count, int is_wifi)
{
	if(_Device.OtpRead!=0)
	{
		return _Device.OtpRead(address, value, count, is_wifi);
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceOtpRead");
}

int DEVICEDLLSPEC DeviceOtpWrite(unsigned int address, unsigned char *value, int count, int is_wifi)
{
	if(_Device.OtpWrite!=0)
	{
		return _Device.OtpWrite(address, value, count, is_wifi);
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceOtpWrite");
}
//
// read the shared memory
//
int DEVICEDLLSPEC DeviceMemoryBase(void)
{
	if(_Device.MemoryBase!=0)
	{
		return _Device.MemoryBase();
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceMemoryBase");
}

DEVICEDLLSPEC unsigned char *DeviceMemoryPtr(unsigned int address)
{
	if(_Device.MemoryPtr!=0)
	{
		return _Device.MemoryPtr(address);
	}
	ErrorPrint(DeviceNoFunction,"DeviceMemoryPtr");
	return 0;
}

int DEVICEDLLSPEC DeviceMemoryRead(unsigned int address, unsigned int *value, int count)
{
	if(_Device.MemoryRead!=0)
	{
		return _Device.MemoryRead(address, value, count);
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceMemoryRead");
}

//
// write the shared memory
//
int DEVICEDLLSPEC DeviceMemoryWrite(unsigned int address, unsigned int *value, int count)
{
	if(_Device.MemoryWrite!=0)
	{
		return _Device.MemoryWrite(address, value, count);
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceMemoryWrite");
}

//
// read a register
//
int DEVICEDLLSPEC DeviceRegisterRead(unsigned int address, unsigned int *value)
{
	if(_Device.RegisterRead!=0)
	{
		return _Device.RegisterRead(address, value);
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceRegisterRead");
}

//
// write a register
//
int DEVICEDLLSPEC DeviceRegisterWrite(unsigned int address, unsigned int value)
{
	if(_Device.RegisterWrite!=0)
	{
		return _Device.RegisterWrite(address, value);
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceRegisterWrite");
}

int DEVICEDLLSPEC DeviceFieldRead(unsigned int address, int low, int high, unsigned int *value)
{
    if(_Device.FieldRead!=0)
    {
        return _Device.FieldRead(address, low, high, value);
    }
    return -ErrorPrint(DeviceNoFunction,"DeviceFieldRead");
}

int DEVICEDLLSPEC DeviceFieldWrite(unsigned int address, int low, int high, unsigned int value)
{
    if(_Device.FieldWrite!=0)
    {
        return _Device.FieldWrite(address, low, high, value);
    }
    return -ErrorPrint(DeviceNoFunction,"DeviceFieldWrite");
}

//
// read temperature from chip
//
int DEVICEDLLSPEC DeviceTemperatureGet(int forceTempRead)
{
	//UserPrint("caller %s\n", caller);
	if(_Device.TemperatureGet!=0)
	{
		return _Device.TemperatureGet(forceTempRead);
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceTemperatureGet");
}

//
// read voltgae from chip
//
int DEVICEDLLSPEC DeviceVoltageGet(void)
{
	if(_Device.VoltageGet!=0)
	{
		return _Device.VoltageGet();
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceVoltageGet");
}

#ifdef ATH_SUPPORT_MCI
int DEVICEDLLSPEC DeviceMCISetup(void)
{
	if(_Device.MCISetup!=0)
	{
		return _Device.MCISetup();
	}
	return -ErrorPrint(DeviceNoFunction,"MCISetup");
}

int DEVICEDLLSPEC DeviceMCIReset(int is_2GHz)
{
	if(_Device.MCIReset!=0)
	{
		return _Device.MCIReset(is_2GHz);
	}
	return -ErrorPrint(DeviceNoFunction,"MCIReset");
}
#endif

//
// set MAC address to chip
//
int DEVICEDLLSPEC DeviceMacAddressSet(unsigned char mac[6])
{
	if(_Device.MacAddressSet!=0)
	{
		return _Device.MacAddressSet(mac);
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceMacAddressSet");
}

//
// get MAC address from chip
//
int DEVICEDLLSPEC DeviceMacAddressGet(unsigned char mac[6])
{
	if(_Device.MacAddressGet!=0)
	{
		return _Device.MacAddressGet(mac);
	}
	memset(mac,0,6);
	return -ErrorPrint(DeviceNoFunction,"DeviceMacAddressGet");
}

int DEVICEDLLSPEC DeviceCustomerDataGet(unsigned char *data, int max)
{
	if(_Device.CustomerDataGet!=0)
	{
		return _Device.CustomerDataGet(data, max);
	}
	memset(data,0,max);
	return -ErrorPrint(DeviceNoFunction,"DeviceCustomerDataGet");
}

int DEVICEDLLSPEC DeviceRxChainSet(int rxchain)
{
	if(_Device.RxChainSet!=0)
	{
		return _Device.RxChainSet(rxchain);
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceRxChainSet");
}

int DEVICEDLLSPEC DeviceEepromTemplatePreference(int value)
{
	if(_Device.EepromTemplatePreference!=0)
	{
		_Device.EepromTemplatePreference(value);
        return 0;
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceEepromTemplatePreference");
}

int DEVICEDLLSPEC DeviceEepromTemplateAllowed(unsigned int *value, unsigned int many)
{
	if(_Device.EepromTemplateAllowed!=0)
	{
		_Device.EepromTemplateAllowed(value, many);
        return 0;
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceEepromTemplateAllowed");
}

int DEVICEDLLSPEC DeviceEepromCompress(unsigned int value)
{
	if(_Device.EepromCompress!=0)
	{
		_Device.EepromCompress(value);
        return 0;
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceEepromCompress");
}

int DEVICEDLLSPEC DeviceEepromOverwrite(unsigned int value)
{
	if(_Device.EepromOverwrite!=0)
	{
		_Device.EepromOverwrite(value);
        return 0;
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceEepromOverwrite");
}

int DEVICEDLLSPEC DeviceEepromSize(void)
{
	if(_Device.EepromSize!=0)
	{
		return _Device.EepromSize();
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceEepromSize");
}

int DEVICEDLLSPEC DeviceEepromSaveMemorySet(int memory)
{
	if(_Device.EepromSaveMemorySet!=0)
	{
		_Device.EepromSaveMemorySet(memory);
        return 0;
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceEepromSaveMemorySet");
}

int DEVICEDLLSPEC DeviceCalibrationDataAddressSet(int size)
{
	if(_Device.CalibrationDataAddressSet!=0)
	{
		_Device.CalibrationDataAddressSet(size);
        return 0;
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceCalibrationDataAddressSet");
}

int DEVICEDLLSPEC DeviceCalibrationDataAddressGet(void)
{
	if(_Device.CalibrationDataAddressGet!=0)
	{
		return _Device.CalibrationDataAddressGet();
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceCalibrationDataAddressGet");
}

int DEVICEDLLSPEC DeviceCalibrationDataSet( int source)
{
	if(_Device.CalibrationDataSet!=0)
	{
		_Device.CalibrationDataSet(source);
        return 0;
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceCalibrationDataSet");
}

int DEVICEDLLSPEC DeviceCalibrationDataGet(void)
{
	if(_Device.CalibrationDataGet!=0)
	{
		return _Device.CalibrationDataGet();
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceCalibrationDataGet");
}

int DEVICEDLLSPEC DeviceCalibrationFromEepromFile(void)
{
	if(_Device.CalibrationFromEepromFile!=0)
	{
		return _Device.CalibrationFromEepromFile();
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceCalibrationFromEepromFile");
}

int DEVICEDLLSPEC DeviceEepromTemplateInstall(int value)
{
	if(_Device.EepromTemplateInstall!=0)
	{
		return _Device.EepromTemplateInstall(value);
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceEepromTemplateInstall");
}

int DEVICEDLLSPEC DeviceEepromReport(void (*print)(char *format, ...), int all)
{
	if(_Device.EepromReport!=0)
	{
		return _Device.EepromReport(print, all);
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceEepromReport");
}

int DEVICEDLLSPEC DeviceRegulatoryDomainGet(void)
{
    if(_Device.RegulatoryDomainGet!=0)
	{
		return _Device.RegulatoryDomainGet();
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceRegulatoryDomainGet");
}

int DEVICEDLLSPEC DeviceRegulatoryDomainOverride(unsigned int regdmn)
{
    if(_Device.RegulatoryDomainOverride!=0)
	{
        return (_Device.RegulatoryDomainOverride(regdmn)/* == AH_TRUE ? 0 : -1*/);
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceRegulatoryDomainOverride");
}

int DEVICEDLLSPEC DeviceNoiseFloorSet(int frequency, int ichain, int nf)
{
	if(_Device.NoiseFloorSet!=0)
	{
		return _Device.NoiseFloorSet( frequency, ichain, nf);
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceNoiseFloorSet");
}

int DEVICEDLLSPEC DeviceNoiseFloorGet(int frequency, int ichain)
{
	if(_Device.NoiseFloorGet!=0)
	{
		return _Device.NoiseFloorGet(frequency, ichain);
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceNoiseFloorGet");
}

int DEVICEDLLSPEC DeviceNoiseFloorPowerSet(int frequency, int ichain, int nfpower)
{
	if(_Device.NoiseFloorPowerSet!=0)
	{
		return _Device.NoiseFloorPowerSet(frequency, ichain, nfpower);
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceNoiseFloorPowerSet");
}

int DEVICEDLLSPEC DeviceNoiseFloorPowerGet(int frequency, int ichain)
{
	if(_Device.NoiseFloorPowerGet!=0)
	{
		return _Device.NoiseFloorPowerGet(frequency, ichain);
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceNoiseFloorPowerGet");
}

int DEVICEDLLSPEC DeviceNoiseFloorTemperatureSet(int frequency, int ichain, int temperature)
{
	if(_Device.NoiseFloorTemperatureSet!=0)
	{
		return _Device.NoiseFloorTemperatureSet(frequency, ichain, temperature);
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceNoiseFloorTemperatureSet");
}

int DEVICEDLLSPEC DeviceNoiseFloorTemperatureGet(int frequency, int ichain)
{
	if(_Device.NoiseFloorTemperatureGet!=0)
	{
		return _Device.NoiseFloorTemperatureGet(frequency, ichain);
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceNoiseFloorTemperatureGet");
}

int DEVICEDLLSPEC DevicePaPredistortionSet(int value)
{
	if(_Device.PaPredistortionSet!=0)
	{
		return _Device.PaPredistortionSet(value);
	}
	return -ErrorPrint(DeviceNoFunction,"DevicePaPredistortionSet");
}

int DEVICEDLLSPEC DevicePsatCalibration(int frequency, unsigned int txchain, unsigned int rxchain, int bandwidth)
{
	if(_Device.PsatCalibration!=0)
	{
		return _Device.PsatCalibration(frequency, txchain, rxchain, bandwidth);
	}
	return -ErrorPrint(DeviceNoFunction,"DevicePsatCalibration");
}
	
int DEVICEDLLSPEC DevicePaPredistortionGet(void)
{
	if(_Device.PaPredistortionGet!=0)
	{
		return _Device.PaPredistortionGet();
	}
	return -ErrorPrint(DeviceNoFunction,"DevicePaPredistortionGet");
}

int DEVICEDLLSPEC DeviceHeavyClipEnableSet(unsigned int enable)
{
	if (_Device.HeavyClipEnableSet != 0)
	{
		return _Device.HeavyClipEnableSet(enable);
	}

	return -ErrorPrint(DeviceNoFunction,"DeviceHeavyClipEnableSet");
}

int DEVICEDLLSPEC DeviceHeavyClipEnableGet(unsigned int *enabled)
{
	if (_Device.HeavyClipEnableGet != 0)
	{
		return _Device.HeavyClipEnableGet(enabled);
	}

	return -ErrorPrint(DeviceNoFunction,"DeviceHeavyClipEnableGet");
}

int DEVICEDLLSPEC DeviceChainMany()
{
	if(_Device.TxChainMany!=0)
	{
		return _Device.TxChainMany();
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceTxChainMany");
}

int DEVICEDLLSPEC DeviceRxChainMany()
{
	if(_Device.RxChainMany!=0)
	{
		return _Device.RxChainMany();
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceRxChainMany");
}

int DEVICEDLLSPEC DeviceTxChainMany()
{
	return DeviceChainMany();
}

int DEVICEDLLSPEC DeviceTxChainMask()
{
	if(_Device.TxChainMask!=0)
	{
		return _Device.TxChainMask();
	}
	switch(DeviceTxChainMany())
	{
		case 1:
			return 1;
		case 2:
			return 3;
		case 3:
			return 7;
	}
	return 0;
}

int DEVICEDLLSPEC DeviceRxChainMask()
{
	if(_Device.RxChainMask!=0)
	{
		return _Device.RxChainMask();
	}
	switch(DeviceRxChainMany())
	{
		case 1:
			return 1;
		case 2:
			return 3;
		case 3:
			return 7;
	}
	return 0;
}
	
int DEVICEDLLSPEC DeviceSpectralScanEnable(void)
{
	if(_Device.SpectralScanEnable!=0)
	{
		return _Device.SpectralScanEnable();
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceSpectralScanEnable");
}

int DEVICEDLLSPEC DeviceSpectralScanProcess(unsigned char *data, int ndata, int *spectrum, int max)
{
	if(_Device.SpectralScanProcess!=0)
	{
		return _Device.SpectralScanProcess(data,ndata,spectrum,max);
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceSpectralScanProcess");
}

int DEVICEDLLSPEC DeviceSpectralScanDisable(void)
{
	if(_Device.SpectralScanEnable!=0)
	{
		return _Device.SpectralScanDisable();
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceSpectralScanDisable");
}


int DEVICEDLLSPEC DeviceTuningCapsSet(int caps)
{
	if(_Device.TuningCapsSet!=0)
	{
		return _Device.TuningCapsSet(caps);
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceTuningCapsSet");
}

int DEVICEDLLSPEC DeviceTuningCapsSave(int caps)
{
	if(_Device.TuningCapsSave!=0)
	{
		return _Device.TuningCapsSave(caps);
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceTuningCapsSave");
}

int DEVICEDLLSPEC DeviceChannelCalculate(int *frequency, int *option, int maxchannel)
{
	if(_Device.ChannelCalculate!=0)
	{
		return _Device.ChannelCalculate(frequency,option,maxchannel);
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceChannelCalculate");
}

DEVICEDLLSPEC int DeviceInitializationCommit(void)
{
	if(_Device.InitializationCommit!=0)
	{
		return _Device.InitializationCommit();
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceInitializationCommit");
}

DEVICEDLLSPEC int DeviceInitializationUsed(void)
{
	if(_Device.InitializationUsed!=0)
	{
		return _Device.InitializationUsed();
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceInitializationUsed");
}

DEVICEDLLSPEC int DeviceInitializationSubVendorSet(unsigned int subvendor)
{
	if(_Device.InitializationSubVendorSet!=0)
	{
		return _Device.InitializationSubVendorSet(subvendor);
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceInitializationSubVendorSet");
}

DEVICEDLLSPEC int DeviceInitializationSubVendorGet(unsigned int *subvendor)
{
	if(_Device.InitializationSubVendorGet!=0)
	{
		return _Device.InitializationSubVendorGet(subvendor);
	}
	*subvendor=0;
	return -ErrorPrint(DeviceNoFunction,"DeviceInitializationSubVendorGet");
}

DEVICEDLLSPEC int DeviceInitializationVendorSet(unsigned int vendor)
{
	if(_Device.InitializationVendorSet!=0)
	{
		return _Device.InitializationVendorSet(vendor);
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceInitializationVendorSet");
}

DEVICEDLLSPEC int DeviceInitializationVendorGet(unsigned int *vendor)
{
	if(_Device.InitializationVendorGet!=0)
	{
		return _Device.InitializationVendorGet(vendor);
	}
	*vendor=0;
	return -ErrorPrint(DeviceNoFunction,"DeviceInitializationVendorGet");
}

DEVICEDLLSPEC int DeviceInitializationSsidSet(unsigned int ssid)
{
	if(_Device.InitializationSsidSet!=0)
	{
		return _Device.InitializationSsidSet(ssid);
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceInitializationSsidSet");
}

DEVICEDLLSPEC int DeviceInitializationSsidVendorGet(unsigned int *ssid)
{
	if(_Device.InitializationSsidVendorGet!=0)
	{
		return _Device.InitializationSsidVendorGet(ssid);
	}
	*ssid=0;
	return -ErrorPrint(DeviceNoFunction,"DeviceInitializationSsidVendorGet");
}

DEVICEDLLSPEC int DeviceInitializationDevidSet(unsigned int devid)
{
	if(_Device.InitializationDevidSet!=0)
	{
		return _Device.InitializationDevidSet(devid);
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceInitializationDevidSet");
}

DEVICEDLLSPEC int DeviceInitializationDevidGet(unsigned int *devid)
{
	if(_Device.InitializationDevidGet!=0)
	{
		return _Device.InitializationDevidGet(devid);
	}
	*devid=0;
	return -ErrorPrint(DeviceNoFunction,"DeviceInitializationDevidGet");
}

DEVICEDLLSPEC int DeviceInitializationSet(unsigned int address, unsigned int value, int size)
{
	if(_Device.InitializationSet!=0)
	{
		return _Device.InitializationSet(address,value, size);
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceInitializationSet");
}

DEVICEDLLSPEC int DeviceInitializationGet(unsigned int address, unsigned int *value)
{
	if(_Device.InitializationGet!=0)
	{
		return _Device.InitializationGet(address,value);
	}
	*value=0;
	return -ErrorPrint(DeviceNoFunction,"DeviceInitializationGet");
}

DEVICEDLLSPEC int DeviceInitializationMany(void)
{
	if(_Device.InitializationMany!=0)
	{
		return _Device.InitializationMany();
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceInitializationMany");
}

DEVICEDLLSPEC int DeviceInitializationGetByIndex(int index, unsigned int *address, unsigned int *value)
{
	if(_Device.InitializationGetByIndex!=0)
	{
		return _Device.InitializationGetByIndex(index,address,value);
	}
	*address=0;
	*value=0;
	return -ErrorPrint(DeviceNoFunction,"DeviceInitializationGetByIndex");
}

DEVICEDLLSPEC int DeviceInitializationRemove(unsigned int address, int size)
{
	if(_Device.InitializationRemove!=0)
	{
		return _Device.InitializationRemove(address, size);
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceInitializationRemove");
}

DEVICEDLLSPEC int DeviceInitializationRestore(void)
{
	if(_Device.InitializationRestore!=0)
	{
		return _Device.InitializationRestore();
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceInitializationRestore");
}

DEVICEDLLSPEC int DeviceNoiseFloorFetch(int *nfc, int *nfe, int nfn)
{
	if(_Device.NoiseFloorFetch!=0)
	{
		return _Device.NoiseFloorFetch(nfc,nfe,nfn);
	}
	memset(nfc,0,nfn*sizeof(int));
	memset(nfe,0,nfn*sizeof(int));
	return -ErrorPrint(DeviceNoFunction,"DeviceNoiseFloorFetch");
}

DEVICEDLLSPEC int DeviceNoiseFloorLoad(int *nfc, int *nfe, int nfn)
{
	if(_Device.NoiseFloorLoad!=0)
	{
		return _Device.NoiseFloorLoad(nfc,nfe,nfn);
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceNoiseFloorLoad");
}

DEVICEDLLSPEC int DeviceNoiseFloorReady(void)
{
	if(_Device.NoiseFloorReady!=0)
	{
		return _Device.NoiseFloorReady();
	}
	ErrorPrint(DeviceNoFunction,"DeviceNoiseFloorReady");
	return 0;
}

DEVICEDLLSPEC int DeviceNoiseFloorEnable(void)
{
	if(_Device.NoiseFloorEnable!=0)
	{
		return _Device.NoiseFloorEnable();
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceNoiseFloorEnable");
}

int DEVICEDLLSPEC DeviceOpflagsGet(void)
{
	if(_Device.OpflagsGet!=0)
	{
		return _Device.OpflagsGet();
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceOpflagsGet");
}

int DEVICEDLLSPEC DeviceIs2GHz(void)
{
	if(_Device.Is2GHz!=0)
	{
		return _Device.Is2GHz();
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceIs2GHz");
}

int DEVICEDLLSPEC DeviceIs5GHz(void)
{
	if(_Device.Is5GHz!=0)
	{
		return _Device.Is5GHz();
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceIs5GHz");
}

int DEVICEDLLSPEC DeviceIs4p9GHz(void)
{
	if(_Device.Is4p9GHz!=0)
	{
		return _Device.Is4p9GHz();
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceIs4p9GHz");
}

int DEVICEDLLSPEC DeviceIsHalfRate(void)
{
	if(_Device.IsHalfRate!=0)
	{
		return _Device.IsHalfRate();
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceIsHalfRate");
}

int DEVICEDLLSPEC DeviceIsQuarterRate(void)
{
	if(_Device.IsQuarterRate!=0)
	{
		return _Device.IsQuarterRate();
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceIsQuarterRate");
}
//
//
// the following functions are used to set the pointers to the correct functions for a specific chip.
//
// When implementing support for a new chip a function performing each of these operations
// must be produced. See Ar5416Device.c for an example.
//
//
//

int DEVICEDLLSPEC DeviceFunctionSelect(struct _DeviceFunction *device)
{
	_Device= *device;
	//
	// later check some of the pointers to make sure there are valid functions
	//
	return 0;
}

//
// clear all device control function pointers and set to default behavior
//
void DEVICEDLLSPEC DeviceFunctionReset(void)
{
    memset(&_Device,0,sizeof(_Device));
}

int DEVICEDLLSPEC DeviceIsEmbeddedArt(void)
{
	if(_Device.IsEmbeddedArt!=0)
	{
		return _Device.IsEmbeddedArt();
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceIsEmbeddedArt");
}

int DEVICEDLLSPEC DeviceStickyWrite(int idx,unsigned int address, int low, int  high, unsigned int *value, int numVal, int prepost)
{
	if(_Device.StickyWrite!=0)
	{
		return _Device.StickyWrite(idx,address, low, high, value, numVal, prepost);
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceStickyWrite");
}

int DEVICEDLLSPEC DeviceStickyClear(int idx,unsigned int address, int low, int  high)
{
	if(_Device.StickyClear!=0)
	{
		return _Device.StickyClear(idx,address, low, high);
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceStickyClear");
}

int DEVICEDLLSPEC DeviceConfigAddrSet(unsigned int address, int low, int  high, unsigned int *value, int numVal, int prepost)
{
	if(_Device.ConfigAddrSet!=0)
	{
		return _Device.ConfigAddrSet(address, low, high, value, numVal, prepost);
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceConfigAddrSet");
}

int DEVICEDLLSPEC DeviceRfBbTestPoint(int frequency, int ht40, int bandwidth, int antennapair, unsigned char chainnum,
                       int mbgain, int rfgain, int coex, int sharedrx, int switchtable, unsigned char AnaOutEn)
{
	if(_Device.RfBbTestPoint!=0)
	{
		return _Device.RfBbTestPoint(frequency, ht40, bandwidth, antennapair, chainnum,
                       mbgain, rfgain, coex, sharedrx, switchtable, AnaOutEn);
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceRfBbTestPoint");
}

int DEVICEDLLSPEC DeviceTransmitDataDut(void *params)
{
	if(_Device.TransmitDataDut!=0)
	{
		return _Device.TransmitDataDut(params);
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceTransmitDataDut");
}

int DEVICEDLLSPEC DeviceTransmitStatusReport(void **txStatus, int stop)
{
	if(_Device.TransmitStatusReport!=0)
	{
		return _Device.TransmitStatusReport(txStatus, stop);
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceTransmitStatusReport");
}

int DEVICEDLLSPEC DeviceTransmitStop(void **txStatus, int calibrate)
{
	if(_Device.TransmitStop!=0)
	{
		return _Device.TransmitStop(txStatus, calibrate);
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceTransmitStop");
}

int DEVICEDLLSPEC DeviceReceiveDataDut(void *params)
{
	if(_Device.ReceiveDataDut!=0)
	{
		return _Device.ReceiveDataDut(params);
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceReceiveDataDut");
}

int DEVICEDLLSPEC DeviceReceiveStatusReport(void **rxStatus, int stop)
{
	if(_Device.ReceiveStatusReport!=0)
	{
		return _Device.ReceiveStatusReport(rxStatus, stop);
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceReceiveStatusReport");
}

int DEVICEDLLSPEC DeviceReceiveStop(void **rxStatus)
{
	if(_Device.ReceiveStop!=0)
	{
		return _Device.ReceiveStop(rxStatus);
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceReceiveStop");
}

int DEVICEDLLSPEC DeviceCalInfoInit()
{
	if(_Device.CalInfoInit!=0)
	{
		return _Device.CalInfoInit();
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceCalInfoInit");
}

int DEVICEDLLSPEC DeviceCalInfoCalibrationPierSet(int pier, int frequency, int chain, int gain, int gainIndex, int dacGain, double power, int correction, int voltage, int temperature, int calPoint)
{
	if(_Device.CalInfoCalibrationPierSet!=0)
	{
		return _Device.CalInfoCalibrationPierSet(pier, frequency, chain, gain, gainIndex, dacGain, power, correction, voltage, temperature, calPoint);
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceCalInfoCalibrationPierSet");
}

int DEVICEDLLSPEC DeviceCalUnusedPierSet(int iChain, int iBand, int iIndex)
{
	if(_Device.CalUnusedPierSet!=0)
	{
		return _Device.CalUnusedPierSet(iChain, iBand, iIndex);
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceCalUnusedPierSet");
}

int DEVICEDLLSPEC DeviceOtpLoad()
{
	if(_Device.OtpLoad!=0)
	{
		return _Device.OtpLoad();
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceOtpLoad");
}

int DEVICEDLLSPEC DeviceSetConfigParameterSplice(struct _ParameterList *list)
{
	if(_Device.SetConfigParameterSplice!=0)
	{
		return _Device.SetConfigParameterSplice(list);
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceSetConfigParameterSplice");
}

int DEVICEDLLSPEC DeviceSetConfigCommand(void *cmd)
{
	if(_Device.SetConfigCommand!=0)
	{
		return _Device.SetConfigCommand(cmd);
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceSetConfigCommand");
}

int DEVICEDLLSPEC DeviceStbcGet()
{
	if(_Device.StbcGet!=0)
	{
		return _Device.StbcGet();
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceStbcGet");
}

int DEVICEDLLSPEC DeviceLdpcGet()
{
	if(_Device.LdpcGet!=0)
	{
		return _Device.LdpcGet();
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceLdpcGet");
}

int DEVICEDLLSPEC DevicePapdGet()
{
	if(_Device.PapdGet!=0)
	{
		return _Device.PapdGet();
	}
	return -ErrorPrint(DeviceNoFunction,"DevicePapdGet");
}

int DEVICEDLLSPEC DeviceEepromSaveSectionSet(unsigned int sectionMask)
{
    if(_Device.EepromSaveSectionSet!=0)
	{
		return _Device.EepromSaveSectionSet(sectionMask);
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceEepromSaveSectionSet");
}

int DEVICEDLLSPEC DevicePapdIsDone(int txPwr)
{
    if(_Device.PapdIsDone!=0)
	{
		return _Device.PapdIsDone(txPwr);
	}
	return -ErrorPrint(DeviceNoFunction,"DevicePapdIsDone");
}

DEVICEDLLSPEC char *DeviceIniVersion(int devid)
{
	if(_Device.IniVersion!=0)
	{
		return _Device.IniVersion(devid);
	}
	ErrorPrint(DeviceNoFunction,"DeviceIniVersion");
	return 0;
}


int DEVICEDLLSPEC DeviceXtalReferencePPMGet(void)
{
    if(_Device.XtalReferencePPMGet!=0)
	{
		return _Device.XtalReferencePPMGet();
	}
	ErrorPrint(DeviceNoFunction,"DeviceXtalReferencePPMGet");
	return 0;
}

DEVICEDLLSPEC double DeviceCmacPowerGet(int chain)
{
    if(_Device.CmacPowerGet!=0)
    {
        return _Device.CmacPowerGet(chain);
    }
    ErrorPrint(DeviceNoFunction,"DeviceCmacPowerGet");
    return 0.0;
}

DEVICEDLLSPEC int DevicePsatCalibrationResultGet(int frequency, int chain, int *olpc_dalta, int *thermal, double *cmac_power_olpc, double *cmac_power_psat, unsigned int *olcp_pcdac, unsigned int *psat_pcdac)
{
    if(_Device.PsatCalibrationResultGet != 0)
    {
        return _Device.PsatCalibrationResultGet(frequency, chain, olpc_dalta, thermal, cmac_power_olpc, cmac_power_psat, olcp_pcdac, psat_pcdac);
    }
    ErrorPrint(DeviceNoFunction,"DevicePsatCalibrationResultGet");
    return 0;
}

int DEVICEDLLSPEC DeviceDiagData(unsigned char *data, unsigned int ndata, unsigned char *respdata, unsigned int *nrespdata)
{
	if(_Device.DiagData!=0)
	{
		return _Device.DiagData(data,ndata,respdata,nrespdata);
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceDiagData");
}
int DEVICEDLLSPEC DeviceSleepMode(int mode)
{
	if(_Device.SleepMode!=0)
	{
		return _Device.SleepMode(mode);
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceSleepMode");
}

int DEVICEDLLSPEC DeviceDeviceHandleGet(unsigned int *handle)
{
	if(_Device.DeviceHandleGet!=0)
	{
		return _Device.DeviceHandleGet(handle);
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceDeviceHandleGet");
}

int DeviceReadPciConfigSpace(unsigned int offset, unsigned int *value)
{
	if(_Device.ReadPciConfigSpace!=0)
	{
		return _Device.ReadPciConfigSpace(offset, value);
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceReadPciConfigSpace");
}

int DeviceWritePciConfigSpace(unsigned int offset, unsigned int value)
{
	if(_Device.WritePciConfigSpace!=0)
	{
		return _Device.WritePciConfigSpace(offset, value);
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceWritePciConfigSpace");
}

int DEVICEDLLSPEC DeviceIs11ACDevice(void)
{
	if(_Device.Is11ACDevice!=0)
	{
		return _Device.Is11ACDevice();
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceIs11ACDevice");
}

int DEVICEDLLSPEC DeviceNonCenterFreqAllowedGet()
{
	if(_Device.NonCenterFreqAllowedGet!=0)
	{
		return _Device.NonCenterFreqAllowedGet();
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceNonCenterFreqAllowedGet");
}

DEVICEDLLSPEC double DeviceGainTableOffset(void)
{
	if(_Device.GainTableOffset!=0)
	{
		return _Device.GainTableOffset();
	}
	ErrorPrint(DeviceNoFunction,"DeviceGainTableOffset");
	return 0.0;
}

int DEVICEDLLSPEC DeviceCalibrationSetting(void)
{
	if(_Device.CalibrationSetting!=0)
	{
		return _Device.CalibrationSetting();
	}
	return -ErrorPrint(DeviceNoFunction,"DeviceCalibrationSetting");
}

//=================================================================
static struct _TlvDeviceFunction _TlvDevice;

int DEVICEDLLSPEC TlvDeviceFunctionSelect(struct _TlvDeviceFunction *device)
{
	_TlvDevice= *device;
	//
	// later check some of the pointers to make sure there are valid functions
	//
	return 0;
}

void DEVICEDLLSPEC TlvDeviceFunctionReset(void)
{
    memset(&_TlvDevice,0,sizeof(_TlvDevice));
}

int DevicetlvCallbackSet(_tlvCallback p_tlvCallback)
{
	if(_TlvDevice.tlvCallbackSet!=0)
	{
		return _TlvDevice.tlvCallbackSet(p_tlvCallback);
	}
	return -ErrorPrint(DeviceNoFunction,"DevicetlvCallbackSet");
} 
int DevicetlvCreate(unsigned char cmd)
{
	if(_TlvDevice.tlvCreate!=0)
	{
		return _TlvDevice.tlvCreate(cmd);
	}
	return -ErrorPrint(DeviceNoFunction,"DevicetlvCreate");
} 
int DevicetlvComplete()
{
	if(_TlvDevice.tlvComplete!=0)
	{
		return _TlvDevice.tlvComplete();
	}
	return -ErrorPrint(DeviceNoFunction,"DevicetlvComplete");
} 
int DevicetlvAddParam(char *pKey, char *pData)
{
	if(_TlvDevice.tlvAddParam!=0)
	{
		return _TlvDevice.tlvAddParam(pKey, pData);
	}
	return -ErrorPrint(DeviceNoFunction,"DevicetlvAddParam");
} 
int DevicetlvGetRspParam(char *pKey, char *pData)
{
	if(_TlvDevice.tlvGetRspParam!=0)
	{
		return _TlvDevice.tlvGetRspParam(pKey, pData);
	}
	return -ErrorPrint(DeviceNoFunction,"DevicetlvGetRspParam");
} 
int DevicetlvCalibration(double pwr)
{
	if(_TlvDevice.tlvCalibration!=0)
	{
		return _TlvDevice.tlvCalibration(pwr);
	}
	return -ErrorPrint(DeviceNoFunction,"DevicetlvCalibration");
} 
int DevicetlvCalibrationInit(int mode)
{
	if(_TlvDevice.tlvCalibrationInit!=0)
	{
		return _TlvDevice.tlvCalibrationInit(mode);
	}
	return -ErrorPrint(DeviceNoFunction,"DevicetlvCalibrationInit");
} 
