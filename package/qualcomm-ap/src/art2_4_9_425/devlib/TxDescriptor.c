
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "ErrorPrint.h"
#include "DeviceError.h"
#include "TxDescriptor.h"

#define MBUFFER 1024


static struct _TxDescriptorFunction _TxDescriptor;



DEVICEDLLSPEC int TxDescriptorLinkPtrSet(void *block, unsigned int ptr)
{
	if(_TxDescriptor.LinkPtrSet!=0)
	{
		return _TxDescriptor.LinkPtrSet(block,ptr);
	}
	ErrorPrint(DeviceNoFunction,"TxDescriptorLinkPtrSet"); return 0;
}

DEVICEDLLSPEC int TxDescriptorTxRateSet(void *block, unsigned int rate)
{
	if(_TxDescriptor.TxRateSet!=0)
	{
		return _TxDescriptor.TxRateSet(block,rate);
	}
	ErrorPrint(DeviceNoFunction,"TxDescriptorTxRateSet"); return 0;
}

DEVICEDLLSPEC int TxDescriptorPrint(void *block, char *buffer, int max)
{
	if(_TxDescriptor.Print!=0)
	{
		return _TxDescriptor.Print(block,buffer,max);
	}
	ErrorPrint(DeviceNoFunction,"TxDescriptorPrint"); return 0;
}

DEVICEDLLSPEC unsigned char TxDescriptorBaStatus(void *block)
{
	if(_TxDescriptor.BaStatus!=0)
	{
		return _TxDescriptor.BaStatus(block);
	}
	ErrorPrint(DeviceNoFunction,"TxDescriptorBaStatus"); return 0;
}

DEVICEDLLSPEC unsigned int TxDescriptorAggLength(void *block)
{
	if(_TxDescriptor.AggLength!=0)
	{
		return _TxDescriptor.AggLength(block);
	}
	ErrorPrint(DeviceNoFunction,"TxDescriptorAggLength"); return 0;
}

DEVICEDLLSPEC unsigned int TxDescriptorBaBitmapLow(void *block)
{
	if(_TxDescriptor.BaBitmapLow!=0)
	{
		return _TxDescriptor.BaBitmapLow(block);
	}
	ErrorPrint(DeviceNoFunction,"TxDescriptorBaBitmapLow"); return 0;
}

DEVICEDLLSPEC unsigned int TxDescriptorBaBitmapHigh(void *block)
{
	if(_TxDescriptor.BaBitmapHigh!=0)
	{
		return _TxDescriptor.BaBitmapHigh(block);
	}
	ErrorPrint(DeviceNoFunction,"TxDescriptorBaBitmapHigh"); return 0;
}

DEVICEDLLSPEC unsigned char TxDescriptorFifoUnderrun(void *block)
{
	if(_TxDescriptor.FifoUnderrun!=0)
	{
		return _TxDescriptor.FifoUnderrun(block);
	}
	ErrorPrint(DeviceNoFunction,"TxDescriptorFifoUnderrun"); return 0;
}

DEVICEDLLSPEC unsigned char TxDescriptorExcessiveRetries(void *block)
{
	if(_TxDescriptor.ExcessiveRetries!=0)
	{
		return _TxDescriptor.ExcessiveRetries(block);
	}
	ErrorPrint(DeviceNoFunction,"TxDescriptorExcessiveRetries"); return 0;
}

DEVICEDLLSPEC unsigned int TxDescriptorRtsFailCount(void *block)
{
	if(_TxDescriptor.RtsFailCount!=0)
	{
		return _TxDescriptor.RtsFailCount(block);
	}
	ErrorPrint(DeviceNoFunction,"TxDescriptorRtsFailCount"); return 0;
}

DEVICEDLLSPEC unsigned int TxDescriptorDataFailCount(void *block)
{
	if(_TxDescriptor.DataFailCount!=0)
	{
		return _TxDescriptor.DataFailCount(block);
	}
	ErrorPrint(DeviceNoFunction,"TxDescriptorDataFailCount"); return 0;
}

