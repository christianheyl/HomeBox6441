

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//#include "ah.h"
//#include "ah_osdep.h"

#include "wlanproto.h"


//#include "NewArt.h"
//#include "ParameterSelect.h"
//#include "Card.h"
#include "LinkStat.h"
#include "DescriptorLinkRx.h"
#include "DescriptorLinkRxStat.h"
#ifdef UNUSED
#include "LinkRxCompare.h"
#endif
//#include "DescriptorMemory.h"
#include "RxDescriptor.h"
#include "Device.h"
#include "TimeMillisecond.h"
#include "UserPrint.h"
// LATERLINK #include "PacketLog.h"


#define MBUFFER 1024
#define MDESCRIPTOR 30

#define MRATE 100

#define MINRSSI (-10)

static struct rxStats _LinkRxStatTotal;
static struct rxStats _LinkRxStat[MRATE];

static int RateLast = -1;		// keep track of last rate for debug UserPrint
static int RateInterleave=0;
static int RateCount;			// count the number at RateLast
static int RateFlip;			// count the number of times we've only gotten one at a certain rate
								// if this happens a lot (>10 times), we turn on RateInterleave


static int ThroughputCalculate(int nbyte, int start, int end)
{
	int duration;
	int throughput;
    
    duration=end-start;	
    if(duration>0 && nbyte>0) 
	{
        throughput = (int)(((double)(nbyte*8))/(0.001*duration));
    }
	else
	{
        throughput = 0;
	}
	return throughput;
}


void LinkRxStatFinish()
{
	int it;
	//
    // finish stats computation, divide by denominator for averages, etc.
    //
    for(it=0; it<MRATE; it++) 
	{
		//
		// calculate throughput for each rate
		//
        _LinkRxStat[it].rxThroughPut=ThroughputCalculate(
			_LinkRxStat[it].byteCount-_LinkRxStat[it].dontCount,_LinkRxStat[it].startTime,_LinkRxStat[it].endTime);
#ifdef UNUSED
        if(_LinkRxStat[it].byteCount!=0)
        {
            UserPrint("%2d: %d=(%d-%d)/(%d-%d)\n",it,_LinkRxStat[it].rxThroughPut,
			    _LinkRxStat[it].byteCount,_LinkRxStat[it].dontCount,_LinkRxStat[it].startTime,_LinkRxStat[it].endTime);
        }
#endif
 
 	}
	//
	// calculate total throughput
	//
    _LinkRxStatTotal.rxThroughPut=ThroughputCalculate(
		_LinkRxStatTotal.byteCount-_LinkRxStatTotal.dontCount,_LinkRxStatTotal.startTime,_LinkRxStatTotal.endTime);

// LATERLINK 	PacketLogDump();
}


struct rxStats *DescriptorLinkRxStatTotalFetch()
{
		return &_LinkRxStatTotal;
}


struct rxStats *DescriptorLinkRxStatFetch(int rate)
{
	if(rate>=0 && rate<MRATE)
	{
		return &_LinkRxStat[rate];
	}
	return 0;
}


void LinkRxStatClear()
{
	int rate;

	RateLast= -1;
	RateInterleave=0;
	RateCount=0;
	RateFlip=0;
    memset(&_LinkRxStatTotal, 0, sizeof(_LinkRxStatTotal));
	_LinkRxStatTotal.rssimin=MINRSSI;
    memset(&_LinkRxStat, 0, sizeof(_LinkRxStat));
	for(rate=0; rate<MRATE; rate++)
	{
		_LinkRxStat[rate].rssimin=MINRSSI;
	}
}


static void HistogramInsert(int *histogram, int min, int max, int value)
{
	if(value>=min && value<max)
	{
        histogram[value-min]++;
	}
}


static void histogramAdd(int *dest, int *source, int length)
{
    int i;
    for (i = 0; i < length; i++)
    {
        dest[i]+=source[i];
    }
}

