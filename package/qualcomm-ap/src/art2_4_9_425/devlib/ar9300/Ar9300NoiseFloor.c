

#include "Field.h"
#include "Ar9300NoiseFloor.h"


int Ar9300NoiseFloorFetch(int *nfc, int *nfe, int nfn)
{
	unsigned long mask;
	int it;
	unsigned long address;
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
	FieldRead("BB_cca_b0.minCCApwr_0", (unsigned long *)&nfc[0]);
	nfc[0]|=mask;
	FieldRead("BB_cca_b1.minCCApwr_1", (unsigned long *)&nfc[1]);   
	nfc[1]|=mask;
	FieldRead("BB_cca_b2.minCCApwr_2", (unsigned long *)&nfc[2]);
	nfc[2]|=mask;
	FieldRead("BB_ext_chan_pwr_thr_2_b0.minCCApwr_ext_0", (unsigned long *)&nfe[0]);
	nfe[0]|=mask;
	FieldRead("BB_ext_chan_pwr_thr_2_b1.minCCApwr_ext_1", (unsigned long *)&nfe[1]);
	nfe[1]|=mask;
	FieldRead("BB_ext_chan_pwr_thr_2_b2.minCCApwr_ext_2", (unsigned long *)&nfe[2]);
	nfe[2]|=mask;

	return 0;    
}

int Ar9300NoiseFloorLoad(int *nfc, int *nfe, int nfn)
{
    FieldWrite("BB_cca_b0.cf_maxCCApwr_0",2*nfc[0]);
    FieldWrite("BB_cca_b1.cf_maxCCApwr_1",2*nfc[1]);
    FieldWrite("BB_cca_b2.cf_maxCCApwr_2",2*nfc[2]);
    FieldWrite("BB_ext_chan_pwr_thr_2_b0.cf_maxCCApwr_ext_0",2*nfe[0]);
    FieldWrite("BB_ext_chan_pwr_thr_2_b1.cf_maxCCApwr_ext_1",2*nfe[1]);
    FieldWrite("BB_ext_chan_pwr_thr_2_b2.cf_maxCCApwr_ext_2",2*nfe[2]);

	FieldWrite("BB_agc_control.enable_noisefloor",0);
	FieldWrite("BB_agc_control.no_update_noisefloor",0);
	FieldWrite("BB_agc_control.do_noisefloor",1);

	return 0;
}

int Ar9300NoiseFloorReady(void)
{
	int ready;

	FieldRead("BB_agc_control.do_noisefloor",&ready);
	return ready;
}


int Ar9300NoiseFloorEnable(void)
{
	FieldWrite("BB_agc_control.enable_noisefloor",1);
	FieldWrite("BB_agc_control.no_update_noisefloor",1);
	FieldWrite("BB_agc_control.do_noisefloor",1);
	return 0;
}


