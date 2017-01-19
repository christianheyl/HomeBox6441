
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ErrorPrint.h"
#include "DeviceError.h"
#include "RxDescriptor.h"

#define MBUFFER 1024

static struct _RxDescriptorFunction _RxDescriptor;




int RxDescriptorPrint(void *block, char *buffer, int max)
{
	if(_RxDescriptor.Print!=0)
	{
		return _RxDescriptor.Print(block,buffer,max);
	}
	ErrorPrint(DeviceNoFunction,"RxDescriptorPrint"); return 0;
}

unsigned int RxDescriptorLinkPtr(void *block)
{
	if(_RxDescriptor.LinkPtr!=0)
	{
		return _RxDescriptor.LinkPtr(block);
	}
	ErrorPrint(DeviceNoFunction,"RxDescriptorLinkPtr"); return 0;
}

unsigned int RxDescriptorBufPtr(void *block)
{
	if(_RxDescriptor.BufPtr!=0)
	{
		return _RxDescriptor.BufPtr(block);
	}
	ErrorPrint(DeviceNoFunction,"RxDescriptorBufPtr"); return 0;
}

unsigned int RxDescriptorBufLen(void *block)
{
	if(_RxDescriptor.BufLen!=0)
	{
		return _RxDescriptor.BufLen(block);
	}
	ErrorPrint(DeviceNoFunction,"RxDescriptorBufLen"); return 0;
}

unsigned char RxDescriptorIntReq(void *block)
{
	if(_RxDescriptor.IntReq!=0)
	{
		return _RxDescriptor.IntReq(block);
	}
	ErrorPrint(DeviceNoFunction,"RxDescriptorIntReq"); return 0;
}

unsigned int RxDescriptorRssiCombined(void *block)
{
	if(_RxDescriptor.RssiCombined!=0)
	{
		return _RxDescriptor.RssiCombined(block);
	}
	ErrorPrint(DeviceNoFunction,"RxDescriptorRssiCombined"); return 0;
}

unsigned int RxDescriptorRssiAnt00(void *block)
{
	if(_RxDescriptor.RssiAnt00!=0)
	{
		return _RxDescriptor.RssiAnt00(block);
	}
	ErrorPrint(DeviceNoFunction,"RxDescriptorRssiAnt00"); return 0;
}

unsigned int RxDescriptorRssiAnt01(void *block)
{
	if(_RxDescriptor.RssiAnt01!=0)
	{
		return _RxDescriptor.RssiAnt01(block);
	}
	ErrorPrint(DeviceNoFunction,"RxDescriptorRssiAnt01"); return 0;
}

unsigned int RxDescriptorRssiAnt02(void *block)
{
	if(_RxDescriptor.RssiAnt02!=0)
	{
		return _RxDescriptor.RssiAnt02(block);
	}
	ErrorPrint(DeviceNoFunction,"RxDescriptorRssiAnt02"); return 0;
}

unsigned int RxDescriptorRssiAnt10(void *block)
{
	if(_RxDescriptor.RssiAnt10!=0)
	{
		return _RxDescriptor.RssiAnt10(block);
	}
	ErrorPrint(DeviceNoFunction,"RxDescriptorRssiAnt10"); return 0;
}

unsigned int RxDescriptorRssiAnt11(void *block)
{
	if(_RxDescriptor.RssiAnt11!=0)
	{
		return _RxDescriptor.RssiAnt11(block);
	}
	ErrorPrint(DeviceNoFunction,"RxDescriptorRssiAnt11"); return 0;
}

unsigned int RxDescriptorRssiAnt12(void *block)
{
	if(_RxDescriptor.RssiAnt12!=0)
	{
		return _RxDescriptor.RssiAnt12(block);
	}
	ErrorPrint(DeviceNoFunction,"RxDescriptorRssiAnt12"); return 0;
}

unsigned int RxDescriptorRxRate(void *block)
{
	if(_RxDescriptor.RxRate!=0)
	{
		return _RxDescriptor.RxRate(block);
	}
	ErrorPrint(DeviceNoFunction,"RxDescriptorRxRate"); return 0;
}

unsigned int RxDescriptorDataLen(void *block)
{
	if(_RxDescriptor.DataLen!=0)
	{
		return _RxDescriptor.DataLen(block);
	}
	ErrorPrint(DeviceNoFunction,"RxDescriptorDataLen"); return 0;
}

unsigned char RxDescriptorMore(void *block)
{
	if(_RxDescriptor.More!=0)
	{
		return _RxDescriptor.More(block);
	}
	ErrorPrint(DeviceNoFunction,"RxDescriptorMore"); return 0;
}

