
#include <stdio.h>
#include <stdlib.h>
#include <string.h>



#include "wlanproto.h"

#include "AnwiDriverInterface.h"
#include "smatch.h"
#include "UserPrint.h"
#include "TimeMillisecond.h"


#define MBUFFER 1024


#include "TimeMillisecond.h"
//#include "ParameterSelect.h"
//#include "Card.h"
#include "DescriptorLinkRx.h"
#include "DescriptorLinkTxPacket.h"




//
// The 802.11 messages
// if aggregation is enabled, there is a different message for each framein the aggregated group
//
static int _LinkTxLoopBufferSize[MAGGREGATE];
static unsigned int _LinkTxLoopBuffer[MAGGREGATE];

//
// the location and size of the 802.11 block acknowledge request (BAR)  message
//
static int _LinkTxBarBufferSize=20;
static unsigned int _LinkTxBarBuffer;


int LinkTxLoopBuffer(int aggregate)
{
    if(aggregate>=0 && aggregate<MAGGREGATE)
    {
        return _LinkTxLoopBuffer[aggregate];
    }
    return -1;
}


int LinkTxLoopBufferSize(int aggregate)
{
    if(aggregate>=0 && aggregate<MAGGREGATE)
    {
        return _LinkTxLoopBufferSize[aggregate];
    }
    return -1;
}


int LinkTxBarBuffer()
{
    return _LinkTxBarBuffer;
}


int LinkTxBarBufferSize()
{
    return _LinkTxBarBufferSize;
}


//
// creates a block acknowledgement request (BAR) 802.11 message.
// return the size of the message
//
int LinkTxBarCreate(unsigned char *source, unsigned char *destination)
{
//	int it;
//	int nwrite;
//    unsigned int pPktAddr;             
    unsigned int pPkt[24];
    WLAN_BAR_CONTROL_MAC_HEADER  *pPktHeader;
//	unsigned char *pBody;
//	int pPktSize;
    //
    // how big is the packet?
	//
    pPktHeader=(WLAN_BAR_CONTROL_MAC_HEADER *)pPkt;
	_LinkTxBarBufferSize = sizeof(WLAN_BAR_CONTROL_MAC_HEADER);		// should be 20;
    //
	// create the packet Header, all fields are listed here to enable easy changing
	//
    pPktHeader->frameControl.protoVer = 0;
    pPktHeader->frameControl.fType = FRAME_CTRL;
    pPktHeader->frameControl.fSubtype = SUBT_QOS;
    pPktHeader->frameControl.ToDS = 0;
    pPktHeader->frameControl.FromDS = 0;
    pPktHeader->frameControl.moreFrag = 0;
    pPktHeader->frameControl.retry = 0;
    pPktHeader->frameControl.pwrMgt = 0;
    pPktHeader->frameControl.moreData = 0;
    pPktHeader->frameControl.wep = 0;
    pPktHeader->frameControl.order = 0;
// Swap the bytes of the frameControl
#if 0
//#ifdef ARCH_BIG_ENDIAN
    {
        unsigned short* ptr;

        ptr = (unsigned short *)(&(pPktHeader->frameControl));
        *ptr = btol_s(*ptr);
    }
#endif
//  pPktHeader->durationNav.clearBit = 0;
//  pPktHeader->durationNav.duration = 0;
    pPktHeader->durationNav = 0;  // aman redefined the struct to be just unsigned short
// Swap the bytes of the durationNav
#if 0
//#ifdef ARCH_BIG_ENDIAN
    {
        unsigned short* ptr;

        ptr = (unsigned short *)(&(pPktHeader->durationNav));
        *ptr = btol_s(*ptr);
    }
#endif
	//
	// set the address fields
	//
    memcpy(pPktHeader->address1.octets, destination, WLAN_MAC_ADDR_SIZE);
    memcpy(pPktHeader->address2.octets, source, WLAN_MAC_ADDR_SIZE);
	//
	// set the bar control 
	// should define the fields in a bit map
	//
	pPktHeader->barControl = 0x7ff00005;
	//
	// reserve a block in the shared memory
	//
    _LinkTxBarBuffer = MemoryLayout(_LinkTxBarBufferSize);
    if(_LinkTxBarBuffer==0) 
	{
        UserPrint("createTransmitPacket: unable to allocate memory for BAR buffer\n");
        return 0;
    }
    // 
	// write the packet to physical memory
    //
	MyMemoryWrite(_LinkTxBarBuffer, pPkt, _LinkTxBarBufferSize);
//	UserPrint("bar = %x %d\n",_LinkTxBarBuffer,_LinkTxBarBufferSize);

//	for(it=0; it<_LinkTxBarBufferSize; it++)
//	{
//		UserPrint("02x ",pPkt[it]);
//	}
//	UserPrint("\n");

    return _LinkTxBarBufferSize;
}


