
//
// This file contains the functions that control the radio chip set and make it do the correct thing.
//
// These are the only functions that should be used by higher level software. These functions, in turn, call
// the appropriate device dependent functions based on the currently selected chip set.
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "smatch.h"
#include "ErrorPrint.h"

#include "LinkError.h"
#include "LinkTxRx.h"


// LATERLINK
#define LinkNoFunction 7510

static struct _LinkFunction _Link;


DEVICEDLLSPEC int LinkTxSetup(int *rate, int nrate, int interleave,
	unsigned char *bssid, unsigned char *source, unsigned char *destination, 
	int PacketMany, int PacketLength,
	int RetryMax, int Antenna, int Bc, int Ifs,
	int shortGi, unsigned int txchain,
	int naggregate,
	unsigned char *pattern, int npattern)
{
	if(_Link.TxSetup!=0)
	{
		return _Link.TxSetup(rate, nrate, interleave,
			bssid, source, destination, 
			PacketMany, PacketLength,
			RetryMax, Antenna, Bc, Ifs,
			shortGi, txchain,
			naggregate,
			pattern, npattern);
	}
	return -ErrorPrint(LinkNoFunction,"LinkTxSetup");
}

DEVICEDLLSPEC int LinkTxStart(void)
{
	//UserPrint("LinkTxStart\n");
	if(_Link.TxStart!=0)
	{
		return _Link.TxStart();
	}
	return -ErrorPrint(LinkNoFunction,"LinkTxStart");
}

DEVICEDLLSPEC int LinkTxComplete(int timeout, int (*ison)(), int (*done)(), int chipTemperature, int calibrate)
{
	if(_Link.TxComplete!=0)
	{
		return _Link.TxComplete(timeout, ison, done, chipTemperature, calibrate);
	}
	return -ErrorPrint(LinkNoFunction,"LinkTxComplete");
}

DEVICEDLLSPEC struct txStats *LinkTxStatFetch(int rate)
{
	if(_Link.TxStatFetch!=0)
	{
		return _Link.TxStatFetch(rate);
	}
	ErrorPrint(LinkNoFunction,"LinkTxStatFetch");
	return 0;
}

DEVICEDLLSPEC int LinkRxSetup(unsigned char *bssid, unsigned char *macaddr)
{
	if(_Link.RxSetup!=0)
	{
		return _Link.RxSetup(bssid, macaddr);
	}
	return -ErrorPrint(LinkNoFunction,"LinkRxSetup");
}

DEVICEDLLSPEC int LinkRxStart(int promiscuous)
{
	if(_Link.RxStart!=0)
	{
		return _Link.RxStart(promiscuous);
	}
	return -ErrorPrint(LinkNoFunction,"LinkRxStart");
}

DEVICEDLLSPEC int LinkRxComplete(int timeout, 
		int enableCompare, unsigned char *dataPattern, int dataPatternLength, int (*done)())
{
	if(_Link.RxComplete!=0)
	{
		return _Link.RxComplete(timeout, enableCompare, dataPattern, dataPatternLength, done);
	}
	return -ErrorPrint(LinkNoFunction,"LinkRxComplete");
}

DEVICEDLLSPEC struct rxStats *LinkRxStatFetch(int rate)
{
	if(_Link.RxStatFetch!=0)
	{
		return _Link.RxStatFetch(rate);
	}
	ErrorPrint(LinkNoFunction,"LinkRxStatFetch");
	return 0;
}

DEVICEDLLSPEC int LinkRxSpectralScan(int spectralscan, void (*f)(int rssi, int *rssic, int *rssie, int nchain, int *spectrum, int nspectrum))
{
	if(_Link.RxSpectralScan!=0)
	{
		return _Link.RxSpectralScan(spectralscan, f);
	}
	return -ErrorPrint(LinkNoFunction,"LinkRxSpectralScan");
}

DEVICEDLLSPEC int LinkTxForPAPD(int ChainNum)
{
	if(_Link.TxForPAPD!=0)
	{
		return _Link.TxForPAPD(ChainNum);
	}
	return -ErrorPrint(LinkNoFunction,"LinkTxForPAPD");
}

DEVICEDLLSPEC int LinkTxDataStartCmd(int freq, int cenFreqUsed, int tpcm, int pcdac, double *txPower, int gainIndex, int dacGain,
                            int ht40, int *rate, int nrate, int ir,
                            unsigned char *bssid, unsigned char *source, unsigned char *destination,
                            int numDescPerRate, int *dataBodyLength,
                            int retries, int antenna, int broadcast, int ifs,
	                        int shortGi, int txchain, int naggregate,
	                        unsigned char *pattern, int npattern, int carrier, int bandwidth, int psatcal, int dutycycle)
{
	if(_Link.TxDataStartCmd!=0)
	{
		return _Link.TxDataStartCmd(freq, cenFreqUsed, tpcm, pcdac, txPower, gainIndex, dacGain, ht40, rate, nrate, ir,
                            bssid, source, destination, numDescPerRate, dataBodyLength,
                            retries, antenna, broadcast, ifs, shortGi, txchain, naggregate,
	                        pattern, npattern, carrier, bandwidth, psatcal, dutycycle);
	}
	return -ErrorPrint(LinkNoFunction,"LinkTxDataStartCmd");
}

DEVICEDLLSPEC int LinkRxDataStartCmd(int freq, int cenFreqUsed, int antenna, int rxChain, int promiscuous, int broadcast,
                                    unsigned char *bssid, unsigned char *destination,
                                    int numDescPerRate, int *rate, int nrate, int bandwidth, int datacheck)
{
	if(_Link.RxDataStartCmd!=0)
	{
		return _Link.RxDataStartCmd(freq, cenFreqUsed, antenna, rxChain, promiscuous, broadcast,
                                    bssid, destination, numDescPerRate, rate, nrate, bandwidth, datacheck);
	}
	return -ErrorPrint(LinkNoFunction,"LinkRxDataStartCmd");
}

DEVICEDLLSPEC int LinkTxStatTemperatureGet()
{
	if(_Link.TxStatTemperatureGet!=0)
	{
		return _Link.TxStatTemperatureGet();
	}
	return -ErrorPrint(LinkNoFunction,"LinkTxStatTemperatureGet");
}

//
//
// the following functions are used to set the pointers to the correct functions for a specific chip.
//
// When implementing support for a new chip a function performing each of these operations
// must be produced. See Ar5416Link.c for an example.
//
//
//

int DEVICEDLLSPEC LinkFunctionSelect(struct _LinkFunction *device)
{
	_Link= *device;
	//
	// later check some of the pointers to make sure there are valid functions
	//
	return 0;
}


//
// clear all device control function pointers and set to default behavior
//
void DEVICEDLLSPEC LinkFunctionReset(void)
{
	memset(&_Link, 0, sizeof(_Link));
}


DEVICEDLLSPEC int LinkRxFindSpur(int findspur)
{
	if(_Link.RxFindSpur!=0)
	{
		return _Link.RxFindSpur(findspur);
	}
	return -ErrorPrint(LinkNoFunction,"LinkRxFindSpur");
}