unsigned int RxDescriptorNumDelim(void *block)
{
	if(_RxDescriptor.NumDelim!=0)
	{
		return _RxDescriptor.NumDelim(block);
	}
	ErrorPrint(DeviceNoFunction,"RxDescriptorNumDelim"); return 0;
}

unsigned int RxDescriptorRcvTimestamp(void *block)
{
	if(_RxDescriptor.RcvTimestamp!=0)
	{
		return _RxDescriptor.RcvTimestamp(block);
	}
	ErrorPrint(DeviceNoFunction,"RxDescriptorRcvTimestamp"); return 0;
}

unsigned char RxDescriptorGi(void *block)
{
	if(_RxDescriptor.Gi!=0)
	{
		return _RxDescriptor.Gi(block);
	}
	ErrorPrint(DeviceNoFunction,"RxDescriptorGi"); return 0;
}

unsigned char RxDescriptorH2040(void *block)
{
	if(_RxDescriptor.H2040!=0)
	{
		return _RxDescriptor.H2040(block);
	}
	ErrorPrint(DeviceNoFunction,"RxDescriptorH2040"); return 0;
}

unsigned char RxDescriptorDuplicate(void *block)
{
	if(_RxDescriptor.Duplicate!=0)
	{
		return _RxDescriptor.Duplicate(block);
	}
	ErrorPrint(DeviceNoFunction,"RxDescriptorDuplicate"); return 0;
}

unsigned int RxDescriptorRxAntenna(void *block)
{
	if(_RxDescriptor.RxAntenna!=0)
	{
		return _RxDescriptor.RxAntenna(block);
	}
	ErrorPrint(DeviceNoFunction,"RxDescriptorRxAntenna"); return 0;
}

double RxDescriptorEvm0(void *block)
{
	if(_RxDescriptor.Evm0!=0)
	{
		return _RxDescriptor.Evm0(block);
	}
	ErrorPrint(DeviceNoFunction,"RxDescriptorEvm0"); return 0;
}

double RxDescriptorEvm1(void *block)
{
	if(_RxDescriptor.Evm1!=0)
	{
		return _RxDescriptor.Evm1(block);
	}
	ErrorPrint(DeviceNoFunction,"RxDescriptorEvm1"); return 0;
}

double RxDescriptorEvm2(void *block)
{
	if(_RxDescriptor.Evm2!=0)
	{
		return _RxDescriptor.Evm2(block);
	}
	ErrorPrint(DeviceNoFunction,"RxDescriptorEvm2"); return 0;
}

unsigned char RxDescriptorDone(void *block)
{
	if(_RxDescriptor.Done!=0)
	{
		return _RxDescriptor.Done(block);
	}
	ErrorPrint(DeviceNoFunction,"RxDescriptorDone"); return 0;
}

unsigned char RxDescriptorFrameRxOk(void *block)
{
	if(_RxDescriptor.FrameRxOk!=0)
	{
		return _RxDescriptor.FrameRxOk(block);
	}
	ErrorPrint(DeviceNoFunction,"RxDescriptorFrameRxOk"); return 0;
}

unsigned char RxDescriptorCrcError(void *block)
{
	if(_RxDescriptor.CrcError!=0)
	{
		return _RxDescriptor.CrcError(block);
	}
	ErrorPrint(DeviceNoFunction,"RxDescriptorCrcError"); return 0;
}

unsigned char RxDescriptorDecryptCrcErr(void *block)
{
	if(_RxDescriptor.DecryptCrcErr!=0)
	{
		return _RxDescriptor.DecryptCrcErr(block);
	}
	ErrorPrint(DeviceNoFunction,"RxDescriptorDecryptCrcErr"); return 0;
}

unsigned char RxDescriptorPhyErr(void *block)
{
	if(_RxDescriptor.PhyErr!=0)
	{
		return _RxDescriptor.PhyErr(block);
	}
	ErrorPrint(DeviceNoFunction,"RxDescriptorPhyErr"); return 0;
}

unsigned char RxDescriptorMicError(void *block)
{
	if(_RxDescriptor.MicError!=0)
	{
		return _RxDescriptor.MicError(block);
	}
	ErrorPrint(DeviceNoFunction,"RxDescriptorMicError"); return 0;
}

unsigned char RxDescriptorPreDelimCrcErr(void *block)
{
	if(_RxDescriptor.PreDelimCrcErr!=0)
	{
		return _RxDescriptor.PreDelimCrcErr(block);
	}
	ErrorPrint(DeviceNoFunction,"RxDescriptorPreDelimCrcErr"); return 0;
}

