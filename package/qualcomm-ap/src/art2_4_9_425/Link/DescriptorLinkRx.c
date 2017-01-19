

#include <stdio.h>
#include <stdlib.h>

#include <string.h>
#include <ctype.h>

//#include "ah.h"
//#include "ah_osdep.h"


#include "AnwiDriverInterface.h"
//#include "NewArt.h"
#include "UserPrint.h"
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
#include "MyDelay.h"


#define MBUFFER 1024
#define MDESCRIPTOR 300

#define MPACKETSIZEBUFFER 4096
#define MPACKETSIZE 4092


//
// configuration parameters to adapt to different chips
//
static int LinkRxDescriptorFifo=0;

static int LinkRxEnableFirst=0;

static int _LinkRxSpectralScan=0;
static void (*_LinkRxSpectralScanFunction)(int rssi, int *rssic, int *rssie, int nchain, int *spectrum, int nspectrum);



#define MQUEUE 200		// the number of descriptors in the loop

static unsigned int LinkRxLoopDescriptor[MQUEUE];
static unsigned int LinkRxLoopBuffer[MQUEUE];
static int LinkRxLoopMany=0;

static unsigned int memInit=0;
static unsigned int memNext=0;

// lock the initial size of Memory for special use
unsigned int MemoryInit(int size)
{
	unsigned int buffer;
	buffer=memNext;
	memInit+=size;
	memNext = memInit;
	return buffer+DeviceMemoryBase();
}

unsigned int MemoryLayout(int size)
{
	unsigned int buffer;
	//
	// should check for violation of memory buffer size
	//
	buffer=memNext;
	memNext+=size;

	return buffer+DeviceMemoryBase();
}


// this frees everything.
void MemoryFree(unsigned int buffer)
{
	memNext=memInit;
//	memNext=0;
}


//
// Return address of first rx descriptor.
//
unsigned int LinkRxLoopFirst()
{
//	UserPrint("LinkRxLoopFirst %x %x\n",LinkRxLoopDescriptor[0],LinkRxLoopBuffer[0]);
	return LinkRxLoopDescriptor[0];
}


//
// Destroy loop of rx descriptors and free all of the memory.
//
int DescriptorLinkRxLoopDestroy()
{
//	UserPrint("LinkRxLoopDestroy %x %x\n",LinkRxLoopDescriptor[0],LinkRxLoopBuffer[0]);
	if(LinkRxLoopBuffer[0]!=0)
	{
        MemoryFree(LinkRxLoopBuffer[0]);
	}
    LinkRxLoopBuffer[0] = 0;

	if(LinkRxLoopDescriptor[0]!=0)
	{
        MemoryFree(LinkRxLoopDescriptor[0]);
	}
	LinkRxLoopDescriptor[0]=0;

//	LinkRxLoopMany=0;

	return 0;
}


//
// Create a loop of rx descriptors.
// Return the address of the first one.
//
int DescriptorLinkRxLoopCreate(int many)
{
    int it;
	unsigned int dr[MDESCRIPTOR];
	unsigned int descriptorNext;

    many=LinkRxLoopMany;                // we ignore the suggested queue size
//	MemoryLayout(1024);
    /*
     * create a few descriptors linked in a loop 
	 * OSPREY requires that the memory buffer directly follow the descriptor. Older chip sets
	 * include a pointer to the buffer in the descriptor. This code does both.
     */
	for(it=0; it<many; it++)
	{
		LinkRxLoopDescriptor[it] = MemoryLayout(RxDescriptorSize()+MPACKETSIZEBUFFER);
		if(LinkRxLoopDescriptor[it]==0) 
		{
			UserPrint( "Device Number %d:rxDataSetup: unable to allocate memory for %d descriptors\n", 0, many);
			return -1;
		}
		LinkRxLoopBuffer[it]=LinkRxLoopDescriptor[it]+RxDescriptorSize();
	}
	//
	// fill in the descriptors
	//
    for (it = 0; it < many; it++)
	{
		//
		// figure out next pointer, last one loops back to first
		//
        if (it == many - 1) 
		{ 
            descriptorNext = LinkRxLoopDescriptor[0];
        }
        else 
		{
            descriptorNext = LinkRxLoopDescriptor[it+1];	//descriptor + RxDescriptorSize();	
        }
        //
		// make the descriptor
		//
//		UserPrint("%2d: %8x %8x %8x\n",it,LinkRxLoopDescriptor[it],LinkRxLoopBuffer[it],descriptorNext);
        RxDescriptorSetup(dr,descriptorNext,LinkRxLoopBuffer[it],MPACKETSIZE);
		//
		// copy it to the shared memory
		//
		DeviceMemoryWrite(LinkRxLoopDescriptor[it],dr,RxDescriptorSize());

//        memset(dr, 0x55, RxDescriptorSize());
//		DeviceMemoryWrite(LinkRxLoopDescriptor[it]+48,dr,RxDescriptorSize());
	}

//	LinkRxLoopMany=many;

	return 0;
}