static void LinkTxPacketBody(unsigned char *body, int blength, unsigned char *fill, int flength)
{
	int nwrite;

    while (blength>0) 
    {
        if(blength > flength) 
		{
            nwrite = flength;
        }
        else 
		{
            nwrite = blength;
        }

        memcpy(body, fill, nwrite);
		blength-=nwrite;
        body += nwrite;
    }
}

#define MPACKET 4096		// maximum packet size


static void LinkTxAggregatePacketHeader(WLAN_QOS_DATA_MAC_HEADER3 *pPktHeader, int wepEnable,
	unsigned char *bssid, unsigned char *source, unsigned char *destination, int agg, int bar)
{
//    unsigned int *pIV;
    //
	// create the packet Header, all fields are listed here to enable easy changing
	//
    pPktHeader->frameControl.protoVer = 0;
    pPktHeader->frameControl.fType = FRAME_DATA;
    pPktHeader->frameControl.fSubtype = SUBT_QOS;
    pPktHeader->frameControl.ToDS = 0;
    pPktHeader->frameControl.FromDS = 0;
    pPktHeader->frameControl.moreFrag = 0;
    pPktHeader->frameControl.retry = 0;
    pPktHeader->frameControl.pwrMgt = 0;
    pPktHeader->frameControl.moreData = 0;
    pPktHeader->frameControl.wep = wepEnable;
    pPktHeader->frameControl.order = 0;
// Swap the bytes of the frameControl
#if 0
//#ifdef ARCH_BIG_ENDIAN
    {
        unsigned short* ptr;

        ptr = (unsigned short *)(&(pPktHeader->frameControl));
        *ptr = btol_s(*ptr);
    }
#endif
//  pPktHeader->durationNav.clearBit = 0;
//  pPktHeader->durationNav.duration = 0;
    pPktHeader->durationNav = 0;  // aman redefined the struct to be just unsigned short
// Swap the bytes of the durationNav
#if 0
//#ifdef ARCH_BIG_ENDIAN
    {
        unsigned short* ptr;

        ptr = (unsigned short *)(&(pPktHeader->durationNav));
        *ptr = btol_s(*ptr);
    }
#endif
	//
	// set the address fields
	//
    memcpy(pPktHeader->address1.octets, destination, WLAN_MAC_ADDR_SIZE);
    memcpy(pPktHeader->address2.octets, source, WLAN_MAC_ADDR_SIZE);
    memcpy(pPktHeader->address3.octets, bssid, WLAN_MAC_ADDR_SIZE);
    //
	// set the sequence number fields
	//
    WLAN_SET_FRAGNUM(pPktHeader->seqControl, 0);
    WLAN_SET_SEQNUM(pPktHeader->seqControl, agg);
#if 0
//#ifdef ARCH_BIG_ENDIAN
    {
        unsigned short* ptr;

        ptr = (unsigned short *)(&(pPktHeader->seqControl));
        *ptr = btol_s(*ptr);
    }
#endif
	// Add qos control field
// WLAN_QOS_DATA_MAC_HEADER3
    if(bar)
    {
	    pPktHeader->qosControl.tid = 6;
	    pPktHeader->qosControl.eosp = 1;
	    pPktHeader->qosControl.ackpolicy = 1;
	    pPktHeader->qosControl.txop = 0;
    }
    else
    {
	    pPktHeader->qosControl.tid = 0;
	    pPktHeader->qosControl.eosp = 0;
	    pPktHeader->qosControl.ackpolicy = 0;
	    pPktHeader->qosControl.txop = 0;
    }



#ifdef UNUSED
    /* fill in the IV if required */
    if (pLibDev->wepEnable && (mdkType == MDK_NORMAL_PKT)) {
        pIV = (unsigned int *)pPkt;

        *pIV = 0;
//        *pIV = (pLibDev->wepKey << 30) | 0x123456;  //fixed IV for now
        *pIV = 0xffffffff;
        pPkt += 4;
        *pPkt = 0xff;
        pPkt++;
        if (pLibDev->wepKey > 3) {
            *pPkt = 0x0;
        }
        else {
            *pPkt = (unsigned char)(pLibDev->wepKey << 6);
        }
        pPkt++;
    }
#endif
}



