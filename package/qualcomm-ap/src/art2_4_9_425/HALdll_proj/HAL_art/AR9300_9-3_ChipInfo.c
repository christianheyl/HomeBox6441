#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

#include "ah.h"
//#include "ah_devid.h"
#include "ChipIdentify.h"
#include "ah_internal.h"
#include "ar9300reg.h"
#include "Field.h"
#include "Ar9300Field.h"

#include "Ar9300_ChipInfo.h"
#include "AnwiDriverInterface.h"
#include "AquilaNewmaMapping.h"

#ifdef MDK_AP          // MDK_AP is defined only for NART. 
#include "linux_hw.h"
#endif

extern  *AH;
static int devidSel=0;

#define AR955X_ENT_OTP 0xb80600b4

#ifndef NART_SCORPION_SUPPORT // To Do:: This 'ifndef endif' block needs to be removed when wlan/hal/ah_devid.h is updated with scorpion device ID. Only 'scorpion_dev' HAL branch defines this device ID now. 
#define AR9300_DEVID_AR955X       0x0039        /* Scorpion */
#endif

int Ar9300_FieldSelect(int devid)
{
	int error = 0;
	devidSel = devid;

    switch (devid) 
	{
		case AR9300_DEVID_AR946X_PCIE:			// jupiter
			Ar9300_FieldSelect_Jupiter();
			break;
		case AR9300_DEVID_AR9380_PCIE:			// osprey
		case AR9300_DEVID_EMU_PCIE:
			Ar9300_2_0_FieldSelect();
			break;
	    case AR9300_DEVID_AR9580_PCIE:			// peacock
			Ar9300_2_0_FieldSelect();
			break;
		case AR9300_DEVID_AR9340:				// wasp
			Ar9340FieldSelect();
			break;
		case AR9300_DEVID_AR9330:				// hornet
			Ar9330_FieldSelect();
			break;
		case AR9300_DEVID_AR9485_PCIE:			// poseidon
			Ar9485FieldSelect();
			break;
		case AR9300_DEVID_AR955X:			// scorpion
			Ar955X_FieldSelect();
			break;
		case AR9300_DEVID_AR956X:			// dragonfly
			Ar956X_FieldSelect();
			break;
		case AR9300_DEVID_AR953X:			// honeybee
			Ar953X_FieldSelect();
			break;
		default:
			error=-1;
			break;
	}

	return error;
}


int Ar9300pcieDefault(int devid)
{
	int error = 0;

    switch (devid) 
	{
		case AR9300_DEVID_AR946X_PCIE:			// jupiter
			Ar946XpcieDefault(devid);
			break;
		case AR9300_DEVID_AR9380_PCIE:			// osprey
		case AR9300_DEVID_EMU_PCIE:
			Ar9380pcieDefault(devid);
			break;
	    case AR9300_DEVID_AR9580_PCIE:			// peacock
			Ar9580pcieDefault(devid);
			break;
		case AR9300_DEVID_AR9340:				// wasp
			Ar934XpcieDefault(devid);
			break;
		case AR9300_DEVID_AR9330:				// hornet
			Ar9330pcieDefault(devid);
			break;
		case AR9300_DEVID_AR9485_PCIE:			// poseidon
			Ar9485pcieDefault(devid);
			break;
		case AR9300_DEVID_AR955X:			// scorpion
			break;
		case AR9300_DEVID_AR956X:			// dragonfly
			break;
		case AR9300_DEVID_AR953X:			// honeybee
			break;
		default:
			error=-1;
			break;
	}

	return error;
}