static void copyLinkRxStatEntriesandZero(int destIdx, int sourceIdx)
{
	int i;
    //
	// rssi histogram for good packets
	//
	histogramAdd(_LinkRxStat[destIdx].rssi,_LinkRxStat[sourceIdx].rssi,MRSSI);
    for(i = 0; i < MCHAIN;i++) 
    {
	    histogramAdd(_LinkRxStat[destIdx].rssic[i],_LinkRxStat[sourceIdx].rssic[i],MRSSI);
	    histogramAdd(_LinkRxStat[destIdx].rssie[i],_LinkRxStat[sourceIdx].rssie[i],MRSSI);
    }
	histogramAdd(_LinkRxStat[destIdx].badrssi,_LinkRxStat[sourceIdx].badrssi,MRSSI);
    for(i = 0; i < MCHAIN;i++) 
    {
	    histogramAdd(_LinkRxStat[destIdx].badrssic[i],_LinkRxStat[sourceIdx].badrssic[i],MRSSI);
	    histogramAdd(_LinkRxStat[destIdx].badrssie[i],_LinkRxStat[sourceIdx].badrssie[i],MRSSI);
    }
    for(i = 0; i < MSTREAM;i++) 
    {
	    histogramAdd(_LinkRxStat[destIdx].evm[i],_LinkRxStat[sourceIdx].evm[i],MRSSI);
	    histogramAdd(_LinkRxStat[destIdx].badevm[i],_LinkRxStat[sourceIdx].badevm[i],MRSSI);
    }


    _LinkRxStat[destIdx].goodPackets+=_LinkRxStat[sourceIdx].goodPackets;
    _LinkRxStat[destIdx].otherError+=_LinkRxStat[sourceIdx].otherError;
    _LinkRxStat[destIdx].crcPackets+=_LinkRxStat[sourceIdx].crcPackets;
    _LinkRxStat[destIdx].decrypErrors+=_LinkRxStat[sourceIdx].decrypErrors;
    _LinkRxStat[destIdx].byteCount+=_LinkRxStat[sourceIdx].byteCount;
    if(_LinkRxStat[destIdx].startTime ==0) {
        _LinkRxStat[destIdx].dontCount+=_LinkRxStat[sourceIdx].dontCount;
    }

    memset(&_LinkRxStat[sourceIdx], 0, sizeof(_LinkRxStat[sourceIdx]));
}




