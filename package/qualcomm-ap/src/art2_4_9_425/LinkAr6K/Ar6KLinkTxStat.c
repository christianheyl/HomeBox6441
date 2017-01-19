
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "wlanproto.h"

#include "art_utf_common.h"
#include "LinkStat.h"

//static int RateLast = -1;		// keep track of last rate for debug UserPrint
//static int RateInterleave=0;
//static int RateCount;			// count the number at RateLast
//static int RateFlip;			// count the number of times we've only gotten one at a certain rate
								// if this happens a lot (>10 times), we turn on RateInterleave
//static int AggSize=0;
//static int AggCount=0;
static struct txStats _LinkTxStatTotal;
static struct txStats _LinkTxStat[MRATE];
static int currentRate;

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


void LinkTxStatFinish()
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
        _LinkTxStat[it].newThroughput=ThroughputCalculate(
			_LinkTxStat[it].byteCount-_LinkTxStat[it].dontCount,_LinkTxStat[it].startTime,_LinkTxStat[it].endTime);
 	}
	//
	// calculate total throughput
	//
    _LinkTxStatTotal.newThroughput=ThroughputCalculate(
		_LinkTxStatTotal.byteCount-_LinkTxStatTotal.dontCount,_LinkTxStatTotal.startTime,_LinkTxStatTotal.endTime);

	//PacketLogDump();
}

void LinkTxStatClear()
{
    memset(&_LinkTxStatTotal, 0, sizeof(_LinkTxStatTotal));
    memset(&_LinkTxStat, 0, sizeof(_LinkTxStat));
	//AggCount=0;
	//AggSize=0;
	//RateLast= -1;
	//RateInterleave=0;
	//RateCount=0;
	//RateFlip=0;
}

static void HistogramInsert(int *histogram, int max, int value)
{
	if(value>=0 && value<max)
	{
        histogram[value]++;
	}
}

struct txStats *Ar6KLinkTxStatFetch(int rate)
{
    if(rate>=0 && rate<MRATE)
    {
        return &_LinkTxStat[rate];
    }
    return 0;
}

void Ar6KLinkTxStatExtract(TX_STATS_STRUCT_UTF *txStatus_utf, int rate)
{
    int i;
    
    _LinkTxStatTotal.goodPackets += txStatus_utf->goodPackets;
    if (txStatus_utf->goodPackets > 0)
    {
        _LinkTxStatTotal.byteCount += txStatus_utf->byteCount;
    }
    if(_LinkTxStatTotal.startTime == 0)
    {
        _LinkTxStatTotal.startTime = txStatus_utf->startTime;
    }
    _LinkTxStatTotal.endTime = txStatus_utf->endTime;
    _LinkTxStatTotal.underruns += txStatus_utf->underruns;
    _LinkTxStatTotal.excessiveRetries += txStatus_utf->excessiveRetries;
    _LinkTxStatTotal.otherError += txStatus_utf->otherError;
    _LinkTxStatTotal.dontCount += txStatus_utf->dontCount;

    _LinkTxStat[rate].startTime = txStatus_utf->startTime;
    _LinkTxStat[rate].endTime = txStatus_utf->endTime;
    _LinkTxStat[rate].byteCount = txStatus_utf->byteCount;
    _LinkTxStat[rate].dontCount = txStatus_utf->dontCount;
    _LinkTxStat[rate].goodPackets = txStatus_utf->goodPackets;
    _LinkTxStat[rate].underruns = txStatus_utf->underruns;
    _LinkTxStat[rate].otherError = txStatus_utf->otherError;
    _LinkTxStat[rate].excessiveRetries = txStatus_utf->excessiveRetries;
    if (txStatus_utf->thermCal != 0)
        _LinkTxStat[rate].temperature = txStatus_utf->thermCal;
    else 
        _LinkTxStat[rate].temperature = DeviceTemperatureGet(1);
    currentRate = rate;
    
    HistogramInsert(_LinkTxStat[rate].shortRetry,MRETRY,txStatus_utf->shortRetry);
    HistogramInsert(_LinkTxStat[rate].longRetry,MRETRY,txStatus_utf->longRetry);
    HistogramInsert(_LinkTxStat[rate].rssi,MRSSI,txStatus_utf->rssi);
    
    for (i=0; i<MCHAIN_UTF; i++)
    {
        HistogramInsert(_LinkTxStat[rate].rssic[i],MRSSI,txStatus_utf->rssic[i]);
        HistogramInsert(_LinkTxStat[rate].rssie[i],MRSSI,txStatus_utf->rssie[i]);
    }

    return;
}

int Ar6KLinkTxStatTemperatureGet()
{
    return _LinkTxStat[currentRate].temperature;
}
