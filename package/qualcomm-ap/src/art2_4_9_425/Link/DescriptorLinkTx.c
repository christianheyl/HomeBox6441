
#include <stdio.h>
#include <stdlib.h>


#include "wlanproto.h"
#include "rate_constants.h"

#include "smatch.h"
#include "UserPrint.h"
#include "TimeMillisecond.h"
//#include "CommandParse.h"

//#include "AnwiDriverInterface.h"
//#include "NewArt.h"
//#include "ParameterSelect.h"
//#include "Card.h"
#include "MyDelay.h"
#include "Device.h"
#include "RxDescriptor.h"
#include "TxDescriptor.h"
#include "ResetForce.h"

#define MBUFFER 1024



#include "TimeMillisecond.h"
#include "Device.h"

#include "LinkStat.h"
#include "DescriptorLinkTx.h"
#include "DescriptorLinkTxStat.h"
#include "DescriptorLinkTxPacket.h"
#include "DescriptorLinkRx.h"
#include "Field.h"


#define MRATE 100
#define MDESCRIPTOR 30


//
// save the parameters we need to requeue descriptors
//
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

static int DeafMode=0;

static int RateLast=0;				// keep track of last rate for debugging UserPrint
static int RateLastQueue= -1;
static int PAPDTrainChainNum = -1;

static unsigned char PN9Data[] = {0xff, 0x87, 0xb8, 0x59, 0xb7, 0xa1, 0xcc, 0x24, 
                            0x57, 0x5e, 0x4b, 0x9c, 0x0e, 0xe9, 0xea, 0x50, 
                            0x2a, 0xbe, 0xb4, 0x1b, 0xb6, 0xb0, 0x5d, 0xf1, 
                            0xe6, 0x9a, 0xe3, 0x45, 0xfd, 0x2c, 0x53, 0x18, 
                            0x0c, 0xca, 0xc9, 0xfb, 0x49, 0x37, 0xe5, 0xa8, 
                            0x51, 0x3b, 0x2f, 0x61, 0xaa, 0x72, 0x18, 0x84, 
                            0x02, 0x23, 0x23, 0xab, 0x63, 0x89, 0x51, 0xb3, 
                            0xe7, 0x8b, 0x72, 0x90, 0x4c, 0xe8, 0xfb, 0xc0};


#define MQUEUE 400		// the number of descriptors in the loop

//
// Configuration parameters to adapt to different chips
//

//
// Is the transmit descriptor register a fifo
//
static int LinkTxFifo=0;

//
// do we get status descriptors for intermediate agg packets?
//
static int LinkTxAggregateStatus=1;

//
// split descriptors? control and status
//
static int LinkTxSplitDescriptor=0;

//
// do we have to enable the transmitter before we set the first descriptor
//
static int LinkTxEnableFirst=0;




//
// the total number of packets
//
static int LinkTxMany=0;

//
// the number of valid status descriptors we expect to receive
// don't count nonterminating aggregate descriptors. Just skip those.
//
static int LinkTxStatusMany=0;

//
// the number of descriptors we queue in each batch
//
// this number is adjusted inside LinkTxSetup()
//
// internal to a batch the descriptors are chained with the link pointer
// the last descriptor in the batch has link pointer = 0
//
static int LinkTxBatchMany=MQUEUE/2;


//
// the number of descriptors in use
// this number is adjusted inside LinkTxSetup() 
//
static int LinkTxDescriptorMany=MQUEUE;

//
// a bunch of descriptors.
// we reuse these, over and over again
//
static unsigned int LinkTxLoopDescriptor[MQUEUE];

//
// and these are the status descriptors
//
// on Merlin and before the status desciptors are the same as the control descriptors
//
static unsigned int LinkTxLoopDescriptorStatus[MQUEUE];

//
// index of the queued control descriptor associated with the expected status descriptor.
// we keep these pointers since Osprey does not copy the tx_desc_id field as expected.
// we do not get status descriptors for non-terminating aggregate packets so these counters
// do not increment together.
// 
static int LinkTxQueued[MQUEUE];

//
// the last entry written in the above array
// entries are written modulo MQUEUE
//
static int LinkTxQueuedLast;




