
//
// Get the device id field. Return 0 on success.
//
extern int Ar9287_ConfigurationDeviceIdGet(A_UCHAR *id);


//
// Get the device id field. Return 0 on success.
//
extern int Ar9287_ConfigurationSubSystemIdGet(A_UCHAR *id);


//
// Get the device id field. Return 0 on success.
//
extern int Ar9287_ConfigurationVendorIdGet(A_UCHAR *id);


//
// Get the device id field. Return 0 on success.
//
extern int Ar9287_ConfigurationSubVendorIdGet(A_UCHAR *id);
//
// Get pcieAddress configure structure value data at address. Return 0 on success, -3 for the address not found
extern A_INT32 Ar9287_ConfigurationpcieAddressValueDataGet(unsigned char *sValue, int address);

extern A_INT32 Ar9287_ConfigurationpcieAddressValueDataOfNumGet(int num, unsigned char *sValue);

extern A_INT32 Ar9287_ConfigPCIeOnBoard(int iItem, unsigned char *buffer);

