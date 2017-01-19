#include <stdio.h>
#include <string.h>
//
// hal header files
//
#include "ah.h"
#include "ah_internal.h"
#include "wlantype.h"
#include "ParameterConfigDef.h"
#include "ConfigurationStatus.h"
#include "Ar9287EepromStructGet.h"
#include "Ar9287PcieConfig.h"
//
// Get the device id field. Return 0 on success.
//
extern int Ar9287_ConfigurationDeviceIdGet(unsigned int *id)
{
	unsigned int iID;
	int status;
	status = Ar9287deviceIDGet(&iID);
    return status; 
}

//
// Get the device id field. Return 0 on success.
//
extern int Ar9287_ConfigurationSubSystemIdGet(unsigned int *id)
{
	unsigned int SSID;
	int status;
	status = Ar9287SSIDGet(&SSID);
    return status; 
}

//
// Get the device id field. Return 0 on success.
//
extern int Ar9287_ConfigurationVendorIdGet(unsigned int *id)
{
	unsigned int iID;
	int status;
	status = Ar9287vendorGet(&iID);
    return status; 
}

//
// Get the device id field. Return 0 on success.
//
extern int Ar9287_ConfigurationSubVendorIdGet(unsigned int *id)
{
	unsigned int iID;
	int status;
	status = Ar9287SubVendorGet(&iID);
    return status; 
}

//
// Get the device id field. Return 0 on success.
//
A_INT32 Ar9287_ConfigurationpcieAddressValueDataGet(unsigned char *sValue, int address)
// address is A_UINT16
{
	unsigned int data;
	int status=VALUE_OK;

	if(address<0 || address>(int)(0xffff)) {
		sprintf(sValue, "address supposed to be a UINT16, you entered 0x%x", address);		
		status = ERR_VALUE_BAD;
	} else {
		status = Ar9287pcieAddressValueDataGet(address, &data);
		if (status==VALUE_OK)
			sprintf(sValue, "0x%08x",data);
	}
    return status; 
}

A_INT32 Ar9287_ConfigurationpcieAddressValueDataOfNumGet(int num, unsigned char *sValue)
// address is A_UINT16
{
	unsigned int address;
	unsigned int data;

	int status;
	status =  Ar9287pcieAddressValueDataOfNumGet(num, &address, &data);
	sprintf(sValue, "addr:%04x, data:%08x", address, data);

	return status;
}

A_INT32 Ar9287_ConfigPCIeOnBoard(int iItem, unsigned char *sValue)
{
	int status;
	unsigned int address;
	unsigned int data;
	status =  Ar9287ConfigPCIeOnBoard(iItem, &address, &data);
	sprintf(sValue, "addr:%04x, data:%08x", address, data);
	return status;
}