#define RATE_UNKNOWN 31
void LinkRxStatExtract(unsigned int *descriptor, int np)
{
//    int isDuplicate;
	int rssic[MCHAIN], rssie[MCHAIN], rssi;
	int evm[MSTREAM];
	int it;
	int plength;
	int timestamp;
	int rate;
	char tempBuffer[MBUFFER];
    int diff;
    static int ltimestamp;
    static int aggcount=0;
    static int agglength=0;

    //
	// skip anything that has the more bit set?
	//
	if(RxDescriptorMore(descriptor))
	{
		return;
	}

    if(!RxDescriptorAggregate(descriptor) || !RxDescriptorMoreAgg(descriptor))
	{

        //
	    // figure out rate
	    //
	    rate=RxDescriptorRxRate(descriptor);
	    if(rate==20 || rate>=MRATE) {
		    RxDescriptorPrint(descriptor, tempBuffer, MBUFFER);
		    UserPrint("\nbad: %s\n", tempBuffer);
		    return;
	    }

        //
        // add in unknown rate information
        //
        copyLinkRxStatEntriesandZero(rate, RATE_UNKNOWN);

        //
        // on first packet save start time for throughput calculation
        // don't look at anything except the last aggregate because the time stamp is not set correctly in the others
        //
        timestamp=RxDescriptorRcvTimestamp(descriptor);
        //
        // check that times are increasing
        //
        diff=timestamp-ltimestamp;
        if(diff<0)
        {
            UserPrint("timestamp: %d %d=%d-%d\n",np,diff,timestamp,ltimestamp);
        }
        ltimestamp=timestamp;
        //
        // record start and end times for throughput calculation
        //
	    if(_LinkRxStatTotal.startTime == 0 )
	    {
		    _LinkRxStatTotal.startTime = timestamp;
	    }
	    if(_LinkRxStat[rate].startTime == 0 )
	    {
		    _LinkRxStat[rate].startTime = timestamp;
	    }
	    //
	    // always save end time for throughput calculation
	    //
	    _LinkRxStatTotal.endTime = timestamp; 
	    _LinkRxStat[rate].endTime = timestamp;
        //
	    // this code figures out if the incoming packets have interleaved rates
	    // this is solely for debug purposes
	    //
	    if(RateInterleave==0 && rate!=RateLast)
	    {
		    if(RateCount==0)
		    {
			    RateFlip++;
			    if(RateFlip>5)
			    {
				    RateInterleave=1;
				    UserPrint("[ir]");
			    }
		    }
		    else
		    {
			    RateFlip--;
		    }
		    UserPrint("[%d]",rate);
		    RateLast=rate;
		    RateCount=0;
	    }
	    else
	    {
		    RateCount++;
	    }
    } 
    else
    {
        rate = RATE_UNKNOWN;
    }

    //
	// extract rssi and evm measurements. we need these for both good and bad packets
	//
    rssi=RxDescriptorRssiCombined(descriptor);
    rssic[0]=RxDescriptorRssiAnt00(descriptor);
    rssic[1]=RxDescriptorRssiAnt01(descriptor);
    rssic[2]=RxDescriptorRssiAnt02(descriptor);
    rssie[0]=RxDescriptorRssiAnt10(descriptor);
    rssie[1]=RxDescriptorRssiAnt11(descriptor);
    rssie[2]=RxDescriptorRssiAnt12(descriptor);
	evm[0]=(int)(0.5+RxDescriptorEvm0(descriptor));
	evm[1]=(int)(0.5+RxDescriptorEvm1(descriptor));
	evm[2]=(int)(0.5+RxDescriptorEvm2(descriptor));

    //
	// check for good packets 
	//
    if(RxDescriptorFrameRxOk(descriptor)) //pStatsInfo->status2 & DESC_FRM_RCV_OK)
    {
// LATERLINK 		PacketLog(timestamp,rate,0,np);
		//UserPrint("O");
		//
		// record packet length
		//
		plength=RxDescriptorDataLen(descriptor);	//(pStatsInfo->status1 & 0xfff);
        //
        // don't count header or check sum
        //
        if(RxDescriptorAggregate(descriptor))
        {
            plength-=(sizeof(WLAN_QOS_DATA_MAC_HEADER3));
        }
        else
        {
            plength-=(sizeof(WLAN_DATA_MAC_HEADER3));
        }
        _LinkRxStatTotal.byteCount += (plength);
		_LinkRxStat[rate].byteCount += (plength);
        //
        // don't count packets that come in before a valid timestamp in the throughput calculation
        //
        if(_LinkRxStatTotal.startTime==0)
        {
            _LinkRxStatTotal.dontCount+=(plength);
        }
        if(_LinkRxStat[rate].startTime==0)
        {
            _LinkRxStat[rate].dontCount+=(plength);
        }
        //
		// count number of good packets
		//
        _LinkRxStatTotal.goodPackets+=1;
        _LinkRxStat[rate].goodPackets+=1;
		//
		// accumulate rssi histograms
		//
        HistogramInsert(_LinkRxStatTotal.rssi,MINRSSI,MRSSI+MINRSSI,rssi);
        HistogramInsert(_LinkRxStat[rate].rssi,MINRSSI,MRSSI+MINRSSI,rssi);
		for(it=0; it<MCHAIN; it++)
		{
            HistogramInsert(_LinkRxStatTotal.rssic[it],MINRSSI,MRSSI+MINRSSI,rssic[it]);
            HistogramInsert(_LinkRxStat[rate].rssic[it],MINRSSI,MRSSI+MINRSSI,rssic[it]);
            HistogramInsert(_LinkRxStatTotal.rssie[it],MINRSSI,MRSSI+MINRSSI,rssie[it]);
            HistogramInsert(_LinkRxStat[rate].rssie[it],MINRSSI,MRSSI+MINRSSI,rssie[it]);
		}
		//
		// Accumulate evm histograms
		//
		for(it=0; it<MSTREAM; it++)
		{
            HistogramInsert(_LinkRxStatTotal.evm[it],0,MEVM,evm[it]);
            HistogramInsert(_LinkRxStat[rate].evm[it],0,MEVM,evm[it]);
		}
#ifdef UNUSED
		//
		// ppm
		//
        extractPPM(devNum, pStatsInfo);
#endif
    } 
	else 
	{
		//
		// count crc errors
		//
        if(RxDescriptorCrcError(descriptor))	//pStatsInfo->status2 & DESC_CRC_ERR) 
		{
// LATERLINK 			PacketLog(timestamp,rate,1,np);
            _LinkRxStatTotal.crcPackets++;
            _LinkRxStat[rate].crcPackets++;
        } 
		//
		// count decryption errors
		//
		else if(RxDescriptorDecryptCrcErr(descriptor))	//pStatsInfo->status2 & pLibDev->decryptErrMsk) 
		{
// LATERLINK			PacketLog(timestamp,rate,2,np);
            _LinkRxStatTotal.decrypErrors++;
            _LinkRxStat[rate].decrypErrors++;
        }
		//
		// some other error
		//
		else 
		{
// LATERLINK			PacketLog(timestamp,rate,3,np);
            _LinkRxStatTotal.otherError++;
            _LinkRxStat[rate].otherError++;
        }
		//
		// accumulate rssi histograms for bad packets
		//
        HistogramInsert(_LinkRxStatTotal.badrssi,MINRSSI,MRSSI+MINRSSI,rssi);
        HistogramInsert(_LinkRxStat[rate].badrssi,MINRSSI,MRSSI+MINRSSI,rssi);
		for(it=0; it<MCHAIN; it++)
		{
            HistogramInsert(_LinkRxStatTotal.badrssic[it],MINRSSI,MRSSI+MINRSSI,rssic[it]);
            HistogramInsert(_LinkRxStat[rate].badrssic[it],MINRSSI,MRSSI+MINRSSI,rssic[it]);
            HistogramInsert(_LinkRxStatTotal.badrssie[it],MINRSSI,MRSSI+MINRSSI,rssie[it]);
            HistogramInsert(_LinkRxStat[rate].badrssie[it],MINRSSI,MRSSI+MINRSSI,rssie[it]);
		}
		//
		// Accumulate evm histograms for bad packets
		//
		for(it=0; it<MSTREAM; it++)
		{
            HistogramInsert(_LinkRxStatTotal.badevm[it],0,MEVM,evm[it]);
            HistogramInsert(_LinkRxStat[rate].badevm[it],0,MEVM,evm[it]);
		}
	}

    return;
}

