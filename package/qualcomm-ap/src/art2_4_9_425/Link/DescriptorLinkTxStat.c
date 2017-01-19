
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


//#define OSPREY 1

#include "wlanproto.h"
//#include "rate_constants.h"

#include "smatch.h"
#include "TimeMillisecond.h"
#include "CommandParse.h"

#include "NewArt.h"
#include "ParameterSelect.h"
#include "Card.h"
//#include "DescriptorMemory.h"
#include "RxDescriptor.h"
#include "TxDescriptor.h"
#ifdef OSPREY
//#include "Ar9300TxDescriptor.h"
#endif

// LATERLINK #include "PacketLog.h"

#define MBUFFER 1024


#include "UserPrint.h"

#include "TimeMillisecond.h"
#include "Device.h"

#include "LinkStat.h"
#include "DescriptorLinkTx.h"
#include "DescriptorLinkTxPacket.h"
#include "DescriptorLinkTxStat.h"
#include "DescriptorLinkRx.h"


static int RateLast = -1;		// keep track of last rate for debug UserPrint
static int RateInterleave=0;
static int RateCount;			// count the number at RateLast
static int RateFlip;			// count the number of times we've only gotten one at a certain rate
								// if this happens a lot (>10 times), we turn on RateInterleave


#define MRATE 100

static int AggSize=0;
static int AggCount=0;
static struct txStats _LinkTxStatTotal;
static struct txStats _LinkTxStat[MRATE];


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

// LATERLINK	PacketLogDump();
}


struct txStats *DescriptorLinkTxStatTotalFetch()
{
	return &_LinkTxStatTotal;
}


struct txStats *DescriptorLinkTxStatFetch(int rate)
{
	if(rate>=0 && rate<MRATE)
	{
		return &_LinkTxStat[rate];
	}
	return 0;
}


void LinkTxStatClear()
{
    memset(&_LinkTxStatTotal, 0, sizeof(_LinkTxStatTotal));
    memset(&_LinkTxStat, 0, sizeof(_LinkTxStat));
	AggCount=0;
	AggSize=0;
	RateLast= -1;
	RateInterleave=0;
	RateCount=0;
	RateFlip=0;
}


static void HistogramInsert(int *histogram, int max, int value)
{
	if(value>=0 && value<max)
	{
        histogram[value]++;
	}
}
			  

