#include "wlantype.h"
#include "ah.h"
#include "ar5416/ar5416.h"
#include "ar5416/ar5416phy.h"
#include "UserPrint.h"  
#include "ar9287/Ar9287Device.h"
#include "ar9287/Ar9287EepromSave.h"
#include "ar9287/Ar9287ConfigurationCommand.h"
#include "ar9287/Ar9287PcieConfig.h"
#include "ar9287/Ar9287EepromStructGet.h"
#include "ar9287/Ar9287EepromStructSet.h"
#include "ar9287/Ar9287TargetPower.h"
#include "ar9287/Ar9287Temperature.h"

extern struct ath_hal *AH;

void Ar5416SetTargetPowerFromEeprom(struct ath_hal *ah, u_int16_t freq)
{
    struct ath_hal_5416 *ahp = AH5416(ah);
    struct ath_hal_private *ahpriv = AH_PRIVATE(ah);
    HAL_CHANNEL_INTERNAL *ichan = ahpriv->ah_curchan;
    HAL_CHANNEL *chan = (HAL_CHANNEL *)ichan;
    
    if (ar5416EepromSetTransmitPower(ah, &ahp->ah_eeprom, ichan,
        ath_hal_getctl(ah, chan), ath_hal_getantennaallowed(ah, chan),
        chan->max_reg_tx_power * 2,
        AH_MIN(MAX_RATE_POWER, ahpriv->ah_power_limit)) != HAL_OK)
    {
        UserPrint("ar5416EepromSetTransmitPower error\n");
    }
}

int Ar5416EepromRead(unsigned long address, unsigned char *value, int count)
{
    struct ath_hal_5416 *ahp = AH5416(AH);
    
	if (ahp->ah_eep_map == EEP_MAP_AR9287)
	{
		return Ar9287EepromRead(address,value, count);
	}
	else if (ahp->ah_eep_map == EEP_MAP_4KBITS)
	{
		//TODO
	}
	else
	{
	    //TODO
	}
	return 1; //bad return    
}

int Ar5416EepromWrite(unsigned long address, unsigned char *value, int count)
{
    struct ath_hal_5416 *ahp = AH5416(AH);
    
	if (ahp->ah_eep_map == EEP_MAP_AR9287)
	{
		return Ar9287EepromWrite(address,value, count);
	}
	else if (ahp->ah_eep_map == EEP_MAP_4KBITS)
	{
		//TODO
	}
	else
	{
	    //TODO
	}
	return 1; //bad return    
}

int Ar5416GetCommand(int code, int client, int ip, int *out_error, int *out_done, unsigned char* in_name)
{
    struct ath_hal_5416 *ahp = AH5416(AH);
    
	if (ahp->ah_eep_map == EEP_MAP_AR9287)
	{
		return Ar9287GetCommand(client);
	}
	else if (ahp->ah_eep_map == EEP_MAP_4KBITS)
	{
		//TODO
	}
	else
	{
	    //TODO
	}
	return 0;
}
int Ar5416SetCommand(int code, int client, int *out_ip, int *out_error, int *out_done, unsigned char *in_name)
{
    struct ath_hal_5416 *ahp = AH5416(AH);
    
	if (ahp->ah_eep_map == EEP_MAP_AR9287)
	{
		return Ar9287SetCommand(client);
	}
	else if (ahp->ah_eep_map == EEP_MAP_4KBITS)
	{
		//TODO
	}
	else
	{
	    //TODO
	}
	return 0;
}

A_INT32 Ar5416ConfigSpaceCommit(void) 
{
    struct ath_hal_5416 *ahp = AH5416(AH);
    
	if (ahp->ah_eep_map == EEP_MAP_AR9287)
	{
		return Ar9287ConfigSpaceCommit();
	}
	else if (ahp->ah_eep_map == EEP_MAP_4KBITS)
	{
		//TODO
	}
	else
	{
	    //TODO
	}
	return 0;   
}

A_INT32 Ar5416pcieAddressValueDataInit(void)
{
    struct ath_hal_5416 *ahp = AH5416(AH);
    
	if (ahp->ah_eep_map == EEP_MAP_AR9287)
	{
		return Ar9287pcieAddressValueDataInit();
	}
	else if (ahp->ah_eep_map == EEP_MAP_4KBITS)
	{
		//TODO
	}
	else
	{
	    //TODO
	}
	return -1;    
}