static int TxDescriptorPointerGet()
{
    unsigned int txdp;
 
    DeviceRegisterRead(0x0800,&txdp);

    return txdp;
}


static int TxStatusCheck()
{
    unsigned int txe;
    static unsigned int txeLast;

    DeviceRegisterRead(0x0840,&txe);
//    if(txe!=txeLast)
    {
        if(txe==0)
        {
            UserPrint(" OFF");
        }
        else
        {
            UserPrint(" ON");
        }
    }
    txeLast=txe;

    return txe;
}


//
// Return address of first tx descriptor.
//
static unsigned int LinkTxLoopFirst()
{
//	UserPrint("LinkTxLoopFirst %x %x\n",LinkTxLoopDescriptor[0],LinkTxLoopBuffer[0]);
	return LinkTxLoopDescriptor[0];
}


//
// Destroy loop of tx descriptors and free all of the memory.
//
static int LinkTxLoopDestroy()
{
	if(LinkTxLoopDescriptor[0]!=0)
	{
        MemoryFree(LinkTxLoopDescriptor[0]);
	}
	LinkTxLoopDescriptor[0]=0;

	if(LinkTxLoopDescriptorStatus[0]!=0)
	{
        MemoryFree(LinkTxLoopDescriptorStatus[0]);
	}
	LinkTxLoopDescriptorStatus[0]=0;

	return 0;
}


static void HistogramInsert(int *histogram, int max, int value)
{
	if(value>=0 && value<max)
	{
        histogram[value]++;
	}
}


//
// take an existing descriptor and alter it to be the next desciptor:
//    clear all status fields
//	  alter rate and other control parameters
//    copy it back to the shared memory
//
// if this is the last required descriptor, zero the link pointer 
// so that transmission will terminate
//
static int LinkTxNextDescriptor(int nd)
{
	int rate;		// which rate should we use
	int packet;		// which packet number
	int agg;
	int slot;
	int ht40;
	int sgi;
	unsigned int dr[MDESCRIPTOR];
	unsigned int dptr, nptr;
//	unsigned char buffer[MBUFFER];

	if(PacketMany<=0 || nd<LinkTxMany)
	{
        if(InterleaveRate)
		{
		    packet=nd/(RateMany*(AggregateMany+AggregateBar));
		    rate=nd%(RateMany*(AggregateMany+AggregateBar));
			agg=rate%(AggregateMany+AggregateBar);
			rate=rate/(AggregateMany+AggregateBar);
		}
	    else if(PacketMany>0)
		{
		    rate=nd/(PacketMany*(AggregateMany+AggregateBar));
		    packet=nd%(PacketMany*(AggregateMany+AggregateBar));
			agg=packet%(AggregateMany+AggregateBar);
			packet=packet/(AggregateMany+AggregateBar);
		}
	    else
		{
		    rate=0;
		    packet=nd;
			agg=0;
		}

		slot=nd%(2*LinkTxBatchMany);
        dptr=LinkTxLoopDescriptor[slot];
		//
		// set link pointer
		// we make a loop if the number of required descriptors is greater than MQUEUE
		//
//        UserPrint("make %d %d\n",nd,slot);
        if((slot%LinkTxBatchMany)==LinkTxBatchMany-1 || (LinkTxMany>0 && nd>=LinkTxMany-1))
		{
			//
			// we're done
			//
			nptr=0;
//			UserPrint("null ptr on %d %d\n",nd,slot);
		}
		else 
		{
			//
			// point to next descriptor
			//
            nptr=LinkTxLoopDescriptor[(slot+1)%(2*LinkTxBatchMany)];
        } 

		if(AggregateMany>1 && AggregateBar!=0 && agg==AggregateMany)
		{
			//
			// special bar packet for aggregates
			//
			TxDescriptorSetup(dr, nptr, 
				LinkTxBarBuffer(), LinkTxBarBufferSize(),
				Broadcast, Retry,
				Rate[32], 0, 0, 1, 
				0, 0,
				nd&0xffff);
			//
			// keep pointer from expected status descriptor to the control descriptor
			//
			LinkTxQueued[LinkTxQueuedLast]=nd;
//            UserPrint(" c[%d]=%d",LinkTxQueuedLast,nd);
			LinkTxQueuedLast=(LinkTxQueuedLast+1)%LinkTxDescriptorMany;
		}
		else
		{
			//
			// this code figures out if the incoming packets have interleaved rates
			// this is solely for debug purposes
			//
			if(InterleaveRate==0 && Rate[rate]!=RateLastQueue)
			{
				UserPrint("(%d)",Rate[rate]);
				RateLastQueue=Rate[rate];
			}
           //
			// normal packet
			//
			ht40=IS_HT40_RATE_INDEX(Rate[rate]);
			// support short gi for both ht20/ht40 rates. 
			sgi=ShortGi;
            //
			// don't allow aggregates at legacy rates
			// so just send multiple packets
			//
			if(IS_LEGACY_RATE_INDEX(Rate[rate]))
			{
				TxDescriptorSetup(dr, nptr, 
					LinkTxLoopBuffer(agg), LinkTxLoopBufferSize(agg),
					Broadcast, Retry,
					Rate[rate], 0, 0, TxChain, 
					0, 0,
					nd&0xffff);
			    //
			    // keep pointer from expected status descriptor to the control descriptor
			    //
			    LinkTxQueued[LinkTxQueuedLast]=nd;
//            UserPrint(" c[%d]=%d",LinkTxQueuedLast,nd);
			    LinkTxQueuedLast=(LinkTxQueuedLast+1)%LinkTxDescriptorMany;
			}
			else
			{
				TxDescriptorSetup(dr, nptr, 
					LinkTxLoopBuffer(agg), LinkTxLoopBufferSize(agg),
					Broadcast, Retry,
					Rate[rate], ht40, sgi, TxChain, 
					AggregateMany, (agg!=AggregateMany-1),
					nd&0xffff);
				if(LinkTxAggregateStatus || agg==AggregateMany-1)
				{
					//
					// keep pointer from expected status descriptor to the control descriptor
					//
					LinkTxQueued[LinkTxQueuedLast]=nd;
//            UserPrint(" c[%d]=%d",LinkTxQueuedLast,nd);
					LinkTxQueuedLast=(LinkTxQueuedLast+1)%LinkTxDescriptorMany;
				}
			}
		}
		// Check if this packet is for PAPD
		if(PAPDTrainChainNum != -1)
		{
			TxDescriptorPAPDSetup(dr, PAPDTrainChainNum);
		}
		//
		// copy it back to the shared memory
		//
		DeviceMemoryWrite(dptr,dr,TxDescriptorSize());

		return 0;
	}
	return -1;
}


