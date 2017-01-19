

#include <stdio.h>
#include <stdlib.h>

#include <string.h>
#include <ctype.h>

#include "wlantype.h"

#include "LinkStat.h"
#include "LinkTxRx.h"
#include "LinkTxPacket.h"
#include "art_utf_common.h"
#include "rate_constants.h"
#include "vrate_constants.h"
#include "dk_cmds.h"
#include "Qc9KLink.h"
#include "Ar6KLinkTxStat.h"

#include "Device.h"
#include "TimeMillisecond.h"
#include "UserPrint.h"
#include "LinkList.h"
#include "testcmd.h"

static int AggregateMany=0;	
static int AggregateBar=0;		// set to 1 is using block acknowledgements	
static int PacketMany= -1;
static int RateMany= -1;
static int Rate[MRATE];
static int InterleaveRate=1;
static int PacketSize;
static unsigned int TxChain;
static int ShortGi;
static int Broadcast;
static int Retry;
static int Tx100Packet;

static A_UINT32 RateMask[RATE_MASK_ROW_MAX];
static A_UINT32 RateMaskReported[RATE_MASK_ROW_MAX];

//
// Configuration parameters to adapt to different chips
//

//
// do we get status descriptors for intermediate agg packets?
//
static int LinkTxAggregateStatus=1;

//
// the total number of packets
//
static int LinkTxMany=0;

//
// the number of valid status descriptors we expect to receive
// don't count nonterminating aggregate descriptors. Just skip those.
//
static int LinkTxStatusMany=0;

static int LinkCarrier = 0;

const static double rateMbps[RATE_MASK_ROW_MAX*32] =  {
    1,    2,     2,    5.5,   5.5,    11,    11,    0.25,       //CCK
    6,    9,    12,    18,    24,     36,    48,    54,         //OFTM
   6.5,  13.0,  19.5,  26.0,  39.0,   52.0,   58.5,  65.0,      //HT20 MCS0-7 
   13.5, 27.0,  40.5,  54.0,  81.0,  108.0,  121.5, 135.0,      //HT40 MCS0-7 
   13.0, 26.0,  39.0,  52.0,  78.0,  104.0,  117.0, 130.0,      //HT20 MCS8-15
   27.0, 54.0,  81.0, 108.0, 162.0,  216.0,  243.0, 270.0,      //HT40 MCS8-15
   19.5, 39.0,  58.5,  78.0, 117.0,  156.0,  175.5, 195.0,      //HT20 MCS16-23
   40.5, 81.0, 121.5, 162.0, 243.0,  324.0,  364.5, 405.0,		//HT40 MVC16-23

   6.5,   13.0,  19.5,  26.0,  39.0,   52.0,   58.5,  65.0,   78.0,   78.0,		//vHT20 MCS0-9 
   13.5,  27.0,  40.5,  54.0,  81.0,  108.0,  121.5, 135.0,  162.0,  180.0,		//vHT40 MCS0-9 
   29.3,  58.5,  87.8, 117.0, 175.5,  234.0,  263.3, 292.5,  351.0,  390.0,		//vHT80 MCS0-9
   13.0,  26.0,  39.0,  52.0,  78.0,  104.0,  117.0, 130.0,  156.0,  156.0,		//vHT20 MCS10-19
   27.0,  54.0,  81.0, 108.0, 162.0,  216.0,  243.0, 270.0,  324.0,  360.0,		//vHT40 MCS10-19
   58.5, 117.0, 175.5, 234.0, 351.0,  468.0,  526.5, 585.0,  702.0,  780.0,		//vHT80 MCS10-19
   19.5,  39.0,  58.5,  78.0, 117.0,  156.0,  175.5, 195.0,  234.0,  260.0,		//vHT20 MCS20-29
   40.5,  81.0, 121.5, 162.0, 243.0,  324.0,  364.5, 405.0,	 486.0,  540.0, 	//vHT40 MCS20-29
   97.5, 195.0, 292.5, 390.0, 585.0,  780.0,  780.0, 975.0, 1170.0, 1300.0,		//vHT80 MCS20-29
};
    
