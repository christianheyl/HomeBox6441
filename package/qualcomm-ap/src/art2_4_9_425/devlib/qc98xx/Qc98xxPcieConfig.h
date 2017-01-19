#ifndef	_QC98XX_PCIE_CONFIG_H
#define	_QC98XX_PCIE_CONFIG_H

#define PCIE_BASE_ADDRESS               0x60000
#define PCIE_VENDOR_ID_OFFSET			    0x0
#define PCIE_DEVICE_ID_OFFSET				0x2
#define PCIE_SUBVENDOR_ID_OFFSET			0x2c
#define PCIE_SUBSYSTEM_ID_OFFSET			0x2e

#define MAX_pcieAddressValueData			100	

#define PCIE_HASVALUE_FLAG              0x80
#define PCIE_CHANGED_FLAG				0x40
#define PCIE_FROMOTP_FLAG				0x20
#define PCIE_DEFAULT_FLAG				0x10
#define PCIE_OP_FLAG                    0x03

typedef struct pcieConfigStruct {
    A_UINT32 offset;		// 4-byte aligned
    union
    {
        A_UINT8	 bData[4];
        A_UINT16 wData[2];
        A_UINT32 dwData;
    } data;
    A_UINT32 flags;
} PCIE_CONFIG_STRUCT;

extern A_INT32 Qc98xxSSIDSet(unsigned int SSID);
extern A_INT32 Qc98xxSSIDGet(unsigned int *SSID);
extern A_INT32 Qc98xxSubVendorSet(unsigned int subVendorID);
extern A_INT32 Qc98xxSubVendorGet(unsigned int *subVendorID);
extern A_INT32 Qc98xxDeviceIDSet(unsigned int deviceID);
extern A_INT32 Qc98xxDeviceIDGet(unsigned int *deviceID);
extern A_INT32 Qc98xxVendorSet(unsigned int vendorID);
extern A_INT32 Qc98xxVendorGet(unsigned int *vendorID);
extern A_INT32 Qc98xxConfigSpaceCommit(void);
extern A_INT32 Qc98xxConfigSpaceUsed(void);
extern A_INT32 Qc98xxPcieAddressValueDataInit(void);
extern A_INT32 Qc98xxPcieAddressValueDataSet(unsigned int address, unsigned int data, int size);
extern A_INT32 Qc98xxPcieAddressValueDataGet(unsigned int address, unsigned int *data);
extern A_INT32 Qc98xxPcieAddressValueDataRemove(unsigned int address, int size);
extern A_INT32 Qc98xxPcieAddressValueDataOfNumGet(int num, unsigned int *address, unsigned int *data);
extern A_INT32 Qc98xxConfigPCIeOnBoard(int iItem, unsigned int *addr, unsigned int *data);
extern A_INT32 Qc98xxPcieMany(void);
extern A_INT32 Qc98xxPcieAddressValueDataInit(void);
extern A_BOOL Qc98xxPcieOtpStreamParse(A_UCHAR *pBuffer, A_UINT32 nbytes);


#endif //_QC98XX_PCIE_CONFIG_H