unsigned char RxDescriptorKeyIdxValid(void *block)
{
	if(_RxDescriptor.KeyIdxValid!=0)
	{
		return _RxDescriptor.KeyIdxValid(block);
	}
	ErrorPrint(DeviceNoFunction,"RxDescriptorKeyIdxValid"); return 0;
}

unsigned int RxDescriptorKeyIdx(void *block)
{
	if(_RxDescriptor.KeyIdx!=0)
	{
		return _RxDescriptor.KeyIdx(block);
	}
	ErrorPrint(DeviceNoFunction,"RxDescriptorKeyIdx"); return 0;
}

unsigned char RxDescriptorFirstAgg(void *block)
{
	if(_RxDescriptor.FirstAgg!=0)
	{
		return _RxDescriptor.FirstAgg(block);
	}
	ErrorPrint(DeviceNoFunction,"RxDescriptorFirstAgg"); return 0;
}

unsigned char RxDescriptorMoreAgg(void *block)
{
	if(_RxDescriptor.MoreAgg!=0)
	{
		return _RxDescriptor.MoreAgg(block);
	}
	ErrorPrint(DeviceNoFunction,"RxDescriptorMoreAgg"); return 0;
}

unsigned char RxDescriptorAggregate(void *block)
{
	if(_RxDescriptor.Aggregate!=0)
	{
		return _RxDescriptor.Aggregate(block);
	}
	ErrorPrint(DeviceNoFunction,"RxDescriptorAggregate"); return 0;
}

unsigned char RxDescriptorPostDelimCrcErr(void *block)
{
	if(_RxDescriptor.PostDelimCrcErr!=0)
	{
		return _RxDescriptor.PostDelimCrcErr(block);
	}
	ErrorPrint(DeviceNoFunction,"RxDescriptorPostDelimCrcErr"); return 0;
}

unsigned char RxDescriptorDecryptBusyErr(void *block)
{
	if(_RxDescriptor.DecryptBusyErr!=0)
	{
		return _RxDescriptor.DecryptBusyErr(block);
	}
	ErrorPrint(DeviceNoFunction,"RxDescriptorDecryptBusyErr"); return 0;
}

unsigned char RxDescriptorKeyMiss(void *block)
{
	if(_RxDescriptor.KeyMiss!=0)
	{
		return _RxDescriptor.KeyMiss(block);
	}
	ErrorPrint(DeviceNoFunction,"RxDescriptorKeyMiss"); return 0;
}
	
//
// setup a descriptor with the standard required fields
//
int RxDescriptorSetup(void *block, 
	unsigned int link_ptr, unsigned int buf_ptr, unsigned int buf_len)
{
	if(_RxDescriptor.Setup!=0)
	{
	    return _RxDescriptor.Setup(block,link_ptr,buf_ptr,buf_len);
	}
	ErrorPrint(DeviceNoFunction,"RxDescriptorSetup"); return 0;
}

//
// reset the descriptor so that it can be used again
//
int RxDescriptorReset(void *block)
{  
	if(_RxDescriptor.Reset!=0)
	{
		return _RxDescriptor.Reset(block);
	}
	ErrorPrint(DeviceNoFunction,"RxDescriptorReset"); return 0;
}

//
// return the size of a descriptor 
//
int RxDescriptorSize()
{
	if(_RxDescriptor.Size!=0)
	{
		return _RxDescriptor.Size();
	}
	ErrorPrint(DeviceNoFunction,"name"); return 0;
}

//
// return 1 if the descriptor contains spectral scan data
//
int RxDescriptorSpectralScan(void *block)
{
	if(_RxDescriptor.SpectralScan!=0)
	{
		return _RxDescriptor.SpectralScan(block);
	}
	ErrorPrint(DeviceNoFunction,"name"); return 0;
}

#ifdef UNUSED
//
// copy the descriptor from application memory to the shared memory
//
void RxDescriptorWrite(void *block, unsigned int physical)
{
	if(_RxDescriptor.Write!=0)
	{
		_RxDescriptor.Write(block,physical);
	}
}

//
// copy the descriptor from the shared memory to application memory
//
void RxDescriptorRead(void *block, unsigned int physical)
{
	if(_RxDescriptor.Read!=0)
	{
		_RxDescriptor.Read(block,physical);
	}
}
#endif

int DEVICEDLLSPEC RxDescriptorFunctionSelect(struct _RxDescriptorFunction *rxDescriptor)
{
	_RxDescriptor= *rxDescriptor;
	//
	// later check some of the pointers to make sure there are valid functions
	//
	return 0;
}

//
// clear all rx descriptor function pointers and set to default behavior
//
int DEVICEDLLSPEC RxDescriptorFunctionReset(void)
{
	memset(&_RxDescriptor, 0, sizeof(_RxDescriptor));
	return 0;
}