DEVICEDLLSPEC unsigned int TxDescriptorLinkPtr(void *block)
{
	if(_TxDescriptor.LinkPtr!=0)
	{
		return _TxDescriptor.LinkPtr(block);
	}
	ErrorPrint(DeviceNoFunction,"TxDescriptorLinkPtr"); return 0;
}

DEVICEDLLSPEC unsigned int TxDescriptorBufPtr(void *block)
{
	if(_TxDescriptor.BufPtr!=0)
	{
		return _TxDescriptor.BufPtr(block);
	}
	ErrorPrint(DeviceNoFunction,"TxDescriptorBufPtr"); return 0;
}

DEVICEDLLSPEC unsigned int TxDescriptorBufLen(void *block)
{
	if(_TxDescriptor.BufLen!=0)
	{
		return _TxDescriptor.BufLen(block);
	}
	ErrorPrint(DeviceNoFunction,"TxDescriptorBufLen"); return 0;
}

DEVICEDLLSPEC unsigned char TxDescriptorIntReq(void *block)
{
	if(_TxDescriptor.IntReq!=0)
	{
		return _TxDescriptor.IntReq(block);
	}
	ErrorPrint(DeviceNoFunction,"TxDescriptorIntReq"); return 0;
}

DEVICEDLLSPEC unsigned int TxDescriptorRssiCombined(void *block)
{
	if(_TxDescriptor.RssiCombined!=0)
	{
		return _TxDescriptor.RssiCombined(block);
	}
	ErrorPrint(DeviceNoFunction,"TxDescriptorRssiCombined"); return 0;
}

DEVICEDLLSPEC unsigned int TxDescriptorRssiAnt00(void *block)
{
	if(_TxDescriptor.RssiAnt00!=0)
	{
		return _TxDescriptor.RssiAnt00(block);
	}
	ErrorPrint(DeviceNoFunction,"TxDescriptorRssiAnt00"); return 0;
}

DEVICEDLLSPEC unsigned int TxDescriptorRssiAnt01(void *block)
{
	if(_TxDescriptor.RssiAnt01!=0)
	{
		return _TxDescriptor.RssiAnt01(block);
	}
	ErrorPrint(DeviceNoFunction,"TxDescriptorRssiAnt01"); return 0;
}

DEVICEDLLSPEC unsigned int TxDescriptorRssiAnt02(void *block)
{
	if(_TxDescriptor.RssiAnt02!=0)
	{
		return _TxDescriptor.RssiAnt02(block);
	}
	ErrorPrint(DeviceNoFunction,"TxDescriptorRssiAnt02"); return 0;
}

DEVICEDLLSPEC unsigned int TxDescriptorRssiAnt10(void *block)
{
	if(_TxDescriptor.RssiAnt10!=0)
	{
		return _TxDescriptor.RssiAnt10(block);
	}
	ErrorPrint(DeviceNoFunction,"TxDescriptorRssiAnt10"); return 0;
}

DEVICEDLLSPEC unsigned int TxDescriptorRssiAnt11(void *block)
{
	if(_TxDescriptor.RssiAnt11!=0)
	{
		return _TxDescriptor.RssiAnt11(block);
	}
	ErrorPrint(DeviceNoFunction,"TxDescriptorRssiAnt11"); return 0;
}

DEVICEDLLSPEC unsigned int TxDescriptorRssiAnt12(void *block)
{
	if(_TxDescriptor.RssiAnt12!=0)
	{
		return _TxDescriptor.RssiAnt12(block);
	}
	ErrorPrint(DeviceNoFunction,"TxDescriptorRssiAnt12"); return 0;
}

