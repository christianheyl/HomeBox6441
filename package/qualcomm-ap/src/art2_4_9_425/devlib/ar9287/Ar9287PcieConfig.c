#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

#include "wlantype.h"
#include "Ar9287Device.h"
#include "Ar9287PcieConfig.h"
#include "ConfigurationStatus.h"
#include "ah_osdep.h"
#include "opt_ah.h"
#include "ah.h"
#include "ah_internal.h"
#include "ah_devid.h"
#include "Ar9287EepromSave.h"
#include "mEepStruct9287.h"
#include "UserPrint.h"
#include "ErrorPrint.h"
#include "EepromError.h"
#include "instance.h"
#include "NewArt.h"

// 
// this is the hal pointer, 
// returned by ath_hal_attach
// used as the first argument by most (all?) HAL routines
//
extern struct ath_hal *AH;

//static PCIE_CONFIG_STRUCT pcieAddressValueData[20];
static PCIE_CONFIG_STRUCT pcieAddressValueData[MAX_pcieAddressValueData];
unsigned char pcieByteArray[6*MAX_pcieAddressValueData+2];
static int num_pcieAddressValueData;
static int num_pcieAddressValueDataFromCard;
extern int32_t ar5416_calibration_data_get(struct ath_hal *ah);
#ifdef UNUSED
int Ar9287SSIDSet(unsigned int SSID);
int Ar9287SubVendorSet(unsigned int subVendorID);
int Ar9287deviceIDSet(unsigned int deviceID);
int Ar9287vendorSet(unsigned int vendorID);
int Ar9287pcieAddressValueDataSet( unsigned int address, unsigned int data, int size);
int Ar9287pcieAddressValueDataAdd( unsigned int address, unsigned int dataLow, unsigned int dataHigh);
#endif

#define MPCIE 100

static int PcieTop;

struct _PcieDefault
{
	int address;
	int low;
	int high;
};

int Ar9287pcieAddressValueDataInit(void)
// read eep from card, 
// if it's blank, wirte Atheros header, SSID... and hardcoded reg addr and value into eep structure
// if it's not blank, read the eep context from card, compare with atheros default setting,
// if not the same update structure data to the default, will write them into eep when runs commit
{
	int status = 0;	
	int i;
	unsigned int addr;

	unsigned char pcieConfigHeader[6] = {0x5a, 0xa5, 0, 0, 3, 0};
	unsigned char pcieConfigHeader0[6], pcieConfigDataSet[6];

	// first read eep back to see if it's a blank card
	if (Ar9287EepromRead(0, pcieConfigHeader0, 6))	
		return ERR_EEP_READ;	// bad eep reading

	num_pcieAddressValueDataFromCard = 0;
	num_pcieAddressValueData = 0;
	if (!(pcieConfigHeader0[0] == 0	||			// for blank EEPROM 
		  pcieConfigHeader0[0] == 0xFF)) {	// for blank OTP
		// not a blank eep card, read back card's eep contents
		for (i=0; i<6; i++) {
			if (pcieConfigHeader0[i] != pcieConfigHeader[i])
				return ERR_VALUE_BAD;		// eep header is not match atheros standard.
		}
		// from last 2 byte header to figure out the eep start address.
		addr = ((unsigned int)(pcieConfigHeader[4]) + (unsigned int)(pcieConfigHeader[5] <<8)) * 2;		
		while(1) {
			if (Ar9287EepromRead(addr, pcieConfigDataSet, 6))	
			{
				return -1;	// bad eep reading	
			}
			if ( (pcieConfigDataSet[0]==0 && pcieConfigDataSet[1]==0) ||	// for blank EEPROM
				  (pcieConfigDataSet[0]==0xFF && pcieConfigDataSet[1]==0xFF)) {	// for blank OTP
				break;	// found the end of eep data on card
			} else {
				addr+=6;
				pcieAddressValueData[num_pcieAddressValueData].address = (unsigned int)(pcieConfigDataSet[0]) + (unsigned int)(pcieConfigDataSet[1] <<8);
				pcieAddressValueData[num_pcieAddressValueData].dataLow = (unsigned int)(pcieConfigDataSet[2]) + (unsigned int)(pcieConfigDataSet[3] <<8);
				pcieAddressValueData[num_pcieAddressValueData].dataHigh = (unsigned int)(pcieConfigDataSet[4]) + (unsigned int)(pcieConfigDataSet[5] <<8);
				num_pcieAddressValueData++;
				if (num_pcieAddressValueData==MAX_pcieAddressValueData)
					return ERR_MAX_REACHED;	
			}
		}
	}
	return 0;
}



