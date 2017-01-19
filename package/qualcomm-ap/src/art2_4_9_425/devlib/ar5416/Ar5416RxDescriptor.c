



#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>


#include "wlantype.h"
#include "rate_constants.h"

#include "smatch.h"

#include "RxDescriptor.h"
#include "Ar5416RxDescriptor.h"

#define MBUFFER 1024


//
// ar9223 chip. merlin.
//

#ifdef ARCH_BIG_ENDIAN
struct Ar5416RxDescriptor
{
	unsigned long link_ptr;

	unsigned long buf_ptr;

	unsigned long res;

	unsigned long res1:18;
	unsigned long int_req:1;
	unsigned long res0:1;
	unsigned long buf_len:12;

	unsigned long rx_rate:8;
	unsigned long res2:8;
	unsigned long rssi_ant01:8;
	unsigned long rssi_ant00:8;

	unsigned long res4:10;
	unsigned long num_delim:8;
	unsigned long res3:1;
	unsigned long more:1;
	unsigned long data_len:12;

	unsigned long rcv_timestamp;

	unsigned long rx_antenna:24;
	unsigned long res5:5;
	unsigned long duplicate:1;
	unsigned long h20_40:1;
	unsigned long gi:1;

	unsigned long rssi_combined:8;
	unsigned long res6:8;
	unsigned long rssi_ant11:8;
	unsigned long rssi_ant10:8;

    unsigned long evm0:8,evm1:8,evm2:8,evm3:8;

    unsigned long evm4:8,evm5:8,evm6:8,evm7:8;

    unsigned long evm8:8,evm9:8,evm10:8,evm11:8;

	unsigned long key_miss:1;
	unsigned long decrypt_busy_err:1;
	unsigned long res8:11;
	unsigned long post_delim_crc_err:1;
	unsigned long aggregate:1;
	unsigned long more_agg:1;
	unsigned long key_idx:7;
	unsigned long key_idx_valid:1;
	unsigned long res7:1;
	unsigned long pre_delim_crc_err:1;
	unsigned long mic_error:1;
	unsigned long phy_err:1;
	unsigned long decrypt_crc_err:1;
	unsigned long crc_error:1;
	unsigned long frame_rx_ok:1;
	unsigned long done:1;
};

#else
struct Ar5416RxDescriptor
{
	unsigned long link_ptr;

	unsigned long buf_ptr;

	unsigned long res;

	unsigned long buf_len:12;
	unsigned long res0:1;
	unsigned long int_req:1;
	unsigned long res1:18;

	unsigned long rssi_ant00:8;
	unsigned long rssi_ant01:8;
	unsigned long res2:8;
	unsigned long rx_rate:8;

	unsigned long data_len:12;
	unsigned long more:1;
	unsigned long res3:1;
	unsigned long num_delim:8;
	unsigned long res4:10;

	unsigned long rcv_timestamp;

	unsigned long gi:1;
	unsigned long h20_40:1;
	unsigned long duplicate:1;
	unsigned long res5:5;
	unsigned long rx_antenna:24;

	unsigned long rssi_ant10:8;
	unsigned long rssi_ant11:8;
	unsigned long res6:8;
	unsigned long rssi_combined:8;

    unsigned long evm3:8,evm2:8,evm1:8,evm0:8;

    unsigned long evm7:8,evm6:8,evm5:8,evm4:8;

    unsigned long evm11:8,evm10:8,evm9:8,evm8:8;

	unsigned long done:1;
	unsigned long frame_rx_ok:1;
	unsigned long crc_error:1;
	unsigned long decrypt_crc_err:1;
	unsigned long phy_err:1;
	unsigned long mic_error:1;
	unsigned long pre_delim_crc_err:1;
	unsigned long res7:1;
	unsigned long key_idx_valid:1;
	unsigned long key_idx:7;
	unsigned long more_agg:1;
	unsigned long aggregate:1;
	unsigned long post_delim_crc_err:1;
	unsigned long res8:11;
	unsigned long decrypt_busy_err:1;
	unsigned long key_miss:1;
};
#endif

