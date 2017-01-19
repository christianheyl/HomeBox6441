



#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>


#include "wlantype.h"
#include "rate_constants.h"

#include "smatch.h"

#include "RxDescriptor.h"
#include "Ar9300RxDescriptor.h"

#define MBUFFER 1024


//
// ar9300 chip. osprey.
//

#ifdef ARCH_BIG_ENDIAN
struct Ar9300RxDescriptor
{
	//0
	unsigned int atheros_id:16;	// must be 0x168c
	unsigned int desc_tx_rx:1;		// must be 0
	unsigned int desc_ctrl_stat:1;	// must be 0	
	unsigned int reserved:5;
	unsigned int rx_priority:1;
	unsigned int desc_length:8;	// must be 0x0c

	//1
	unsigned int rx_rate:8;
	unsigned int rssi_ant02:8;
	unsigned int rssi_ant01:8;
	unsigned int rssi_ant00:8;

	//2
	unsigned int res4:9;
	unsigned int is_h:1;
	unsigned int num_delim:8;
	unsigned int res3:1;
	unsigned int more:1;
	unsigned int data_len:12;

	//3
	unsigned int rcv_timestamp;

	//4
	unsigned int rx_antenna:24;
	unsigned int h_valid:1;
	unsigned int ness:2;
	unsigned int not_sounding:1;
	unsigned int stbc:1;
	unsigned int duplicate:1;
	unsigned int h20_40:1;
	unsigned int gi:1;

	//5
	unsigned int rssi_combined:8;
	unsigned int rssi_ant12:8;
	unsigned int rssi_ant11:8;
	unsigned int rssi_ant10:8;

#ifdef UNUSED
	I think the order of these fields is backwards. That would mean they're
	backwards up above in the other endian code too. 
	This is an experiment to confirm. 11/29/2009 th

	//6
    unsigned int evm0:8,evm1:8,evm2:8,evm3:8;

	//7
    unsigned int evm4:8,evm5:8,evm6:8,evm7:8;

	//8
    unsigned int evm8:8,evm9:8,evm10:8,evm11:8;

	//9
    unsigned int evm12:8,evm13:8,evm14:8,evm15:8;

    //10
    unsigned int evm16:16,evm17:8,res9:8;
#else

	//6
    unsigned int evm3:8,evm2:8,evm1:8,evm0:8;

	//7
    unsigned int evm7:8,evm6:8,evm5:8,evm4:8;

	//8
    unsigned int evm11:8,evm10:8,evm9:8,evm8:8;

	//9
    unsigned int evm15:8,evm14:8,evm13:8,evm12:8;

    //10
    unsigned int res9:16,evm17:8,evm16:8;
#endif

	//11
	unsigned int key_miss:1;
	unsigned int decrypt_busy_err:1;
	unsigned int first_agg:1;
	unsigned int hi_rx_chain:1;
	unsigned int res8:9;
	unsigned int post_delim_crc_err:1;
	unsigned int aggregate:1;
	unsigned int more_agg:1;
	unsigned int key_idx:7;
	unsigned int key_idx_valid:1;
	unsigned int apsd_trig:1;
	unsigned int pre_delim_crc_err:1;
	unsigned int mic_error:1;
	unsigned int phy_err:1;
	unsigned int decrypt_crc_err:1;
	unsigned int crc_error:1;
	unsigned int frame_rx_ok:1;
	unsigned int done:1;
};
#else
// for little endian architecture
struct Ar9300RxDescriptor
{
	//0
	unsigned int desc_length:8;	// must be 0x0c
    unsigned int rx_priority:1;
    unsigned int reserved:5;
    unsigned int desc_ctrl_stat:1;	// must be 0	
    unsigned int desc_tx_rx:1;		// must be 0
	unsigned int atheros_id:16;	// must be 0x168c

	//1
	unsigned int rssi_ant00:8;
	unsigned int rssi_ant01:8;
	unsigned int rssi_ant02:8;
	unsigned int rx_rate:8;

	//2
	unsigned int data_len:12;
	unsigned int more:1;
	unsigned int res3:1;
	unsigned int num_delim:8;
    unsigned int is_h:1;
	unsigned int res4:9;