int Ar9287pcieMany(void)
{
	return num_pcieAddressValueData;
}

int Ar9287pcieAddressValueDataAdd( unsigned int address, unsigned int dataLow, unsigned int dataHigh)
{
	int i;
	int status = VALUE_NEW;		
	
	for (i=0; i< num_pcieAddressValueData; i++) {
		if (pcieAddressValueData[i].address == address) {
			if ( pcieAddressValueData[i].dataHigh == dataHigh &&
				 pcieAddressValueData[i].dataLow == dataLow )
				return VALUE_SAME;		
		}
	}
	if (status == VALUE_NEW) {
		if (num_pcieAddressValueData>=MAX_pcieAddressValueData)
				return ERR_MAX_REACHED;		
		else {
			pcieAddressValueData[num_pcieAddressValueData].address = address;
			pcieAddressValueData[num_pcieAddressValueData].dataLow = dataLow;
			pcieAddressValueData[num_pcieAddressValueData].dataHigh = dataHigh;
			num_pcieAddressValueData++;
		}
	}
    return status;
}


#define PTR_CONFIG_ADDR_DATA_SETS 3  //immediately after required fields
#define MAGIC_WORD_LOC            0
#define MAGIC_WORD				  0xA55A
#define EEPROM_PROTECT_LOC        1
#define PTR_LOCATION			  2
#define END_CONFIG_WRITES		  0xFFFF

#define PCI_VENDOR_DEVICE_ADDR	                  0x6000
#define PCI_CLASS_CODE_ADDR                       0x6008
#define PCI_SUBSYS_VENDOR_DEVICE_ADDR             0x602c
#define PCI_EXPRESS_VENDOR_DEVICE_ADDR	          0x5000
#define PCI_EXPRESS_CLASS_CODE_ADDR               0x5008
#define PCI_EXPRESS_SUBSYS_VENDOR_DEVICE_ADDR     0x502c
#define PCI_EXPRESS_BYPASS_LOS                    0x570c
#define RF_SILENT_POLARITY_ADDR                   0x4084
#define RF_SILENT_GPIO_SEL_ADDR                   0x405c
#define RF_SILENT_GPIO_CONFIG_ADDR                0x404c
#define RF_SILENT_CONFIGURE                       0x4054
#define ADDRESS_LOC				0
#define DATA_LOWER				1
#define DATA_UPPER				2
#define AR5416_EEP_START_LOC         256
#define F2_VENDOR_ID			0x168C		/* vendor ID for our device */
#define KIWI_DEV_AR5416_PCI    0x002d

#define KIWI_DEV_AR5416_PCIE   0x002e


int Ar9287pcieDefault(void)
{
	int it;
	static struct _PcieDefault pdefault[]=
	{
		{MAGIC_WORD, 0, PTR_CONFIG_ADDR_DATA_SETS},
		{PCI_VENDOR_DEVICE_ADDR, F2_VENDOR_ID, KIWI_DEV_AR5416_PCI},
		{PCI_CLASS_CODE_ADDR, 0x0001, 0x0280},
		{PCI_SUBSYS_VENDOR_DEVICE_ADDR, F2_VENDOR_ID, KIWI_DEV_AR5416_PCI},
		{PCI_EXPRESS_VENDOR_DEVICE_ADDR, F2_VENDOR_ID, KIWI_DEV_AR5416_PCIE},
		{PCI_EXPRESS_CLASS_CODE_ADDR, 0x0001, 0x0280},
		{PCI_EXPRESS_SUBSYS_VENDOR_DEVICE_ADDR, F2_VENDOR_ID, KIWI_DEV_AR5416_PCI},
		{0x5064, 0x8CC0, 0x0504},
		{PCI_EXPRESS_BYPASS_LOS, 0x3f01, 0x2200},
		{0x506c, 0x3C11, 0x0003}, 
		{0x4004, 0x050b, 0x004a},
		{0x4074, 0x0003, 0x0000},
		{0x4000, 0x5001, 0x01c2},  
		{0x6034, 0x0044, 0x0000},
		{0x510c, 0x2010, 0x0006},
		{0x5164, 0x1412, 0xff24},
		{0x5168, 0x17ff, 0x0015},
		{0x5068, 0x2010, 0x0019},
	};

	for(it=0; it<sizeof(pdefault)/sizeof(pdefault[0]); it++)
	{
	    if (Ar9287pcieAddressValueDataAdd(pdefault[it].address,pdefault[it].low,pdefault[it].high)== ERR_MAX_REACHED)
		    return ERR_MAX_REACHED;
	}
	return num_pcieAddressValueData;
}


