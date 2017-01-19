

//  "ACI $Id: //depot/sw/branches/art2_main_per_cs/src/art2/shared/DeviceError.c#1 $, $Header: //depot/sw/branches/art2_main_per_cs/src/art2/shared/DeviceError.c#1 $"




#include "ErrorPrint.h"
#include "DeviceError.h"


static struct _Error _DeviceError[]=
{
    {DeviceNoFunction,ErrorDebug,DeviceNoFunctionFormat},
	{DeviceNoLoad,ErrorWarning,DeviceNoLoadFormat},
	{DeviceFound,ErrorDebug,DeviceFoundFormat},
	{DeviceMissing,ErrorDebug,DeviceMissingFormat},
	{DeviceSummary,ErrorDebug,DeviceSummaryFormat},
	{DeviceLoadBad,ErrorFatal,DeviceLoadBadFormat},
	{DeviceLoadGood,ErrorInformation,DeviceLoadGoodFormat},
	{DeviceFunction,ErrorInformation,DeviceFunctionFormat},

	{LinkNoFunction,ErrorDebug,LinkNoFunctionFormat},
	{LinkNoLoad,ErrorFatal,LinkNoLoadFormat},
	{LinkFound,ErrorDebug,LinkFoundFormat},
	{LinkMissing,ErrorDebug,LinkMissingFormat},
	{LinkSummary,ErrorDebug,LinkSummaryFormat},
	{LinkLoadBad,ErrorFatal,LinkLoadBadFormat},
	{LinkLoadGood,ErrorInformation,LinkLoadGoodFormat},

    {BusNoFunction,ErrorDebug,BusNoFunctionFormat},
	{BusNoLoad,ErrorFatal,BusNoLoadFormat},
	{BusFound,ErrorDebug,BusFoundFormat},
	{BusMissing,ErrorDebug,BusMissingFormat},
	{BusSummary,ErrorDebug,BusSummaryFormat},
	{BusLoadBad,ErrorFatal,BusLoadBadFormat},
	{BusLoadGood,ErrorInformation,BusLoadGoodFormat},

	{CalibrationNoFunction,ErrorFatal,CalibrationNoFunctionFormat},
	{CalibrationNoLoad,ErrorFatal,CalibrationNoLoadFormat},
	{CalibrationMissing,ErrorFatal,CalibrationMissingFormat},
};

static int _ErrorFirst=1;

PARSEDLLSPEC void DeviceErrorInit(void)
{
    if(_ErrorFirst)
    {
        ErrorHashCreate(_DeviceError,sizeof(_DeviceError)/sizeof(_DeviceError[0]));
    }
    _ErrorFirst=0;
}