#define MPACKET 4096		// maximum packet size

/**************************************************************************
* createTransmitPacket - create packet for transmission
*
*/
int LinkTxAggregatePacketCreate(
    unsigned int dataBodyLength,
    unsigned char *dataPattern,
    unsigned int dataPatternLength,
    unsigned char *bssid, unsigned char *source, unsigned char *destination,
	int specialTx100Pkt,
    int wepEnable,
	int agg, int bar)
{
//	int it;
//	int nwrite;
//    unsigned int pPktAddr;             
    unsigned char pPkt[MPACKET];
    WLAN_QOS_DATA_MAC_HEADER3  *pPktHeader;
	unsigned char *pBody;
    //
    // how big is the packet?
	//
	//    802.11 header (not for tx100)
	//    IV (only if WEP enabled)
	//    packet body
	//
    pPktHeader=(WLAN_QOS_DATA_MAC_HEADER3 *)pPkt;
	_LinkTxLoopBufferSize[agg] = dataBodyLength;
	pBody=pPkt;
    if(!specialTx100Pkt) 
	{
        _LinkTxLoopBuffer[agg] += sizeof(WLAN_QOS_DATA_MAC_HEADER3);
        pBody += sizeof(WLAN_QOS_DATA_MAC_HEADER3);
        dataBodyLength -= sizeof(WLAN_QOS_DATA_MAC_HEADER3);
	}
#ifdef UNUSED
    if(wepEnable) 
	{
        _LinkTxLoopBuffer[agg] += (WEP_IV_FIELD + 2);
        pBody += (WEP_IV_FIELD + 2);
    }
#else
	wepEnable=0;
#endif
    //
    // fill in the 802.11 packet header (including the IV field if wep enabled)
	//
	LinkTxAggregatePacketHeader(pPktHeader, wepEnable, bssid, source, destination, agg, bar);
    //
	// fill in the packet body with the specified pattern
	//
	LinkTxPacketBody(pBody,dataBodyLength,dataPattern,dataPatternLength);
	//
	// reserve a block in the shared memory
	//
    _LinkTxLoopBuffer[agg] = MemoryLayout(_LinkTxLoopBufferSize[agg]);
    if(_LinkTxLoopBuffer[agg]==0) 
	{
        UserPrint("createTransmitPacket: unable to allocate memory for transmit buffer\n");
        return 0;
    }
    // 
	// write the packet to physical memory
    //
    MyMemoryWrite(_LinkTxLoopBuffer[agg], (unsigned int *)pPkt, _LinkTxLoopBufferSize[agg]);
//	UserPrint("buffer[%d] = %x %d\n",agg,_LinkTxLoopBuffer[agg],_LinkTxLoopBufferSize[agg]);

    return _LinkTxLoopBufferSize[agg];
}