int Ar9287pcieAddressValueDataSet( unsigned int address, unsigned int data, int size)
{
	unsigned int dataLow, dataHigh;
	dataLow  = (unsigned int)(data & 0xFFFF);
	dataHigh = (unsigned int)(data >> 16);
    return Ar9287pcieAddressValueDataAdd(address, dataLow, dataHigh);
}

int Ar9287pcieAddressValueDataGet( unsigned int address, unsigned int *data)
{
	int i;
	int status = ERR_NOT_FOUND;	
	for (i=0; i< num_pcieAddressValueData; i++) {
		if (pcieAddressValueData[i].address == address) {
			*data = (unsigned int) pcieAddressValueData[i].dataLow +
					(unsigned int) (pcieAddressValueData[i].dataHigh << 16);
			status = VALUE_OK;
			break;
		}
	}
    return status;
}


int Ar9287pcieAddressValueDataOfNumGet( int num, unsigned int *address, unsigned int *data)
{
	if (num <0 || num>=num_pcieAddressValueData)
		return ERR_RETURN;
	*address = pcieAddressValueData[num].address;
	*data = (unsigned int) pcieAddressValueData[num].dataLow +
			(unsigned int) (pcieAddressValueData[num].dataHigh << 16);
	return VALUE_OK;
}

int Ar9287pcieAddressValueDataRemove( unsigned int address)
{
	int i, iFound=ERR_NOT_FOUND;
	
	for (i=0; i< num_pcieAddressValueData; i++) {
		if (pcieAddressValueData[i].address == address) 
			iFound = i;
	}
	if (iFound!=ERR_NOT_FOUND) {
		for (i=iFound; i<num_pcieAddressValueData-1; i++) {
			pcieAddressValueData[i].address = pcieAddressValueData[i+1].address;
			pcieAddressValueData[i].dataLow = pcieAddressValueData[i+1].dataLow;
			pcieAddressValueData[i].dataHigh = pcieAddressValueData[i+1].dataHigh;
		}
		pcieAddressValueData[num_pcieAddressValueData].address = 0xFFFF;
		pcieAddressValueData[num_pcieAddressValueData].dataLow = 0xFFFF;
		pcieAddressValueData[num_pcieAddressValueData].dataHigh = 0xFFFF;
		num_pcieAddressValueData--;
	}
    return iFound;
}


int Ar9287deviceIDSet(unsigned int deviceID) 
{  
	int it;

	for(it=0; it<num_pcieAddressValueData; it++)
	{
		if(pcieAddressValueData[it].address == PCI_EXPRESS_VENDOR_DEVICE_ADDR)
		{
			pcieAddressValueData[it].dataHigh = deviceID;
			return 0;
		}
	}

    if(it<MAX_pcieAddressValueData)
	{
		pcieAddressValueData[it].address = PCI_EXPRESS_VENDOR_DEVICE_ADDR;
		pcieAddressValueData[it].dataLow = 0;
		pcieAddressValueData[it].dataHigh = deviceID;
		num_pcieAddressValueData++;
		return 0;
	}

	return ERR_MAX_REACHED;
}

int Ar9287deviceIDGet(unsigned int *deviceID) 
{  
	int it;

	for(it=0; it<num_pcieAddressValueData; it++)
	{
		if(pcieAddressValueData[it].address == PCI_EXPRESS_VENDOR_DEVICE_ADDR)
		{
			*deviceID = pcieAddressValueData[it].dataHigh;
			return 0;
		}
	}
	*deviceID = 0xDEAD;
	return ERR_RETURN;
}

int Ar9287vendorSet(unsigned int vendorID)
{   
	int it;

	for(it=0; it<num_pcieAddressValueData; it++)
	{
		if(pcieAddressValueData[it].address == PCI_EXPRESS_VENDOR_DEVICE_ADDR)
		{
			pcieAddressValueData[it].dataLow = vendorID;
			return 0;
		}
	}

    if(it<MAX_pcieAddressValueData)
	{
		pcieAddressValueData[it].address = PCI_EXPRESS_VENDOR_DEVICE_ADDR;
		pcieAddressValueData[it].dataLow = vendorID;
		pcieAddressValueData[it].dataHigh = 0;
		num_pcieAddressValueData++;
		return 0;
	}

	return ERR_MAX_REACHED;
}