int DescriptorLinkRxSetup(unsigned char *bssid, unsigned char *macaddr)
{
	DescriptorLinkRxOptions();

	_LinkRxSpectralScan=0;

    DeviceReceiveDisable();
    // 
	// cleanup descriptors created last time
	//
	DescriptorLinkRxLoopDestroy(0);
    /*
     * create a few descriptors linked in a loop 
     */
    DescriptorLinkRxLoopCreate(MQUEUE);

#ifdef UNUSED
	this should always be turned on

    ar5kInitData[pLibDev->ar5kInitIndex].pMacAPI->setPPM(0, enablePPM);

    pLibDev->rx.rxEnable = 1;
#endif

	DeviceStationIdSet(macaddr);
	DeviceBssIdSet(bssid);
    DeviceReceiveDeafMode(0);
    return 0;
}


//
// Setup to do spectral scan. Must be called after LinkRxSetup() and before LinkRxStart().
// Spectral scan fft data is returned through the specified callback function.
// In addition regular processing of rssi and other errors occurs.
//
int DescriptorLinkRxSpectralScan(int spectralscan, void (*f)(int rssi, int *rssic, int *rssie, int nchain, int *spectrum, int nspectrum))
{
	_LinkRxSpectralScan=spectralscan;
	_LinkRxSpectralScanFunction=f;
	return 0;
}


int DescriptorLinkRxStart(int promiscuous)
{
	int it;

	LinkRxStatClear();
    //
    // enable before because Osprey requires it
    //
    if(LinkRxEnableFirst)
    {
        DeviceReceiveEnable();
    }
    //
    // set up rx filters
    //
    DeviceReceiveUnicast(1);
    DeviceReceiveBroadcast(1);
    DeviceReceivePromiscuous(promiscuous);
    //
    // for all chips, we have to load the first descriptor
    //
	DeviceReceiveDescriptorPointer(LinkRxLoopDescriptor[0]);
    //
    // if the chip has an rx fifo, we need to load the rest of the descriptors
    //
    if(LinkRxDescriptorFifo)
    {
	    for(it=1; it<LinkRxLoopMany; it++)
	    {
	        DeviceReceiveDescriptorPointer(LinkRxLoopDescriptor[it]);
	    }
    }

	if(_LinkRxSpectralScan)
	{
		DeviceSpectralScanEnable();
	}

    //
    // do it again because Merlin likes it after the descriptor pointer is set
    //
    if(!LinkRxEnableFirst)
    {
        DeviceReceiveEnable();
    }
	return 0;
}


void LinkRxDump(void *dr, int ndump, int np)
{
	char buffer[MBUFFER];
	int datalen;
	int it;

	if(ndump>MBUFFER)
	{
		ndump=MBUFFER;
	}
	RxDescriptorPrint(dr,buffer,MBUFFER);
	UserPrint("%s\n",buffer);
    //
	// fetch the data
	//
	datalen=RxDescriptorDataLen(dr);
	if(ndump>datalen)
	{
		ndump=datalen;
	}
    DeviceMemoryRead(LinkRxLoopBuffer[np], (unsigned int *)buffer, ndump);
	//
	// print
	//
	for(it=0; it<ndump; it++)
	{
		UserPrint("%02x ",buffer[it]);
	}
	UserPrint("\n");
}


