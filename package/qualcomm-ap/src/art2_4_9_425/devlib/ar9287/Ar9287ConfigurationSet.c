

#include <stdio.h>

//
// hal header files
//
//#include "ah.h"
//#include "ah_internal.h"
//#include "ar9300.h"
//#include "ar9300eep.h"


#include "wlantype.h"

#include "UserPrint.h"
#include "ParameterConfigDef.h"

//#include "mEepStruct9300.h"
//#include "Ar9300PcieConfig.h"
//#include "Ar9300EepromStructSet.h"
#include "ConfigurationStatus.h"
#include "Ar9287EepromStructSet.h"
#include "Ar9287ConfigurationSet.h"
#include "Ar9287PcieConfig.h"
//
// Set the mac address. Return 0 on success.
//
int Ar9287_ConfigurationMacAddressSet(unsigned char mac[6])
{
    UserPrint("mac address = %02x:%02x:%02x:%02x:%02x:%02x\n",mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
    return Ar9287_MacAdressSet(mac);
}


//
// Set the customer data field. Return 0 on success.
//
int Ar9287_ConfigurationCustomerDataSet(unsigned char *data, int len)
{
    UserPrint("customer data = %s\n",data);
    return Ar9287_CustomerDataSet(data, len);
}

//
// Set the device id field. Return 0 on success.
//
// set config=HexValue; a=hexAddress;
int Ar9287_ConfigurationConfigSet(A_UINT16 address, A_UINT32 data)
{
	int status=Ar9287pcieAddressValueDataSet(address, data, 0);
	if (status==VALUE_SAME || status==VALUE_UPDATED || status==VALUE_NEW)
		status = VALUE_OK;
    return status;
}

//
// Set the device id field. Return 0 on success.
//
int Ar9287_ConfigurationDeviceIdSet(unsigned short id)
{
	int status=Ar9287deviceIDSet(id);
	if (status==VALUE_SAME || status==VALUE_UPDATED)
		status = VALUE_OK;
    return status;
}


//
// Set the device id field. Return 0 on success.
//
int Ar9287_ConfigurationSubSystemIdSet(unsigned short id)
{
	int status=Ar9287SSIDSet(id);
	if (status==VALUE_SAME || status==VALUE_UPDATED)
		status = VALUE_OK;
    return status;
}


//
// Set the device id field. Return 0 on success.
//
int Ar9287_ConfigurationVendorIdSet(unsigned short id)
{
	int status=Ar9287vendorSet(id);
	if (status==VALUE_SAME || status==VALUE_UPDATED)
		status = VALUE_OK;
    return status;
}


//
// Set the device id field. Return 0 on success.
//
int Ar9287_ConfigurationSubVendorIdSet(unsigned short id)
{
	int status=Ar9287SubVendorSet(id);
	if (status==VALUE_SAME || status==VALUE_UPDATED)
		status = VALUE_OK;
    return status;
}


//
// Commit PCIE config space changes to the card
//
int Ar9287_ConfigurationCommit()
{
    return Ar9287ConfigSpaceCommit();
}


int Ar9287_ConfigurationRemove()
{
    return Ar9287pcieAddressValueDataInit();
}