	//3
	unsigned int rcv_timestamp;

	//4
	unsigned int gi:1;
	unsigned int h20_40:1;
	unsigned int duplicate:1;
    unsigned int stbc:1;
    unsigned int not_sounding:1;
    unsigned int ness:2;
    unsigned int h_valid:1;
	unsigned int rx_antenna:24;

	//5
	unsigned int rssi_ant10:8;
	unsigned int rssi_ant11:8;
	unsigned int rssi_ant12:8;
	unsigned int rssi_combined:8;

#ifdef UNUSED
	I think the order of these fields is backwards. That would mean they're
	backwards up above in the other endian code too. 
	This is an experiment to confirm. 11/29/2009 th

	//6
    unsigned int evm3:8,evm2:8,evm1:8,evm0:8;

	//7
    unsigned int evm7:8,evm6:8,evm5:8,evm4:8;

	//8
    unsigned int evm11:8,evm10:8,evm9:8,evm8:8;

	//9
    unsigned int evm15:8,evm14:8,evm13:8,evm12:8;

    //10
    unsigned int res9:16,evm17:8,evm16:8;
#else
	//6
    unsigned int evm0:8,evm1:8,evm2:8,evm3:8;

	//7
    unsigned int evm4:8,evm5:8,evm6:8,evm7:8;

	//8
    unsigned int evm8:8,evm9:8,evm10:8,evm11:8;

	//9
    unsigned int evm12:8,evm13:8,evm14:8,evm15:8;

    //10
    unsigned int evm16:8,evm17:8,res9:16;
#endif

	//11
	unsigned int done:1;
	unsigned int frame_rx_ok:1;
	unsigned int crc_error:1;
	unsigned int decrypt_crc_err:1;
	unsigned int phy_err:1;
	unsigned int mic_error:1;
	unsigned int pre_delim_crc_err:1;
	unsigned int apsd_trig:1;
	unsigned int key_idx_valid:1;
	unsigned int key_idx:7;
	unsigned int more_agg:1;
	unsigned int aggregate:1;
	unsigned int post_delim_crc_err:1;
	unsigned int res8:9;
    unsigned int hi_rx_chain:1;
    unsigned int first_agg:1;
	unsigned int decrypt_busy_err:1;
	unsigned int key_miss:1;
};
#endif