static unsigned int NextWaitingTxStatusTimeMiliSec()
{
    int i, RateBit;
    unsigned int WaitMiliSec = 0;
	int n;

    RateBit = -1;
	for (n = 0; n < RATE_MASK_ROW_MAX; ++n)
	{
		if (RateMaskReported[n] !=  RateMask[n])
		{
			for (i = 0; i < 32; i++)
			{
				if ((((RateMask[n] >> i) & 1) == 1) && (((RateMaskReported[n] >> i) & 1) == 0)) break;
			}
			RateBit = (i < 32) ? (i + 32*n): -1;
			break;
		}
	}
	if (RateBit >= 0)
    {
        WaitMiliSec = (unsigned int)(PacketSize * 8 / 1000 * (LinkTxMany / RateMany) / rateMbps[RateBit]) + 1;
        //UserPrint("NextWaitingTxStatusTimeMiliSec = %u ms; (PacketSize,PacketCount,RateBit,Mbps) = (%d, %d, %d, %f, \n", WaitMiliSec, PacketSize, LinkTxMany/RateMany, RateBit, rateMbps[RateBit]);
        return WaitMiliSec;
    }
    return 1000;
}

static void TxStatusDump(TX_STATS_STRUCT_UTF *txStatus_utf)
{
    UserPrint("\n====dump tx status===\n");
    UserPrint("rateBit           = %d\n", txStatus_utf->rateBit);
    UserPrint("totalPackets      = %d\n", txStatus_utf->totalPackets);
    UserPrint("goodPackets       = %d\n", txStatus_utf->goodPackets);
    UserPrint("underruns         = %d\n", txStatus_utf->underruns);
    UserPrint("otherError        = %d\n", txStatus_utf->otherError);
    UserPrint("excessiveRetries  = %d\n", txStatus_utf->excessiveRetries);
    UserPrint("startTime         = %d\n", txStatus_utf->startTime);
    UserPrint("endTime           = %d\n", txStatus_utf->endTime);
    UserPrint("byteCount         = %d\n", txStatus_utf->byteCount);
    UserPrint("rssi              = %d\n", txStatus_utf->rssi);
    UserPrint("=====================\n");
}

int ProcessTxStatusReport (unsigned char *pTxStatusBuf, int *txDone)
{
    unsigned int NumOfReports;
    int iRate, RateIndx, indx;
    TX_STATS_STRUCT_UTF *pTxStatus;

    *txDone = 0;
    // get 4-byte length
    NumOfReports = (int)(*((A_UINT32*)pTxStatusBuf));
    if (NumOfReports == 0)
    {
        return 1;
    }
    // there is report, process it
    pTxStatus = (TX_STATS_STRUCT_UTF *)(pTxStatusBuf + 4);
    for (iRate = 0; iRate < (int) NumOfReports; ++iRate)
    {
        pTxStatus = pTxStatus + iRate;
        RateIndx = UtfvRateBit2RateIndx(pTxStatus->rateBit);
        
		indx = pTxStatus->rateBit / 32;
        RateMaskReported[indx] = RateMaskReported[indx] | (1 << (pTxStatus->rateBit - indx*32));
        // check for unexpected rateBit
        if ((RateMaskReported[indx] | RateMask[indx]) != RateMask[indx])
        {
            UserPrint("Unexpected rateBit (%d) received from UTF. RateMask[%d] = 0x%08x\n", pTxStatus->rateBit, indx, RateMask[indx]);
            return 0;
        }

        TxStatusDump(pTxStatus);
        Ar6KLinkTxStatExtract(pTxStatus, RateIndx);
    }

	// check if all rates have been transmitted
    *txDone = 1;
	for (indx = 0; indx < RATE_MASK_ROW_MAX; indx++)
	{
		if (RateMaskReported[indx] !=  RateMask[indx])
		{
			*txDone = 0;
			break;
		}
    }
    return 1;
}