void LinkRxStatSpectralScanExtract(unsigned int *descriptor, int np)
{
	int rssic[MCHAIN], rssie[MCHAIN], rssi;
	int it;
	int rate;
    static int ltimestamp;
    static int aggcount=0;
    static int agglength=0;

    //
    // figure out rate
    //
    rate=20;				// RxDescriptorRxRate(descriptor);
    //
	// extract rssi and evm measurements. we need these for both good and bad packets
	//
    rssi=RxDescriptorRssiCombined(descriptor);
	if(rssi&0x80)
	{
		rssi|=0xffffff00;
	}
    rssic[0]=RxDescriptorRssiAnt00(descriptor);
    rssic[1]=RxDescriptorRssiAnt01(descriptor);
    rssic[2]=RxDescriptorRssiAnt02(descriptor);
    rssie[0]=RxDescriptorRssiAnt10(descriptor);
    rssie[1]=RxDescriptorRssiAnt11(descriptor);
    rssie[2]=RxDescriptorRssiAnt12(descriptor);
	for(it=0; it<3; it++)
	{
		if(rssic[it]&0x80)
		{
			rssic[it]|=0xffffff00;
		}
		if(rssie[it]&0x80)
		{
			rssie[it]|=0xffffff00;
		}
	}
    //
	// count number of good packets
	//
    _LinkRxStat[rate].goodPackets+=1;
	//
	// accumulate rssi histograms
	//
    HistogramInsert(_LinkRxStat[rate].rssi,MINRSSI,MRSSI+MINRSSI,rssi);
	for(it=0; it<MCHAIN; it++)
	{
        HistogramInsert(_LinkRxStat[rate].rssic[it],MINRSSI,MRSSI+MINRSSI,rssic[it]);
        HistogramInsert(_LinkRxStat[rate].rssie[it],MINRSSI,MRSSI+MINRSSI,rssie[it]);
	}
    return;
}