static int LinkTxBatchQueueFifo(int nd)
{
	//
	// push into hardware fifo
	//
//	UserPrint("queue batch starting at %d\n",nd);
    DeviceTransmitEnable(1);
	DeviceTransmitDescriptorPointer(0, LinkTxLoopDescriptor[nd%(2*LinkTxBatchMany)]);

    return 0;
}


static int LinkTxBatchQueueLinkPtr(int nd)
{
	unsigned int descriptor[MDESCRIPTOR];	// private copy of current descriptor
    int first,last;
	int batch;
    int behind, elapsed, start, end;

//    UserPrint("batch queue\n");
    TxStatusCheck();
    if(nd>0)
    {
	    batch=nd/LinkTxBatchMany;
        //
        // Find the first descriptor queued in this batch.
        //
        first=(batch%2)*(LinkTxBatchMany);
        //
        // Find last descriptor queued from previous batch.
        //
        last=((batch+1)%2)*(LinkTxBatchMany)+LinkTxBatchMany-1;
        //
        // read the descriptor from the shared memory
        //
        DeviceMemoryRead(LinkTxLoopDescriptor[last], descriptor, TxDescriptorSize());
//        UserPrint("link %d -> %d\n",last,first);
        //
        // check if it is done
        //
        if(TxDescriptorDone(descriptor))
        {
            UserPrint("\nBATCH %d to %d: STALL. Previous batch done. Restart transmitter. %d %d\n",nd,nd+LinkTxBatchMany-1,last,first);
            //
            // this is where we need to push the descriptor into txdp
            // and restart the transmitter
            //
 	        DeviceTransmitDescriptorPointer(0, LinkTxLoopDescriptor[first]);
            DeviceTransmitEnable(1);
        }
        else
        {
            start=TxDescriptorPointerGet();

            TxDescriptorLinkPtrSet(descriptor, LinkTxLoopDescriptor[first]);
            DeviceMemoryWrite(LinkTxLoopDescriptor[last], descriptor, TxDescriptorSize());

            end=TxDescriptorPointerGet();
            elapsed=(end-start)/TxDescriptorSize();
            behind=(LinkTxLoopDescriptor[last]-end)/TxDescriptorSize();
            UserPrint("\nBATCH %d to %d: %08x -> %08x margin=%d\n",nd,nd+LinkTxBatchMany-1,LinkTxLoopDescriptor[last],LinkTxLoopDescriptor[first],behind);
            //
            // this is where we need to check that the chip didn't do this descriptor while
            // we were fooling around.
            //
            DeviceMemoryRead(LinkTxLoopDescriptor[last], descriptor, TxDescriptorSize());		
            //
            // check if it is done. 
            //
            if(TxDescriptorDone(descriptor))
            {
                if(TxStatusCheck()==0)
                {
            UserPrint("\nBATCH %d to %d: STALL. Previous batch finished while linking. Restart transmitter. %d %d\n",nd,nd+LinkTxBatchMany-1,last,first);
                    //
                    // this is where we need to push the descriptor into txdp
                    // and restart the transmitter
                    //
 	                DeviceTransmitDescriptorPointer(0, LinkTxLoopDescriptor[first]);
                    DeviceTransmitEnable(1);
                }

            }
            //
            // check and take remedial action if necessary, push descriptor into txdp and
            // restart transmitter
            //
        }
    }
    else
    {
	    //
	    // push into hardware, no fifo, so only one at a time
	    //
	    UserPrint("\nBATCH %d to %d\n",0,LinkTxBatchMany-1);
	    DeviceTransmitDescriptorPointer(0, LinkTxLoopDescriptor[0]);
//        DeviceTransmitEnable(1);
//        TxStatusCheck();
    }

    return 0;
}


