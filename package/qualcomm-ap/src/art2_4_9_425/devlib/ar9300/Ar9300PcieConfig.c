#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

#include "AquilaNewmaMapping.h"

#include "wlantype.h"

#include "Ar9300Device.h"
#include "Ar9300PcieConfig.h"
#include "ConfigurationStatus.h"
#include "ah_osdep.h"
#include "opt_ah.h"

#include "ah.h"
#include "ah_internal.h"
#include "ah_devid.h"
#include "ar9300eep.h"
#include "ar9300.h"

#include "Ar9300EepromSave.h"
#include "Ar9300EepromStructSet.h"
#include "ar9300EepromPrint.h"
#include "ar9300reg.h"

#include "UserPrint.h"
#include "ErrorPrint.h"
#include "EepromError.h"

#include "AR9300ChipIdentify.h"
// 
// this is the hal pointer, 
// returned by ath_hal_attach
// used as the first argument by most (all?) HAL routines
//
extern struct ath_hal *AH;

//static PCIE_CONFIG_STRUCT pcieAddressValueData[20];
static PCIE_CONFIG_STRUCT pcieAddressValueData[MAX_pcieAddressValueData];
static int num_pcieAddressValueData;
static int num_pcieAddressValueDataFromCard;

A_INT32 Ar9300SSIDSet(unsigned int SSID);
A_INT32 Ar9300SubVendorSet(unsigned int subVendorID);
A_INT32 Ar9300deviceIDSet(unsigned int deviceID);
A_INT32 Ar9300vendorSet(unsigned int vendorID);
A_INT32 Ar9300pcieAddressValueDataSet( unsigned int address, unsigned int data);
A_INT32 Ar9300pcieAddressValueDataAdd( unsigned int address, unsigned int dataLow, unsigned int dataHigh);

#define MPCIE 100
#define PCIE_OTP_BASE (32)


static int PcieTop;

/* DEBUG TEST
#define Ar9300OtpWrite(ADDRESS,BUFFER,LENGTH) Ar9300EepromWrite(ADDRESS+0x30,BUFFER,LENGTH)
#define Ar9300OtpRead(ADDRESS,BUFFER,LENGTH) MyAr9300EepromRead(ADDRESS+0x30,BUFFER,LENGTH)
*/

static int WriteIt(int address, unsigned int value)
{
	unsigned char byte[4];

	byte[0]=(value&0xff);
	byte[1]=((value>>8)&0xff);
	byte[2]=((value>>16)&0xff);
	byte[3]=((value>>24)&0xff);
	return Ar9300OtpWrite(address,byte,4,1);
}

static int ReadIt(int address, unsigned int *value)
{
	unsigned char byte[4];
	int error;

	error=Ar9300OtpRead(address,byte,4,1);
	if(error==0)
	{
	    *value=byte[3];
		*value=((*value)<<8)|byte[2];
		*value=((*value)<<8)|byte[1];
		*value=((*value)<<8)|byte[0];
	}
	return error;
}