DEVICEDLLSPEC unsigned int TxDescriptorTxRate(void *block)
{
	if(_TxDescriptor.TxRate!=0)
	{
		return _TxDescriptor.TxRate(block);
	}
	ErrorPrint(DeviceNoFunction,"TxDescriptorTxRate"); return 0;
}

DEVICEDLLSPEC unsigned int TxDescriptorDataLen(void *block)
{
	if(_TxDescriptor.DataLen!=0)
	{
		return _TxDescriptor.DataLen(block);
	}
	ErrorPrint(DeviceNoFunction,"TxDescriptorDataLen"); return 0;
}

DEVICEDLLSPEC unsigned char TxDescriptorMore(void *block)
{
	if(_TxDescriptor.More!=0)
	{
		return _TxDescriptor.More(block);
	}
	ErrorPrint(DeviceNoFunction,"TxDescriptorMore"); return 0;
}

DEVICEDLLSPEC unsigned int TxDescriptorNumDelim(void *block)
{
	if(_TxDescriptor.NumDelim!=0)
	{
		return _TxDescriptor.NumDelim(block);
	}
	ErrorPrint(DeviceNoFunction,"TxDescriptorNumDelim"); return 0;
}

DEVICEDLLSPEC unsigned int TxDescriptorSendTimestamp(void *block)
{
	if(_TxDescriptor.SendTimestamp!=0)
	{
		return _TxDescriptor.SendTimestamp(block);
	}
	ErrorPrint(DeviceNoFunction,"TxDescriptorSendTimestamp"); return 0;
}

DEVICEDLLSPEC unsigned char TxDescriptorGi(void *block)
{
	if(_TxDescriptor.Gi!=0)
	{
		return _TxDescriptor.Gi(block);
	}
	ErrorPrint(DeviceNoFunction,"TxDescriptorGi"); return 0;
}

DEVICEDLLSPEC unsigned char TxDescriptorH2040(void *block)
{
	if(_TxDescriptor.H2040!=0)
	{
		return _TxDescriptor.H2040(block);
	}
	ErrorPrint(DeviceNoFunction,"TxDescriptorH2040"); return 0;
}

DEVICEDLLSPEC unsigned char TxDescriptorDuplicate(void *block)
{
	if(_TxDescriptor.Duplicate!=0)
	{
		return _TxDescriptor.Duplicate(block);
	}
	ErrorPrint(DeviceNoFunction,"TxDescriptorDuplicate"); return 0;
}

DEVICEDLLSPEC unsigned int TxDescriptorTxAntenna(void *block)
{
	if(_TxDescriptor.TxAntenna!=0)
	{
		return _TxDescriptor.TxAntenna(block);
	}
	ErrorPrint(DeviceNoFunction,"TxDescriptorTxAntenna"); return 0;
}

DEVICEDLLSPEC double TxDescriptorEvm0(void *block)
{
	if(_TxDescriptor.Evm0!=0)
	{
		return _TxDescriptor.Evm0(block);
	}
	ErrorPrint(DeviceNoFunction,"TxDescriptorEvm0"); return 0;
}

DEVICEDLLSPEC double TxDescriptorEvm1(void *block)
{
	if(_TxDescriptor.Evm1!=0)
	{
		return _TxDescriptor.Evm1(block);
	}
	ErrorPrint(DeviceNoFunction,"TxDescriptorEvm1"); return 0;
}

DEVICEDLLSPEC double TxDescriptorEvm2(void *block)
{
	if(_TxDescriptor.Evm2!=0)
	{
		return _TxDescriptor.Evm2(block);
	}
	ErrorPrint(DeviceNoFunction,"TxDescriptorEvm2"); return 0;
}

DEVICEDLLSPEC unsigned char TxDescriptorDone(void *block)
{
	if(_TxDescriptor.Done!=0)
	{
		return _TxDescriptor.Done(block);
	}
	ErrorPrint(DeviceNoFunction,"TxDescriptorDone"); return 0;
}

