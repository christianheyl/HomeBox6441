#define PCI_EXPRESS_VENDOR_DEVICE_ADDR	          0x5000
#define PCI_EXPRESS_SUBSYS_VENDOR_DEVICE_ADDR     0x502c

typedef struct pcieConfigStruct {
    unsigned int address;
    unsigned int dataLow;
    unsigned int dataHigh;
} PCIE_CONFIG_STRUCT;

extern A_INT32 Ar9300SSIDSet(unsigned int SSID);
extern A_INT32 Ar9300SSIDGet(unsigned int *SSID);
extern A_INT32 Ar9300SubVendorSet(unsigned int subVendorID);
extern A_INT32 Ar9300SubVendorGet(unsigned int *subVendorID);
extern A_INT32 Ar9300deviceIDSet(unsigned int deviceID);
extern A_INT32 Ar9300deviceIDGet(unsigned int *deviceID);
extern A_INT32 Ar9300vendorSet(unsigned int vendorID);
extern A_INT32 Ar9300vendorGet(unsigned int *vendorID);
extern A_INT32 Ar9300ConfigSpaceCommit(void);
extern A_INT32 Ar9300ConfigSpaceUsed(void);
extern A_INT32 Ar9300pcieAddressValueDataInit(void);
extern A_INT32 Ar9300pcieAddressValueDataSet(unsigned int address, unsigned int data);
extern A_INT32 Ar9300pcieAddressValueDataGet(unsigned int address, unsigned int *data);
extern A_INT32 Ar9300pcieAddressValueDataRemove(unsigned int address);
extern A_INT32 Ar9300pcieAddressValueDataOfNumGet(int num, unsigned int *address, unsigned int *data);
extern A_INT32 Ar9300ConfigPCIeOnBoard(int iItem, unsigned int *addr, unsigned int *data);
extern A_INT32 Ar9300pcieMany(void);

/*
extern int Ar9380pcieDefault(void);
extern int Ar9580pcieDefault(void);
extern int Ar9330pcieDefault(void);
extern int Ar9485pcieDefault(void);
extern int Ar946XpcieDefault(int devid);	// Jupiter
extern int Ar956XpcieDefault(int devid);	// aphrodite
extern int Ar934XpcieDefault(void);		// wasp
*/
#define MAX_pcieAddressValueData  340	// 16Kb->2kByte/6



