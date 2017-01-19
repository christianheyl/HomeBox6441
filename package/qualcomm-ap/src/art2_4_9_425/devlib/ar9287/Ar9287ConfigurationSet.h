#include "ParameterConfigDef.h"
//
// Set the mac address. Return 0 on success.
//
extern int Ar9287_ConfigurationMacAddressSet(unsigned char mac[6]);
//
// Set the customer data field. Return 0 on success.
//
extern int Ar9287_ConfigurationCustomerDataSet(unsigned char *data, int len);
//
// Set the device id field. Return 0 on success.
//
extern int Ar9287_ConfigurationDeviceIdSet(unsigned short id);
//
// Set the device id field. Return 0 on success.
//
extern int Ar9287_ConfigurationConfigSet(A_UINT16 address, A_UINT32 data);
//
// Set the device id field. Return 0 on success.
//
extern int Ar9287_ConfigurationSubSystemIdSet(unsigned short id);
//
// Set the device id field. Return 0 on success.
//
extern int Ar9287_ConfigurationVendorIdSet(unsigned short id);
//
// Set the device id field. Return 0 on success.
//
extern int Ar9287_ConfigurationSubVendorIdSet(unsigned short id);

//
// Commit PCIE config space changes to the card
//
extern int Ar9287_ConfigurationCommit(void);