int Ar9287vendorGet(unsigned int *vendorID)
{   
	int it;

	for(it=0; it<num_pcieAddressValueData; it++)
	{
		if(pcieAddressValueData[it].address == PCI_EXPRESS_VENDOR_DEVICE_ADDR)
		{
			*vendorID = pcieAddressValueData[it].dataLow;
			return 0;
		}
	}
	*vendorID = 0xDEAD;
	return ERR_RETURN;
}

int Ar9287SSIDSet(unsigned int SSID) 
{  
	int it;

	for(it=0; it<num_pcieAddressValueData; it++)
	{
		if(pcieAddressValueData[it].address == PCI_EXPRESS_SUBSYS_VENDOR_DEVICE_ADDR)
		{
			pcieAddressValueData[it].dataHigh = SSID;
			return 0;
		}
	}

    if(it<MAX_pcieAddressValueData)
	{
		pcieAddressValueData[it].address = PCI_EXPRESS_SUBSYS_VENDOR_DEVICE_ADDR;
		pcieAddressValueData[it].dataLow = 0;
		pcieAddressValueData[it].dataHigh = SSID;
		num_pcieAddressValueData++;
		return 0;
	}

	return ERR_MAX_REACHED;
}

int Ar9287SSIDGet(unsigned int *SSID) 
{  
	int it;

	for(it=0; it<num_pcieAddressValueData; it++)
	{
		if(pcieAddressValueData[it].address == PCI_EXPRESS_SUBSYS_VENDOR_DEVICE_ADDR)
		{
			*SSID = pcieAddressValueData[it].dataHigh;
			return 0;
		}
	}
	*SSID = 0xDEAD;
	return ERR_RETURN;
}

int Ar9287SubVendorSet(unsigned int subVendorID)
{   
	int it;

	for(it=0; it<num_pcieAddressValueData; it++)
	{
		if(pcieAddressValueData[it].address == PCI_EXPRESS_SUBSYS_VENDOR_DEVICE_ADDR)
		{
			pcieAddressValueData[it].dataLow = subVendorID;
			return 0;
		}
	}

    if(it<MAX_pcieAddressValueData)
	{
		pcieAddressValueData[it].address = PCI_EXPRESS_SUBSYS_VENDOR_DEVICE_ADDR;
		pcieAddressValueData[it].dataLow = subVendorID;
		pcieAddressValueData[it].dataHigh = 0;
		num_pcieAddressValueData++;
		return 0;
	}

	return ERR_MAX_REACHED;
}

int Ar9287SubVendorGet(unsigned int *subVendorID)
{   
	int it;

	for(it=0; it<num_pcieAddressValueData; it++)
	{
		if(pcieAddressValueData[it].address == PCI_EXPRESS_SUBSYS_VENDOR_DEVICE_ADDR)
		{
			*subVendorID = pcieAddressValueData[it].dataLow;
			return 0;
		}
	}
	*subVendorID = 0xDEAD;
	return ERR_RETURN;
}

int Ar9287ConfigSpaceCommitEeprom(void) 
{
	int limit;
	unsigned int addr;
    int i;
    unsigned char pcieConfigHeader[6] = {0x5a, 0xa5, 0, 0, 3, 0};
	addr = ((unsigned int)(pcieConfigHeader[4]) + (unsigned int)(pcieConfigHeader[5] <<8)) * 2;		

	limit=0x100; //100;

	if(limit<=addr+num_pcieAddressValueData*6+2)
	{
		ErrorPrint(PcieWontFit,addr+num_pcieAddressValueData*6+2,limit);
		return-1;
	}

    Ar9287EepromWrite(0, pcieConfigHeader, 6);

	// need tp add FFFF at the end of value for mark
	// [4] is the lsb for eep addr start, [5] is the msb for eep addr start
	if (num_pcieAddressValueData+1<MAX_pcieAddressValueData) {	// add 0xFFFF to mark as the end
		pcieAddressValueData[num_pcieAddressValueData].address =0xFFFF;
	}

    // Convert to byte array
    // Fill up pcie Byte Array with default values (0xff)
    for(i=0; i<(6*MAX_pcieAddressValueData+2); i++)
        pcieByteArray[i] = 0xff;
    for(i=0; i<num_pcieAddressValueData; i++)
    {
        pcieByteArray[6*i+0] = pcieAddressValueData[i].address & 0xff;
        pcieByteArray[6*i+1] = (pcieAddressValueData[i].address >> 8) & 0xff;
        pcieByteArray[6*i+2] = pcieAddressValueData[i].dataLow & 0xff;
        pcieByteArray[6*i+3] = (pcieAddressValueData[i].dataLow >> 8) & 0xff;
        pcieByteArray[6*i+4] = pcieAddressValueData[i].dataHigh & 0xff;
        pcieByteArray[6*i+5] = (pcieAddressValueData[i].dataHigh >> 8) & 0xff;
    }
	Ar9287EepromWrite(addr, pcieByteArray, num_pcieAddressValueData*6+2);
    return 0;
}