//
// print all of the fields of the descriptor
//
AR9300DLLSPEC int Ar9300RxDescriptorPrint(void *block, char *buffer, int max)
{
	struct Ar9300RxDescriptor *dr=(struct Ar9300RxDescriptor *)block;
	int lc, nc;

	lc=SformatOutput(buffer,max-1,"`%x %x %x %x %x %x`", 
	    dr->atheros_id,
        dr->desc_tx_rx,
        dr->desc_ctrl_stat,		
        dr->reserved,
        dr->rx_priority,
		dr->desc_length
 		);

	nc=SformatOutput(&buffer[lc],max-lc-1,"%x %x %x %x`", 
		dr->rx_rate,
		dr->rssi_ant02, 
		dr->rssi_ant01, 
		dr->rssi_ant00 
		);
	if(nc>0)
	{
		lc+=nc;
	}

	nc=SformatOutput(&buffer[lc],max-lc-1,"%x %x %x %x %x %x`", 
		dr->res4,
		dr->is_h, 
		dr->num_delim, 
		dr->res3, 
		dr->more, 
		dr->data_len 
		);
	if(nc>0)
	{
		lc+=nc;
	}

	nc=SformatOutput(&buffer[lc],max-lc-1,"%x`", dr->rcv_timestamp);
	if(nc>0)
	{
		lc+=nc;
	}

	nc=SformatOutput(&buffer[lc],max-lc-1,"%x %x %x %x %x %x %x %x`", 
		dr->rx_antenna,
		dr->h_valid, 
		dr->ness, 
        dr->not_sounding, 
		dr->stbc,
		dr->duplicate, 
		dr->h20_40, 
		dr->gi 
 		);
	if(nc>0)
	{
		lc+=nc;
	}

	nc=SformatOutput(&buffer[lc],max-lc-1,"%x %x %x %x`", 
		dr->rssi_combined,
		dr->rssi_ant12, 
		dr->rssi_ant11, 
		dr->rssi_ant10 
		);
	if(nc>0)
	{
		lc+=nc;
	}

	nc=SformatOutput(&buffer[lc],max-lc-1,"%x %x %x %x`", dr->evm0,dr->evm1,dr->evm2,dr->evm3);
	if(nc>0)
	{
		lc+=nc;
	}

	nc=SformatOutput(&buffer[lc],max-lc-1,"%x %x %x %x`", dr->evm4,dr->evm5,dr->evm6,dr->evm7);
	if(nc>0)
	{
		lc+=nc;
	}

	nc=SformatOutput(&buffer[lc],max-lc-1,"%x %x %x %x`", dr->evm8,dr->evm9,dr->evm10,dr->evm11);
	if(nc>0)
	{
		lc+=nc;
	}

	nc=SformatOutput(&buffer[lc],max-lc-1,"%x %x %x %x`", dr->evm12,dr->evm13,dr->evm14,dr->evm15);
	if(nc>0)
	{
		lc+=nc;
	}

	nc=SformatOutput(&buffer[lc],max-lc-1,"%x %x %x`", dr->evm16, dr->evm17, dr->res9);
	if(nc>0)
	{
		lc+=nc;
	}

	nc=SformatOutput(&buffer[lc],max-lc-1,"%x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x`", 
		dr->key_miss,
        dr->decrypt_busy_err, 
		dr->first_agg,
		dr->hi_rx_chain, 
		dr->res8, 
		dr->post_delim_crc_err, 
		dr->aggregate, 
		dr->more_agg, 
		dr->key_idx, 
		dr->key_idx_valid, 
		dr->apsd_trig, 
		dr->pre_delim_crc_err, 
		dr->mic_error, 
		dr->phy_err, 
		dr->decrypt_crc_err, 
		dr->crc_error, 
		dr->frame_rx_ok, 
		dr->done 
		);
	if(nc>0)
	{
		lc+=nc;
	}

	return lc;
}

AR9300DLLSPEC unsigned int Ar9300RxDescriptorLinkPtr(void *block)
{
	struct Ar9300RxDescriptor *dr=(struct Ar9300RxDescriptor *)block;

	if(dr!=0)
	{
		return dr->desc_length+dr->data_len;		// offset to next descriptor, not absolute address
	}
	return 0;
}

AR9300DLLSPEC unsigned int Ar9300RxDescriptorBufPtr(void *block)
{
	struct Ar9300RxDescriptor *dr=(struct Ar9300RxDescriptor *)block;

	if(dr!=0)
	{
		return dr->desc_length;					// offset to data, not absolute address
	}
	return 0;
}


AR9300DLLSPEC unsigned int Ar9300RxDescriptorBufLen(void *block)
{
	struct Ar9300RxDescriptor *dr=(struct Ar9300RxDescriptor *)block;

	if(dr!=0)
	{
		return dr->data_len;		
	}
	return 0;
}

AR9300DLLSPEC unsigned int Ar9300RxDescriptorRssiCombined(void *block)
{
	struct Ar9300RxDescriptor *dr=(struct Ar9300RxDescriptor *)block;

	if(dr!=0)
	{
		return dr->rssi_combined;
	}
	return 0;
}

AR9300DLLSPEC unsigned int Ar9300RxDescriptorRssiAnt00(void *block)
{
	struct Ar9300RxDescriptor *dr=(struct Ar9300RxDescriptor *)block;

	if(dr!=0)
	{
		return dr->rssi_ant00;
	}
	return 0;
}

AR9300DLLSPEC unsigned int Ar9300RxDescriptorRssiAnt01(void *block)
{
	struct Ar9300RxDescriptor *dr=(struct Ar9300RxDescriptor *)block;

	if(dr!=0)
	{
		return dr->rssi_ant01;
	}
	return 0;
}

