

#include <stdio.h>
#include <stdlib.h>

#include <string.h>
#include <ctype.h>

#include "wlantype.h"

#include "UserPrint.h"
#include "LinkStat.h"
#include "art_utf_common.h"
#include "LinkRx.h"
#include "Ar6KLinkRxStat.h"
#include "Device.h"
#include "TimeMillisecond.h"
#include "rate_constants.h"
#include "vrate_constants.h"
#include "dk_cmds.h"
#include "LinkList.h"

#define MBUFFER 1024
#define MQUEUE 200		// the number of descriptors in the loop

int _LinkRxFindSpur=0;
struct rxSpurStats _LinkRxSpurStat;

static A_UINT32 RateMask[RATE_MASK_ROW_MAX];
static A_UINT32 RateMaskReported[RATE_MASK_ROW_MAX];

static void RxStatusDump(RX_STATS_STRUCT_UTF *rxStatus_utf)
{
    UserPrint("\n====dump rx status===\n");
    UserPrint("rateBit           = %d\n", rxStatus_utf->rateBit);
    UserPrint("totalPackets      = %d\n", rxStatus_utf->totalPackets);
    UserPrint("goodPackets       = %d\n", rxStatus_utf->goodPackets);
    UserPrint("otherError        = %d\n", rxStatus_utf->otherError);
    UserPrint("crcPackets        = %d\n", rxStatus_utf->crcPackets);
    UserPrint("startTime         = %d\n", rxStatus_utf->startTime);
    UserPrint("endTime           = %d\n", rxStatus_utf->endTime);
    UserPrint("byteCount         = %d\n", rxStatus_utf->byteCount);
    UserPrint("rssi              = %d\n", rxStatus_utf->rssi);
    UserPrint("evm0              = %d\n", rxStatus_utf->evm[0]);
    UserPrint("evm1              = %d\n", rxStatus_utf->evm[1]);
    UserPrint("=====================\n");
}
void ProcessRxStatusReport (unsigned char *pRxStatusBuf, int *allRateReceived)
{
    int NumOfReports;
    int iRate, RateIndx, indx;
    RX_STATS_STRUCT_UTF *pRxStatus;

    *allRateReceived = 0;
    // get 4-byte length
    NumOfReports = (int)(*((A_UINT32*)pRxStatusBuf));
    if (NumOfReports == 0)
    {
        return;
    }
    // there is report, process it
    pRxStatus = (RX_STATS_STRUCT_UTF *)(pRxStatusBuf + 4);
    for (iRate = 0; iRate < NumOfReports; ++iRate)
    {
        pRxStatus = pRxStatus + iRate;
        RateIndx = UtfvRateBit2RateIndx(pRxStatus->rateBit);

		indx = pRxStatus->rateBit / 32;
        RateMaskReported[indx] = RateMaskReported[indx] | (1 << (pRxStatus->rateBit - indx*32));
        // check for unexpected rateBit
        if ((RateMaskReported[indx] | RateMask[indx]) != RateMask[indx])
        {
            UserPrint("Unexpected rateBit (%d) received from UTF. RateMask[%d] = 0x%08x\n", pRxStatus->rateBit, indx, RateMask[indx]);
            continue;
        }

        RxStatusDump(pRxStatus);
        LinkRxStatExtractDut(pRxStatus, RateIndx);
    }

	// check if all rates have been received
	*allRateReceived = 1;
	for (indx = 0; indx < RATE_MASK_ROW_MAX; indx++)
	{
		if (RateMaskReported[indx] !=  RateMask[indx])
		{
			*allRateReceived = 0;
			break;
		}
    }
}

int Qc9KLinkRxComplete(int timeout, int ndump, unsigned char *dataPattern, int dataPatternLength, int (*done)())
{
    unsigned int startTime, endTime, ctime;
	int it, i;
	int stop;
	int scount;
	int pass;
    unsigned char *pRxStatusBuf;
    int allRateReceived = 0;
    
	memset(RateMaskReported, 0, sizeof(RateMaskReported));
    
    //
	// Loop timeout condition.
	// This number can be large since it is only used for
	// catastrophic failure of cart. The normal terminating
	// condition is a message from cart saying STOP.
	//
    startTime=TimeMillisecond();
	ctime=startTime;
	if(timeout<=0)
	{
		timeout=60*60*1000;             // one hour
	}
	endTime=startTime+timeout;		
	
	stop=0;
	scount=0;
	pass=0;

	// loop until receive "stop" from cart or timeout
	for (it = 0; ; ++it) 
	{
#ifdef FASTER
		if((it%100)==0)
		{
			UserPrint(" %d",it);
		}
#endif         
		if(stop)
		{
			scount++;
			if(scount>200)
			{
			    break;
			}
		}
		//
		// check for message from cart telling us to stop
		// this is the normal terminating condition
		//
		pass++;
		if(pass>100)
		{
            if(done!=0)
			{
				if((*done)())
				{
					UserPrint(" %d Stop",it);
					stop=1;
					scount=0;
				}
			}
			pass=0;
		    //
		    // check for timeout
		    // rare terminating condition when cart crashes or disconnects
		    //
		    ctime=TimeMillisecond();
		    if((endTime>startTime && (ctime>endTime || ctime<startTime)) || (endTime<startTime && ctime>endTime && ctime<startTime))
		    {
			    UserPrint("%d Timeout",it);
			    break;
		    }
		}
    }
    pRxStatusBuf = NULL;
    if (DeviceReceiveStop((void **)&pRxStatusBuf) != 0)
    {
        UserPrint("Error in DeviceReceiveStop\n");
    }
    else if (pRxStatusBuf)
    {
        ProcessRxStatusReport (pRxStatusBuf, &allRateReceived);
        
        // poll for the reports of all rates
        while (allRateReceived == 0)
        {
            pRxStatusBuf = NULL;
            if (DeviceReceiveStatusReport((void **)&pRxStatusBuf, 0) != 0)
            {
                UserPrint("Error in DeviceReceiveStatusReport\n");
            }
            else if (pRxStatusBuf)
            {
                ProcessRxStatusReport(pRxStatusBuf, &allRateReceived);
            }
            else
            {
                break;
            }
        }
    }
    if (allRateReceived)
    { 
        UserPrint("all rates have been received\n");
    }
    else
    { 
		UserPrint("NOT all rates have been received:\n");
		for (i = 0; i < RATE_MASK_ROW_MAX; ++i)
		{
			UserPrint("expected 0x%08x; received 0x%08x\n", RateMask[i], RateMaskReported[i]);
		}
    }
        
    UserPrint(" %d\n",it);

	LinkRxStatFinish();

    return 0;
}