A_INT32 Ar9300pcieAddressValueDataInitEeprom(void)
// read eep from card, 
// if it's blank, wirte Atheros header, SSID... and hardcoded reg addr and value into eep structure
// if it's not blank, read the eep context from card, compare with atheros default setting,
// if not the same update structure data to the default, will write them into eep when runs commit
{
	int status = 0;	
	int i;
	unsigned int addr;

	unsigned char pcieConfigHeader[6] = {0x5a, 0xa5, 0, 0, 3, 0};
	unsigned char pcieConfigHeader0[6], pcieConfigDataSet[6], pcieConfigDataSet_8[8];
	unsigned char *wordPtr;
	int addrOffset = 0;

	// first read eep back to see if it's a blank card
	if (Ar9300EepromRead(0, pcieConfigHeader0, 6))	
		return ERR_EEP_READ;	// bad eep reading

	addrOffset = getPcieAddressOffset(AH);
	if(addrOffset == 8){
		wordPtr = &pcieConfigDataSet_8[0];
	}
	else
	{
		wordPtr = &pcieConfigDataSet[0];
	}
	
	num_pcieAddressValueDataFromCard = 0;
	num_pcieAddressValueData = 0;
	if (!(pcieConfigHeader0[0] == 0	||			// for blank OTP 
		  pcieConfigHeader0[0] == 0xFF)) {	// for blank EEPROM
		// not a blank eep card, read back card's eep contents
		for (i=0; i<6; i++) {
			if (pcieConfigHeader0[i] != pcieConfigHeader[i])
				return ERR_VALUE_BAD;		// eep header is not match atheros standard.
		}
		// from last 2 byte header to figure out the eep start address.
		addr = ((unsigned int)(pcieConfigHeader[4]) + (unsigned int)(pcieConfigHeader[5] <<8)) * 2;		
		while(1) {
			if (Ar9300EepromRead(addr, wordPtr, addrOffset))	
				return -1;	// bad eep reading	
			if ( (wordPtr[0]==0 && wordPtr[1]==0 && wordPtr[2]==0 && wordPtr[3]==0) ||	// for blank OTP
				  (wordPtr[0]==0xFF && wordPtr[1]==0xFF && wordPtr[2]==0xFF && wordPtr[3]==0xFF)) {	// for blank EEPROM
				break;	// found the end of eep data on card
			} else {
			if(addrOffset == 8){
				addr+=addrOffset;
				pcieAddressValueData[num_pcieAddressValueData].address = (unsigned int)(wordPtr[0]) + (unsigned int)(wordPtr[1] <<8) + (unsigned int)(wordPtr[2] <<16) + (unsigned int)(wordPtr[3] <<24);
				pcieAddressValueData[num_pcieAddressValueData].dataLow = (unsigned int)(wordPtr[4]) + (unsigned int)(wordPtr[5] <<8);
				pcieAddressValueData[num_pcieAddressValueData].dataHigh = (unsigned int)(wordPtr[6]) + (unsigned int)(wordPtr[7] <<8);
				num_pcieAddressValueData++;
				if (num_pcieAddressValueData==MAX_pcieAddressValueData)
					return ERR_MAX_REACHED;	
			}
			else
			{
				addr+=addrOffset;
				pcieAddressValueData[num_pcieAddressValueData].address = (unsigned int)(wordPtr[0]) + (unsigned int)(wordPtr[1] <<8);
				pcieAddressValueData[num_pcieAddressValueData].dataLow = (unsigned int)(wordPtr[2]) + (unsigned int)(wordPtr[3] <<8);
				pcieAddressValueData[num_pcieAddressValueData].dataHigh = (unsigned int)(wordPtr[4]) + (unsigned int)(wordPtr[5] <<8);
				num_pcieAddressValueData++;
				if (num_pcieAddressValueData==MAX_pcieAddressValueData)
					return ERR_MAX_REACHED;	
			}
			}
		}
	}
	return 0;
}


A_INT32 Ar9300pcieAddressValueDataInitOtp(void)
{
	int address, addrMax, start_address;
	unsigned int eregister,evalue;

	num_pcieAddressValueDataFromCard = 0;
	num_pcieAddressValueData = 0;

        addrMax = getPcieOtpTopAddress(AH);	
    
#ifdef UNUSED
    if (AR_SREV_POSEIDON_10(AH))
        start_address = PCIE_OTP_BASE_POSEIDON_10;
    else
#endif
        start_address = PCIE_OTP_BASE;

	for(address=start_address; address<addrMax; address+=8)
	{
		ReadIt(address,&eregister);
		//
		// are we done?
		//
		if(eregister==0 /*|| eregister==0xffffffff*/)			// DEBUG TEST 
		{
//			UserPrint("end of exisiting at %x out=%d\n",address,out);
			break;
		}
		if((eregister&0x3)==0)
		{
			ReadIt(address+4,&evalue);
//				UserPrint("old=%08x new=%08x",evalue,nvalue[out]);
			pcieAddressValueData[num_pcieAddressValueData].address = (unsigned int)(eregister);
			pcieAddressValueData[num_pcieAddressValueData].dataLow = (unsigned int)(evalue&0xffff);
			pcieAddressValueData[num_pcieAddressValueData].dataHigh = (unsigned int)((evalue>>16)&0xffff);
			num_pcieAddressValueData++;
			if (num_pcieAddressValueData==MAX_pcieAddressValueData)
				return ERR_MAX_REACHED;	
		}
	}
	return 0;
}

A_INT32 Ar9300pcieMany(void)
{
	return num_pcieAddressValueData;
}