AR9300DLLSPEC unsigned int Ar9300RxDescriptorRssiAnt02(void *block)
{
	struct Ar9300RxDescriptor *dr=(struct Ar9300RxDescriptor *)block;

	if(dr!=0)
	{
		return dr->rssi_ant02;
	}
	return 0;
}

AR9300DLLSPEC unsigned int Ar9300RxDescriptorRssiAnt10(void *block)
{
	struct Ar9300RxDescriptor *dr=(struct Ar9300RxDescriptor *)block;

	if(dr!=0)
	{
		return dr->rssi_ant10;
	}
	return 0;
}

AR9300DLLSPEC unsigned int Ar9300RxDescriptorRssiAnt11(void *block)
{
	struct Ar9300RxDescriptor *dr=(struct Ar9300RxDescriptor *)block;

	if(dr!=0)
	{
		return dr->rssi_ant11;
	}
	return 0;
}

AR9300DLLSPEC unsigned int Ar9300RxDescriptorRssiAnt12(void *block)
{
	struct Ar9300RxDescriptor *dr=(struct Ar9300RxDescriptor *)block;

	if(dr!=0)
	{
		return dr->rssi_ant12;
	}
	return 0;
}

AR9300DLLSPEC unsigned int Ar9300RxDescriptorRxRate(void *block)
{
	struct Ar9300RxDescriptor *dr=(struct Ar9300RxDescriptor *)block;

	if(dr!=0)
	{
        return descRate2RateIndex(dr->rx_rate, (unsigned char)dr->h20_40);
//		return dr->rx_rate;
	}
	return 0;
}

AR9300DLLSPEC unsigned int Ar9300RxDescriptorDataLen(void *block)
{
	struct Ar9300RxDescriptor *dr=(struct Ar9300RxDescriptor *)block;

	if(dr!=0)
	{
		return dr->data_len;
	}
	return 0;
}

AR9300DLLSPEC unsigned char Ar9300RxDescriptorMore(void *block)
{
	struct Ar9300RxDescriptor *dr=(struct Ar9300RxDescriptor *)block;

	if(dr!=0)
	{
		return (unsigned char)dr->more;
	}
	return 0;
}

AR9300DLLSPEC unsigned int Ar9300RxDescriptorNumDelim(void *block)
{
	struct Ar9300RxDescriptor *dr=(struct Ar9300RxDescriptor *)block;

	if(dr!=0)
	{
		return dr->num_delim;
	}
	return 0;
}

AR9300DLLSPEC unsigned int Ar9300RxDescriptorRcvTimestamp(void *block)
{
	struct Ar9300RxDescriptor *dr=(struct Ar9300RxDescriptor *)block;

	if(dr!=0)
	{
		return dr->rcv_timestamp;
	}
	return 0;
}

AR9300DLLSPEC unsigned char Ar9300RxDescriptorGi(void *block)
{
	struct Ar9300RxDescriptor *dr=(struct Ar9300RxDescriptor *)block;

	if(dr!=0)
	{
		return (unsigned char)dr->gi;
	}
	return 0;
}


AR9300DLLSPEC unsigned char Ar9300RxDescriptorH2040(void *block)
{
	struct Ar9300RxDescriptor *dr=(struct Ar9300RxDescriptor *)block;

	if(dr!=0)
	{
		return (unsigned char)dr->h20_40;
	}
	return 0;
}

AR9300DLLSPEC unsigned char Ar9300RxDescriptorDuplicate(void *block)
{
	struct Ar9300RxDescriptor *dr=(struct Ar9300RxDescriptor *)block;

	if(dr!=0)
	{
		return (unsigned char)dr->duplicate;
	}
	return 0;
}

AR9300DLLSPEC unsigned int Ar9300RxDescriptorRxAntenna(void *block)
{
	struct Ar9300RxDescriptor *dr=(struct Ar9300RxDescriptor *)block;

	if(dr!=0)
	{
		return dr->rx_antenna;
	}
	return 0;
}