int Qc9KLinkRxDataStartCmd(int freq, int cenFreqUsed, int antenna, int rxChain, int promiscuous, int broadcast,
                                    unsigned char *bssid, unsigned char *destination,
                                    int numDescPerRate, int *rate, int nrate, int bandwidth, int datacheck)
{
    A_UINT32 rateMask, rateMaskMcs20, rateMaskMcs40;
    A_UINT32 vrateMaskMcs20, vrateMaskMcs40, vrateMaskMcs80;
    RX_DATA_START_PARAMS Params;
	int i;

    RateMaskGet(&rateMask, &rateMaskMcs20, &rateMaskMcs40, rate, nrate);
	vRateMaskGet(&vrateMaskMcs20, &vrateMaskMcs40, &vrateMaskMcs80, rate, nrate);

    Params.freq = freq;
    Params.antenna = antenna;
    Params.promiscuous = promiscuous;
    Params.bandwidth = 0;
    Params.wlanMode = TCMD_WLAN_MODE_HT20;


    switch (bandwidth)
    {
        case BW_QUARTER:
            Params.bandwidth = QUARTER_SPEED_MODE;
            break;
        case BW_HALF:
            Params.bandwidth = HALF_SPEED_MODE;
            break;
        case BW_VHT80_3:
            Params.wlanMode = TCMD_WLAN_MODE_VHT80_3;
            break;
        case BW_VHT80_2:
            Params.wlanMode = TCMD_WLAN_MODE_VHT80_2;
            break;
        case BW_VHT80_1:
            Params.wlanMode = TCMD_WLAN_MODE_VHT80_1;
            break;
        case BW_VHT80_0:
            Params.wlanMode = TCMD_WLAN_MODE_VHT80_0;
            break;    
        case BW_HT40_MINUS:
            Params.wlanMode = (vrateMaskMcs40 !=0) ? TCMD_WLAN_MODE_VHT40MINUS : TCMD_WLAN_MODE_HT40MINUS;
            break;
        case BW_HT40_PLUS:
            Params.wlanMode = (vrateMaskMcs40 !=0) ? TCMD_WLAN_MODE_VHT40PLUS : TCMD_WLAN_MODE_HT40PLUS;
            break;
        default: // in case the bandwidth is invalid, derive wlanMode from rate masks
	        if (vrateMaskMcs80 !=0)
	        {
                Params.wlanMode = TCMD_WLAN_MODE_VHT80_0;
	        }
	        else if (vrateMaskMcs40 !=0)
	        {
                Params.wlanMode = TCMD_WLAN_MODE_VHT40PLUS;
            }
	        else if (vrateMaskMcs20 !=0)
	        {
                Params.wlanMode = TCMD_WLAN_MODE_VHT20;
	        }
	        else if (rateMaskMcs40 != 0) 
	        {
                Params.wlanMode = TCMD_WLAN_MODE_HT40PLUS;
            } 
	        else 
	        {
                Params.wlanMode = TCMD_WLAN_MODE_HT20;
            }
            break;
    }
    // adjust frequency for HT/VHT40
    if (!cenFreqUsed && ((vrateMaskMcs40 !=0) || (rateMaskMcs40 !=0)))
    {
        if ((Params.wlanMode == TCMD_WLAN_MODE_VHT40MINUS) || (Params.wlanMode == TCMD_WLAN_MODE_HT40MINUS))
        {
            Params.freq = freq - 10;
        }
        else
        {
            Params.freq = freq + 10;
        }
    }

    Params.enANI = 0;   //TRANG - set to disable for now
    
    Params.rxChain = rxChain;
    Params.broadcast = broadcast;
    memcpy (Params.bssid, bssid, 6);
    memcpy (Params.rxStation, destination, 6);

    memset (RateMask, 0, sizeof(RateMask));
    RateMask2UtfRateMask(rateMask, rateMaskMcs20, rateMaskMcs40, RateMask);
	vRateMask2UtfRateMask(vrateMaskMcs20, vrateMaskMcs40, vrateMaskMcs80, RateMask);
    
	for (i = 0; i < RATE_MASK_ROW_MAX; ++i)
	{
		Params.rateMask[i] = RateMask[i];
	}
    Params.numPackets = numDescPerRate;
    
    Params.bandwidth = 0;
    if (bandwidth == BW_QUARTER)
    {
        Params.bandwidth = QUARTER_SPEED_MODE;
    }
    else if (bandwidth == BW_HALF)
    {
        Params.bandwidth = HALF_SPEED_MODE;
    }
    
    // send rx command
    DeviceReceiveDataDut(&Params);

    UserPrint("RX_DATA_START_CMD sent\n");

    LinkRxStatClear();

    return 0;
}

int Qc9KLinkRxStart(int promiscuous)
{
    return 0;
}