int Ar9380pcieDefault(int devid)	// osprey
{
	Ar9300deviceIDSet(AR9300_DEVID_AR9380_PCIE);

	Ar9300vendorSet(0x168c);

	if (Ar9300pcieAddressValueDataAdd(0x5008, 0x0001, 0x0280)== ERR_MAX_REACHED)
		return ERR_MAX_REACHED;
	if (Ar9300pcieAddressValueDataAdd(0x407c, 0x0001, 0x0000)== ERR_MAX_REACHED)
		return ERR_MAX_REACHED;
	if (Ar9300pcieAddressValueDataAdd(0x4004, 0x021b, 0x0102)== ERR_MAX_REACHED)
		return ERR_MAX_REACHED;
	if (Ar9300pcieAddressValueDataAdd(0x4040, 0x2e5e, 0x0821)== ERR_MAX_REACHED)
		return ERR_MAX_REACHED;
	if (Ar9300pcieAddressValueDataAdd(0x4040, 0x003b, 0x0008)== ERR_MAX_REACHED)
		return ERR_MAX_REACHED;
	if (Ar9300pcieAddressValueDataAdd(0x4044, 0x0000, 0x0000)== ERR_MAX_REACHED)
		return ERR_MAX_REACHED;            
    if (Ar9300pcieAddressValueDataAdd(0x570C, 0x3f01, 0x173f)== ERR_MAX_REACHED)
        return ERR_MAX_REACHED;
    
	return num_pcieAddressValueData;
}

int Ar934XpcieDefault(int devid)	// wasp
{
	Ar9300deviceIDSet(devid);

	Ar9300vendorSet(0x168c);

	if (Ar9300pcieAddressValueDataAdd(0x5008, 0x0001, 0x0280)== ERR_MAX_REACHED)
		return ERR_MAX_REACHED;
	if (Ar9300pcieAddressValueDataAdd(0x407c, 0x0001, 0x0000)== ERR_MAX_REACHED)
		return ERR_MAX_REACHED;
	if (Ar9300pcieAddressValueDataAdd(0x4004, 0x021b, 0x0102)== ERR_MAX_REACHED)
		return ERR_MAX_REACHED;
	if (Ar9300pcieAddressValueDataAdd(0x4040, 0x2e5e, 0x0821)== ERR_MAX_REACHED)
		return ERR_MAX_REACHED;
	if (Ar9300pcieAddressValueDataAdd(0x4040, 0x003b, 0x0008)== ERR_MAX_REACHED)
		return ERR_MAX_REACHED;
	if (Ar9300pcieAddressValueDataAdd(0x4044, 0x0000, 0x0000)== ERR_MAX_REACHED)
		return ERR_MAX_REACHED;            
    if (Ar9300pcieAddressValueDataAdd(0x570C, 0x3f01, 0x173f)== ERR_MAX_REACHED)
        return ERR_MAX_REACHED;
    
	return num_pcieAddressValueData;
}

int Ar9580pcieDefault(int devid)	// peacock
{
	Ar9300deviceIDSet(devid);

	Ar9300vendorSet(0x168c);

	if (Ar9300pcieAddressValueDataAdd( 0x5008, 0x0001, 0x0280)== ERR_MAX_REACHED)
		return ERR_MAX_REACHED;
	if (Ar9300pcieAddressValueDataAdd( 0x407c, 0x0001, 0x0000)== ERR_MAX_REACHED)
		return ERR_MAX_REACHED;
	if (Ar9300pcieAddressValueDataAdd( 0x4004, 0x021b, 0x0102)== ERR_MAX_REACHED)
		return ERR_MAX_REACHED;
	if (Ar9300pcieAddressValueDataAdd( 0x4040, 0x2e5e, 0x0831)== ERR_MAX_REACHED)
		return ERR_MAX_REACHED;
	if (Ar9300pcieAddressValueDataAdd( 0x4040, 0x003b, 0x0008)== ERR_MAX_REACHED)
		return ERR_MAX_REACHED;
	if (Ar9300pcieAddressValueDataAdd( 0x4044, 0x0000, 0x0000)== ERR_MAX_REACHED)
		return ERR_MAX_REACHED;            
    if (Ar9300pcieAddressValueDataAdd( 0x570C, 0x3f01, 0x173f)== ERR_MAX_REACHED)
        return ERR_MAX_REACHED;
             
	return num_pcieAddressValueData;
}