AR9300DLLSPEC double Ar9300RxDescriptorEvm0(void *block)
{
	struct Ar9300RxDescriptor *dr=(struct Ar9300RxDescriptor *)block;
	double evm;

	if(dr!=0)
	{
		if(dr->h20_40 == 1) 
		{
            if(dr->evm0!=80 && dr->evm5!=80)
            {
			    evm = ((double)(dr->evm0 + dr->evm1 + dr->evm2 + dr->evm3 + dr->evm4 + dr->evm5))/6.0;
            }
            else
            {
                evm=0;
            }
		} 
		else 
		{
            if(dr->evm0!=80)
            {
			    evm = ((double)(dr->evm0 + dr->evm1 + dr->evm2 + dr->evm3))/4.0;
            }
            else
            {
                evm=0;
            }
		}
		return evm;
	}
	return 0;
}

AR9300DLLSPEC double Ar9300RxDescriptorEvm1(void *block)
{
	struct Ar9300RxDescriptor *dr=(struct Ar9300RxDescriptor *)block;
	double evm;

	if(dr!=0)
	{
		if(dr->h20_40 == 1) 
		{
            if(dr->evm6!=80 && dr->evm11!=80)
            {
    			evm = ((double)(dr->evm6 + dr->evm7 + dr->evm8 + dr->evm9 + dr->evm10 + dr->evm11))/6.0;
            }
            else
            {
                evm=0;
            }
		} 
		else 
		{
            if(dr->evm4!=80)
            {
    			evm = ((double)(dr->evm4 + dr->evm5 + dr->evm6 + dr->evm7))/4.0;
            }
            else
            {
                evm=0;
            }
		}
		return evm;
	}
	return 0;
}

AR9300DLLSPEC double Ar9300RxDescriptorEvm2(void *block)
{
	struct Ar9300RxDescriptor *dr=(struct Ar9300RxDescriptor *)block;
	double evm;

	if(dr!=0)
	{
		if(dr->h20_40 == 1) 
		{
            if(dr->evm12!=80 && dr->evm17!=80)
            {
    			evm = ((double)(dr->evm12 + dr->evm13 + dr->evm14 + dr->evm15 + dr->evm16 + dr->evm17))/6.0;
            }
            else
            {
                evm=0;
            }
		} 
		else 
		{
            if(dr->evm8!=80)
            {
    			evm = ((double)(dr->evm8 + dr->evm9 + dr->evm10 + dr->evm11))/4.0;
            }
            else
            {
                evm=0;
            }
		}
		return evm;
	}
	return 0;
}

AR9300DLLSPEC unsigned char Ar9300RxDescriptorDone(void *block)
{
	struct Ar9300RxDescriptor *dr=(struct Ar9300RxDescriptor *)block;

	if(dr!=0)
	{
		return (unsigned char)dr->done;
	}
	return 0;
}

AR9300DLLSPEC unsigned char Ar9300RxDescriptorFrameRxOk(void *block)
{
	struct Ar9300RxDescriptor *dr=(struct Ar9300RxDescriptor *)block;

	if(dr!=0)
	{
		return (unsigned char)dr->frame_rx_ok;
	}
	return 0;
}

AR9300DLLSPEC unsigned char Ar9300RxDescriptorCrcError(void *block)
{
	struct Ar9300RxDescriptor *dr=(struct Ar9300RxDescriptor *)block;

	if(dr!=0)
	{
		return (unsigned char)dr->crc_error;
	}
	return 0;
}

AR9300DLLSPEC unsigned char Ar9300RxDescriptorDecryptCrcErr(void *block)
{
	struct Ar9300RxDescriptor *dr=(struct Ar9300RxDescriptor *)block;

	if(dr!=0)
	{
		return (unsigned char)dr->decrypt_crc_err;
	}
	return 0;
}

AR9300DLLSPEC unsigned char Ar9300RxDescriptorPhyErr(void *block)
{
	struct Ar9300RxDescriptor *dr=(struct Ar9300RxDescriptor *)block;

	if(dr!=0)
	{
		return (unsigned char)dr->phy_err;
	}
	return 0;
}

AR9300DLLSPEC unsigned char Ar9300RxDescriptorMicError(void *block)
{
	struct Ar9300RxDescriptor *dr=(struct Ar9300RxDescriptor *)block;

	if(dr!=0)
	{
		return (unsigned char)dr->mic_error;
	}
	return 0;
}