#ifdef UNUSED
/**************************************************************************
* extractPPM() - Pull PPM value from the packet body
*
*/
void extractPPM(A_UINT32 devNum, RX_STATS_TEMP_INFO *pStatsInfo)
{
    LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
    A_INT32 ppm;
    double f_ppm;
    double scale_factor;

	pStatsInfo=0;
	
            /* Pull PPM from registers */
            ppm = REGR(devNum, 0x9CF4) & 0xFFF;
            // Sign extend ppm
            if ((ppm >> 11) == 1) 
			{
                ppm = ppm | 0xfffff000;
            }

        // scale ppm to parts per million
        scale_factor = pLibDev->freqForResetDevice * 3.2768 / 1000;
        //scale ppm appropriately for turbo, half and quarter modes
        if (pLibDev->turbo == TURBO_ENABLE) {
            scale_factor = scale_factor / 2;
        } else if (pLibDev->turbo == HALF_SPEED_MODE) {
            scale_factor = scale_factor * 2;
        } else if (pLibDev->turbo == QUARTER_SPEED_MODE) {
            scale_factor = scale_factor * 4;
        }
        f_ppm = ppm/scale_factor;
//        ppm = (A_INT32)(ppm /scale_factor);

//		UserPrint("ppm=%d sf=%lg fppm=%lg\n",ppm,scale_factor,f_ppm);

#ifdef UNUSED

        pStatsInfo->ppmAccum[0] += f_ppm;
        pStatsInfo->ppmSamples[0]++;
        _LinkRxStatTotal.ppmAvg = (A_INT32)(pStatsInfo->ppmAccum[0] /
            pStatsInfo->ppmSamples[0]);

        /* max and min */
        if(ppm > _LinkRxStatTotal.ppmMax) {
            _LinkRxStatTotal.ppmMax = ppm;
        }
        if (ppm < _LinkRxStatTotal.ppmMin) {
            _LinkRxStatTotal.ppmMin = ppm;
        }

        pStatsInfo->ppmAccum[rate] += f_ppm;
        pStatsInfo->ppmSamples[rate]++;
        _LinkRxStat[rate].ppmAvg = (A_INT32)(pStatsInfo->ppmAccum[rate] /
            pStatsInfo->ppmSamples[rate]);

        /* max and min */
        if (ppm > _LinkRxStat[rate].ppmMax) {
            _LinkRxStat[rate].ppmMax = ppm;
        }
        if (ppm < _LinkRxStat[rate].ppmMin) {
            _LinkRxStat[rate].ppmMin = ppm;
        }
        return;
    }
#endif
}
#endif


