#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ErrorPrint.h"
#include "DeviceError.h"
#include "DevDeviceFunction.h"

static struct _DevDeviceFunction _DevDevice;

//
// These functions are only for AR600x (mobile chips). AR93xx and AR54xx don't need these functions.
// The following functions are used to set the pointers to the correct functions for a specific chip.
// When implementing support for a new chip a function performing each of these operations
// must be produced.
//
//
//
int DevDeviceFunctionSelect(struct _DevDeviceFunction *device)
{
	_DevDevice= *device;
	//
	// later check some of the pointers to make sure there are valid functions
	//
	return 0;
}

//
// clear all device control function pointers and set to default behavior
//
void DevDeviceFunctionReset(void)
{
    memset(&_DevDevice,0,sizeof(_DevDevice));
}

int DevDeviceRegulatoryDomainGet()
{
	if(_DevDevice.RegulatoryDomainGet!=0)
	{
		return _DevDevice.RegulatoryDomainGet();
	}
	ErrorPrint(DeviceNoFunction,"DevDeviceRegulatoryDomainGet");
	return 0;
}

int DevDeviceRegulatoryDomain1Get()
{
	if(_DevDevice.RegulatoryDomain1Get!=0)
	{
		return _DevDevice.RegulatoryDomain1Get();
	}
	ErrorPrint(DeviceNoFunction,"DevDeviceRegulatoryDomain1Get");
	return 0;
}

int DevDeviceOpFlagsGet()
{
	if(_DevDevice.OpFlagsGet!=0)
	{
		return _DevDevice.OpFlagsGet();
	}
	ErrorPrint(DeviceNoFunction,"DevDeviceOpFlagsGet");
	return 0;
}

int DevDeviceOpFlags2Get()
{
	if(_DevDevice.OpFlags2Get!=0)
	{
		return _DevDevice.OpFlags2Get();
	}
	ErrorPrint(DeviceNoFunction,"DevDeviceOpFlags2Get");
	return 0;
}

int DevDeviceIs4p9GHz()
{
	if(_DevDevice.Is4p9GHz!=0)
	{
		return _DevDevice.Is4p9GHz();
	}
	ErrorPrint(DeviceNoFunction,"DevDeviceIs4p9GHz");
	return 0;
}

int DevDeviceHalfRate()
{
	if(_DevDevice.HalfRate!=0)
	{
		return _DevDevice.HalfRate();
	}
	ErrorPrint(DeviceNoFunction,"DevDeviceHalfRate");
	return 0;
}

int DevDeviceQuarterRate()
{
	if(_DevDevice.QuarterRate!=0)
	{
		return _DevDevice.QuarterRate();
	}
	ErrorPrint(DeviceNoFunction,"DevDeviceQuarterRate");
	return 0;
}

int DevDeviceCustomNameGet(char *name)
{
	if(_DevDevice.CustomNameGet!=0)
	{
		return _DevDevice.CustomNameGet(name);
	}
	ErrorPrint(DeviceNoFunction,"DevDeviceCustomNameGet");
	return 0;
}

int DevDeviceSwapCalStruct()
{
	if(_DevDevice.SwapCalStruct!=0)
	{
		return _DevDevice.SwapCalStruct();
	}
	ErrorPrint(DeviceNoFunction,"DevDeviceSwapCalStruct");
	return 0;
}