static void LinkTxPacketHeader(WLAN_DATA_MAC_HEADER3 *pPktHeader, int wepEnable,
	unsigned char *bssid, unsigned char *source, unsigned char *destination, int agg)
{
//    unsigned int *pIV;
    //
	// create the packet Header, all fields are listed here to enable easy changing
	//
    pPktHeader->frameControl.protoVer = 0;
    pPktHeader->frameControl.fType = FRAME_DATA;
    pPktHeader->frameControl.fSubtype = SUBT_DATA;		// SUBT_QOS
    pPktHeader->frameControl.ToDS = 0;
    pPktHeader->frameControl.FromDS = 0;
    pPktHeader->frameControl.moreFrag = 0;
    pPktHeader->frameControl.retry = 0;
    pPktHeader->frameControl.pwrMgt = 0;
    pPktHeader->frameControl.moreData = 0;
    pPktHeader->frameControl.wep = wepEnable;
    pPktHeader->frameControl.order = 0;
// Swap the bytes of the frameControl
#if 0
//#ifdef ARCH_BIG_ENDIAN
    {
        unsigned short* ptr;

        ptr = (unsigned short *)(&(pPktHeader->frameControl));
        *ptr = btol_s(*ptr);
    }
#endif
//  pPktHeader->durationNav.clearBit = 0;
//  pPktHeader->durationNav.duration = 0;
    pPktHeader->durationNav = 0;  // aman redefined the struct to be just unsigned short
// Swap the bytes of the durationNav
#if 0
//#ifdef ARCH_BIG_ENDIAN
    {
        unsigned short* ptr;

        ptr = (unsigned short *)(&(pPktHeader->durationNav));
        *ptr = btol_s(*ptr);
    }
#endif
	//
	// set the address fields
	//
    memcpy(pPktHeader->address1.octets, destination, WLAN_MAC_ADDR_SIZE);
    memcpy(pPktHeader->address2.octets, source, WLAN_MAC_ADDR_SIZE);
    memcpy(pPktHeader->address3.octets, bssid, WLAN_MAC_ADDR_SIZE);
    //
	// set the sequence number fields
	//
    WLAN_SET_FRAGNUM(pPktHeader->seqControl, 0);
    WLAN_SET_SEQNUM(pPktHeader->seqControl, agg);
#if 0
//#ifdef ARCH_BIG_ENDIAN
    {
        unsigned short* ptr;

        ptr = (unsigned short *)(&(pPktHeader->seqControl));
        *ptr = btol_s(*ptr);
    }
#endif


#ifdef UNUSED
    /* fill in the IV if required */
    if (pLibDev->wepEnable && (mdkType == MDK_NORMAL_PKT)) {
        pIV = (unsigned int *)pPkt;

        *pIV = 0;
//        *pIV = (pLibDev->wepKey << 30) | 0x123456;  //fixed IV for now
        *pIV = 0xffffffff;
        pPkt += 4;
        *pPkt = 0xff;
        pPkt++;
        if (pLibDev->wepKey > 3) {
            *pPkt = 0x0;
        }
        else {
            *pPkt = (unsigned char)(pLibDev->wepKey << 6);
        }
        pPkt++;
    }
#endif
}



/**************************************************************************
* createTransmitPacket - create packet for transmission
*
*/
int LinkTxPacketCreate(
    unsigned int dataBodyLength,
    unsigned char *dataPattern,
    unsigned int dataPatternLength,
    unsigned char *bssid, unsigned char *source, unsigned char *destination,
	int specialTx100Pkt,
    int wepEnable)
{
	int it;
    unsigned char pPkt[MPACKET];
    WLAN_DATA_MAC_HEADER3  *pPktHeader;
	unsigned char *pBody;
    int agg;

    agg=0;
    //
    // how big is the packet?
	//
	//    802.11 header (not for tx100)
	//    IV (only if WEP enabled)
	//    packet body
	//
    pPktHeader=(WLAN_DATA_MAC_HEADER3 *)pPkt;
	_LinkTxLoopBufferSize[agg] = dataBodyLength;
	pBody=pPkt;
    if(!specialTx100Pkt) 
	{
        _LinkTxLoopBuffer[agg] += sizeof(WLAN_DATA_MAC_HEADER3);
        pBody += sizeof(WLAN_DATA_MAC_HEADER3);
        dataBodyLength-=sizeof(WLAN_DATA_MAC_HEADER);
	}
#ifdef UNUSED
    if(wepEnable) 
	{
        _LinkTxLoopBuffer[agg] += (WEP_IV_FIELD + 2);
        pBody += (WEP_IV_FIELD + 2);
    }
#else
	wepEnable=0;
#endif
    //
    // fill in the 802.11 packet header (including the IV field if wep enabled)
	//
	LinkTxPacketHeader(pPktHeader, wepEnable, bssid, source, destination, agg);
    //
	// fill in the packet body with the specified pattern
	//
	LinkTxPacketBody(pBody,dataBodyLength,dataPattern,dataPatternLength);
	//
	// reserve a block in the shared memory
	//
    _LinkTxLoopBuffer[agg] = MemoryLayout(_LinkTxLoopBufferSize[agg]);
    if(_LinkTxLoopBuffer[agg]==0) 
	{
        UserPrint("createTransmitPacket: unable to allocate memory for transmit buffer\n");
        return 0;
    }
    // 
	// write the packet to physical memory
    //
    MyMemoryWrite(_LinkTxLoopBuffer[agg], (unsigned int *)pPkt, _LinkTxLoopBufferSize[agg]);
//	UserPrint("buffer[%d] = %x %d\n",agg,_LinkTxLoopBuffer[agg],_LinkTxLoopBufferSize[agg]);

			UserPrint("Packet: ");
			for(it=0; it<60; it++)
			{
				UserPrint("%2x ",pPkt[it]);
			}
			UserPrint("\n");

    return _LinkTxLoopBufferSize[agg];
}