static int LinkTxBatchQueue(int nd)
{
	if(PacketMany<=0 || nd<LinkTxMany)
	{
        if(LinkTxFifo)
        {
            return LinkTxBatchQueueFifo(nd);
        }
        else
        {
            return LinkTxBatchQueueLinkPtr(nd);
        }
    }
    return -1;
}


static int LinkTxBatchNext(int nd, int queue)
{
	int batch;
	int check;
	int it;
    int end;

	if(PacketMany<=0 || nd<LinkTxMany)
	{
		batch=nd/LinkTxBatchMany;
		check=nd%LinkTxBatchMany;

		if(check!=0)
		{
			UserPrint("bad request to make batch: nd=%d many=%d batch=%d check=%d\n",nd,LinkTxBatchMany,batch,check);
			return -1;
		}
        else
		{
//			UserPrint("make batch: nd=%d many=%d batch=%d check=%d\n",nd,LinkTxBatchMany,batch,check);
		}
		//
		// make the descriptors
		//
        end=nd+LinkTxBatchMany;
        if(PacketMany>0 && end>LinkTxMany)
        {
            end=LinkTxMany;
        }
//	    UserPrint("\nMAKE BATCH %d: %d to %d\n",batch,nd,end-1);
		for(it=nd; it<end; it++)
		{
			LinkTxNextDescriptor(it);
		}
        //
        // if autoqueue is set, do it
        //
        if(queue)
        {
            LinkTxBatchQueue(nd);
        }
	}
    else
    {
//        UserPrint("\nBATCH DONE\n");
    }

	return 0;
}