int Ar9300_TxChainMany(int txMask)
{
	int error = 0;

	int ah_enterprise_mode;
	int regChains=1;

    switch (devidSel) 
	{
		case AR9300_DEVID_AR946X_PCIE:			// jupiter
			regChains = 2;
			break;
		case AR9300_DEVID_AR9380_PCIE:			// osprey
		case AR9300_DEVID_EMU_PCIE:
	    	case AR9300_DEVID_AR9580_PCIE:			// peacock
		   // Osprey needs to be configured for 3-chain mode before running AGC/TxIQ cals 
			MyRegisterRead(AR_ENT_OTP,&ah_enterprise_mode);
			if(ah_enterprise_mode&AR_ENT_OTP_CHAIN2_DISABLE)
			{
				regChains = 2;
			}
			else
			{
				regChains = 3;
			} 
			break;
		case AR9300_DEVID_AR9340:				// wasp
			regChains = 2;
			break;
		case AR9300_DEVID_AR955X:				// scorpion
			#ifdef MDK_AP           
			ah_enterprise_mode=FullAddrRead(AR955X_ENT_OTP)<<12;
			#endif
			if(ah_enterprise_mode&AR_ENT_OTP_CHAIN2_DISABLE)
			{
				regChains = 2;
			}
			else
			{
				regChains = 3;
			}
			break;
		case AR9300_DEVID_AR956X:				// dragonfly  (no OTP)
			regChains = 3;
			break;
		case AR9300_DEVID_AR9330:				// hornet
			regChains = 1;
			break;
		case AR9300_DEVID_AR9485_PCIE:			// poseidon
			regChains = 1;
			break;
		case AR9300_DEVID_AR953X:			// honeybee (no OTP)
			regChains = 2;
			break;
		default:
			error=-1;
			break;
	}
	
	if (txMask==7) {
		return regChains;
	} else if (txMask==5 || txMask==3) {
		if (regChains>=2)
			return 2;
		else
			return 1;
	} else {
		return 1;
	} 

	return error;
}

int Ar9300_RxChainMany(int rxMask)
{
	int error = 0;

	int ah_enterprise_mode;
	int regChains=1;

    switch (devidSel) 
	{
		case AR9300_DEVID_AR946X_PCIE:			// jupiter
			regChains = 2;
			break;
		case AR9300_DEVID_AR9380_PCIE:			// osprey
		case AR9300_DEVID_EMU_PCIE:
	    	case AR9300_DEVID_AR9580_PCIE:			// peacock
		   // Osprey needs to be configured for 3-chain mode before running AGC/TxIQ cals 
			MyRegisterRead(AR_ENT_OTP,&ah_enterprise_mode);
			if(ah_enterprise_mode&AR_ENT_OTP_CHAIN2_DISABLE)
			{
				regChains = 2;
			}
			else
			{
				regChains = 3;
			} 
			break;
		case AR9300_DEVID_AR9340:				// wasp
			regChains = 2;
			break;
		case AR9300_DEVID_AR955X:				// scorpion
			#ifdef MDK_AP           
			ah_enterprise_mode=FullAddrRead(AR955X_ENT_OTP)<<12;
			#endif
			if(ah_enterprise_mode&AR_ENT_OTP_CHAIN2_DISABLE)
			{
				regChains = 2;
			}
			else
			{
				regChains = 3;
			}
			break;
		case AR9300_DEVID_AR956X:				// dragonfly (no OTP)
			regChains = 3;
			break;
		case AR9300_DEVID_AR9330:				// hornet
			regChains = 1;
			break;
		case AR9300_DEVID_AR9485_PCIE:			// poseidon
			regChains = 1;
			break;
		case AR9300_DEVID_AR953X:			// honeybee (no OTP)
			regChains = 2;
			break;
		default:
			error=-1;
			break;
	}
	
	if (rxMask==7) {
		return regChains;
	} else if (rxMask==5 || rxMask==3) {
		if (regChains>=2)
			return 2;
		else
			return 1;
	} else {
		return 1;
	}

	return error;
}

