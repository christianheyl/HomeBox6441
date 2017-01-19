#define PCI_EXPRESS_VENDOR_DEVICE_ADDR	          0x5000
#define PCI_EXPRESS_SUBSYS_VENDOR_DEVICE_ADDR     0x502c

typedef struct pcieConfigStruct {
    unsigned int address;
    unsigned int dataLow;
    unsigned int dataHigh;
} PCIE_CONFIG_STRUCT;

extern int Ar9287SSIDSet(unsigned int SSID);
extern int Ar9287SSIDGet(unsigned int *SSID);
extern int Ar9287SubVendorSet(unsigned int subVendorID);
extern int Ar9287SubVendorGet(unsigned int *subVendorID);
extern int Ar9287deviceIDSet(unsigned int deviceID);
extern int Ar9287deviceIDGet(unsigned int *deviceID);
extern int Ar9287vendorSet(unsigned int vendorID);
extern int Ar9287vendorGet(unsigned int *vendorID);
extern int Ar9287ConfigSpaceCommit(void);
extern int Ar9287ConfigSpaceUsed(void);
extern int Ar9287pcieAddressValueDataInit(void);
extern int Ar9287pcieAddressValueDataSet(unsigned int address, unsigned int data, int size);
extern int Ar9287pcieAddressValueDataGet(unsigned int address, unsigned int *data);
extern int Ar9287pcieAddressValueDataRemove(unsigned int address);
extern int Ar9287pcieAddressValueDataOfNumGet(int num, unsigned int *address, unsigned int *data);
extern int Ar9287ConfigPCIeOnBoard(int iItem, unsigned int *addr, unsigned int *data);
extern int Ar9287pcieMany(void);

extern int Ar9287pcieDefault(void);

#define MAX_pcieAddressValueData  340	// 16Kb->2kByte/6