//
// run the transmitter
//
// (*ison)() is called when the transmitter is guaranteed to be on (first descriptor returned done)
//
// (*done)() is called to check if this function should stop early
//
int DescriptorLinkTxComplete(int timeout, int (*ison)(), int (*done)(), int chipTemperature, int calibrate)
{
	int it;
    unsigned int startTime, endTime, ctime, failTime;
	int failRetry;
	int notdone;
	unsigned int mDescriptor;			    // address of descriptor in shared memory
	unsigned int descriptor[MDESCRIPTOR];	// private copy of current descriptor
    int cindex;                             // index of the corresponding control descriptor
	unsigned int *cd,cdescriptor[MDESCRIPTOR];	// private copy of current control descriptor
    int pass;
    int dsize;
//    int behind;
    int batch, lbatch;
	int temperature;
	int doTemp;

    LinkTxStatClear();
    lbatch=0;
    pass=0;
	doTemp=0;
    dsize=TxDescriptorSize();
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
//		timeout=60*60*1000;         // one hour
//	}
	endTime=startTime+timeout; 
	if(Tx100Packet)
	{
		failTime=endTime;
	}
	else
	{
		failTime=startTime+(5*1000);	// 5 seconds
	}
	failRetry=0;
	//
	// set pointer to first descriptor
	//
    mDescriptor = LinkTxLoopFirst();
	//
	// keep track of how many times we look and the descriptor is not done
	// this is used to control the sleep interval
	//
	notdone=0;
//	tlast=0;
	//
	// if this is the first descriptor, announce that the transmitter is really on
	//
	if(ison!=0)
	{
		(*ison)();
	}
	//
	// loop looking for descriptors with transmitted packets.
	// terminate either when we reach a null pointer (Merlin and prior) or
	// when we've done the specified number of packets (Osprey)
	//
	for(it=0; (LinkTxStatusMany<=0 || it<LinkTxStatusMany); it++) 
	{
        //
		// read the next descriptor
		//
		mDescriptor=LinkTxLoopDescriptorStatus[it%LinkTxDescriptorMany];
        DeviceMemoryRead(mDescriptor, descriptor, dsize);
		//
		// skip ahead for aggregates. 
		//
        if(LinkTxAggregateStatus)
        {
		    if(TxDescriptorAggregate(descriptor) && TxDescriptorMoreAgg(descriptor))
		    {
//			    UserPrint("A");
                continue;
		    }
        }
        //
		// is it done?
		//
		if(TxDescriptorDone(descriptor))	
		{
            cindex=LinkTxQueued[it%LinkTxDescriptorMany];
            batch=cindex/LinkTxBatchMany;
//            UserPrint("(%d %d %d)",it,cindex,batch);

            DeviceMemoryRead(mDescriptor, descriptor, dsize);
            //
            // get the control descriptor too
            //
            if(LinkTxSplitDescriptor)
            {
                DeviceMemoryRead(LinkTxLoopDescriptor[cindex%LinkTxDescriptorMany], cdescriptor, TxDescriptorSize());	
                cd=cdescriptor;
            }
            else
            {
                cd=descriptor;
            }
#ifdef UNUSED
            behind=(TxDescriptorPointerGet()-mDescriptor)/dsize;
            if(behind>10)
            {
                UserPrint(" -%d",behind);
            }
#endif
			notdone=0;
			if((it%100)==0)
			{
				UserPrint(" %d",it);
			}
			LinkTxStatExtract(descriptor,cd,AggregateMany,it);
			//
			// clear the status packet
			//
            if(LinkTxSplitDescriptor)
            {
	            TxDescriptorStatusSetup(descriptor);
		        DeviceMemoryWrite(LinkTxLoopDescriptorStatus[it%LinkTxDescriptorMany],descriptor,TxDescriptorStatusSize());
            }
            //
            // Do we need to queue the next batch of descriptors?
            //
			if(batch>lbatch)
			{
//                UserPrint("\n%d %d %d %d\n",it,cindex,lbatch,batch);
				LinkTxBatchNext((batch+1)*LinkTxBatchMany,1);
                lbatch=batch;
			}
		}
		//
		// descriptor isn't done
		//
		else 
		{
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
				notdone++;
			}
        }
		//
		// check temperature
		//
		if(doTemp>10)
		{
			temperature=DeviceTemperatureGet(0);
			//
			// are we supposed to terminate on reaching a certain chip temperature?
			//
			if(chipTemperature>0)
			{
				if(temperature<=chipTemperature)
				{
					UserPrint(" %d Temperature %d <= %d",it,temperature,chipTemperature);
					break;
				}
			}
			doTemp=0;
		}
		doTemp++;
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
    				break;
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
			    UserPrint(" %d Timeout",it);
			    break;
		    }
			//
			// check for failure to start
			//
			if(it<=0)
			{
				if((failTime>startTime && (ctime>failTime || ctime<startTime)) || (failTime<startTime && ctime>failTime && ctime<startTime))
				{
					UserPrint("Failed to start transmit.\n");
					if(failRetry==0)
					{
						UserPrint("Trying to restart.\n");
						DescriptorLinkTxStart();
						failTime=startTime+(5*1000);	// 5 seconds
						failRetry++;
						//
						// set pointer to first descriptor
						//
						mDescriptor = LinkTxLoopFirst();
					}
					else
					{
						ResetForce();
						break;
					}
				}
			}
		}
    } 

    TxStatusCheck();

	DeviceTransmitDisable(0xffff);
	DeviceReceiveDisable();

	LinkTxLoopDestroy();
	DescriptorLinkRxLoopDestroy();
	//
	// finish throughput calculation
	//
	LinkTxStatFinish();

    TxStatusCheck();

	UserPrint(" %d Done\n",it);

    return 0;
}


