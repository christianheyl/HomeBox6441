

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//#include "ah.h"
//#include "ah_osdep.h"

#include "wlanproto.h"


#include "NewArt.h"
#include "ParameterSelect.h"
#include "Card.h"
#include "LinkStat.h"
#include "LinkRx.h"
#include "art_utf_common.h"
#include "LinkRxStat.h"
#include "RxDescriptor.h"
#include "Device.h"


#define MBUFFER 1024

static struct rxStats _LinkRxStatTotal;
static struct rxStats _LinkRxStat[MRATE];

static int RateLast = -1;		// keep track of last rate for debug UserPrint
static int RateInterleave=0;
static int RateCount;			// count the number at RateLast
static int RateFlip;			// count the number of times we've only gotten one at a certain rate
								// if this happens a lot (>10 times), we turn on RateInterleave
#ifndef QDART_BUILD
extern struct rxSpurStats _LinkRxSpurStat;
extern int _LinkRxFindSpur;
#endif

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
 
 	}
	//
	// calculate total throughput
	//
    _LinkRxStatTotal.rxThroughPut=ThroughputCalculate(
		_LinkRxStatTotal.byteCount-_LinkRxStatTotal.dontCount,_LinkRxStatTotal.startTime,_LinkRxStatTotal.endTime);

	//PacketLogDump();
}

void LinkRxStatClear()
{
	RateLast= -1;
	RateInterleave=0;
	RateCount=0;
	RateFlip=0;
    memset(&_LinkRxStatTotal, 0, sizeof(_LinkRxStatTotal));
    memset(&_LinkRxStat, 0, sizeof(_LinkRxStat));
}

static void HistogramInsert(int *histogram, int max, int value)
{
	if(value>=0 && value<max)
	{
        histogram[value]++;
	}
}

struct rxStats *Ar6KLinkRxStatFetch(int rate)
{
#ifndef QDART_BUILD
    if(_LinkRxFindSpur)
        return (struct rxStats *)(&_LinkRxSpurStat);
#endif

    if(rate>=0 && rate<MRATE)
    {
        return &_LinkRxStat[rate];
    }
    return 0;
}

void LinkRxStatExtractDut(void *pRxStatus, int rate)
{
    int i;
    RX_STATS_STRUCT_UTF *rxStatus = (RX_STATS_STRUCT_UTF *)pRxStatus;

    _LinkRxStatTotal.goodPackets += rxStatus->goodPackets;
    if (rxStatus->goodPackets > 0)
    {
        _LinkRxStatTotal.byteCount += rxStatus->byteCount;
    }
    if(_LinkRxStatTotal.startTime == 0)
    {
        _LinkRxStatTotal.startTime = rxStatus->startTime;
    }
    if (rxStatus->endTime > (A_UINT32)_LinkRxStatTotal.endTime)
    {
        _LinkRxStatTotal.endTime = rxStatus->endTime;
    }

    _LinkRxStatTotal.dontCount += rxStatus->dontCount;

    _LinkRxStat[rate].startTime = rxStatus->startTime;
    _LinkRxStat[rate].endTime = rxStatus->endTime;
    _LinkRxStat[rate].goodPackets = rxStatus->goodPackets;
    _LinkRxStat[rate].otherError = rxStatus->otherError;
    _LinkRxStat[rate].crcPackets = rxStatus->crcPackets;
    _LinkRxStat[rate].decrypErrors = rxStatus->decrypErrors;
    _LinkRxStat[rate].byteCount = rxStatus->byteCount;
    _LinkRxStat[rate].dontCount = rxStatus->dontCount;

	//
	// accumulate rssi and evm histograms for good packets
	//
    HistogramInsert(_LinkRxStatTotal.rssi,MRSSI,rxStatus->rssi);
    HistogramInsert(_LinkRxStat[rate].rssi,MRSSI,rxStatus->rssi);
    for(i=0; i<MCHAIN_UTF; i++)
	{
        HistogramInsert(_LinkRxStatTotal.rssic[i],MRSSI,rxStatus->rssic[i]);
        HistogramInsert(_LinkRxStat[rate].rssic[i],MRSSI,rxStatus->rssic[i]);
        HistogramInsert(_LinkRxStatTotal.rssie[i],MRSSI,rxStatus->rssie[i]);
        HistogramInsert(_LinkRxStat[rate].rssie[i],MRSSI,rxStatus->rssie[i]);
    }
    for (i=0; i<MSTREAM_UTF; i++)
    {
        HistogramInsert(_LinkRxStatTotal.evm[i],MEVM,rxStatus->evm[i]);
        HistogramInsert(_LinkRxStat[rate].evm[i],MEVM,rxStatus->evm[i]);
    }
    //
	// Accumulate rssi and evm histograms for bad packets
	//
    HistogramInsert(_LinkRxStatTotal.badrssi,MRSSI,rxStatus->badrssi);
    HistogramInsert(_LinkRxStat[rate].badrssi,MRSSI,rxStatus->badrssi);
    for (i=0; i<MSTREAM_UTF; i++)
    {   
        HistogramInsert(_LinkRxStatTotal.badrssic[i],MRSSI,rxStatus->badrssic[i]);
        HistogramInsert(_LinkRxStat[rate].badrssic[i],MRSSI,rxStatus->badrssic[i]);
        HistogramInsert(_LinkRxStatTotal.badrssie[i],MRSSI,rxStatus->badrssie[i]);
        HistogramInsert(_LinkRxStat[rate].badrssie[i],MRSSI,rxStatus->badrssie[i]);
    }    
    for (i=0; i<MCHAIN_UTF; i++)
    {
        HistogramInsert(_LinkRxStatTotal.badevm[i],MEVM,rxStatus->badevm[i]);
        HistogramInsert(_LinkRxStat[rate].badevm[i],MEVM,rxStatus->badevm[i]);
    }
   
    _LinkRxStat[rate].bitErrorCompares = rxStatus->bitErrorCompares;
    return;
}