int DescriptorLinkRxComplete(int timeout, 
	int ndump, unsigned char *dataPattern, int dataPatternLength, int (*done)())
{
    unsigned int startTime, endTime, ctime;
	int it;
	int notdone;
	int stop;
	int behind;
	int scount;
	unsigned int mDescriptor;			// address of descriptor in shared memory
#ifdef MEMORYREAD
	unsigned int descriptor[MDESCRIPTOR+30];	// private copy of current descriptor
#else
    unsigned int *descriptor;
#endif
	int pass;
    int dsize;
	int isdone;
	int ss;
	int rssic[MCHAIN], rssie[MCHAIN], rssi;
#define MSPECTRUM 1024
	int nspectrum,spectrum[MSPECTRUM];
	int ndata;
	unsigned char data[MBUFFER];
	int sscount=0;

	pass=0;
	dataPatternLength=0;
	dataPattern=0;
	scount=0;
    dsize=RxDescriptorSize();
    //
	// Loop timeout condition.
	// This number can be large since it is only used for
	// catastrophic failure of cart. The normal terminating
	// condition is a message from cart saying STOP.
	//
    startTime=TimeMillisecond();
	ctime=startTime;
//	if(timeout<=0)
//	{
//		timeout=60*60*1000;             // one hour
//	}
	endTime=startTime+timeout;		
	//
	// set pointer to first descriptor
	//
//    mDescriptor = LinkRxLoopFirst(0);
	//
	// keep track of how many times we look and the descriptor is not done
	// this is used to control the sleep interval
	//
	notdone=0;
	stop=0;
	behind=0;
	//
	// loop looking for descriptors with received packets.
	//
	for(it = 0; ; it++) 
	{
		mDescriptor=LinkRxLoopDescriptor[it%LinkRxLoopMany];
#ifdef MEMORYREAD
        //
		// read the next descriptor
		//
        DeviceMemoryRead(mDescriptor, descriptor, dsize);		
#else
        descriptor=(unsigned int *)DeviceMemoryPtr(mDescriptor);
#endif
        //
		// descriptor is ready, we have a new packet
		//
        if(RxDescriptorDone(descriptor)) 
		{
#ifdef MEMORYREAD
            //
		    // read the next descriptor
		    //
            DeviceMemoryRead(mDescriptor, descriptor, dsize);
#endif

		
			notdone=0;
			behind++;
			scount=0;
#ifdef FASTER
			if((it%100)==0)
			{
				UserPrint(" %d",it);
			}
#endif      
			if(_LinkRxSpectralScan)
			{
				ss=RxDescriptorSpectralScan(descriptor);
				if(ss)
				{
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
					ndata=RxDescriptorDataLen(descriptor);
					DeviceMemoryRead(LinkRxLoopBuffer[it%LinkRxLoopMany], (unsigned int *)data, ndata);
					nspectrum=DeviceSpectralScanProcess(data,ndata,spectrum,MSPECTRUM);
					if(_LinkRxSpectralScanFunction!=0)
					{
						(*_LinkRxSpectralScanFunction)(rssi,rssic,rssie,MCHAIN,spectrum,nspectrum);
					}
					//
					// extract stats
					//
					LinkRxStatSpectralScanExtract(descriptor,it);

					sscount++;
					if(sscount>=10)
					{
						break;
					}
				}
			}
			else
			{
				//
				// dump packet contents
				//
				if(ndump>0) 
				{
					UserPrint("\n");
					LinkRxDump(descriptor, ndump, it%LinkRxLoopMany);
				}
				//
				// extract stats
				//
				LinkRxStatExtract(descriptor,it);
			}
			//
			// reset the descriptor so that we can use it again
			//
			RxDescriptorReset(descriptor);
#ifdef MEMORYREAD
			//
			// copy it back to the shared memory
			//
			DeviceMemoryWrite(mDescriptor,descriptor,dsize);
#endif
			//
			// need to queue the descriptor with the hardware
			//
            if(LinkRxDescriptorFifo)
            {
	            DeviceReceiveDescriptorPointer(mDescriptor);
            }
        } 
		//
		// descriptor not ready
		//
		else 
		{
			if(stop)
			{
				scount++;
				if(scount>200)
				{
				    break;
				}
			}
			it--;
			//
			// sleep every other time, need to keep up with fast rates
			//
			if(notdone>100)
			{
			    UserPrint(".");
//			    MyDelay(1);
				notdone=0;
			}
			else
			{
				notdone=1;      // this makes sure we never sleep
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
				isdone=(*done)();
				if(isdone!=0)
				{
					UserPrint(" %d Stop",it);
					if(stop==0)
					{
						stop=1;
						scount=0;
						behind=0;
					}
					//
					// immediate stop
					//
					if(isdone==2)
					{
						break;
					}
	//				break;
				}
			}
			pass=0;
		    //
		    // check for timeout
		    // rare terminating condition when cart crashes or disconnects
		    //
		    ctime=TimeMillisecond();
		    if(timeout>0 && ((endTime>startTime && (ctime>endTime || ctime<startTime)) || (endTime<startTime && ctime>endTime && ctime<startTime)))
		    {
			    UserPrint("%d Timeout",it);
			    break;
		    }
		}
    } 
    //
	// cleanup
	//
	if(_LinkRxSpectralScan)
	{
		DeviceSpectralScanDisable();
	}

	DeviceReceiveDisable();

	DescriptorLinkRxLoopDestroy(0);

    //
	// do data verify if requested
	//
	UserPrint(" %d\n",it);

	LinkRxStatFinish();

    return 0;
}


#ifdef UNSUED
void Ar9300LinkRx()
{
    LinkRxDescriptorFifo=1;
    LinkRxEnableFirst=1;
    LinkRxLoopMany=64;
    if(MQUEUE<LinkRxLoopMany)
    {
        LinkRxLoopMany=MQUEUE;
    }
}


void Ar5416LinkRx()
{
    LinkRxDescriptorFifo=0;
    LinkRxEnableFirst=0;
    LinkRxLoopMany=MQUEUE;
}
#endif


int DescriptorLinkRxOptions()
{
    int maxdescriptor;
	
	LinkRxDescriptorFifo=DeviceReceiveFifo();
	maxdescriptor=DeviceReceiveDescriptorMaximum();
    if(maxdescriptor<=0 || maxdescriptor>MQUEUE)
    {
        LinkRxLoopMany=MQUEUE;
    }
    else
    {
        LinkRxLoopMany=maxdescriptor;
    }
    LinkRxEnableFirst=DeviceReceiveEnableFirst();
	return 0;
}