static int LinkTxStatusLoopCreateSplit(int numDesc)
{
	int it;
	unsigned int dr[MDESCRIPTOR];

    LinkTxLoopDescriptorStatus[0] = MemoryLayout(numDesc * TxDescriptorStatusSize());
    if(LinkTxLoopDescriptorStatus[0]==0) 
	{
        UserPrint("txDataSetup: unable to allocate client memory for %d status descriptors\n",numDesc);
        return -1;
    }
	TxDescriptorStatusSetup(dr);
	for(it=0; it<numDesc; it++)
	{
		LinkTxLoopDescriptorStatus[it]=LinkTxLoopDescriptorStatus[0]+it*TxDescriptorStatusSize();
		DeviceMemoryWrite(LinkTxLoopDescriptorStatus[it],dr,TxDescriptorStatusSize());
	}
	DeviceTransmitDescriptorStatusPointer(LinkTxLoopDescriptorStatus[0],LinkTxLoopDescriptorStatus[0]+numDesc*TxDescriptorStatusSize());
	return 0;
}


static int LinkTxStatusLoopCreateSame(int numDesc)
{
	int it;

	for(it=0; it<numDesc; it++)
	{
		LinkTxLoopDescriptorStatus[it]=LinkTxLoopDescriptor[it];
 	}
	return 0;
}


int LinkTxStatusLoopCreate(int many)
{
    LinkTxQueuedLast=0;
    if(LinkTxSplitDescriptor)
    {
        return LinkTxStatusLoopCreateSplit(many);
    }
    else
    {
        return LinkTxStatusLoopCreateSame(many);
    }
}

