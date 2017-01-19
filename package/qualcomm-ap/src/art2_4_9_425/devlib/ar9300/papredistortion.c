#include <stdio.h>
#include <stdlib.h>

#include "wlantype.h"
#include "rate_constants.h"

#include "LinkTxRx.h"
#include "UserPrint.h"


#include "AquilaNewmaMapping.h"

#ifdef USE_AQUILA_HAL
#include "ar9300papd.h"
#else
#include "ar9300paprd.h"
#endif

unsigned int gain_table_entries[32], gain_vs_table_index[32];
extern struct ath_hal *AH;


//void LinkTransmitPAPD(int freq, int ht40, unsigned int TxChain, unsigned int RxChain, int chainNum )
void LinkTransmitPAPD(int chainNum)
{
	int rate = RATE_INDEX_HT20_MCS0;//32;
  	char bssid[6] = {1,1,1,1,1,1};
 	char mactx[6] = {2,2,2,2,2,2};
	char junkAddr[6] = {0,0,0,0,0,0};
	unsigned char pattern[]={0};
	int chiptemperature=0;
	//
	// get all of the packets descriptors ready to run
	//
	LinkTxForPAPD(chainNum);

	LinkTxSetup(&rate, 1, 0,
		bssid, mactx, junkAddr, 
		 5, 2000, 1, 0, 0, -1,
		0, 1<<chainNum, 1,
		pattern, 0);

	LinkTxStart();
	LinkTxComplete(1000, NULL, NULL, chiptemperature, 0);	
	
	// Clear PAPD ChainNum variable in LinkTx.c
	LinkTxForPAPD(-1);
}

void LinkTransmitPAPDWarmUp(int txchain)
{
	int rate = RATE_INDEX_HT20_MCS0;//32;
  	char bssid[6] = {1,1,1,1,1,1};
 	char mactx[6] = {2,2,2,2,2,2};
	char junkAddr[6] = {0,0,0,0,0,0};
	unsigned char pattern[]={0};
	int chiptemperature=0;
	//
	// get all of the packets descriptors ready to run
	//
	LinkTxSetup(&rate, 1, 0,
		bssid, mactx, junkAddr, 
		 2, 2000, 1, 0, 1, -1,
		0, txchain, 1,
		pattern, 0);

	LinkTxStart();
	LinkTxComplete(1000, NULL, NULL, chiptemperature, 0);	
}

#ifdef UNUSED
void paramSetPAPD()
{
    _LinkCalibrateCount=0;
	_LinkChipTemperature=0;
	_LinkNoiseFloor=0;
	_LinkRssiCalibrate=0;
	_LinkPatternLength=0;
	_LinkReset= -1;
	_LinkDuration=0;
	_LinkPacketLength=1000;
	_LinkPacketMany=100;
	_LinkFrequency=2412;
	_LinkTxChain=0x7;
	_LinkRxChain=0x7;
	_LinkRate[0]=32;
	_LinkRateMany=1;
	_LinkTpcm=TpcmTargetPower;
	_LinkPcdac=30;
	_LinkPdgain=3;
	_LinkTransmitPower= -1;
	_LinkAtt= -1;
	_LinkIss= -1;
	_LinkBc=1;
	_LinkRetryMax=0;
	_LinkStat=3;	
	_LinkIr=0;
	_LinkBandwidth=BW_AUTOMATIC;
	_LinkAgg=1;
	_LinkIfs= -1;
	_LinkDeafMode=0;
	_LinkCWmin= -1;
	_LinkCWmax= -1;
	_LinkNdump=0;
	_LinkPromiscuous=0;
    _LinkShortGI=0;
	_LinkCalibrate=0;

}
#endif

#ifdef UNUSED
void ForceLinkReset()
{
	_Papd_reset = 1;
}
#endif


int papredistortionSingleTable(struct ath_hal *ah, HAL_CHANNEL *chan, int txChainMask)
{
	int chainNum;
	unsigned int PA_table[24], smallSignalGain; 
    int status = 0, disable_papd=0, chain_fail[3]={0,0,0};
    int paprd_retry_count;

	UserPrint("Run PA predistortion algorithm\n");

    /*
     * Before doing PAPRD training, we must disable pal spare of 
     * hw greeen tx function
     */
	ar9300_hwgreentx_set_pal_spare(AH,0);//ar9300_set_pal_spare(AH,0);
	LinkTransmitPAPDWarmUp(txChainMask);
	status=	ar9300_paprd_init_table(ah, chan);
	if(status==-1)
	{
		ar9300_enable_paprd(ah, AH_FALSE, chan);
		UserPrint("Warning:: PA predistortion failed in InitTable\n");
		return -1;
	}
    {
        struct ath_hal_9300 *ahp = AH9300(ah);
        UserPrint("Training power_x2 is %d, channel %d\n", ahp->paprd_training_power, chan->channel);
    }

	for(chainNum=0; chainNum<3; chainNum++)
	{
		unsigned int i, desired_gain, gain_index;
		if(txChainMask&(1<<chainNum)) 
		{
		    paprd_retry_count = 0;
			while (paprd_retry_count < 5)
		    {
			    ar9300_paprd_setup_gain_table(ah,chainNum);
			    LinkTransmitPAPD(chainNum);
			    ar9300_paprd_is_done(ah);
			    status = ar9300_paprd_create_curve(ah, chan, chainNum);
			    if (status != HAL_EINPROGRESS)
			    {    
			        if(status==0) 
                    {
                        ar9300_populate_paprd_single_table(ah, chan, chainNum);
                                if (txChainMask == 0x2){
                                    ar9300_populate_paprd_single_table(ah, chan, 0);
                                }
                    }
                    else
                    {
                        disable_papd = 1;
                        chain_fail[chainNum] = 1;
                    }
                    break;
                }
                else
                {
                    /* need re-train paprd */
                    paprd_retry_count++;
                }
            }
            if (paprd_retry_count > 5)
                UserPrint("Warning: ch%d PAPRD re-train fail\n", (1 << chainNum));
		}
	}
	if(disable_papd==0)
    {
		ar9300_enable_paprd(ah, AH_TRUE, chan);
        /*
         * restore PAL spare of hw green tx function 
         */
		ar9300_hwgreentx_set_pal_spare(AH,1);
    }
	else
    {
		ar9300_enable_paprd(ah, AH_FALSE, chan);
        UserPrint("Warning:: PA predistortion failed. chain_fail_flag %d %d %d\n", chain_fail[0], chain_fail[1], chain_fail[2]);
        /*
         * restore PAL spare of hw green tx function 
         */
		ar9300_hwgreentx_set_pal_spare(AH,1);
        return -1;
    }

	
	return 0;
}