LINKDLLSPEC int Qc9KLinkTxDataStartCmd(int freq, int cenFreqUsed, int tpcm, int pcdac, double *txPower, int gainIndex, int dacGain,
                       int ht40, int *rate, int nrate, int ir,
                       unsigned char *bssid, unsigned char *source, unsigned char *destination,
                       int numDescPerRate, int *dataBodyLength,
                       int retries, int antenna, int broadcast, int ifs,
	                   int shortGi, int txchain, int naggregate,
	                   unsigned char *pattern, int npattern, int carrier, int bandwidth, int psatcal, int dutycycle)
{
    int it;
	static unsigned char bcast[]={0xff,0xff,0xff,0xff,0xff,0xff};
	unsigned char *duse;
    A_UINT32 rateMask, rateMaskMcs20, rateMaskMcs40;
    A_UINT32 vrateMaskMcs20, vrateMaskMcs40, vrateMaskMcs80;
    TX_DATA_START_PARAMS Params;

    //
    // remember some parameters for later use when we queue packets
    //
    TxChain=txchain;
    ShortGi=shortGi;
    Broadcast=broadcast;
    Retry=retries;
    LinkCarrier = carrier;
    PacketMany= numDescPerRate;
    //
    // adjust aggregate parameters to the range [1,MAGGREGATE]
    //
	if(naggregate<=1)
	{
		AggregateMany=1;
		AggregateBar=0;
	}
	else
	{
		AggregateMany=naggregate;
		if(AggregateMany>MAGGREGATE)
		{
			AggregateMany=MAGGREGATE;
		}
        //
        // determine if we should do special block acknowledgement request (BAR) packet
        //
        if(Broadcast)
        {
		    AggregateBar=0;
        }
        else
        {
		    AggregateBar=0;                 // never do this
        }
        //
        // treat the incoming number of packets as the total
        // but we treat it internally as the number of aggregate groups, so divide
        //
        if(PacketMany>0)
        {
            PacketMany/=AggregateMany;
            if(PacketMany<=0)
            {
                PacketMany=1;
            }
        }
        //
        // enforce maximun aggregate size of 64K
        //

      /*  if(AggregateMany*dataBodyLength[0]>65535)
        {
            AggregateMany=65535/dataBodyLength[0];
            UserPrint("reducing AggregateMany from %d to %d=65535/%d\n",naggregate,AggregateMany,dataBodyLength[0]);
        } */
	}
    //
	// Count rates and make the internal rate arrays.
	//
    RateMany = nrate;
    if (RateMany > RATE_POWER_MAX_INDEX)
    {
        UserPrint("WARNING - number of rates in tx commands excesses number of allowable rates in tx command\n");
        RateMany = RATE_POWER_MAX_INDEX;
    }
	for(it=0; it<RateMany; it++)
	{
		Rate[it]=rate[it];
	}
	//
	// Remember other parameters, for descriptor queueing.
	//
	if(PacketMany>0)
	{
        InterleaveRate=ir;
	}
	else
	{
		InterleaveRate=1;
	}
    //
    // If the user asked for an infinite number of packets, set to a million
    // and force interleave rates on.
    //
    if(PacketMany<=0)
    {
        //PacketMany=1000000;
        InterleaveRate=1;
    }
	//
	// if in tx100 mode, we only do one packet at the first rate
	//
	if(ifs==0)		
	{
		//
		// tx100
		//
		RateMany=1;
		PacketMany=1;
        //Tx100Packet = 1;
		Broadcast=1;
	}
	else if(ifs>0)
	{
		//
		// tx99
		//
        //Tx100Packet = 0;
		Broadcast=1;
	}
	else
	{
		//
		// regular
		//
        //Tx100Packet = 0;
	}
	
    RateMaskGet(&rateMask, &rateMaskMcs20, &rateMaskMcs40, Rate, RateMany);
	vRateMaskGet(&vrateMaskMcs20, &vrateMaskMcs40, &vrateMaskMcs80, Rate, RateMany);
    //
    // If broadcast use the special broadcast address. otherwise, use the specified receiver
    //
	if(Broadcast)
	{
		duse=bcast;
	}
	else
	{
		duse=destination;
	}
    
    //
    // now calculate the number of tx responses we expect
    //
    // Merlin and before return a response for every queued descriptor but only
    // the terminating descriptor for an aggregate has the done bit set. Skip the others in LinkTxComplete().
    //
    // Osprey only returns status for the terminating packet of aggregates, so there are
    // fewer status descriptors than control descriptors.
    //
    LinkTxMany=PacketMany*RateMany*(AggregateMany+AggregateBar);
    PacketSize = dataBodyLength[0];
    if(LinkTxAggregateStatus || AggregateMany<=1)
	{
		LinkTxStatusMany=LinkTxMany;
	}
	else
	{
		LinkTxStatusMany=0;
		for(it=0; it<RateMany; it++)
		{
			if(IS_LEGACY_RATE_INDEX(Rate[it]))
			{
				//
				// legacy rates don't support aggrgegates
				//
				LinkTxStatusMany+=(PacketMany*(AggregateMany+AggregateBar));
			}
			else
			{
				//
				// only 1 status message is returned for aggregates
				//
				LinkTxStatusMany+=(PacketMany*(1+AggregateBar));
			}
		}
	}
    UserPrint("TxMany=%d TxStatusMany=%d\n",LinkTxMany,LinkTxStatusMany);
    // Setup the tx parameters
	Params.dutycycle= dutycycle;
    Params.testCmdId = 0;
    Params.freq = freq;

    Params.antenna = antenna;
    Params.enANI = 0;       //TRANG disable for now
    Params.scramblerOff = 0;
    //Params.pktSz = dataBodyLength;
    Params.shortGuard = ShortGi;;
    Params.numPackets = PacketMany;
    Params.broadcast = Broadcast;
    memcpy(Params.bssid, bssid, 6);
    memcpy(Params.txStation, source, 6);
    memcpy(Params.rxStation, duse, 6);

	// Convert and set tpcm
	if (tpcm == TpcmForcedTargetPower)
	{
		Params.tpcm = TPC_FORCED_TGTPWR;
	}
	else if (tpcm == TpcmTxGainIndex || tpcm == TpcmDACGain)
	{
		Params.tpcm = TPC_FORCED_GAINIDX;
	}
	else if (tpcm == TpcmTxPower)
	{
		Params.tpcm = TPC_TX_PWR;
	}
	else if (tpcm == TpcmTxGain)
	{
		Params.tpcm = TPC_FORCED_GAIN;
	}
	else //if (tpcm == TpcmTargetPower)
	{
		Params.tpcm = TPC_TGT_PWR;
	}

	Params.gainIdx = gainIndex;
	Params.dacGain = dacGain;

    Params.retries = Retry;
	Params.miscFlags = TX_STATUS_PER_RATE_MASK | PROCESS_RATE_IN_ORDER_MASK;
    if (Params.tpcm == TPC_FORCED_GAIN) /* If using PCDAC, code always disabled PAPRD */
        Params.miscFlags |= ((DeviceStbcGet() << DESC_STBC_ENA_SHIFT) | (DeviceLdpcGet() << DESC_LDPC_ENA_SHIFT));
    else 
        Params.miscFlags |= ((DeviceStbcGet() << DESC_STBC_ENA_SHIFT) | (DeviceLdpcGet() << DESC_LDPC_ENA_SHIFT) | (DevicePapdGet() << PAPRD_ENA_SHIFT));
    
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
        default:    // in case the bandwidth is invalid, derive wlanMode from rate masks
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
    Params.aifsn = ifs;
    if (ifs == 1)
    {
        Params.mode = TCMD_CONT_TX_TX99;
    }
    else if (ifs == 0)
    {
        Params.mode = TCMD_CONT_TX_TX100;
    }
    else
    {
        Params.mode = TCMD_CONT_TX_FRAME;
        Params.aifsn = (ifs > 1) ? ifs : 1; // set aifsn = 1 fpr now if isf < 0
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
    if (npattern <= 0)
    {
        Params.txPattern = PN9_PATTERN;
        Params.nPattern = 0;
    }
    else
    {
        Params.txPattern = USER_DEFINED_PATTERN;
        Params.nPattern = npattern;
        memcpy(Params.dataPattern, pattern, npattern); // bytes to be written 
    }
    memset (RateMask, 0, sizeof(RateMask));
    RateMask2UtfRateMask(rateMask, rateMaskMcs20, rateMaskMcs40, RateMask);
	vRateMask2UtfRateMask(vrateMaskMcs20, vrateMaskMcs40, vrateMaskMcs80, RateMask);

    for (it = 0; it < RATE_POWER_MAX_INDEX; it++)
    {
        if (it < RateMany)
        {
			Params.rateMaskBitPosition[it] = vRateIndx2UtfRateBit(Rate[it]);
			if (Params.tpcm == TPC_FORCED_GAIN)
			{
				Params.txPower[it] = pcdac;
			}
			else if (Params.tpcm == TPC_TGT_PWR)
			{
				Params.txPower[it] = -101;
			}
			else
			{
				Params.txPower[it] = (int)(txPower[it]*2);
			}
			Params.pktLength[it] = dataBodyLength[it];
        }
        else
        {
            // To indicate no more rate
            Params.rateMaskBitPosition[it] = 0xff;
            Params.txPower[it] = 0xff;
            Params.pktLength[it] = 0;
        }
    }
    Params.ir = InterleaveRate;
    Params.txChain = (TxChain == 0) ? (DeviceTxChainMask()) : TxChain;
    Params.agg = AggregateMany;
    //Params.timeout = 30000;
    //Params.remoteStats = NO_REMOTE_STATS;
    
    if (LinkCarrier)
    {
        Params.mode = TCMD_CONT_TX_SINE;
		if (Params.tpcm != TPC_FORCED_GAINIDX)
		{
			Params.tpcm = TPC_FORCED_GAIN;
			Params.txPwr = pcdac;
		}
     
        Params.wlanMode = TCMD_WLAN_MODE_NOHT;
    }
    // send tx command
    DeviceTransmitDataDut(&Params);

    //TxStatusCheck();
    /* waiting for PAPRD training finish */
	//TRANG - comment out PAPD
    //if (DevicePapdGet() && (Params.tpcm != TPC_FORCED_GAIN))
    //{
    //    DevicePapdIsDone(Params.txPwr);
    //}
    UserPrint("TX_DATA_START_CMD sent\n");

    //TxStatusCheck();

	return 0;
}

LINKDLLSPEC int Qc9KLinkTxComplete(int timeout, int (*ison)(), int (*done)(), int chipTemperature, int calibrate)
{
	int it;
    unsigned int startTime, endTime, ctime, failTime;
	int failRetry;
    int pass;
	int temperature;
	int doTemp;
    unsigned char *pTxStatusBuf;
    unsigned int next_check_tx_status_time = 0; 
    int txDone;
	int actTimeout = timeout;

    LinkTxStatClear();
    pass=0;
	doTemp=0;
    txDone = 0;
    pTxStatusBuf = NULL;

	//UserPrint("calibrate %d\n", calibrate);
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
		actTimeout=60*60*1000;         // one hour
	}
	
	endTime=startTime + actTimeout; 

	if(Tx100Packet || timeout == -1)
	{
		failTime=endTime;
	}
	else
	{
		failTime=startTime+(5*1000);	// 5 seconds
	}
	failRetry=0;
	//
	// if this is the first descriptor, announce that the transmitter is really on
	//
	if(ison!=0)
	{
		(*ison)();
	}
	//
    // loop looking for status reports from UTF
	// terminate either timeout, receiving stop from cart, or tx done from UTF
	//
    for(it=0; ; it++) 
    {
		//if(doTemp>10)
		//{
		//	temperature=DeviceTemperatureGet(0, __FUNCTION__);
		//	//
		//	// are we supposed to terminate on reaching a certain chip temperature?
		//	//
		//	if(chipTemperature>0)
		//	{
		//		if(temperature<=chipTemperature)
		//		{
		//			UserPrint(" %d Temperature %d <= %d",it,temperature,chipTemperature);
		//			break;
		//		}
		//	}
		//	doTemp=0;
		//}
		//doTemp++;
		//
		// check for message from cart telling us to stop
		// this is the normal terminating condition
		//
		pass++;
		if(pass>100)
		{
            //
            // Check tx packets status each 1 second
            //                       
            if ((!LinkCarrier) && ((next_check_tx_status_time == 0) || ((unsigned int)TimeMillisecond()> next_check_tx_status_time)))
            {
                next_check_tx_status_time = TimeMillisecond()+ NextWaitingTxStatusTimeMiliSec();
                pTxStatusBuf = NULL;
                if (DeviceTransmitStatusReport((void **)&pTxStatusBuf, 0) != 0)
                {
                    UserPrint("Error in DeviceTransmitStatusReport\n");
                    break;
                }
                else if (pTxStatusBuf)
                {
                    ProcessTxStatusReport(pTxStatusBuf, &txDone);
                    if (txDone)
                    { 
                        UserPrint(" %d tx Done received",it);
                        break;
                    }
                }
            }

			if(done!=0)
			{
				if((*done)())
				{
					UserPrint(" %d Stop",it);
    				break;
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
			    UserPrint(" %d Timeout",it);
			    break;
		    }
			//
			// check for failure to start
			//
			if(it<=0)
			{
				if((failTime > startTime && 
					(ctime > failTime || ctime < startTime)) || 
					(failTime < startTime && ctime > failTime && ctime < startTime))
				{
					UserPrint("Failed to start transmit.\n");
					if(failRetry==0)
					{
						UserPrint("Trying to restart.\n");
						//TRANG How to restart???? LinkTxStart();
						failTime=startTime+(5*1000);	// 5 seconds
						failRetry++;
						//
						// set pointer to first descriptor
						//
						//mDescriptor = LinkTxLoopFirst();
					}
					else
					{
						break;
					}
				}
			}
		}
    } 
    if (txDone == 0)
    {
        // this happens when timeout or "stop" from cart
        pTxStatusBuf = NULL;
	    if (DeviceTransmitStop((void **)&pTxStatusBuf, calibrate) != 0)
        {
            UserPrint("Error in DeviceTransmitStop\n");
        }
        else if (pTxStatusBuf) 
        {
            ProcessTxStatusReport(pTxStatusBuf, &txDone);
        }
    } 
    //
    // finish throughput calculation
    //
    LinkTxStatFinish();
    UserPrint(" %d Done\n",it);
    return 0;
}