#ifdef LINKLATER
int LinkTxParameterCheck(unsigned int rateMask, unsigned int rateMaskMcs20, unsigned int rateMaskMcs40, int ir,
    int numDescPerRate, int dataBodyLength,
    int broadcast, int ifs,
	unsigned int txchain, int naggregate)
{
    //
    // remember some parameters for later use when we queue packets
    //
    TxChain=txchain;
    Broadcast=broadcast;

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
		    AggregateBar=0;         // never do this
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
        if(AggregateMany*dataBodyLength>65535)
        {
            AggregateMany=65535/dataBodyLength;
            UserPrint("reducing AggregateMany from %d to %d=65535/%d\n",naggregate,AggregateMany,dataBodyLength);
        }
	}
    //
	// Count rates and make the internal rate arrays.
	//
    RateMany = RateCount(rateMask, rateMaskMcs20, rateMaskMcs40, Rate);
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
//        PacketMany=1000000;
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
		Broadcast=1;
	}
	else if(ifs>0)
	{
		//
		// tx99
		//
		Broadcast=1;
	}
	else
	{
		//
		// regular
		//
	}

    return 0;
}
#endif

  
int DescriptorLinkTxSetup(int *rate, int nrate, int ir,
    unsigned char *bssid, unsigned char *source, unsigned char *destination,
    int numDescPerRate, int dataBodyLength,
    int retries, int antenna, int broadcast, int ifs,
	int shortGi, unsigned int txchain, int naggregate,
	unsigned char *pattern, int npattern)
{
    int it;
    unsigned int    antMode = 0;
    int queueIndex;
	int dcuIndex;
    int wepEnable;
	static unsigned char bcast[]={0xff,0xff,0xff,0xff,0xff,0xff};
	unsigned char *duse;
	unsigned char *ptr;

	DescriptorLinkRxOptions();
	DescriptorLinkTxOptions();

	TxStatusCheck();
    DeviceTransmitDisable(0xffff);
	DeviceReceiveDisable();
	TxStatusCheck();
    //
    // do we still need these?
    //
	wepEnable=0;
	queueIndex=0;
	dcuIndex=0;
    //
	// Cleanup any descriptors and memory used in previous iteration
	// Make sure you do these first, since they actually destroy everything.
	//
	LinkTxLoopDestroy();
	DescriptorLinkRxLoopDestroy();
    //
    // remember some parameters for later use when we queue packets
    //
    TxChain=txchain;
    ShortGi=shortGi;
    Broadcast=broadcast;
    Retry=retries;

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
        if(AggregateMany*dataBodyLength>65535)
        {
            AggregateMany=65535/dataBodyLength;
            UserPrint("reducing AggregateMany from %d to %d=65535/%d\n",naggregate,AggregateMany,dataBodyLength);
        }
	}
    //
	// Count rates and make the internal rate arrays.
	//
    RateMany = nrate;
	for(it=0; it<RateMany; it++)
	{
		Rate[it]=rate[it];
	}
	//RateCount(rateMask, rateMaskMcs20, rateMaskMcs40, Rate);
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
//        PacketMany=1000000;
		LinkTxMany= -1;
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
        Tx100Packet = 1;
		Broadcast=1;
	}
	else if(ifs>0)
	{
		//
		// tx99
		//
        Tx100Packet = 0;
		Broadcast=1;
	}
	else
	{
		//
		// regular
		//
        Tx100Packet = 0;
	}
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
	if(!Broadcast && !DeafMode)
	{
		//
		// Make rx descriptor
		//
		DescriptorLinkRxLoopCreate(MQUEUE);
	}
    //
    // calculate the total number of packets we expect to send.
    // and then figure out how many we should do in each batch.
    // if the number of packets is small, we may be able to do them in one batch.
    //
	if(PacketMany>0)
	{
		LinkTxMany=PacketMany*RateMany*(AggregateMany+AggregateBar);
		if(PacketMany>0 && LinkTxMany<=MQUEUE)
		{
			LinkTxBatchMany=LinkTxMany;                         // do in one batch
			LinkTxDescriptorMany=LinkTxMany;
		}
		else
		{
			LinkTxBatchMany=MQUEUE/2;		                    // do in two or more batches
			LinkTxBatchMany/=(AggregateMany+AggregateBar);		// force complete Aggregate into a batch
			LinkTxBatchMany*=(AggregateMany+AggregateBar);	
			LinkTxDescriptorMany=2*LinkTxBatchMany;
		}
	}
	else
	{
		LinkTxBatchMany=MQUEUE/2;		                    // do in two or more batches
		LinkTxBatchMany/=(AggregateMany+AggregateBar);		// force complete Aggregate into a batch
		LinkTxBatchMany*=(AggregateMany+AggregateBar);	
		LinkTxDescriptorMany=2*LinkTxBatchMany;
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
	if(LinkTxAggregateStatus || AggregateMany<=1 || PacketMany<=0)
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
    UserPrint("TxMany=%d TxStatusMany=%d TxBatchMany=%d TxDescriptorMany=%d\n",LinkTxMany,LinkTxStatusMany,LinkTxBatchMany,LinkTxDescriptorMany);
    //
	// create the required number of descriptors
	//
    LinkTxLoopDescriptor[0] = MemoryLayout(LinkTxDescriptorMany * TxDescriptorSize());
    if(LinkTxLoopDescriptor[0]==0) 
	{
        UserPrint("txDataSetup: unable to allocate client memory for %d control descriptors\n",LinkTxDescriptorMany);
        return -1;
    }
	for(it=0; it<LinkTxDescriptorMany; it++)
	{
		LinkTxLoopDescriptor[it]=LinkTxLoopDescriptor[0]+it*TxDescriptorSize();
	}
    //
    // make status descriptors
    //
    // for pre-Osprey chips, these pointers point to the regular tx descriptor
    //
	if(LinkTxStatusLoopCreate(LinkTxDescriptorMany))
	{
		return -1;
	}
	//
	// use PN9 data if no pattern is specified
	//
	UserPrint("npattern=%d pattern=%x\n",npattern,pattern);
	if(npattern<=0)
	{
		pattern=PN9Data;
		npattern=sizeof(PN9Data);
	}
	UserPrint("npattern=%d pattern=%x\n",npattern,pattern);
 		ptr=(unsigned char *)pattern;
		if(ptr!=0)
		{
			UserPrint("pattern: ");
			for(it=0; it<npattern; it++)
			{
				UserPrint("%2x ",ptr[it]);
			}
			UserPrint("\n");
		}
    //
	// setup the transmit packets
	//
    if(AggregateMany>1)
    {
	    for(it=0; it<AggregateMany; it++)
	    {
		    LinkTxAggregatePacketCreate(dataBodyLength, pattern, npattern,
			    bssid, source, duse, Tx100Packet, wepEnable, it, AggregateBar);              
		    if(LinkTxLoopBuffer(it)==0) 
		    {
			    UserPrint("txDataSetup: unable to allocate memory for packet %d\n",it);
			    return -1;
		    }
	    }
        //
	    // setup the BAR packet
	    //
        if(AggregateBar>0)
	    {
		    LinkTxBarCreate(source, duse);              
		    if(LinkTxBarBuffer==0) 
		    {
			    UserPrint("txDataSetup: unable to allocate memory for bar packet\n");
			    return -1;
		    }
	    }
    }
    else
    {
		LinkTxPacketCreate(dataBodyLength,pattern,npattern,
			bssid, source, duse, Tx100Packet,wepEnable);              
		if(LinkTxLoopBuffer(0)==0) 
		{
			UserPrint("txDataSetup: unable to allocate memory for packet\n");
			return -1;
		}
    }

    DeviceTransmitRetryLimit(dcuIndex,retries);	

	//
	// regular mode
	//
	if(ifs<0)
	{
		DeviceTransmitRegularData();
		DeafMode=0;
	}
    //
	// tx99 mode
	//
	else if(ifs>0)
	{
		DeviceTransmitFrameData(ifs);
		DeafMode=1;
	}
	//
	// tx100 mode
	//
	else
	{
		DeviceTransmitContinuousData();
		DeafMode=1;
	}

//#ifdef BADBADBAD	// i don't think this works
	DeviceReceiveDeafMode(DeafMode);
//#endif

	DeviceBssIdSet(bssid);
	DeviceStationIdSet(source);
    //
	// make the first batch of transmit control descriptors, but don't queue them yet
	//
	LinkTxBatchNext(0,0);
	LinkTxBatchNext(LinkTxBatchMany,0);

    return 0;
}


