
#include "wlantype.h"
#include "Field.h"
#include "Qc98xxNoiseFloor.h"


int Qc98xxNoiseFloorFetch(int *nfc, int *nfe, int nfn)
{ 
	A_UINT32 mask;
	int it;
	unsigned int address;
	int high, low;
	//
	// noise floor values are signed. this computes a mask to extend the sign bit
	// it assumes that all the values are the same length.
	//
	mask=0;
	if(FieldFind("BB_cca_b0.minCCApwr_0", &address, &low, &high))
	{
		for(it=high-low+1; it<32; it++)
		{
			mask |= (1<<it);
		}
	}
	//
	// fetch the answers
	//
	FieldRead("BB_cca_b0.minCCApwr_0", (unsigned int *)&nfc[0]);
	nfc[0]|=mask;
	FieldRead("BB_cca_b1.minCCApwr_1", (unsigned int *)&nfc[1]);   
	nfc[1]|=mask;
	FieldRead("BB_cca_b2.minCCApwr_2", (unsigned int *)&nfc[2]);
	nfc[2]|=mask;
    nfe[0] = nfc[0];
    nfe[1] = nfc[1];
    nfe[2] = nfc[2];
	return 0;    
}

int Qc98xxNoiseFloorLoad(int *nfc, int *nfe, int nfn)
{
    FieldWrite("BB_cca_b0.cf_maxCCApwr_0",2*nfc[0]);
    FieldWrite("BB_cca_b1.cf_maxCCApwr_1",2*nfc[1]);
    FieldWrite("BB_cca_b2.cf_maxCCApwr_2",2*nfc[2]);
	FieldWrite("BB_agc_control.enable_noisefloor",0);
	FieldWrite("BB_agc_control.no_update_noisefloor",0);
	FieldWrite("BB_agc_control.do_noisefloor",1);

	return 0;
}

int Qc98xxNoiseFloorReady(void)
{
	unsigned int ready;

	FieldRead("BB_agc_control.do_noisefloor",&ready);
	return ready;
}


int Qc98xxNoiseFloorEnable(void)
{
	FieldWrite("BB_agc_control.enable_noisefloor",1);
	FieldWrite("BB_agc_control.no_update_noisefloor",1);
	FieldWrite("BB_agc_control.do_noisefloor",1);
	return 0;
}