AR9300DLLSPEC unsigned char Ar9300RxDescriptorPreDelimCrcErr(void *block)
{
	struct Ar9300RxDescriptor *dr=(struct Ar9300RxDescriptor *)block;

	if(dr!=0)
	{
		return (unsigned char)dr->pre_delim_crc_err;
	}
	return 0;
}

AR9300DLLSPEC unsigned char Ar9300RxDescriptorKeyIdxValid(void *block)
{
	struct Ar9300RxDescriptor *dr=(struct Ar9300RxDescriptor *)block;

	if(dr!=0)
	{
		return (unsigned char)dr->key_idx_valid;
	}
	return 0;
}

AR9300DLLSPEC unsigned int Ar9300RxDescriptorKeyIdx(void *block)
{
	struct Ar9300RxDescriptor *dr=(struct Ar9300RxDescriptor *)block;

	if(dr!=0)
	{
		return dr->key_idx;
	}
	return 0;
}

AR9300DLLSPEC unsigned char Ar9300RxDescriptorFirstAgg(void *block)
{
	struct Ar9300RxDescriptor *dr=(struct Ar9300RxDescriptor *)block;

	if(dr!=0)
	{
		return (unsigned char)dr->first_agg;
	}
	return 0;
}

AR9300DLLSPEC unsigned char Ar9300RxDescriptorMoreAgg(void *block)
{
	struct Ar9300RxDescriptor *dr=(struct Ar9300RxDescriptor *)block;

	if(dr!=0)
	{
		return (unsigned char)dr->more_agg;
	}
	return 0;
}

AR9300DLLSPEC unsigned char Ar9300RxDescriptorAggregate(void *block)
{
	struct Ar9300RxDescriptor *dr=(struct Ar9300RxDescriptor *)block;

	if(dr!=0)
	{
		return (unsigned char)dr->aggregate;
	}
	return 0;
}

AR9300DLLSPEC unsigned char Ar9300RxDescriptorPostDelimCrcErr(void *block)
{
	struct Ar9300RxDescriptor *dr=(struct Ar9300RxDescriptor *)block;

	if(dr!=0)
	{
		return (unsigned char)dr->post_delim_crc_err;
	}
	return 0;
}

AR9300DLLSPEC unsigned char Ar9300RxDescriptorDecryptBusyErr(void *block)
{
	struct Ar9300RxDescriptor *dr=(struct Ar9300RxDescriptor *)block;

	if(dr!=0)
	{
		return (unsigned char)dr->decrypt_busy_err;
	}
	return 0;
}

AR9300DLLSPEC unsigned char Ar9300RxDescriptorKeyMiss(void *block)
{
	struct Ar9300RxDescriptor *dr=(struct Ar9300RxDescriptor *)block;

	if(dr!=0)
	{
		return (unsigned char)dr->key_miss;
	}
	return 0;
}
	
//
// setup a descriptor with the standard required fields
//
AR9300DLLSPEC int Ar9300RxDescriptorSetup(void *block, 
	unsigned int link_ptr, unsigned int buf_ptr, unsigned int buf_len)
{
	struct Ar9300RxDescriptor *dr=(struct Ar9300RxDescriptor *)block;
    memset(dr, 0xf0, sizeof(struct Ar9300RxDescriptor));
#ifdef UNUSED
	//
	// initialize the descriptors:
	//    buffer size
	//    buffer address
	//    next descriptor pointer 
	//
    dr->buf_len = buf_len;
    dr->buf_ptr = buf_ptr;
    dr->link_ptr = link_ptr;
#endif
	return 0;
}

//
// reset the descriptor so that it can be used again
//
AR9300DLLSPEC int Ar9300RxDescriptorReset(void *block)
{  
	struct Ar9300RxDescriptor *dr=(struct Ar9300RxDescriptor *)block;
#ifdef UNUSED
	unsigned int link_ptr, buf_ptr, buf_len;

    buf_len=dr->buf_len;
	buf_ptr=dr->buf_ptr;
	link_ptr=dr->link_ptr;
    Ar9300RxDescriptorSetup(block, link_ptr, buf_ptr, buf_len);
#else
    memset(dr, 0xf0, sizeof(struct Ar9300RxDescriptor));
#endif
	return 0;
}