A_INT32 Ar5416_MacAdressGet(A_UINT8 *mac)
{
    struct ath_hal_5416 *ahp = AH5416(AH);
    
	if (ahp->ah_eep_map == EEP_MAP_AR9287)
	{
		return Ar9287_MacAdressGet(mac);
	}
	else if (ahp->ah_eep_map == EEP_MAP_4KBITS)
	{
		//TODO
	}
	else
	{
	    //TODO
	}    
    return -1;
}

A_INT32 Ar5416_CustomerDataGet(A_UCHAR *data, A_INT32 max)
{
    struct ath_hal_5416 *ahp = AH5416(AH);
    
	if (ahp->ah_eep_map == EEP_MAP_AR9287)
	{
		return Ar9287_CustomerDataGet(data, max);
	}
	else if (ahp->ah_eep_map == EEP_MAP_4KBITS)
	{
		//TODO
	}
	else
	{
	    //TODO
	}
    return -1;
}

int Ar5416EepromGetTargetPwr(int freq, int rateIndex, double *powerOut) 
{
    struct ath_hal_5416 *ahp = AH5416(AH);
    
	if (ahp->ah_eep_map == EEP_MAP_AR9287)
	{
		return Ar9287TargetPowerGet(freq, rateIndex, powerOut);
	}
	else if (ahp->ah_eep_map == EEP_MAP_4KBITS)
	{
		//TODO
	}
	else
	{
	    //TODO
	}    
    return 0;
}

int Ar5416EepromSave(void)
{
    struct ath_hal_5416 *ahp = AH5416(AH);
    
	if (ahp->ah_eep_map == EEP_MAP_AR9287)
	{
		return Ar9287EepromSave();
	}
	else if (ahp->ah_eep_map == EEP_MAP_4KBITS)
	{
		//TODO
	}
	else
	{
	    //TODO
	}
    return -1;
}

#undef REGR
extern int MyRegisterRead(unsigned long address, unsigned long *value);
static unsigned long REGR(unsigned long devNum, unsigned long address)
{
	unsigned long value;

	devNum=0;

	MyRegisterRead(address,&value);

	return value;
}

int Ar5416TemperatureGet(int forceTempRead)
{
    unsigned long value;
    
    ar5416OpenLoopPowerControlTempCompensation(AH);
    value = REGR(0,AR_PHY_TX_PWRCTRL4);
    //bb_ch0_pd_avg_out            8  TPCRG4_CH0  0xa264   8:1
    value = (value>>1) & 0xFF;
    return value;
}

int Ar5416VoltageGet(void)
{
    struct ath_hal_5416 *ahp = AH5416(AH);
    
	if (ahp->ah_eep_map == EEP_MAP_AR9287)
	{
		return Ar9287VoltageGet();
	}
	else if (ahp->ah_eep_map == EEP_MAP_4KBITS)
	{
		//TODO
	}
	else
	{
	    //TODO
	}
    return -1;
}

int Ar5416Deaf(int deaf) 
{
    struct ath_hal_5416 *ahp = AH5416(AH);
    
	if (ahp->ah_eep_map == EEP_MAP_AR9287)
	{
		return Ar9287Deaf(deaf);
	}
	else if (ahp->ah_eep_map == EEP_MAP_4KBITS)
	{
		//TODO
	}
	else
	{
	    //TODO
	}
    return -1;
}

int Ar5416EepromReport(void (*print)(char *format, ...), int all)
{
    struct ath_hal_5416 *ahp = AH5416(AH);
    
	if (ahp->ah_eep_map == EEP_MAP_AR9287)
	{
		return Ar9287EepromReport(print,all);
	}
	else if (ahp->ah_eep_map == EEP_MAP_4KBITS)
	{
		//TODO
	}
	else
	{
	    //TODO
	}
    return -1;
}
u_int16_t *ar5416RegulatoryDomainGet(struct ath_hal *ah)
{
    struct ath_hal_5416 *ahp = AH5416(ah);
    if (ahp->ah_eep_map == EEP_MAP_AR9287)
    {
        return ar9287RegulatoryDomainGet(ah);
    }
    else
    {
        //TODO
    }
    return NULL;
}