//
// start transmission
//
int DescriptorLinkTxStart()
{
	unsigned int qmask;
	int queueIndex;
	//
	// turn on the reciever so we can get acks
	// WHY DO WE DO THIS FOR tx99 mode????
	//
	if(!DeafMode && !Broadcast)
	{
		DescriptorLinkRxStart(0);
	}
	//
	// enable transmitter
	//
	queueIndex=0;
	qmask=1;
	DeviceTransmitQueueSetup(0,0);
    //
    // enable before because Osprey requires it
    //
    if(LinkTxEnableFirst)
    {
        DeviceTransmitEnable(1);
    }
 
    TxStatusCheck();

	LinkTxBatchQueue(0);
	LinkTxBatchQueue(LinkTxBatchMany);

    TxStatusCheck();
    //
    // enable after because Merlin requires it
    //
    if(!LinkTxEnableFirst)
    {
        DeviceTransmitEnable(1);
    }

    TxStatusCheck();
    UserPrint("tx start done\n");

    TxStatusCheck();

	return 0;
}   


int DescriptorLinkTxOptions()
{
    LinkTxFifo=DeviceTransmitFifo();
    LinkTxSplitDescriptor=DeviceTransmitDescriptorSplit();
    LinkTxAggregateStatus=DeviceTransmitAggregateStatus();
    LinkTxEnableFirst=DeviceTransmitEnableFirst();
	return 0;
}

int DescriptorLinkTxForPAPD(int ChainNum)
{
	PAPDTrainChainNum = ChainNum;
	return 0;
}

