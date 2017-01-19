#include "Field.h"

int Ar9287NoiseFloorFetch(int *nfc, int *nfe, int nfn)
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
	//if(FieldFind("BB_cca_b0.minCCApwr_0", &address, &low, &high))
	if(FieldFind("CCA.bb_minCCApwr", &address, &low, &high))
	{
		for(it=high-low+1; it<32; it++)
		{
			mask |= (1<<it);
		}
	}
	//
	// fetch the answers
	//
#if 1
    FieldRead("CCA.bb_minCCApwr", (unsigned long *)&nfc[0]);
	nfc[0]|=mask;
	nfc[1]=0;
	nfc[2]=0;
	FieldRead("AGC_EXT.bb_minCCApwr_ext", (unsigned long *)&nfe[0]);
	nfe[0]|=mask;
	nfe[1]=0;
	nfe[2]=0;
#else
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
#endif
	return 0;    
}

int Ar9287NoiseFloorLoad(int *nfc, int *nfe, int nfn)
{
#if 1
    FieldWrite("CCA.bb_maxCCApwr",2*nfc[0]);
    FieldWrite("AGC_EXT.bb_maxCCApwr_ext",2*nfe[0]);
#else
    FieldWrite("BB_cca_b0.cf_maxCCApwr_0",2*nfc[0]);
    FieldWrite("BB_cca_b1.cf_maxCCApwr_1",2*nfc[1]);
    FieldWrite("BB_cca_b2.cf_maxCCApwr_2",2*nfc[2]);
    FieldWrite("BB_ext_chan_pwr_thr_2_b0.cf_maxCCApwr_ext_0",2*nfe[0]);
    FieldWrite("BB_ext_chan_pwr_thr_2_b1.cf_maxCCApwr_ext_1",2*nfe[1]);
    FieldWrite("BB_ext_chan_pwr_thr_2_b2.cf_maxCCApwr_ext_2",2*nfe[2]);
#endif

	FieldWrite("AGC_CTL2.bb_enable_noisefloor",0);
	FieldWrite("AGC_CTL2.bb_no_update_noisefloor",0);
	FieldWrite("AGC_CTL2.bb_do_noisefloor",1);

	return 0;
}

int Ar9287NoiseFloorReady(void)
{
	int ready;

	FieldRead("AGC_CTL2.bb_do_noisefloor",&ready);
	return ready;
}

int Ar9287NoiseFloorEnable(void)
{
	FieldWrite("AGC_CTL2.bb_enable_noisefloor",1);
	FieldWrite("AGC_CTL2.bb_no_update_noisefloor",1);
	FieldWrite("AGC_CTL2.bb_do_noisefloor",1);
	return 0;
}

void Ar9287_REG_enable_noisefloor_Write(int value)
{
    FieldWrite("AGC_CTL2.bb_enable_noisefloor",value);
}

void Ar9287_REG_no_update_noisefloor_Write(int value)
{
    FieldWrite("AGC_CTL2.bb_no_update_noisefloor",value);
}

void Ar9287_REG_do_noisefloor_Write(int value)
{
    FieldWrite("AGC_CTL2.bb_do_noisefloor",value);
}

void Ar9287_REG_do_noisefloor_Read(unsigned long *value)
{
    FieldRead("AGC_CTL2.bb_do_noisefloor",value);
}