int Ar9287ConfigSpaceCommitFlash(int client)
{
    A_INT32 ret;
	int i;

    if(AH==0)
    {
        return ERR_EEP_READ;
    }

#ifdef MDK_AP	
	int fd;
	int offset;
	
	if((fd = open("/dev/caldata", O_RDWR)) < 0) {
		perror("Could not open flash\n");
		//status = -1 ;
	}

#define AR9287_FLASH_SIZE 16*1024	// byte addressable
#define FLASH_BASE_CALDATA_OFFSET  0x1000
	offset = instance*AR9287_FLASH_SIZE+FLASH_BASE_CALDATA_OFFSET;  // Need for boards with more than one radio
	lseek(fd, offset, SEEK_SET);

    // Convert to byte array
    // Fill up pcie Byte Array with default values (0xff)
    for(i=0; i<(6*MAX_pcieAddressValueData+2); i++)
        pcieByteArray[i] = 0xff;
    for(i=0; i<num_pcieAddressValueData; i++)
    {
        pcieByteArray[6*i+1] = pcieAddressValueData[i].address & 0xff;
        pcieByteArray[6*i+0] = (pcieAddressValueData[i].address >> 8) & 0xff;
        pcieByteArray[6*i+3] = pcieAddressValueData[i].dataLow & 0xff;
        pcieByteArray[6*i+2] = (pcieAddressValueData[i].dataLow >> 8) & 0xff;
        pcieByteArray[6*i+5] = pcieAddressValueData[i].dataHigh & 0xff;
        pcieByteArray[6*i+4] = (pcieAddressValueData[i].dataHigh >> 8) & 0xff;
    }

	if (write(fd, &pcieByteArray, num_pcieAddressValueData*6+2) < 1) {
		perror("\nwrite\n");
	}
	close(fd);
#endif
	
	//SendDone(client);
	
    return 0;
}

int Ar9287ConfigSpaceCommit() 
{
	int cal_mem;
	unsigned char header[4];

	    if(ar5416_eeprom_size(AH)>0)
		{
			cal_mem=calibration_data_eeprom;
		}
		else if(ar9287_calibration_data_read_flash(AH, 0x1000, header, 1)==AH_TRUE)
		{
			cal_mem=calibration_data_flash;
		}
		else
		{
			cal_mem=calibration_data_otp;
		}

	switch(cal_mem)
	{
		case calibration_data_eeprom:
			return Ar9287ConfigSpaceCommitEeprom();
		case calibration_data_flash:
			return Ar9287ConfigSpaceCommitFlash(0);
		case calibration_data_otp:
			//return Ar9287ConfigSpaceCommitOtp();
			return 0;
	}
	return -1;
}

int Ar9287ConfigSpaceUsed(void)
{
	unsigned int addr;

	unsigned char word[6] = {0x5a, 0xa5, 0, 0, 3, 0};

	// first read eep back to see if it's a blank card
	if (Ar9287EepromRead(0, word, 6) || word[0]==0xff)
	{
		return 0;							// nothing on card, return minimum size
	}

	// from last 2 byte header to figure out the eep start address.
	for(addr = ((unsigned int)(word[4]) + (unsigned int)(word[5] <<8)) * 2;	addr<500; addr+=6)	
	{
		if (Ar9287EepromRead(addr, word, 6))
		{
			break;	// bad eep reading
		}
		if (word[0]==0xFF && word[1]==0xFF) 
		{	
			break;	// found the end of eep data on card
		} 
	}

	return addr;
}

int Ar9287ConfigPCIeOnBoard(int iItem, unsigned int *addr, unsigned int *data) 
{
	// blank card
	if (num_pcieAddressValueDataFromCard==0)
		return ERR_MAX_REACHED;

	if (iItem==0) {
		*addr = 0xa55a;
		*data = 0x00000003;
		return VALUE_OK;
	} else {
		if (iItem<=num_pcieAddressValueDataFromCard) {
			*addr = pcieAddressValueData[iItem-1].address;
			*data = pcieAddressValueData[iItem-1].dataLow + (pcieAddressValueData[iItem-1].dataHigh<<16);
		} else
			return ERR_MAX_REACHED;
	}
	return VALUE_OK;
}