DEVICEDLLSPEC unsigned char TxDescriptorFrameTxOk(void *block)
{
	if(_TxDescriptor.FrameTxOk!=0)
	{
		return _TxDescriptor.FrameTxOk(block);
	}
	ErrorPrint(DeviceNoFunction,"TxDescriptorFrameTxOk"); return 0;
}

DEVICEDLLSPEC unsigned char TxDescriptorCrcError(void *block)
{
	if(_TxDescriptor.CrcError!=0)
	{
		return _TxDescriptor.CrcError(block);
	}
	ErrorPrint(DeviceNoFunction,"TxDescriptorCrcError"); return 0;
}

DEVICEDLLSPEC unsigned char TxDescriptorDecryptCrcErr(void *block)
{
	if(_TxDescriptor.DecryptCrcErr!=0)
	{
		return _TxDescriptor.DecryptCrcErr(block);
	}
	ErrorPrint(DeviceNoFunction,"TxDescriptorDecryptCrcErr"); return 0;
}

DEVICEDLLSPEC unsigned char TxDescriptorPhyErr(void *block)
{
	if(_TxDescriptor.PhyErr!=0)
	{
		return _TxDescriptor.PhyErr(block);
	}
	ErrorPrint(DeviceNoFunction,"TxDescriptorPhyErr"); return 0;
}

DEVICEDLLSPEC unsigned char TxDescriptorMicError(void *block)
{
	if(_TxDescriptor.MicError!=0)
	{
		return _TxDescriptor.MicError(block);
	}
	ErrorPrint(DeviceNoFunction,"TxDescriptorMicError"); return 0;
}

DEVICEDLLSPEC unsigned char TxDescriptorPreDelimCrcErr(void *block)
{
	if(_TxDescriptor.PreDelimCrcErr!=0)
	{
		return _TxDescriptor.PreDelimCrcErr(block);
	}
	ErrorPrint(DeviceNoFunction,"TxDescriptorPreDelimCrcErr"); return 0;
}

DEVICEDLLSPEC unsigned char TxDescriptorKeyIdxValid(void *block)
{
	if(_TxDescriptor.KeyIdxValid!=0)
	{
		return _TxDescriptor.KeyIdxValid(block);
	}
	ErrorPrint(DeviceNoFunction,"TxDescriptorKeyIdxValid"); return 0;
}

DEVICEDLLSPEC unsigned int TxDescriptorKeyIdx(void *block)
{
	if(_TxDescriptor.KeyIdx!=0)
	{
		return _TxDescriptor.KeyIdx(block);
	}
	ErrorPrint(DeviceNoFunction,"TxDescriptorKeyIdx"); return 0;
}

DEVICEDLLSPEC unsigned char TxDescriptorMoreAgg(void *block)
{
	if(_TxDescriptor.MoreAgg!=0)
	{
		return _TxDescriptor.MoreAgg(block);
	}
	ErrorPrint(DeviceNoFunction,"TxDescriptorKeyIdx"); return 0;
}

DEVICEDLLSPEC unsigned char TxDescriptorAggregate(void *block)
{
	if(_TxDescriptor.Aggregate!=0)
	{
		return _TxDescriptor.Aggregate(block);
	}
	ErrorPrint(DeviceNoFunction,"TxDescriptorAggregate"); return 0;
}

DEVICEDLLSPEC unsigned char TxDescriptorPostDelimCrcErr(void *block)
{
	if(_TxDescriptor.PostDelimCrcErr!=0)
	{
		return _TxDescriptor.PostDelimCrcErr(block);
	}
	ErrorPrint(DeviceNoFunction,"TxDescriptorPostDelimCrcErr"); return 0;
}

DEVICEDLLSPEC unsigned char TxDescriptorDecryptBusyErr(void *block)
{
	if(_TxDescriptor.DecryptBusyErr!=0)
	{
		return _TxDescriptor.DecryptBusyErr(block);
	}
	ErrorPrint(DeviceNoFunction,"TxDescriptorDecryptBusyErr"); return 0;
}