void LinkTxStatExtract(unsigned int *descriptor, unsigned int *control, int nagg, int np)
{
	unsigned int i;
    int numRetries;
	unsigned int ba;
	int rssic[MCHAIN], rssie[MCHAIN], rssi;
	int evm[MSTREAM];
	int it;
	int tdiff;
	static unsigned int tlast;
    unsigned int timeStamp;
//	char buffer[MBUFFER];
	int rate;
	int length,alength;
    int good;
    //
	// figure out rate
	//
	rate=TxDescriptorTxRate(control /*descriptor*/);
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
    //
	// extract rssi and evm measurements. we need these for both good and bad packets
	//
    rssi=TxDescriptorRssiCombined(descriptor);
    rssic[0]=TxDescriptorRssiAnt00(descriptor);
    rssic[1]=TxDescriptorRssiAnt01(descriptor);
    rssie[0]=TxDescriptorRssiAnt10(descriptor);
    rssie[1]=TxDescriptorRssiAnt11(descriptor);
	evm[0]=(int)(0.5+TxDescriptorEvm0(descriptor));
	evm[1]=(int)(0.5+TxDescriptorEvm1(descriptor));

    if(TxDescriptorFrameTxOk(descriptor))	
    {
		length=TxDescriptorDataLen(control /*descriptor*/);
        if(TxDescriptorAggregate(control))
        {
            length-=(sizeof(WLAN_QOS_DATA_MAC_HEADER3));
        }
        else
        {
            length-=(sizeof(WLAN_DATA_MAC_HEADER3));
            nagg=1;
        }
 		//
		// last frame in aggreagate
		// does it have good status info?
		// THESE NEED TO BE LOGGED AT THE RATE THE PACKETS WERE SENT, NOT THE RATE OF THE BAR PACKET
		// ONLY DO THIS IF SENDING BAR PACKET
		// OTHERWISE NEED TO COUNT AGG PACKETS AS THEY GO OUT
		//
		if(TxDescriptorBaStatus(descriptor))	
		{
			alength=TxDescriptorAggLength(control /*descriptor*/);
            if(alength>0)
            {
		        // 
			    // good status, so count the bits in the block ack fields
			    //
			    ba=TxDescriptorBaBitmapHigh(descriptor);
//			    UserPrint("BA %d %d %08x",np,rate,ba);
                good=0;
			    for(i=0; i<32; i++)
			    {
				    if(((ba >> i) & 1) == 1) 
				    {
                        good++;	
                    }
			    }
			    ba=TxDescriptorBaBitmapLow(descriptor);
//			    UserPrint(" %08x",ba);
			    for(i=0; i<32; i++)
			    {
				    if(((ba >> i) & 1) == 1) 
				    {
			            good++;
				    }
			    }
                _LinkTxStatTotal.goodPackets+=good;
                _LinkTxStat[rate].goodPackets+=good;
                if(good>0)
                {
                    _LinkTxStatTotal.byteCount+=alength;
                    _LinkTxStat[rate].byteCount+=alength;
                }
            }
		}
		else 
		{
		    // 
			// regular frame, count as 1 packet
			//
            _LinkTxStatTotal.goodPackets+=nagg;
            _LinkTxStat[rate].goodPackets+=nagg;

            _LinkTxStatTotal.byteCount+=nagg*length;
            _LinkTxStat[rate].byteCount+=nagg*length;

            if(_LinkTxStatTotal.startTime==0)
            {
                _LinkTxStatTotal.dontCount+=nagg*length;
            }
            if(_LinkTxStat[rate].startTime==0)
            {
                _LinkTxStat[rate].dontCount+=nagg*length;
            }
		}
		//
		// find the transmit timestamp and save it for the throughput calculation
		//
        timeStamp=TxDescriptorSendTimestamp(descriptor);
        if(_LinkTxStat[rate].startTime == 0)
		{
            _LinkTxStat[rate].startTime = timeStamp;
		}
        _LinkTxStat[rate].endTime = timeStamp;
        if(_LinkTxStatTotal.startTime == 0)
		{
            _LinkTxStatTotal.startTime = timeStamp;
		}
        _LinkTxStatTotal.endTime = timeStamp;

		tdiff=timeStamp-tlast;
		if((tdiff<=0 || tdiff>10000) && timeStamp!=0 && tlast!=0 && np>0)
		{
//			        UserPrint("\n%d %d=%d-%d %x %x\n",np,tdiff,timeStamp,tlast,timeStamp,tlast);
		}
		tlast=timeStamp;
// LATERLINK		PacketLog(timeStamp,rate,0,np);
		//
		// accumulate rssi histograms
		//
        HistogramInsert(_LinkTxStat[rate].rssi,MRSSI,rssi);
		for(it=0; it<MCHAIN; it++)
		{
            HistogramInsert(_LinkTxStat[rate].rssic[it],MRSSI,rssic[it]);
            HistogramInsert(_LinkTxStat[rate].rssie[it],MRSSI,rssie[it]);
		}
		//
		// Accumulate evm histograms
		//
		for(it=0; it<MSTREAM; it++)
		{
            HistogramInsert(_LinkTxStat[rate].evm[it],MEVM,evm[it]);
		}
    }
    else
    {
        timeStamp=TxDescriptorSendTimestamp(descriptor);
        // 
		// process error statistics 
		//
        if(TxDescriptorFifoUnderrun(descriptor))	
		{
// LATERLINK			PacketLog(timeStamp,rate,1,np);
            _LinkTxStatTotal.underruns++;
            _LinkTxStat[rate].underruns++;
		    UserPrint(" %d fifo",np);
		}
        else if(TxDescriptorExcessiveRetries(descriptor))	
		{
// LATERLINK			PacketLog(timeStamp,rate,2,np);
            _LinkTxStatTotal.excessiveRetries++;
            _LinkTxStat[rate].excessiveRetries++;
		    UserPrint(" %d retry",np);
		}
		else
		{
// LATERLINK			PacketLog(timeStamp,rate,3,np);
            _LinkTxStatTotal.otherError++;
            _LinkTxStat[rate].otherError++;
		    UserPrint(" %d error",np);
//			TxDescriptorPrint(descriptor,buffer,MBUFFER);
//			UserPrint("%s\n",buffer);
		}
    }
    // 
	// number of retries
	//
    numRetries=TxDescriptorRtsFailCount(descriptor);  
    HistogramInsert(_LinkTxStat[rate].shortRetry,MRETRY,numRetries);
    numRetries=TxDescriptorDataFailCount(descriptor); 
    HistogramInsert(_LinkTxStat[rate].longRetry,MRETRY,numRetries);
	
    return;
}