int Ar9330pcieDefault(int devid)		// hornet
{
	Ar9300deviceIDSet(AR9300_DEVID_AR9380_PCIE);			// ??????

	Ar9300vendorSet(0x168c);

	if (Ar9300pcieAddressValueDataAdd( 0x5008, 0x0001, 0x0280)== ERR_MAX_REACHED)
		return ERR_MAX_REACHED;
	if (Ar9300pcieAddressValueDataAdd( 0x407c, 0x0001, 0x0000)== ERR_MAX_REACHED)
		return ERR_MAX_REACHED;
	if (Ar9300pcieAddressValueDataAdd( 0x4004, 0x021b, 0x0102)== ERR_MAX_REACHED)
		return ERR_MAX_REACHED;
	if (Ar9300pcieAddressValueDataAdd( 0x4040, 0x2e5e, 0x0821)== ERR_MAX_REACHED)
		return ERR_MAX_REACHED;
	if (Ar9300pcieAddressValueDataAdd( 0x4040, 0x003b, 0x0008)== ERR_MAX_REACHED)
		return ERR_MAX_REACHED;
	if (Ar9300pcieAddressValueDataAdd( 0x4044, 0x0000, 0x0000)== ERR_MAX_REACHED)
		return ERR_MAX_REACHED;            
    if (Ar9300pcieAddressValueDataAdd( 0x570C, 0x3f01, 0x173f)== ERR_MAX_REACHED)
        return ERR_MAX_REACHED;
               
	return num_pcieAddressValueData;
}


int Ar9485pcieDefault(int devid)	// poseidon
{
	Ar9300deviceIDSet(devid);

	Ar9300vendorSet(0x168c);

	if (Ar9300pcieAddressValueDataAdd( 0x5008, 0x0001, 0x0280)== ERR_MAX_REACHED)
		return ERR_MAX_REACHED;
	if (Ar9300pcieAddressValueDataAdd( 0x407c, 0x0001, 0x0000)== ERR_MAX_REACHED)
		return ERR_MAX_REACHED;
	if (Ar9300pcieAddressValueDataAdd( 0x4004, 0x021b, 0x0102)== ERR_MAX_REACHED)
		return ERR_MAX_REACHED;
    if (Ar9300pcieAddressValueDataAdd( 0x570C, 0x3f01, 0x173f)== ERR_MAX_REACHED)
        return ERR_MAX_REACHED;
            
    if (AR_SREV_POSEIDON_11(AH)) 
	{
//
// Poseidon1.0 bug. Must enable rx_clk_inv ( PCIE_reg0x18c00=0x1021265e ) 
// and write it into OTP. So the start_address = 0x28.
// |or|0020|8c00|
// |or|0022|0001|
// |or|0024|265e|
// |or|0026|1021|
//
		// did i get this in the right order? 
        if (Ar9300pcieAddressValueDataSet( 0x18c00, 0x1021265e)== ERR_MAX_REACHED)
            return ERR_MAX_REACHED;
	}

    if (AR_SREV_POSEIDON_11(AH)) 
	{
        if (Ar9300pcieAddressValueDataAdd( 0x4078, 0x0006, 0x0000)== ERR_MAX_REACHED)
            return ERR_MAX_REACHED;
    }
    
	return num_pcieAddressValueData;
}

#ifndef USE_AQUILA_HAL
int Ar946XpcieDefault(int devid)	// Jupiter
{
	Ar9300deviceIDSet(devid);

	Ar9300vendorSet(0x168c);

	if (Ar9300pcieAddressValueDataAdd( 0x5008, 0x0001, 0x0280)== ERR_MAX_REACHED)
		return ERR_MAX_REACHED;
	if (Ar9300pcieAddressValueDataAdd( 0x407c, 0x0001, 0x0000)== ERR_MAX_REACHED)
		return ERR_MAX_REACHED;
	if (Ar9300pcieAddressValueDataAdd( 0x4004, 0x021b, 0x0102)== ERR_MAX_REACHED)
		return ERR_MAX_REACHED;
    if (Ar9300pcieAddressValueDataAdd( 0x570C, 0x3f01, 0x273F)== ERR_MAX_REACHED)
        return ERR_MAX_REACHED;
    
	return num_pcieAddressValueData;
}
int Ar956XpcieDefault(int devid)	//
{
	Ar9300deviceIDSet(devid);

	Ar9300vendorSet(0x168c);

	if (Ar9300pcieAddressValueDataAdd( 0x5008, 0x0001, 0x0280)== ERR_MAX_REACHED)
		return ERR_MAX_REACHED;
	if (Ar9300pcieAddressValueDataAdd( 0x407c, 0x0001, 0x0000)== ERR_MAX_REACHED)
		return ERR_MAX_REACHED;
	if (Ar9300pcieAddressValueDataAdd( 0x4004, 0x021b, 0x0102)== ERR_MAX_REACHED)
		return ERR_MAX_REACHED;
    if (Ar9300pcieAddressValueDataAdd( 0x570C, 0x3f01, 0x173F)== ERR_MAX_REACHED)
        return ERR_MAX_REACHED;
    
	return num_pcieAddressValueData;
}