DEVICEDLLSPEC unsigned char TxDescriptorKeyMiss(void *block)
{
	if(_TxDescriptor.KeyMiss!=0)
	{
		return _TxDescriptor.KeyMiss(block);
	}
	ErrorPrint(DeviceNoFunction,"TxDescriptorKeyMiss"); return 0;
}
	
//
// setup a descriptor with the standard required fields
//
DEVICEDLLSPEC int TxDescriptorSetup(void *block, 
	unsigned int link_ptr, unsigned int buf_ptr, int buf_len,
	int broadcast, int retry,
	int rate, int ht40, int shortGi, unsigned int txchain,
	int isagg, int moreagg,
	int id)
{
	if(_TxDescriptor.Setup!=0)
	{
	    return _TxDescriptor.Setup(block,link_ptr,buf_ptr,buf_len,
			broadcast,retry,rate,ht40,shortGi,txchain,
			isagg,moreagg,
			id);
	}
	ErrorPrint(DeviceNoFunction,"TxDescriptorSetup"); return -1;
}

//
// setup a descriptor with the standard required fields
//
DEVICEDLLSPEC int TxDescriptorStatusSetup(void *block)
{
	if(_TxDescriptor.StatusSetup!=0)
	{
	    return _TxDescriptor.StatusSetup(block);
	}
	ErrorPrint(DeviceNoFunction,"TxDescriptorStatusSetup"); return -1;
}

//
// reset the descriptor so that it can be used again
//
DEVICEDLLSPEC int TxDescriptorReset(void *block)
{  
	if(_TxDescriptor.Reset!=0)
	{
		_TxDescriptor.Reset(block);
	}
	ErrorPrint(DeviceNoFunction,"TxDescriptorReset"); return -1; 
}

//
// return the size of a descriptor 
//
DEVICEDLLSPEC int TxDescriptorSize()
{
	if(_TxDescriptor.Size!=0)
	{
		return _TxDescriptor.Size();
	}
	ErrorPrint(DeviceNoFunction,"TxDescriptorSize"); return 0;
}

//
// return the size of a descriptor 
//
DEVICEDLLSPEC int TxDescriptorStatusSize()
{
	if(_TxDescriptor.StatusSize!=0)
	{
		return _TxDescriptor.StatusSize();
	}
	ErrorPrint(DeviceNoFunction,"TxDescriptorStatusSize"); return 0;
}

// Setup PA predistortion training chain number
DEVICEDLLSPEC int TxDescriptorPAPDSetup(void *block, int chainNum)
{
	if(_TxDescriptor.PAPDSetup!=0)
	{
		return _TxDescriptor.PAPDSetup(block, chainNum);
	}
	ErrorPrint(DeviceNoFunction,"TxDescriptorPAPDSetup"); return -1;
}

#ifdef UNUSED
//
// copy the descriptor from application memory to the shared memory
//
void TxDescriptorWrite(void *block, unsigned int physical)
{
	if(_TxDescriptor.Write!=0)
	{
		_TxDescriptor.Write(block,physical);
	}
}

//
// copy the descriptor from the shared memory to application memory
//
void TxDescriptorRead(void *block, unsigned int physical)
{
	if(_TxDescriptor.Read!=0)
	{
		_TxDescriptor.Read(block,physical);
	}
}
#endif


DEVICEDLLSPEC int TxDescriptorFunctionSelect(struct _TxDescriptorFunction *txDescriptor)
{
	_TxDescriptor= *txDescriptor;
	//
	// later check some of the pointers to make sure there are valid functions
	//
	return 0;
}

//
// clear all rx descriptor function pointers and set to default behavior
//
DEVICEDLLSPEC int TxDescriptorFunctionReset(void)
{
	memset(&_TxDescriptor, 0, sizeof(_TxDescriptor));
	return 0;
}


