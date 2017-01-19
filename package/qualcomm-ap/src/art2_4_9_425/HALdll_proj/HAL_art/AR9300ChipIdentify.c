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
//#include "ah_devid.h"
#include "ChipIdentify.h"
#include "ah_internal.h"
#include "ar9300reg.h"

#include "ErrorPrint.h"
#include "CardError.h"
#include "ParameterSelect.h"

#ifdef MDK_AP          // MDK_AP is defined only for NART. 
#include "linux_hw.h"
#endif

#include "AnwiDriverInterface.h"
#include "AR9300ChipIdentify.h"

#ifndef NART_SCORPION_SUPPORT // To Do:: This 'ifndef endif' block needs to be removed when wlan/hal/ah_devid.h is updated with scorpion device ID. Only 'scorpion_dev' HAL branch defines this device ID now. 
#define AR9300_DEVID_AR955X       0x0039        /* Scorpion */
#endif

static struct _ParameterList ChipDevidParameter[]=
{
	{ChipUnknown,{"unknown","automatic",0},0,0,0,0,0,0,0,0,0},						// unknown
	{ChipTest,{"NoChip",0,0},0,0,0,0,0,0,0,0,0},										// text based test device
	{ChipLinkTest,{"LinkNoChip",0,0},0,0,0,0,0,0,0,0,0},								// text based test device
	{AR9300_DEVID_AR9380_PCIE,{"ar938x","ar939x","osprey"},0,0,0,0,0,0,0,0,0},		// osprey
	{AR9300_DEVID_AR9580_PCIE,{"ar9580","peacock",0},0,0,0,0,0,0,0,0,0},			// peacock
	{AR9300_DEVID_AR9340,{"ar9340","wasp",0},0,0,0,0,0,0,0,0,0},					// wasp
	{AR9300_DEVID_AR955X,{"ar955x","scorpion",0},0,0,0,0,0,0,0,0,0},					// scorpion
	{AR9300_DEVID_AR956X,{"ar956x","dragonfly",0},0,0,0,0,0,0,0,0,0},                    // dragonfly
	{AR9300_DEVID_AR953X,{"ar953x","honeybee",0},0,0,0,0,0,0,0,0,0},					// honeybee
//	{AR9300_DEVID_AR946X_PCIE,{"ar946x","jupiter",0},0,0,0,0,0,0,0,0,0},			// jupiter
//	{AR9300_DEVID_AR9485_PCIE,{"ar9485","poseidon",0},0,0,0,0,0,0,0,0,0},			// poseidon
//	{AR9300_DEVID_AR9330,{"ar9330","hornet",0},0,0,0,0,0,0,0,0,0},					// hornet
//	{AR6300_DEVID,{"ar6300","mckinley",0},0,0,0,0,0,0,0,0,0},						// mckinley
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

int Ar9300ChipIdentify(void)
{
    unsigned int macid;
    unsigned int macrev;
    int status;
    int devid=-1;
#ifdef LINUX
    int chip_rev_id=-1;
#endif
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
        switch(macid)
		{
			case AR_SREV_VERSION_OSPREY:
				if(macrev >= AR_SREV_REVISION_AR9580_10)
				{	// peacock
					devid=AR9300_DEVID_AR9580_PCIE;
				}
				else
				{	// osprey
					devid=AR9300_DEVID_AR9380_PCIE;
				}
				break;
			//case AR_SREV_VERSION_POSEIDON:
			//	devid=AR9300_DEVID_AR9485_PCIE;
			//	break;
			//case AR_SREV_VERSION_JUPITER:
			//	devid=AR9300_DEVID_AR946X_PCIE;
			//	break;
			//case AR_SREV_VERSION_HORNET:
			//	devid=AR9300_DEVID_AR9330;
			//	break;
			default:
#ifdef LINUX
				chip_rev_id=FullAddrRead(CHIP_ID_LOCATION);
				chip_rev_id=(chip_rev_id&0x0ff0) >> 4;
				switch (chip_rev_id){
					case CHIP_REV_ID_WASP:
						devid= AR9300_DEVID_AR9340;
						break;
					case CHIP_REV_ID_SCORPION:
						devid= AR9300_DEVID_AR955X;
						break;
					case CHIP_REV_ID_HONEYBEE:
						devid= AR9300_DEVID_AR953X;
						break;
					case CHIP_REV_ID_DRAGONFLY:
						devid= AR9300_DEVID_AR956X;
						break;
				}
#else
                	devid=0;
#endif
			break;
		}
    }
    AnwiDriverDetach();
    return devid;
}

#ifndef DYNAMIC_DEVICE_DLL

#include "Ar9300Device.h"

int ChipSelect(int devid)
{
	switch(devid)
	{
		case AR9300_DEVID_AR9380_PCIE:
#ifndef USE_AQUILA_HAL
		case AR9300_DEVID_AR946X_PCIE:
#endif
		case AR9300_DEVID_AR9580_PCIE:
		case AR9300_DEVID_AR9485_PCIE:
		case AR9300_DEVID_AR9330:
		case AR9300_DEVID_AR9340:
		case AR9300_DEVID_AR955X:
		case AR9300_DEVID_AR956X:
		case AR9300_DEVID_AR953X:
			Ar9300DeviceSelect();
			return 0;
	}
	return -1;
}
#endif