int Ar9300_is2GHz(int opflag)
{
	int error = 0;
	int ah_mode;
	int regFlag=1;

    switch (devidSel) 
	{
		case AR9300_DEVID_AR946X_PCIE:			// jupiter
			regFlag = 1;
			break;
		case AR9300_DEVID_AR9380_PCIE:			// osprey
		case AR9300_DEVID_EMU_PCIE:
	    	case AR9300_DEVID_AR9580_PCIE:			// peacock
			regFlag = 1;
			break;
		case AR9300_DEVID_AR9340:				// wasp
			regFlag = 1;
			break;
		case AR9300_DEVID_AR955X:				// scorpion
			regFlag = 1;
			break;
		case AR9300_DEVID_AR956X:				// dragonfly
			regFlag = 1;
			break;
		case AR9300_DEVID_AR953X:				// honeybee
			regFlag = 1;
			break;
		case AR9300_DEVID_AR9330:				// hornet
			regFlag = 1;
			break;
		case AR9300_DEVID_AR9485_PCIE:			// poseidon
			regFlag = 1;
			break;
		default:
			error=-1;
			break;
	}
	if (regFlag==1) {
		if (opflag&(0x2))
			return 1;
		else
			return 0;
	} else {
		return 0;
	}
}

int Ar9300_is5GHz(int opflag)
{
	int error = 0;
	int ah_mode;
	int regFlag=1;

    switch (devidSel) 
	{
		case AR9300_DEVID_AR946X_PCIE:			// jupiter
			regFlag = 1;
			break;
		case AR9300_DEVID_AR9380_PCIE:			// osprey
		case AR9300_DEVID_EMU_PCIE:
	    case AR9300_DEVID_AR9580_PCIE:			// peacock
			MyRegisterRead(AR_ENT_OTP,&ah_mode);
			if(ah_mode&AR_ENT_OTP_DUAL_BAND_DISABLE)
			{
				 regFlag=0;
			}
			break;
		case AR9300_DEVID_AR9340:				// wasp
			regFlag = 1;
			break;
		case AR9300_DEVID_AR955X:				// scorpion
			#ifdef MDK_AP           
			ah_mode=FullAddrRead(AR955X_ENT_OTP)<<12;
			#else
			MyRegisterRead(AR955X_ENT_OTP,&ah_mode);
			#endif
			if(ah_mode&AR_ENT_OTP_DUAL_BAND_DISABLE)
			{
				 regFlag=0;
			}
			break;
		case AR9300_DEVID_AR956X:				// dragonfly
			regFlag = 0;
			break;
		case AR9300_DEVID_AR953X:				// honeybee
			regFlag = 0;
			break;
		case AR9300_DEVID_AR9330:				// hornet
			regFlag = 1;
			break;
		case AR9300_DEVID_AR9485_PCIE:			// poseidon
			regFlag = 1;
			break;
		default:
			error=-1;
			break;
	}
	if (regFlag==1) {
		if ( opflag&(0x1) )
			return 1;
		else
			return 0;
	} else {
		return 0;
	}
}

int Ar9300_is4p9GHz(void)
{
	int is4p9G = 0;
	int ah_mode;

    switch (devidSel) 
	{
		case AR9300_DEVID_AR946X_PCIE:			// jupiter
			is4p9G = 0;
			break;
		case AR9300_DEVID_AR9380_PCIE:			// osprey
		case AR9300_DEVID_EMU_PCIE:
	    case AR9300_DEVID_AR9580_PCIE:			// peacock
		MyRegisterRead(AR_ENT_OTP,&ah_mode);
			if(ah_mode&AR_ENT_OTP_49GHZ_DISABLE)
			{
				is4p9G = 0;
			}
			else
			{
				is4p9G = 1;
			}
			break;
		case AR9300_DEVID_AR9340:				// wasp
			is4p9G = 0;
			break;
		case AR9300_DEVID_AR955X:				// scorpion
			#ifdef MDK_AP           
			ah_mode=FullAddrRead(AR955X_ENT_OTP)<<12;
			#else
			MyRegisterRead(AR955X_ENT_OTP,&ah_mode);
			#endif
			if(ah_mode&AR_ENT_OTP_49GHZ_DISABLE)
			{
				 is4p9G=0;
			}
			else
			{
				is4p9G = 1;
			}
			break;
		case AR9300_DEVID_AR9330:				// hornet
			is4p9G = 0;
			break;
		case AR9300_DEVID_AR956X:				// dragonfly
			is4p9G = 0;
			break;
		case AR9300_DEVID_AR953X:				// honeybee
			is4p9G = 0;
			break;
		case AR9300_DEVID_AR9485_PCIE:			// poseidon
			is4p9G = 0;
			break;
		default:
			is4p9G=-1;
			break;
	}
	return is4p9G;
}