//
// return the size of a descriptor 
//
AR9300DLLSPEC int Ar9300RxDescriptorSize()
{
   return sizeof(struct Ar9300RxDescriptor);
}

//
// return 1 if the descriptor contains specral scan data
//
AR9300DLLSPEC int Ar9300RxDescriptorSpectralScan(void *block)
{  
	int rx_phyerr_status;
	int rx_phyerr_code;

	rx_phyerr_status=RxDescriptorPhyErr(block);
	rx_phyerr_code=(RxDescriptorKeyIdx(block) << 1) + RxDescriptorKeyIdxValid(block);
	return (rx_phyerr_status == 1 && rx_phyerr_code == 38);
}


#ifdef UNUSED
//
// copy the descriptor from application memory to the shared memory
//
static void Ar9300RxDescriptorWrite(void *block, unsigned int physical)
{
    OSmemWrite(physical, block, sizeof(Ar9300RxDescriptor));
}

//
// copy the descriptor from the shared memory to application memory
//
static void Ar9300RxDescriptorRead(void *block, unsigned int physical, void (*read)()
{
    LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];

    pLibDev->devMap.OSmemRead(devNum, physical,
                block, sizeof(Ar9300RxDescriptor));

    OSmemRead(physical, block, );
}
#endif


static struct _RxDescriptorFunction _Ar9300RxDescriptor=
{
    Ar9300RxDescriptorLinkPtr,
    Ar9300RxDescriptorBufPtr,
    Ar9300RxDescriptorBufLen,
    0,										// Ar9300RxDescriptorIntReq,
    Ar9300RxDescriptorRssiCombined,
    Ar9300RxDescriptorRssiAnt00,
    Ar9300RxDescriptorRssiAnt01,
    Ar9300RxDescriptorRssiAnt02,
    Ar9300RxDescriptorRssiAnt10,
    Ar9300RxDescriptorRssiAnt11,
    Ar9300RxDescriptorRssiAnt12,
    Ar9300RxDescriptorRxRate,
    Ar9300RxDescriptorDataLen,
    Ar9300RxDescriptorMore,
    Ar9300RxDescriptorNumDelim,
    Ar9300RxDescriptorRcvTimestamp,
    Ar9300RxDescriptorGi,
    Ar9300RxDescriptorH2040,
    Ar9300RxDescriptorDuplicate,
    Ar9300RxDescriptorRxAntenna,
    Ar9300RxDescriptorEvm0,
    Ar9300RxDescriptorEvm1,
    Ar9300RxDescriptorEvm2,
    Ar9300RxDescriptorDone,
    Ar9300RxDescriptorFrameRxOk,
    Ar9300RxDescriptorCrcError,
    Ar9300RxDescriptorDecryptCrcErr,
    Ar9300RxDescriptorPhyErr,
    Ar9300RxDescriptorMicError,
    Ar9300RxDescriptorPreDelimCrcErr,
    Ar9300RxDescriptorKeyIdxValid,
    Ar9300RxDescriptorKeyIdx,
    Ar9300RxDescriptorMoreAgg,
    Ar9300RxDescriptorFirstAgg,
    Ar9300RxDescriptorAggregate,
    Ar9300RxDescriptorPostDelimCrcErr,
    Ar9300RxDescriptorDecryptBusyErr,
    Ar9300RxDescriptorKeyMiss,
    Ar9300RxDescriptorSetup,
    Ar9300RxDescriptorReset,
    Ar9300RxDescriptorSize,
    Ar9300RxDescriptorPrint,
	Ar9300RxDescriptorSpectralScan,
#ifdef UNUSED
    Ar9300RxDescriptorWrite,
    Ar9300RxDescriptorRead,
#endif
};


int Ar9300RxDescriptorFunctionSelect()
{
	RxDescriptorFunctionSelect(&_Ar9300RxDescriptor);
	//
	// later check some of the pointers to make sure there are valid functions
	//
	return 0;
}

