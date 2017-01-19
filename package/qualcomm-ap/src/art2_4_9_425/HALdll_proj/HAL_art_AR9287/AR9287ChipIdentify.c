#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

#include "wlantype.h"

//
// hal header files
//
#include "ah.h"
#include "ah_osdep.h"
#ifdef __APPLE__
#include "osdep.h"
#endif

//#include "opt_ah.h"
#include "ah_devid.h"
#include "ah_internal.h"
#include "ar5416/ar5416reg.h"
//#include "ar9300reg.h"

#include "ErrorPrint.h"
#include "CardError.h"
#include "ParameterSelect.h"


#include "AnwiDriverInterface.h"
#include "AR9287ChipIdentify.h"

static struct _ParameterList ChipDevidParameter[]=
{
	{ChipUnknown,{"unknown","automatic",0},0,0,0,0,0,0,0,0,0},						// unknown
	{ChipTest,{"NoChip",0,0},0,0,0,0,0,0,0,0,0},										// text based test device
	{ChipLinkTest,{"LinkNoChip",0,0},0,0,0,0,0,0,0,0,0},								// text based test device
	{AR5416_DEVID_AR9287_PCIE,{"ar9287","kiwi",0},0,0,0,0,0,0,0,0,0},		
};

#define CHIP_MAC_ID (0x4020)

/*
*   Get address offset between each pcie config entry in eeprom.
*/
int getPcieAddressOffset(void *ah)
{
    return 6;
}

#define PCIE_OTP_TOP (0x200)

int getPcieOtpTopAddress(void *ah)
{
    return PCIE_OTP_TOP;
}

void ChipDevidParameterSplice(struct _ParameterList *list)
{
    list->nspecial=sizeof(ChipDevidParameter)/sizeof(ChipDevidParameter[0]);
    list->special=ChipDevidParameter;
}

int Ar9287ChipIdentify(void)
{
    unsigned int macid;
    unsigned int macrev;
    int status;
    int devid=-1;
    //
    // connect to the ANWI driver
    //
    status=AnwiDriverAttach(-1); 
    if(status<0) 
    {
	ErrorPrint(CardLoadAnwi);
	return -2;
    }

    macid=0;
    if(MyRegisterRead(CHIP_MAC_ID,&macid)==0)
    {
		macrev = (macid & AR_SREV_REVISION2) >> AR_SREV_REVISION2_S;
		macid = (macid & AR_SREV_VERSION2) >> AR_SREV_TYPE2_S;
		UserPrint("Ar9287ChipIdentify: macid=0x%x\n", macid);//rcc
        switch(macid)
		{
#define AR_SREV_VERSION_KIWI                  0x180  // Kiwi Version 
			case AR_SREV_VERSION_KIWI:
				devid=AR5416_DEVID_AR9287_PCIE;
				break;
			default:
				devid=0;
				break;
		}
    } 
    AnwiDriverDetach();
    return devid;
}

#ifndef DYNAMIC_DEVICE_DLL

#include "Ar9287Device.h"

int ChipSelect(int devid)
{
	switch(devid)
	{
		case AR5416_DEVID_AR9287_PCIE:
			Ar9287DeviceSelect();
			return 0;
	}
	return -1;
}
#endif