int Ar9300_HalfRate(void)
{
	int Half = 0;
	int ah_mode;

    switch (devidSel) 
	{
		case AR9300_DEVID_AR946X_PCIE:			// jupiter
			Half = 0;
			break;
		case AR9300_DEVID_AR9380_PCIE:			// osprey
		case AR9300_DEVID_EMU_PCIE:
	    case AR9300_DEVID_AR9580_PCIE:			// peacock
			MyRegisterRead(AR_ENT_OTP,&ah_mode);
	        if(ah_mode&AR_ENT_OTP_10MHZ_DISABLE)
			{
				Half = 0;
			}
			else
			{
				Half = 1;
			}
			break;
		case AR9300_DEVID_AR9340:				// wasp
			Half = 0;
			break;
		case AR9300_DEVID_AR9330:				// hornet
			Half = 0;
			break;
		case AR9300_DEVID_AR953X:				// honeybee
			Half = 0;
			break;
		case AR9300_DEVID_AR955X:				// scorpion
			#ifdef MDK_AP           
			ah_mode=FullAddrRead(AR955X_ENT_OTP)<<12;
			#else
			MyRegisterRead(AR955X_ENT_OTP,&ah_mode);
			#endif
			if(ah_mode&AR_ENT_OTP_10MHZ_DISABLE)
			{
				Half = 0;
			}
			else
			{
				Half = 1;
			}
			break;
		case AR9300_DEVID_AR956X:				// dragonfly
			Half = 0;
			break;
		case AR9300_DEVID_AR9485_PCIE:			// poseidon
			Half = 0;
			break;
		default:
			Half=-1;
			break;
	}
	return Half;
}

int Ar9300_QuarterRate(void)
{
	int Quarter = 0;
	int ah_mode;

    switch (devidSel) 
	{
		case AR9300_DEVID_AR946X_PCIE:			// jupiter
			Quarter = 0;
			break;
		case AR9300_DEVID_AR9380_PCIE:			// osprey
		case AR9300_DEVID_EMU_PCIE:
	    case AR9300_DEVID_AR9580_PCIE:			// peacock
			MyRegisterRead(AR_ENT_OTP,&ah_mode);
	        if(ah_mode&AR_ENT_OTP_5MHZ_DISABLE)
			{
				Quarter = 0;
			}
			else
			{
				Quarter = 1;
			}
			break;
		case AR9300_DEVID_AR9340:				// wasp
			Quarter = 0;
			break;
		case AR9300_DEVID_AR955X:				// scorpion
			#ifdef MDK_AP           
			ah_mode=FullAddrRead(AR955X_ENT_OTP)<<12;
			#else
			MyRegisterRead(AR955X_ENT_OTP,&ah_mode);
			#endif
			if(ah_mode&AR_ENT_OTP_5MHZ_DISABLE)
			{
				Quarter = 0;
			}
			else
			{
				Quarter = 1;
			}
			break;
		case AR9300_DEVID_AR956X:				// dragonfly
			Quarter = 0;
			break;
		case AR9300_DEVID_AR9330:				// hornet
			Quarter = 0;
			break;
		case AR9300_DEVID_AR953X:				// honeybee
			Quarter = 0;
			break;
		case AR9300_DEVID_AR9485_PCIE:			// poseidon
			Quarter = 0;
			break;
		default:
			Quarter=-1;
			break;
	}
	return Quarter;

}