#endif


A_INT32 Ar9300pcieAddressValueDataInit(void)
{
	int error;

	error=0;
	switch(ar9300_calibration_data_get(AH))
	{
		case calibration_data_eeprom:
			error=Ar9300pcieAddressValueDataInitEeprom();
			break;
		case calibration_data_flash:
		case calibration_data_otp:
			error=Ar9300pcieAddressValueDataInitOtp();
			break;
	}
	if(error==0)
	{
		return num_pcieAddressValueData;
	}
	else
	{
		return error;
	}
}

#ifdef _UNUSED
A_INT32 Ar9300pcieAddressValueDataUpdate( unsigned int address, unsigned int dataLow, unsigned int dataHigh)
{
	int i;
	int status = VALUE_NEW;		
	
	for (i=0; i< num_pcieAddressValueData; i++) {
		if (pcieAddressValueData[i].address == address) {
			if ( pcieAddressValueData[i].dataHigh == dataHigh &&
				 pcieAddressValueData[i].dataLow == dataLow )
				return VALUE_SAME;		
			else {
				pcieAddressValueData[i].dataHigh = dataHigh;
				pcieAddressValueData[i].dataLow = dataLow;
				return VALUE_UPDATED;		
			}
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
#endif

A_INT32 Ar9300pcieAddressValueDataAdd( unsigned int address, unsigned int dataLow, unsigned int dataHigh)
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


A_INT32 Ar9300pcieAddressValueDataSet( unsigned int address, unsigned int data)
{
	unsigned int dataLow, dataHigh;
	dataLow  = (unsigned int)(data & 0xFFFF);
	dataHigh = (unsigned int)(data >> 16);
    return Ar9300pcieAddressValueDataAdd(address, dataLow, dataHigh);
}

A_INT32 Ar9300pcieAddressValueDataGet( unsigned int address, unsigned int *data)
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


A_INT32 Ar9300pcieAddressValueDataOfNumGet( int num, unsigned int *address, unsigned int *data)
{
	if (num <0 || num>=num_pcieAddressValueData)
		return ERR_RETURN;
	*address = pcieAddressValueData[num].address;
	*data = (unsigned int) pcieAddressValueData[num].dataLow +
			(unsigned int) (pcieAddressValueData[num].dataHigh << 16);
	return VALUE_OK;
}

A_INT32 Ar9300pcieAddressValueDataRemove( unsigned int address)
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
		pcieAddressValueData[num_pcieAddressValueData].address = 0xFFFFFFFF;
		pcieAddressValueData[num_pcieAddressValueData].dataLow = 0xFFFF;
		pcieAddressValueData[num_pcieAddressValueData].dataHigh = 0xFFFF;
		num_pcieAddressValueData--;
	}
    return iFound;
}


A_INT32 Ar9300deviceIDSet(unsigned int deviceID) 
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

A_INT32 Ar9300deviceIDGet(unsigned int *deviceID) 
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

A_INT32 Ar9300vendorSet(unsigned int vendorID)
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

A_INT32 Ar9300vendorGet(unsigned int *vendorID)
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

A_INT32 Ar9300SSIDSet(unsigned int SSID) 
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

A_INT32 Ar9300SSIDGet(unsigned int *SSID) 
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

A_INT32 Ar9300SubVendorSet(unsigned int subVendorID)
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

A_INT32 Ar9300SubVendorGet(unsigned int *subVendorID)
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

#ifdef UNUSED
A_INT32 Ar9300ConfigPCIeOnBoard(int iItem, unsigned int *addr, unsigned int *data) 
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
#endif

A_INT32 Ar9300ConfigSpaceCommitEeprom() 
{
	int limit;
	unsigned int addr;
	unsigned char pcieByteArray[2*1024]; //16Kb->2kByte
	int addrOffset = 0;
    int i;
    unsigned char pcieConfigHeader[6] = {0x5a, 0xa5, 0, 0, 3, 0};
	addr = ((unsigned int)(pcieConfigHeader[4]) + (unsigned int)(pcieConfigHeader[5] <<8)) * 2;		

        addrOffset = getPcieAddressOffset(AH);	
	limit=Ar9300EepromUsed();
	if(limit<=addr+num_pcieAddressValueData*addrOffset+2)
	{
		ErrorPrint(PcieWontFit,addr+num_pcieAddressValueData*addrOffset+2,limit);
		return-1;
	}

    Ar9300EepromWrite(0, pcieConfigHeader, 6);

	// need tp add FFFF at the end of value for mark
	// [4] is the lsb for eep addr start, [5] is the msb for eep addr start
	if (num_pcieAddressValueData+1<MAX_pcieAddressValueData) {	// add 0xFFFF to mark as the end
		pcieAddressValueData[num_pcieAddressValueData].address =0xFFFFFFFF;
	}

    // Convert to byte array
    // Fill up pcie Byte Array with default values (0xff)
    for(i=0; i<(2*1024); i++)
        pcieByteArray[i] = 0xff;
    for(i=0; i<num_pcieAddressValueData; i++)
    {
	if(addrOffset == 8){
	        pcieByteArray[addrOffset*i+0] = pcieAddressValueData[i].address & 0xff;
	        pcieByteArray[addrOffset*i+1] = (pcieAddressValueData[i].address >> 8) & 0xff;
	        pcieByteArray[addrOffset*i+2] = (pcieAddressValueData[i].address >> 16) & 0xff;
	        pcieByteArray[addrOffset*i+3] = (pcieAddressValueData[i].address >> 24) & 0xff;
	        pcieByteArray[addrOffset*i+4] = pcieAddressValueData[i].dataLow & 0xff;
	        pcieByteArray[addrOffset*i+5] = (pcieAddressValueData[i].dataLow >> 8) & 0xff;
	        pcieByteArray[addrOffset*i+6] = pcieAddressValueData[i].dataHigh & 0xff;
	        pcieByteArray[addrOffset*i+7] = (pcieAddressValueData[i].dataHigh >> 8) & 0xff;
	}
	else
	{
	        pcieByteArray[addrOffset*i+0] = pcieAddressValueData[i].address & 0xff;
	        pcieByteArray[addrOffset*i+1] = (pcieAddressValueData[i].address >> 8) & 0xff;
	        pcieByteArray[addrOffset*i+2] = pcieAddressValueData[i].dataLow & 0xff;
	        pcieByteArray[addrOffset*i+3] = (pcieAddressValueData[i].dataLow >> 8) & 0xff;
	        pcieByteArray[addrOffset*i+4] = pcieAddressValueData[i].dataHigh & 0xff;
	        pcieByteArray[addrOffset*i+5] = (pcieAddressValueData[i].dataHigh >> 8) & 0xff;
	}
    }
	Ar9300EepromWrite(addr, pcieByteArray, num_pcieAddressValueData*addrOffset+2);
    
    return 0;
}

A_INT32 Ar9300ConfigSpaceCommitOtp() 
{
	int it;
	int address, addrMax, start_address;
	unsigned int eregister,evalue;
	int nmany;
	unsigned int nregister[MPCIE],nvalue[MPCIE];
	unsigned int check;
	unsigned int invalidate=0xffff;
	unsigned int readit=0x4;
	int out;
	int limit;

        addrMax = getPcieOtpTopAddress(AH);	

#ifdef UNUSED
	if (AR_SREV_POSEIDON_10(AH))
        start_address = PCIE_OTP_BASE_POSEIDON_10;
    else
#endif
        start_address = PCIE_OTP_BASE;	
		
	limit=Ar9300EepromUsed();
	limit-=8;		// becuase we do the check on the address field (4 bytes) and the value follows (another 4 bytes)
	limit-=4;		// to allow 4 bytes of zeros before calibration structure
	//
	// form new value array and check whether it is already in the existing array
	//
	nmany=num_pcieAddressValueData;
    for(it=0; it<nmany; it++)
    {
        nregister[it] = pcieAddressValueData[it].address;
		nvalue[it]=(pcieAddressValueData[it].dataLow&0xffff)|((pcieAddressValueData[it].dataHigh&0xffff)<<16);
    }
//	UserPrint("need to write %d pairs\n",nmany);
	//
	// read the existing pcie space
	// if the register is the same as the new one, check the value
	//     if equal, go on to the next
	//     if not equal, see if we can fix it
	//         if not invalidate existing, and go on to next exisitng pair
	// if not the same register, invalidate it and go on to the next exisiting
	//
	out=0;
	for(address=start_address; address<addrMax; address+=8)
	{
		ReadIt(address,&eregister);
		//
		// are we done?
		//
		if(eregister==0 /*|| eregister==0xffffffff*/)			// DEBUG TEST 
		{
//			UserPrint("end of exisiting at %x out=%d\n",address,out);
			break;
		}
		if((eregister&0x3)==0)
		{
			//
			// if no more new registers, then we invalidate all of the rest
			//
			if(out>=nmany)
			{
				WriteIt(address,invalidate);
				ReadIt(address+4,&check);
				if((check&0x3)==0)
				{
					//
					// otp error, reject chip
					//
					ErrorPrint(PcieVerify,address,check,invalidate);
					return -1;
				}
			}
			//
			// if registers are the same
			//
			else if(eregister==nregister[out])
			{
				ReadIt(address+4,&evalue);
//				UserPrint("old=%08x new=%08x",evalue,nvalue[out]);
				//
				// register same, value same
				//
				if(evalue==nvalue[out])
				{
//					UserPrint("register same,value same, address=%x out=%d",address,out);
					out++;
					continue;
				}
				else if((evalue&(~nvalue[out]))==0)
				{
//					UserPrint("register same,value fixable, address=%x out=%d old=%08x new=%08x",address,out,evalue,nvalue[out]);
					//
					// register same, value fixable
					//
					WriteIt(address+4,nvalue[out]); // Update new fixable config data
					ReadIt(address+4,&check);
					if(check!=nvalue[out])
					{
                        printf("Verify failed\n");
						WriteIt(address,invalidate);
						ReadIt(address,&check);
						if((check&0x3)==0)
						{
							//
							// otp error, reject chip
							//
							ErrorPrint(PcieVerify,address,check,invalidate);
							return -1;
						}
					}
                    else
                        out++; // Scan next config pair.
				}
				else
				{
//					UserPrint("register same,value different, address=%x out=%d",address,out);
					//
					// register same, value bad
					//
					WriteIt(address,invalidate);
					ReadIt(address,&check);
					if((check&0x3)==0)
					{
						//
						// otp error, reject chip
						//
						ErrorPrint(PcieVerify,address,check,invalidate);
						return -1;
					}
                    {
                        int i2;
                        // Move new data at the end of array, and move the rest backward.
                        nregister[nmany]=nregister[out];
                        nvalue[nmany]=nvalue[out];
                        pcieAddressValueData[nmany].address=pcieAddressValueData[out].address;
                        pcieAddressValueData[nmany].dataLow=pcieAddressValueData[out].dataLow;
                        pcieAddressValueData[nmany].dataHigh=pcieAddressValueData[out].dataHigh;
                        for (i2=out; i2<nmany; i2++)
                        {
                            nregister[i2]=nregister[i2+1]; 
                            nvalue[i2]=nvalue[i2+1];
                            pcieAddressValueData[i2].address = pcieAddressValueData[i2+1].address;
                            pcieAddressValueData[i2].dataLow = pcieAddressValueData[i2+1].dataLow;
                            pcieAddressValueData[i2].dataHigh = pcieAddressValueData[i2+1].dataHigh;

                        }
                    }                        
				}
			}
			//
			// registers are different
			//
			else
			{
//				UserPrint("register different, address=%x out=%d",address,out);
				//
				// register same, value bad
				//
				WriteIt(address,invalidate);
				ReadIt(address,&check);
				if((check&0x3)==0)
				{
					//
					// otp error, reject chip
					//
					ErrorPrint(PcieVerify,address,check,invalidate);
					return -1;
				}
			}
		}
        else
        {
            printf("eregister=%x skip it.\n", eregister);
		}
	}
	//
	// if any new pairs are left over, add them to the end
	//
	for( ; out<nmany; out++)
	{
		if(address>limit)
		{
			//
			// otp error, reject chip
			//
			ErrorPrint(PcieWontFit,address,limit);
			return -1;
		}
		//
		// write and verify register address
		//
//		UserPrint("new, address=%x out=%d",address,out);
		WriteIt(address,nregister[out]);
		ReadIt(address,&check);
		if(check!=nregister[out])
		{
			WriteIt(address,invalidate);
			ReadIt(address,&check);
			if((check&0x3)==0)
			{
				//
				// otp error, reject chip
				//
				ErrorPrint(PcieVerify,address,check,invalidate);
				return -1;
			}
			else
			{
				//
				// try again
				//
				out--;
			}
		}
		//
		// write and verify register value
		//
		WriteIt(address+4,nvalue[out]);
		ReadIt(address+4,&check);
		if(check!=nvalue[out])
		{
			WriteIt(address,invalidate);
			ReadIt(address,&check);
			if((check&0x3)==0)
			{
				//
				// otp error, reject chip
				//
				ErrorPrint(PcieVerify,address,check,invalidate);
				return -1;
			}
			else
			{
				//
				// try again
				//
				out--;
			}
		}

		address+=8;
	}
	//
	// check that next word is all zero
	//
//	UserPrint("check, address=%x",address);
	ReadIt(address,&check);
	if(check!=0 /*&& check!=0xffffffff*/)				// DEBUG TEST
	{
		//
		// otp error, reject chip
		//
		ErrorPrint(PcieVerify,address,check,0);
		return -1;
	}
	//
	// Save highest address
	//
	PcieTop=address+8;
	//
	// and we have to write bit 2 of byte 0 to get the chip to look at the initialization data
	//
	ReadIt(0, &check);
	if((check&readit)==0)
	{
 		WriteIt(0, readit);
		ReadIt(0, &check);
		//
		// just check the bit we tried to write, we don't care about other bits
		//
		if(readit!=(check&readit))
		{
			//
			// otp error, reject chip
			//
			ErrorPrint(PcieVerify,address,check,readit);
			return -1;
		}
	}
   
    return PcieTop;
}

A_INT32 Ar9300ConfigSpaceCommit() 
{
	switch(ar9300_calibration_data_get(AH))
	{
		case calibration_data_eeprom:
			return Ar9300ConfigSpaceCommitEeprom();
		case calibration_data_flash:
		case calibration_data_otp:
			return Ar9300ConfigSpaceCommitOtp();
	}
	return -1;
}


A_INT32 Ar9300ConfigSpaceUsedEeprom(void)
{
	unsigned int addr;
	int addrOffset = 0;

	unsigned char word[6] = {0x5a, 0xa5, 0, 0, 3, 0};
	unsigned char word_8[8] = {0, 0, 0, 0, 0, 0, 0, 0};
	unsigned char *wordPtr;

	// first read eep back to see if it's a blank card
	if (Ar9300EepromRead(0, word, 6) || word[0]==0xff)
	{
		return 0;							// nothing on card, return minimum size
	}

        addrOffset = getPcieAddressOffset(AH);
        if(addrOffset == 8)		
            wordPtr = &word_8[0];
        else
            wordPtr = &word[0];

	// from last 2 byte header to figure out the eep start address.
	for(addr = ((unsigned int)(word[4]) + (unsigned int)(word[5] <<8)) * 2;	addr<500; addr+=addrOffset)	
	{
		if (Ar9300EepromRead(addr, wordPtr, addrOffset))
		{
			break;	// bad eep reading
		}
		if (wordPtr[0]==0xFF && wordPtr[1]==0xFF) 
		{	
			break;	// found the end of eep data on card
		} 
	}

	return addr;
}


A_INT32 Ar9300ConfigSpaceUsedOtp(void)
{
	int address, addrMax, start_address;
	unsigned int eregister,evalue;

        addrMax = getPcieOtpTopAddress(AH);	

#ifdef UNUSED
    if (AR_SREV_POSEIDON_10(AH))
        start_address = PCIE_OTP_BASE_POSEIDON_10;
    else
#endif
        start_address = PCIE_OTP_BASE;

	for(address=start_address; address<addrMax; address+=8)
	{
		ReadIt(address,&eregister);
		//
		// are we done?
		//
		if(eregister==0 /*|| eregister==0xffffffff*/)			// DEBUG TEST 
		{
//			UserPrint("end of exisiting at %x out=%d\n",address,out);
			break;
		}
	}
	PcieTop=address;
	return PcieTop;
}


A_INT32 Ar9300ConfigSpaceUsed() 
{
	switch(ar9300_calibration_data_get(AH))
	{
		case calibration_data_eeprom:
			return Ar9300ConfigSpaceUsedEeprom();
		case calibration_data_flash:
		case calibration_data_otp:
			return Ar9300ConfigSpaceUsedOtp();
	}
	return -1;
}