//
// print all of the fields of the descriptor
//
static int Ar5416RxDescriptorPrint(void *block, char *buffer, int max)
{
	struct Ar5416RxDescriptor *dr=(struct Ar5416RxDescriptor *)block;
	int lc, nc;

	lc=SformatOutput(buffer,max-1,"`%x`", dr->link_ptr);

	nc=SformatOutput(&buffer[lc],max-lc-1,"%x`", dr->buf_ptr);
	if(nc>0)
	{
		lc+=nc;
	}

	nc=SformatOutput(&buffer[lc],max-lc-1,"%x`", dr->res);
	if(nc>0)
	{
		lc+=nc;
	}

	nc=SformatOutput(&buffer[lc],max-lc-1,"%x %x %x %x`", 
		dr->res1,
		dr->int_req, 
		dr->res0, 
		dr->buf_len 
		);
	if(nc>0)
	{
		lc+=nc;
	}

	nc=SformatOutput(&buffer[lc],max-lc-1,"%x %x %x %x`", 
		dr->rx_rate,
		dr->res2, 
		dr->rssi_ant01, 
		dr->rssi_ant00 
		);
	if(nc>0)
	{
		lc+=nc;
	}

	nc=SformatOutput(&buffer[lc],max-lc-1,"%x %x %x %x %x`", 
		dr->res4,
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

	nc=SformatOutput(&buffer[lc],max-lc-1,"%x %x %x %x %x`", 
		dr->rx_antenna,
		dr->res5, 
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
		dr->res6, 
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

	nc=SformatOutput(&buffer[lc],max-lc-1,"%x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x`", 
		dr->key_miss,
		dr->decrypt_busy_err, 
		dr->res8, 
		dr->post_delim_crc_err, 
		dr->aggregate, 
		dr->more_agg, 
		dr->key_idx, 
		dr->key_idx_valid, 
		dr->res7, 
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

static unsigned int Ar5416RxDescriptorLinkPtr(void *block)
{
	struct Ar5416RxDescriptor *dr=(struct Ar5416RxDescriptor *)block;

	if(dr!=0)
	{
		return dr->link_ptr;
	}
	return 0;
}

static unsigned int Ar5416RxDescriptorBufPtr(void *block)
{
	struct Ar5416RxDescriptor *dr=(struct Ar5416RxDescriptor *)block;

	if(dr!=0)
	{
		return dr->buf_ptr;
	}
	return 0;
}

static unsigned int Ar5416RxDescriptorBufLen(void *block)
{
	struct Ar5416RxDescriptor *dr=(struct Ar5416RxDescriptor *)block;

	if(dr!=0)
	{
		return dr->buf_len;
	}
	return 0;
}

static unsigned char Ar5416RxDescriptorIntReq(void *block)
{
	struct Ar5416RxDescriptor *dr=(struct Ar5416RxDescriptor *)block;

	if(dr!=0)
	{
		return (unsigned char)dr->int_req;
	}
	return 0;
}

static unsigned int Ar5416RxDescriptorRssiCombined(void *block)
{
	struct Ar5416RxDescriptor *dr=(struct Ar5416RxDescriptor *)block;

	if(dr!=0)
	{
		return dr->rssi_combined;
	}
	return 0;
}

static unsigned int Ar5416RxDescriptorRssiAnt00(void *block)
{
	struct Ar5416RxDescriptor *dr=(struct Ar5416RxDescriptor *)block;

	if(dr!=0)
	{
		return dr->rssi_ant00;
	}
	return 0;
}

static unsigned int Ar5416RxDescriptorRssiAnt01(void *block)
{
	struct Ar5416RxDescriptor *dr=(struct Ar5416RxDescriptor *)block;

	if(dr!=0)
	{
		return dr->rssi_ant01;
	}
	return 0;
}

static unsigned int Ar5416RxDescriptorRssiAnt10(void *block)
{
	struct Ar5416RxDescriptor *dr=(struct Ar5416RxDescriptor *)block;

	if(dr!=0)
	{
		return dr->rssi_ant10;
	}
	return 0;
}

static unsigned int Ar5416RxDescriptorRssiAnt11(void *block)
{
	struct Ar5416RxDescriptor *dr=(struct Ar5416RxDescriptor *)block;

	if(dr!=0)
	{
		return dr->rssi_ant11;
	}
	return 0;
}

static unsigned int Ar5416RxDescriptorRxRate(void *block)
{
	struct Ar5416RxDescriptor *dr=(struct Ar5416RxDescriptor *)block;

	if(dr!=0)
	{
        return descRate2RateIndex(dr->rx_rate, (unsigned char)dr->h20_40);
//		return dr->rx_rate;
	}
	return 0;
}

static unsigned int Ar5416RxDescriptorDataLen(void *block)
{
	struct Ar5416RxDescriptor *dr=(struct Ar5416RxDescriptor *)block;

	if(dr!=0)
	{
		return dr->data_len;
	}
	return 0;
}

static unsigned char Ar5416RxDescriptorMore(void *block)
{
	struct Ar5416RxDescriptor *dr=(struct Ar5416RxDescriptor *)block;

	if(dr!=0)
	{
		return (unsigned char)dr->more;
	}
	return 0;
}

static unsigned int Ar5416RxDescriptorNumDelim(void *block)
{
	struct Ar5416RxDescriptor *dr=(struct Ar5416RxDescriptor *)block;

	if(dr!=0)
	{
		return dr->num_delim;
	}
	return 0;
}

static unsigned int Ar5416RxDescriptorRcvTimestamp(void *block)
{
	struct Ar5416RxDescriptor *dr=(struct Ar5416RxDescriptor *)block;

	if(dr!=0)
	{
		return dr->rcv_timestamp;
	}
	return 0;
}

static unsigned char Ar5416RxDescriptorGi(void *block)
{
	struct Ar5416RxDescriptor *dr=(struct Ar5416RxDescriptor *)block;

	if(dr!=0)
	{
		return (unsigned char)dr->gi;
	}
	return 0;
}


static unsigned char Ar5416RxDescriptorH2040(void *block)
{
	struct Ar5416RxDescriptor *dr=(struct Ar5416RxDescriptor *)block;

	if(dr!=0)
	{
		return (unsigned char)dr->h20_40;
	}
	return 0;
}

static unsigned char Ar5416RxDescriptorDuplicate(void *block)
{
	struct Ar5416RxDescriptor *dr=(struct Ar5416RxDescriptor *)block;

	if(dr!=0)
	{
		return (unsigned char)dr->duplicate;
	}
	return 0;
}

static unsigned int Ar5416RxDescriptorRxAntenna(void *block)
{
	struct Ar5416RxDescriptor *dr=(struct Ar5416RxDescriptor *)block;

	if(dr!=0)
	{
		return dr->rx_antenna;
	}
	return 0;
}

static double Ar5416RxDescriptorEvm0(void *block)
{
	struct Ar5416RxDescriptor *dr=(struct Ar5416RxDescriptor *)block;
	double evm;

	if(dr!=0)
	{
		if(dr->h20_40 == 1) 
		{
            if(dr->evm0!=80 && dr->evm5!=0)
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

static double Ar5416RxDescriptorEvm1(void *block)
{
	struct Ar5416RxDescriptor *dr=(struct Ar5416RxDescriptor *)block;
	double evm;

	if(dr!=0)
	{
		if(dr->h20_40 == 1) 
		{
            if(dr->evm6!=80 && dr->evm11!=0)
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

#ifdef UNUSED
static double Ar5416RxDescriptorEvm2(void *block)
{
	return 0;
}
#endif

static unsigned char Ar5416RxDescriptorDone(void *block)
{
	struct Ar5416RxDescriptor *dr=(struct Ar5416RxDescriptor *)block;

	if(dr!=0)
	{
		return (unsigned char)dr->done;
	}
	return 0;
}

static unsigned char Ar5416RxDescriptorFrameRxOk(void *block)
{
	struct Ar5416RxDescriptor *dr=(struct Ar5416RxDescriptor *)block;

	if(dr!=0)
	{
		return (unsigned char)dr->frame_rx_ok;
	}
	return 0;
}

static unsigned char Ar5416RxDescriptorCrcError(void *block)
{
	struct Ar5416RxDescriptor *dr=(struct Ar5416RxDescriptor *)block;

	if(dr!=0)
	{
		return (unsigned char)dr->crc_error;
	}
	return 0;
}

static unsigned char Ar5416RxDescriptorDecryptCrcErr(void *block)
{
	struct Ar5416RxDescriptor *dr=(struct Ar5416RxDescriptor *)block;

	if(dr!=0)
	{
		return (unsigned char)dr->decrypt_crc_err;
	}
	return 0;
}

static unsigned char Ar5416RxDescriptorPhyErr(void *block)
{
	struct Ar5416RxDescriptor *dr=(struct Ar5416RxDescriptor *)block;

	if(dr!=0)
	{
		return (unsigned char)dr->phy_err;
	}
	return 0;
}

static unsigned char Ar5416RxDescriptorMicError(void *block)
{
	struct Ar5416RxDescriptor *dr=(struct Ar5416RxDescriptor *)block;

	if(dr!=0)
	{
		return (unsigned char)dr->mic_error;
	}
	return 0;
}

static unsigned char Ar5416RxDescriptorPreDelimCrcErr(void *block)
{
	struct Ar5416RxDescriptor *dr=(struct Ar5416RxDescriptor *)block;

	if(dr!=0)
	{
		return (unsigned char)dr->pre_delim_crc_err;
	}
	return 0;
}

static unsigned char Ar5416RxDescriptorKeyIdxValid(void *block)
{
	struct Ar5416RxDescriptor *dr=(struct Ar5416RxDescriptor *)block;

	if(dr!=0)
	{
		return (unsigned char)dr->key_idx_valid;
	}
	return 0;
}

static unsigned int Ar5416RxDescriptorKeyIdx(void *block)
{
	struct Ar5416RxDescriptor *dr=(struct Ar5416RxDescriptor *)block;

	if(dr!=0)
	{
		return dr->key_idx;
	}
	return 0;
}

static unsigned char Ar5416RxDescriptorMoreAgg(void *block)
{
	struct Ar5416RxDescriptor *dr=(struct Ar5416RxDescriptor *)block;

	if(dr!=0)
	{
		return (unsigned char)dr->more_agg;
	}
	return 0;
}

static unsigned char Ar5416RxDescriptorAggregate(void *block)
{
	struct Ar5416RxDescriptor *dr=(struct Ar5416RxDescriptor *)block;

	if(dr!=0)
	{
		return (unsigned char)dr->aggregate;
	}
	return 0;
}

static unsigned char Ar5416RxDescriptorPostDelimCrcErr(void *block)
{
	struct Ar5416RxDescriptor *dr=(struct Ar5416RxDescriptor *)block;

	if(dr!=0)
	{
		return (unsigned char)dr->post_delim_crc_err;
	}
	return 0;
}

static unsigned char Ar5416RxDescriptorDecryptBusyErr(void *block)
{
	struct Ar5416RxDescriptor *dr=(struct Ar5416RxDescriptor *)block;

	if(dr!=0)
	{
		return (unsigned char)dr->decrypt_busy_err;
	}
	return 0;
}

static unsigned char Ar5416RxDescriptorKeyMiss(void *block)
{
	struct Ar5416RxDescriptor *dr=(struct Ar5416RxDescriptor *)block;

	if(dr!=0)
	{
		return (unsigned char)dr->key_miss;
	}
	return 0;
}
	
//
// setup a descriptor with the standard required fields
//
static int Ar5416RxDescriptorSetup(void *block, 
	unsigned int link_ptr, unsigned int buf_ptr, unsigned int buf_len)
{
	struct Ar5416RxDescriptor *dr=(struct Ar5416RxDescriptor *)block;
    memset(dr, 0, sizeof(struct Ar5416RxDescriptor));
	//
	// initialize the descriptors:
	//    buffer size
	//    buffer address
	//    next descriptor pointer 
	//
    dr->buf_len = buf_len;
    dr->buf_ptr = buf_ptr;
    dr->link_ptr = link_ptr;

	return 0;
}

//
// reset the descriptor so that it can be used again
//
static int Ar5416RxDescriptorReset(void *block)
{  
	struct Ar5416RxDescriptor *dr=(struct Ar5416RxDescriptor *)block;
	unsigned long link_ptr, buf_ptr, buf_len;

    buf_len=dr->buf_len;
	buf_ptr=dr->buf_ptr;
	link_ptr=dr->link_ptr;
    return Ar5416RxDescriptorSetup(block, link_ptr, buf_ptr, buf_len);
}

//
// return the size of a descriptor 
//
int Ar5416RxDescriptorSize()
{
   return sizeof(struct Ar5416RxDescriptor);
}

#ifdef UNUSED
//
// copy the descriptor from application memory to the shared memory
//
static void Ar5416RxDescriptorWrite(void *block, unsigned long physical)
{
    OSmemWrite(physical, block, sizeof(Ar5416RxDescriptor));
}

//
// copy the descriptor from the shared memory to application memory
//
static void Ar5416RxDescriptorRead(void *block, unsigned long physical, void (*read)()
{
    LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];

    pLibDev->devMap.OSmemRead(devNum, physical,
                block, sizeof(Ar5416RxDescriptor));

    OSmemRead(physical, block, );
}
#endif

static struct _RxDescriptorFunction _Ar5416RxDescriptor=
{
    Ar5416RxDescriptorLinkPtr,
    Ar5416RxDescriptorBufPtr,
    Ar5416RxDescriptorBufLen,
    0,										// Ar5416RxDescriptorIntReq,
    Ar5416RxDescriptorRssiCombined,
    Ar5416RxDescriptorRssiAnt00,
    Ar5416RxDescriptorRssiAnt01,
    0,										// Ar5416RxDescriptorRssiAnt02,
    Ar5416RxDescriptorRssiAnt10,
    Ar5416RxDescriptorRssiAnt11,
    0,										// Ar5416RxDescriptorRssiAnt12,
    Ar5416RxDescriptorRxRate,
    Ar5416RxDescriptorDataLen,
    Ar5416RxDescriptorMore,
    Ar5416RxDescriptorNumDelim,
    Ar5416RxDescriptorRcvTimestamp,
    Ar5416RxDescriptorGi,
    Ar5416RxDescriptorH2040,
    Ar5416RxDescriptorDuplicate,
    Ar5416RxDescriptorRxAntenna,
    Ar5416RxDescriptorEvm0,
    Ar5416RxDescriptorEvm1,
    0,										// Ar5416RxDescriptorEvm2,
    Ar5416RxDescriptorDone,
    Ar5416RxDescriptorFrameRxOk,
    Ar5416RxDescriptorCrcError,
    Ar5416RxDescriptorDecryptCrcErr,
    Ar5416RxDescriptorPhyErr,
    Ar5416RxDescriptorMicError,
    Ar5416RxDescriptorPreDelimCrcErr,
    Ar5416RxDescriptorKeyIdxValid,
    Ar5416RxDescriptorKeyIdx,
    Ar5416RxDescriptorMoreAgg,
    0,										// CHANGEKIWI Ar5416RxDescriptorFirstAgg,
    Ar5416RxDescriptorAggregate,
    Ar5416RxDescriptorPostDelimCrcErr,
    Ar5416RxDescriptorDecryptBusyErr,
    Ar5416RxDescriptorKeyMiss,
    Ar5416RxDescriptorSetup,
    Ar5416RxDescriptorReset,
    Ar5416RxDescriptorSize,
    Ar5416RxDescriptorPrint,
	0,											// Ar5416RxDescriptorSpectralScan,
#ifdef UNUSED
    Ar5416RxDescriptorWrite,
    Ar5416RxDescriptorRead,
#endif
};


int Ar5416RxDescriptorFunctionSelect()
{
	RxDescriptorFunctionSelect(&_Ar5416RxDescriptor);
	//
	// later check some of the pointers to make sure there are valid functions
	//
	return 0;
}

