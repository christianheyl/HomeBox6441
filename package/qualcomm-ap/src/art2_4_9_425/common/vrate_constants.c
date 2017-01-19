#ifdef __ATH_DJGPPDOS__
#include <unistd.h>
#ifndef EILSEQ
    #define EILSEQ EIO
#endif  // EILSEQ

 #define __int64    long long
 #define HANDLE long
 typedef unsigned long DWORD;
 #define Sleep  delay
 #include <bios.h>
 #include <dir.h>
#endif  // #ifdef __ATH_DJGPPDOS__

#include "wlantype.h"
#include "athreg.h"
#include "LinkStat.h"
#include "manlib.h"
#include "rate_constants.h"
#include "vrate_constants.h"

const A_UCHAR vRateValues[vNumRateCodes] = {
// 0-47 VHT20 rate 0 - 29 (plus soare
		  0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x86, 0x87,	// mcs0-9
          0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f, 0x86, 0x87,	// mcs10-19
		  0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x86, 0x87,	// mcs20-29
		  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	//mcs30-39 4 stream in future
		  0, 0, 0, 0, 0, 0, 0, 0,		//40 - 47 spare
// 48-95  VHT40 rates 0 - 29 (plus spare
          0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x86, 0x87,	// mcs0-9
          0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f, 0x86, 0x87,	// mcs10-19
		  0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x86, 0x87,	// mcs20-29
		  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	//mcs30-39 4 stream in future
		  0, 0, 0, 0, 0, 0, 0, 0,		// 88 - 95 spare
// 96-143  VHT80 rates 0 - 29 (plus spare
          0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x86, 0x87,	// mcs0-9
          0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f, 0x86, 0x87,	// mcs10-19
		  0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x86, 0x87,	// mcs20-29
		  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	//mcs30-39 4 stream in future
};

//change strings to map to the same array setup as the rateValues above
char *vRateStrAll[vNumRateCodes] = {
        "vt0", "vt1", "vt2", "vt3", "vt4", "vt5", "vt6", "vt7","vt8", "vt9", 
        "vt10", "vt11", "vt12", "vt13", "vt14", "vt15", "vt16", "vt17", "vt18", "vt19",
        "vt20", "vt21", "vt22", "vt23", "vt24", "vt25", "vt26", "vt27", "vt28", "vt29",
        "vt30", "vt31", "vt32", "vt33", "vt34", "vt35", "vt36", "vt37", "vt38", "vt39",
		"", "", "", "", "", "", "", "",
        "vf0", "vf1", "vf2", "vf3", "vf4", "vf5", "vf6", "vf7", "vf8", "vf9", 
        "vf10", "vf11", "vf12", "vf13", "vf14", "vf15", "vf16", "vf17", "vf18", "vf19",
        "vf20", "vf21", "vf22", "vf23", "vf24", "vf25", "vf26", "vf27", "vf28", "vf29",
        "vf30", "vf31", "vf32", "vf33", "vf34", "vf35", "vf36", "vf37", "vf38", "vf39",
		"", "", "", "", "", "", "", "",
        "ve0", "ve1", "ve2", "ve3", "ve4", "ve5", "ve6", "ve7", "ve8", "ve9", 
        "ve10", "ve11", "ve12", "ve13", "ve14", "ve15", "ve16", "ve17", "ve18", "ve19",
        "ve20", "ve21", "ve22", "ve23", "ve24", "ve25", "ve26", "ve27", "ve28", "ve29",
        "ve30", "ve31", "ve32", "ve33", "ve34", "ve35", "ve36", "ve37", "ve38", "ve39",
};

static int vrht20[]={0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29};
static int vrht40[]={48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77};
static int vrht80[]={96,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,123,124,125};
static int vrall[]={0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,
		    48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,
			96,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,123,124,125};

/*
A_UINT32 descvRate2RateIndex(A_UINT32 descRateCode, unsigned char htMode) {
    A_UINT32 rateBin = UNKNOWN_RATE_CODE, i;

    for(i = 0; i < vNumRateCodes; i++) {
        if(descRateCode == vRateValues[i]) {
            rateBin = i;
            break;
        }
    }
    // HT20 & 40 share same code, extra bit decides if it's a 40 rate 
    if ((i >= 48) && (i <= 96) && htMode) {
        rateBin += 48;
    }
    return rateBin;
}*/

// from rateMask to create int Rate[numInputRate] array
// Rate[i], is the ith inputed rate index defined in vRATE_INDEX
int vRateCount(A_UINT32 rateMaskMcs20, A_UINT32 rateMaskMcs40, A_UINT32 rateMaskMcs80, int *Rate)
{
	int ir;
	int numRates;

	numRates=0;
    for(ir = vRATE_INDEX_HT20_MCS0; ir < vRATE_INDEX_HT20_MCS29; ir++)
	{
        if(rateMaskMcs20 & (1 << (ir - vRATE_INDEX_HT20_MCS0))) 
		{
            Rate[numRates] = ir;
			numRates++;
        }
    }
    for(ir = vRATE_INDEX_HT40_MCS0; ir < vRATE_INDEX_HT40_MCS29; ir++)
	{
        if(rateMaskMcs40 & (1 << (ir - vRATE_INDEX_HT40_MCS0))) 
		{
            Rate[numRates] = ir;
			numRates++;
        }
    }
    for(ir = vRATE_INDEX_HT80_MCS0; ir < vRATE_INDEX_HT80_MCS29; ir++)
	{
        if(rateMaskMcs80 & (1 << (ir - vRATE_INDEX_HT80_MCS0))) 
		{
            Rate[numRates] = ir;
			numRates++;
        }
    }
	return numRates;
}

static void vRateInsert(int *list, int many, int *nlist, int nmany)
{
	int im;

	for(im=many-1; im>0; im--)
	{
		list[im+nmany-1]=list[im];
	}
	for(im=0; im<nmany; im++)
	{
		list[im]=nlist[im];
	}
}

// rate[] rate index array constructed from vRateCount() call
// nrate number of rate saved in rate[] 
int vRateExpand(int *rate, int nrate)
{
	int ir;
	int nmove;
	//
	// expand any of the special codes
	//
	for(ir=0; ir<nrate; ir++)
	{
		if(rate[ir]==vRateAll)
		{
			nmove=sizeof(vrall)/sizeof(vrall[0]);
			vRateInsert(&rate[ir],nrate-ir,vrall,nmove);
			nrate+=(nmove-1);
		}
		else if(rate[ir]==vRateHt20)
		{
			nmove=sizeof(vrht20)/sizeof(vrht20[0]);
			vRateInsert(&rate[ir],nrate-ir,vrht20,nmove);
			nrate+=(nmove-1);
		}
		else if(rate[ir]==vRateHt40)
		{
			nmove=sizeof(vrht40)/sizeof(vrht40[0]);
			vRateInsert(&rate[ir],nrate-ir,vrht40,nmove);
			nrate+=(nmove-1);
		}
		else if(rate[ir]==vRateHt80)
		{
			nmove=sizeof(vrht80)/sizeof(vrht80[0]);
			vRateInsert(&rate[ir],nrate-ir,vrht80,nmove);
			nrate+=(nmove-1);
		}
	}
	return nrate;
}

void vRateMaskGet (A_UINT32 *rateMaskMcs20, A_UINT32 *rateMaskMcs40, A_UINT32 *rateMaskMcs80, int *Rate, int nrate)
{
    int i, j;

    *rateMaskMcs20 = 0;
    *rateMaskMcs40 = 0;
    *rateMaskMcs80 = 0;

    for (i = 0; i < nrate; ++i)
    {
        // ht20
        if (Rate[i] < vRATE_INDEX_HT40_MCS0)
        {
            for (j = 0; j < 32; ++j)
            {
                if (Rate[i] == vRATE_INDEX_HT20_MCS0+j)
                {
                    *rateMaskMcs20 |= (1 << j);
                    break;
                }
            }
        }
        // ht40
        else if (Rate[i] < vRATE_INDEX_HT80_MCS0)
        {
            for (j = 0; j < 32; ++j)
            {
                if (Rate[i] == vRATE_INDEX_HT40_MCS0+j)
                {
                    *rateMaskMcs40 |= (1 << j);
                    break;
                }
            }
        }
        // ht80
        else
        {
            for (j = 0; j < 32; ++j)
            {
                if (Rate[i] ==vRATE_INDEX_HT80_MCS0+j)
                {
                    *rateMaskMcs80 |= (1 << j);
                    break;
                }
            }
        }
    }
}

void vRateMask2UtfRateMask (A_UINT32 vrateMaskMcs20, A_UINT32 vrateMaskMcs40, A_UINT32 vrateMaskMcs80,
						   A_UINT32 *utfRateMask)
{
    utfRateMask[2] = (((vrateMaskMcs20) & 0x000003FF) |				//vHT20 (0-9)
                          ((vrateMaskMcs40 << 12) & 0x003FF000) |	//vHt40 (0-9)
                          ((vrateMaskMcs80 << 24) & 0xFF000000));	//vHt80 (0-7)
    utfRateMask[3] = (((vrateMaskMcs80 >> 8) & 0x00000003) |		//vHt80 (8-9)
						  ((vrateMaskMcs20 >> 6) & 0x00003FF0) |	//vHT20 (10-19)
                          ((vrateMaskMcs40 << 6) & 0x03FF0000) |    //vHT40 (10-19)
                          ((vrateMaskMcs80 << 18)& 0xF0000000));	//vHT80 (10-13)
    utfRateMask[4] = (((vrateMaskMcs80 >> 14) & 0x0000003F) |		//vHt80 (14-19)
						  ((vrateMaskMcs20 >> 12) & 0x0003FF00) |	//vHT20 (20-29)
                          ((vrateMaskMcs40) & 0x3FF00000));			//vHT40 (20-29)
    utfRateMask[5] = ((vrateMaskMcs80 >> 20) & 0x000003FF);			//vHT80 (20-29)
}

// UTF's utfRateMask[2] format:
// A_UINT32 utfRateMask[0]   0 - byte 0 is CCK mask
// A_UINT32 utfRateMask[0]   8 - byte 1 is Legacy mask
// A_UINT32 utfRateMask[0]  16 - byte 2 is HT20 stream1 mask (MCS0-7)
// A_UINT32 utfRateMask[0]  24 - byte 3 is HT40 stream1 mask (MCS0-7)
// A_UINT32 utfRateMask[1]  32 - byte 0 is HT20 stream2 mask (MCS8-15)
// A_UINT32 utfRateMask[1]  40 - byte 1 is HT40 stream2 mask (MCS8-15)
// A_UINT32 utfRateMask[1]  48 - byte 2 is HT20 stream3 mask (MCS16-23)
// A_UINT32 utfRateMask[1]  56 - byte 3 is HT40 stream3 mask (MCS16-23)
// A_UINT32 utfRateMask[2]  64 - bits 0:9   is vHT20 stream1 mask (MCS0-9)
// A_UINT32 utfRateMask[2]  76 - bits 12:21 is vHT40 stream1 mask (MCS0-9)
// A_UINT32 utfRateMask[2]  88 - bits 24:31 is vHT80 stream1 mask (MCS0-7)
// A_UINT32 utfRateMask[3]  96 - bits 0:1   is vHT80 stream1 mask (MCS8-9)
// A_UINT32 utfRateMask[3] 100 - bits 4:13  is vHT20 stream2 mask (MCS10-19)
// A_UINT32 utfRateMask[3] 112 - bits 16:25 is vHT40 stream2 mask (MCS10-19)
// A_UINT32 utfRateMask[3] 124 - bits 28:31 is vHT80 stream2 mask (MCS10-13)
// A_UINT32 utfRateMask[4] 128 - bits 0:5   is vHT80 stream2 mask (MCS14-19)
// A_UINT32 utfRateMask[4] 136 - bits 8:17  is vHT20 stream3 mask (MCS20-29)
// A_UINT32 utfRateMask[4] 148 - bits 20:29 is vHT40 stream3 mask (MCS20-29)
// A_UINT32 utfRateMask[5] 160 - bits 0:9   is vHT80 stream3 mask (MCS20-29)

int UtfvRateBit2RateIndx(A_UINT32 rateBit)
{
	int rateIndx = -1;

	if ( rateBit < 64)
	{
		return UtfRateBit2RateIndx(rateBit);
	}
	//Check for invalid rateBit
    if ((rateBit >= 64) && (rateBit < 74))			//vHT20 (0-9)
    {
        rateIndx = vRATE_INDEX_HT20_MCS0 + rateBit - 64;
    }
    else if ((rateBit >= 76) && (rateBit < 86))		//vHt40 (0-9)
    {
        rateIndx = vRATE_INDEX_HT40_MCS0 + rateBit - 76;
    }
    else if ((rateBit >= 88) && (rateBit < 98))		//vHt80 (0-9)
    {
        rateIndx = vRATE_INDEX_HT80_MCS0 + rateBit - 88;
    }
    else if ((rateBit >= 100) && (rateBit < 110))	//vHT20 (10-19)
    {
        rateIndx = vRATE_INDEX_HT20_MCS10 + rateBit - 100;
	}
    else if ((rateBit >= 112) && (rateBit < 122))	//vHT40 (10-19)
    {
        rateIndx = vRATE_INDEX_HT40_MCS10 + rateBit - 112;
	}
    else if ((rateBit >= 124) && (rateBit < 134))	//vHT80 (10-19)
    {
        rateIndx = vRATE_INDEX_HT80_MCS10 + rateBit - 124;
	}
    else if ((rateBit >= 136) && (rateBit < 146))	//vHT20 (20-29)
    {
        rateIndx = vRATE_INDEX_HT20_MCS20 + rateBit - 136;
	}
    else if ((rateBit >= 148) && (rateBit < 158))	//vHT40 (20-29)
    {
        rateIndx = vRATE_INDEX_HT40_MCS20 + rateBit - 148;
	}
    else if ((rateBit >= 160) && (rateBit < 170))	//vHT80 (20-29)
    {
        rateIndx = vRATE_INDEX_HT80_MCS20 + rateBit - 160;
	}
	else
	{
		printf ("UtfvRateBit2RateIndx - Invalid rate bit\n");
	}
	return rateIndx;
}

int vRateIndx2UtfRateBit(int rateIndx)
{
    int rateBit = -1;

	if (rateIndx < vRATE_INDEX_HT20_MCS0)
	{
		return RateIndx2UtfRateBit(rateIndx);
	}

    if (rateIndx <= vRATE_INDEX_HT20_MCS9)
    {
        rateBit = rateIndx - vRATE_INDEX_HT20_MCS0 + 64;
    }
    else if (rateIndx <= vRATE_INDEX_HT20_MCS19)
    {
        rateBit = rateIndx - vRATE_INDEX_HT20_MCS10 + 100;
    }
    else if (rateIndx <= vRATE_INDEX_HT20_MCS29)
    {
        rateBit = rateIndx - vRATE_INDEX_HT20_MCS20 + 136;
    }
    else if (rateIndx <= vRATE_INDEX_HT40_MCS9)
    {
        rateBit = rateIndx - vRATE_INDEX_HT40_MCS0 + 76;
    }
    else if (rateIndx <= vRATE_INDEX_HT40_MCS19)
    {
        rateBit = rateIndx - vRATE_INDEX_HT40_MCS10 + 112;
    }
    else if (rateIndx <= vRATE_INDEX_HT40_MCS29)
    {
        rateBit = rateIndx - vRATE_INDEX_HT40_MCS20 + 148;
    }
	else if (rateIndx <= vRATE_INDEX_HT80_MCS9)
	{
        rateBit = rateIndx - vRATE_INDEX_HT80_MCS0 + 88;
    }
	else if (rateIndx <= vRATE_INDEX_HT80_MCS19)
	{
        rateBit = rateIndx - vRATE_INDEX_HT80_MCS10 + 124;
    }
	else if (rateIndx <= vRATE_INDEX_HT80_MCS29)
	{
        rateBit = rateIndx - vRATE_INDEX_HT80_MCS20 + 160;
    }
    return rateBit;
}

    // UTF's utfRateMask[2] format:
// A_UINT32 utfRateMask[0]   0 - byte 0 is CCK mask
// A_UINT32 utfRateMask[0]   8 - byte 1 is Legacy mask
// A_UINT32 utfRateMask[0]  16 - byte 2 is HT20 stream1 mask (MCS0-7)
// A_UINT32 utfRateMask[0]  24 - byte 3 is HT40 stream1 mask (MCS0-7)
// A_UINT32 utfRateMask[1]  32 - byte 0 is HT20 stream2 mask (MCS8-15)
// A_UINT32 utfRateMask[1]  40 - byte 1 is HT40 stream2 mask (MCS8-15)
// A_UINT32 utfRateMask[1]  48 - byte 2 is HT20 stream3 mask (MCS16-23)
// A_UINT32 utfRateMask[1]  56 - byte 3 is HT40 stream3 mask (MCS16-23)
// A_UINT32 utfRateMask[2]  64 - bits 0:9   is vHT20 stream1 mask (MCS0-9)
// A_UINT32 utfRateMask[2]  76 - bits 12:21 is vHT40 stream1 mask (MCS0-9)
// A_UINT32 utfRateMask[2]  88 - bits 24:31 is vHT80 stream1 mask (MCS0-7)
// A_UINT32 utfRateMask[3]  96 - bits 0:1   is vHT80 stream1 mask (MCS8-9)
// A_UINT32 utfRateMask[3] 100 - bits 4:13  is vHT20 stream2 mask (MCS10-19)
// A_UINT32 utfRateMask[3] 112 - bits 16:25 is vHT40 stream2 mask (MCS10-19)
// A_UINT32 utfRateMask[3] 124 - bits 28:31 is vHT80 stream2 mask (MCS10-13)
// A_UINT32 utfRateMask[4] 128 - bits 0:5   is vHT80 stream2 mask (MCS14-19)
// A_UINT32 utfRateMask[4] 136 - bits 8:17  is vHT20 stream3 mask (MCS20-29)
// A_UINT32 utfRateMask[4] 148 - bits 20:29 is vHT40 stream3 mask (MCS20-29)
// A_UINT32 utfRateMask[5] 160 - bits 0:9   is vHT80 stream3 mask (MCS20-29)
void WLAN_Rate2UtfRateMask(unsigned int wlanRate, unsigned int *utfRateMask)
{
	int i=0;
	for (i=0; i<6; i++)
		utfRateMask[i] = 0;
	if (wlanRate <= RATE_11B_SHORT_11_MBPS) {
		utfRateMask[0] = 1 << wlanRate;
	} else if (wlanRate <= RATE_11A_54_MBPS) {
		utfRateMask[0] = 1 << (wlanRate-RATE_11A_6_MBPS+8);
	} else if (wlanRate <= RATE_11N_HT20_MCS7) {
		utfRateMask[0] = 1 << (wlanRate-RATE_11N_HT20_MCS0+16);
	} else if (wlanRate <= RATE_11N_HT40_MCS7) {
		utfRateMask[0] = 1 << (wlanRate-RATE_11N_HT40_MCS0+24);
	} else if (wlanRate <= RATE_11AC_HT20_MCS9) {
		utfRateMask[2] = 1 << (wlanRate-RATE_11AC_HT20_MCS0);
	} else if (wlanRate <= RATE_11AC_HT40_MCS9) {
		utfRateMask[2] = 1 << (wlanRate-RATE_11AC_HT40_MCS0+12);
	} else if (wlanRate <= RATE_11AC_HT80_MCS7) {
		utfRateMask[2] = 1 << (wlanRate-RATE_11AC_HT80_MCS0+24);
	} else if (wlanRate <= RATE_11AC_HT80_MCS9) {
		utfRateMask[3] = 1 << (wlanRate-RATE_11AC_HT80_MCS8);
	} else if (wlanRate <= RATE_11N_HT20_MCS15) {
		utfRateMask[1] = 1 << (wlanRate-RATE_11N_HT20_MCS8);
	} else if (wlanRate <= RATE_11N_HT40_MCS15) {
		utfRateMask[1] = 1 << (wlanRate-RATE_11N_HT40_MCS8+8);
	} else if (wlanRate <= RATE_11AC_HT20_MCS9_2S) {
		utfRateMask[3] = 1 << (wlanRate-RATE_11AC_HT20_MCS0_2S+4);
	} else if (wlanRate <= RATE_11AC_HT40_MCS9_2S) {
		utfRateMask[3] = 1 << (wlanRate-RATE_11AC_HT40_MCS0_2S+16);
	} else if (wlanRate <= RATE_11AC_HT80_MCS3_2S) {
		utfRateMask[3] = 1 << (wlanRate-RATE_11AC_HT80_MCS0_2S+28);
	} else if (wlanRate <= RATE_11AC_HT80_MCS9_2S) {
		utfRateMask[4] = 1 << (wlanRate-RATE_11AC_HT80_MCS4_2S);
	} else if (wlanRate <= RATE_11N_HT20_MCS23) {
		utfRateMask[1] = 1 << (wlanRate-RATE_11N_HT20_MCS16+16);
	} else if (wlanRate <= RATE_11N_HT40_MCS23) {
		utfRateMask[1] = 1 << (wlanRate-RATE_11N_HT40_MCS16+24);
	} else if (wlanRate <= RATE_11AC_HT20_MCS9_3S) {
		utfRateMask[4] = 1 << (wlanRate-RATE_11AC_HT20_MCS0_3S+8);
	} else if (wlanRate <= RATE_11AC_HT40_MCS9_3S) {
		utfRateMask[4] = 1 << (wlanRate-RATE_11AC_HT40_MCS0_3S+20);
	} else if (wlanRate <= RATE_11AC_HT80_MCS9_3S) {
		utfRateMask[5] = 1 << (wlanRate-RATE_11AC_HT80_MCS0_3S);
	}
}

// find the Next Rx RateIndex from Rx rateMask
int findNextRxRateIndx(int RateIndxStart, unsigned int *utfRateMask)
{
	int i=0, RateIndx = -1;
	if (RateIndxStart <= RATE_INDEX_54) {
		if ((utfRateMask[0] & 0x0000FF00)!=0) {
			for (i=RATE_INDEX_6; i<= RATE_INDEX_54; i++) {
				if ( ( (utfRateMask[0]>>8) & (1<<(i-RATE_INDEX_6)) )!=0) {
					if (RateIndxStart<i) 
						return i;
				}
			}
		}
	}
	if (RateIndxStart <= RATE_INDEX_11S) {
		if ((utfRateMask[0] & 0x000000FF)!=0) {
			for (i=RATE_INDEX_1L; i<= RATE_INDEX_11S; i++) {
				if ( (utfRateMask[0] & (1<<(i-RATE_INDEX_1L)) )!=0) {
					if (RateIndxStart<i) 
						return i;
				}
			}
		}
	}
	if (RateIndxStart <= RATE_INDEX_HT20_MCS7) {
		if ((utfRateMask[0] & 0x00FF0000)!=0) {
			for (i=RATE_INDEX_HT20_MCS0; i<= RATE_INDEX_HT20_MCS7; i++) {
				if ( ( (utfRateMask[0]>>16) & (1<<(i-RATE_INDEX_HT20_MCS0)) )!=0) {
					if (RateIndxStart<i) 
						return i;
				}
			}
		}
	}
	if (RateIndxStart <= RATE_INDEX_HT20_MCS15) {
		if ((utfRateMask[1] & 0x000000FF)!=0) {
			for (i=RATE_INDEX_HT20_MCS8; i<= RATE_INDEX_HT20_MCS15; i++) {
				if ( ( (utfRateMask[1]) & (1<<(i-RATE_INDEX_HT20_MCS8)) )!=0) {
					if (RateIndxStart<i) 
						return i;
				}
			}
		}
	}
	if (RateIndxStart <= RATE_INDEX_HT20_MCS23) {
		if ((utfRateMask[1] & 0x00FF0000)!=0) {
			for (i=RATE_INDEX_HT20_MCS16; i<= RATE_INDEX_HT20_MCS23; i++) {
				if ( ( (utfRateMask[1]>>16) & (1<<(i-RATE_INDEX_HT20_MCS16)) )!=0) {
					if (RateIndxStart<i) 
						return i;
				}
			}
		}
	}
	if (RateIndxStart <= RATE_INDEX_HT40_MCS7) {
		if ((utfRateMask[0] & 0xFF000000)!=0) {
			for (i=RATE_INDEX_HT40_MCS0; i<= RATE_INDEX_HT40_MCS7; i++) {
				if ( ( (utfRateMask[0]>>24) & (1<<(i-RATE_INDEX_HT40_MCS0)) )!=0) {
					if (RateIndxStart<i) 
						return i;
				}
			}
		}
	}
	if (RateIndxStart <= RATE_INDEX_HT40_MCS15) {
		if ((utfRateMask[1] & 0x0000FF00)!=0) {
			for (i=RATE_INDEX_HT40_MCS8; i<= RATE_INDEX_HT40_MCS15; i++) {
				if ( ( (utfRateMask[1]>>8) & (1<<(i-RATE_INDEX_HT40_MCS8)) )!=0) {
					if (RateIndxStart<i) 
						return i;
				}
			}
		}
	}
	if (RateIndxStart <= RATE_INDEX_HT40_MCS23) {
		if ((utfRateMask[1] & 0xFF000000)!=0) {
			for (i=RATE_INDEX_HT40_MCS16; i<= RATE_INDEX_HT40_MCS23; i++) {
				if ( ( (utfRateMask[1]>>24) & (1<<(i-RATE_INDEX_HT40_MCS16)) )!=0) {
					if (RateIndxStart<i) 
						return i;
				}
			}
		}
	}
	if (RateIndxStart <= vRATE_INDEX_HT20_MCS9) {
		if ((utfRateMask[2] & 0x000003FF)!=0) {
			for (i=vRATE_INDEX_HT20_MCS0; i<= vRATE_INDEX_HT20_MCS9; i++) {
				if ( ( (utfRateMask[2]) & (1<<(i-vRATE_INDEX_HT20_MCS0)) )!=0) {
					if (RateIndxStart<i) 
						return i;
				}
			}
		}
	}
	if (RateIndxStart <= vRATE_INDEX_HT20_MCS19) {
		if ((utfRateMask[3] & 0x00003FF0)!=0) {
			for (i=vRATE_INDEX_HT20_MCS10; i<= vRATE_INDEX_HT20_MCS19; i++) {
				if ( ( (utfRateMask[3]>>4) & (1<<(i-vRATE_INDEX_HT20_MCS10)) )!=0) {
					if (RateIndxStart<i) 
						return i;
				}
			}
		}
	}
	if (RateIndxStart <= vRATE_INDEX_HT20_MCS29) {
		if ((utfRateMask[4] & 0x0003FF00)!=0) {
			for (i=vRATE_INDEX_HT20_MCS20; i<= vRATE_INDEX_HT20_MCS29; i++) {
				if ( ( (utfRateMask[4]>>8) & (1<<(i-vRATE_INDEX_HT20_MCS20)) )!=0) {
					if (RateIndxStart<i) 
						return i;
				}
			}
		}
	}
	if (RateIndxStart <= vRATE_INDEX_HT40_MCS9) {
		if ((utfRateMask[2] & 0x003FF000)!=0) {
			for (i=vRATE_INDEX_HT40_MCS0; i<= vRATE_INDEX_HT40_MCS9; i++) {
				if ( ( (utfRateMask[2]>>12) & (1<<(i-vRATE_INDEX_HT40_MCS0)) )!=0) {
					if (RateIndxStart<i) 
						return i;
				}
			}
		}
	}
	if (RateIndxStart <= vRATE_INDEX_HT40_MCS19) {
		if ((utfRateMask[3] & 0x03FF0000)!=0) {
			for (i=vRATE_INDEX_HT40_MCS10; i<= vRATE_INDEX_HT40_MCS19; i++) {
				if ( ( (utfRateMask[3]>>16) & (1<<(i-vRATE_INDEX_HT40_MCS10)) )!=0) {
					if (RateIndxStart<i) 
						return i;
				}
			}
		}
	}
	if (RateIndxStart <= vRATE_INDEX_HT40_MCS29) {
		if ((utfRateMask[4] & 0x3FF00000)!=0) {
			for (i=vRATE_INDEX_HT40_MCS20; i<= vRATE_INDEX_HT40_MCS29; i++) {
				if ( ( (utfRateMask[4]>>20) & (1<<(i-vRATE_INDEX_HT40_MCS20)) )!=0) {
					if (RateIndxStart<i) 
						return i;
				}
			}
		}
	}
	if (RateIndxStart <= vRATE_INDEX_HT80_MCS7) {
		if ((utfRateMask[2] & 0xFF000000)!=0) {
			for (i=vRATE_INDEX_HT80_MCS0; i<= vRATE_INDEX_HT80_MCS7; i++) {
				if ( ( (utfRateMask[2]>>24) & (1<<(i-vRATE_INDEX_HT80_MCS0)) )!=0) {
					if (RateIndxStart<i) 
						return i;
				}
			}
		}
	}
	if (RateIndxStart <= vRATE_INDEX_HT80_MCS9) {
		if ((utfRateMask[3] & 0x00000003)!=0) {
			for (i=vRATE_INDEX_HT80_MCS8; i<= vRATE_INDEX_HT80_MCS9; i++) {
				if ( ( (utfRateMask[3]) & (1<<(i-vRATE_INDEX_HT80_MCS8)) )!=0) {
					if (RateIndxStart<i) 
						return i;
				}
			}
		}
	}
	if (RateIndxStart <= vRATE_INDEX_HT80_MCS13) {
		if ((utfRateMask[3] & 0xF0000000)!=0) {
			for (i=vRATE_INDEX_HT80_MCS10; i<= vRATE_INDEX_HT80_MCS13; i++) {
				if ( ( (utfRateMask[3]>>28) & (1<<(i-vRATE_INDEX_HT80_MCS10)) )!=0) {
					if (RateIndxStart<i) 
						return i;
				}
			}
		}
	}
	if (RateIndxStart <= vRATE_INDEX_HT80_MCS19) {
		if ((utfRateMask[4] & 0x0000003F)!=0) {
			for (i=vRATE_INDEX_HT80_MCS14; i<= vRATE_INDEX_HT80_MCS19; i++) {
				if ( ( (utfRateMask[4]) & (1<<(i-vRATE_INDEX_HT80_MCS14)) )!=0) {
					if (RateIndxStart<i) 
						return i;
				}
			}
		}
	}
	if (RateIndxStart <= vRATE_INDEX_HT80_MCS29) {
		if ((utfRateMask[5] & 0x000003FF)!=0) {
			for (i=vRATE_INDEX_HT80_MCS20; i<= vRATE_INDEX_HT80_MCS29; i++) {
				if ( ( (utfRateMask[5]) & (1<<(i-vRATE_INDEX_HT80_MCS20)) )!=0) {
					if (RateIndxStart<i) 
						return i;
				}
			}
		}
	}

	return -1;
}

A_BOOL Is40MHzCenterFrequency(int frequency)
{
    switch (frequency)
    {
        case 5190:
        case 5230:
        case 5270:
        case 5310:
        case 5510:
        case 5550:
        case 5590:
        case 5630:
        case 5670:
        case 5710:
        case 5755:
        case 5795:
            return TRUE;
        default:
            return FALSE;
    }
}

A_BOOL Is80MHzCenterFrequency(int frequency)
{
    switch (frequency)
    {
        case 5210:
        case 5290:
        case 5530:
        case 5610:
        case 5690:
        case 5775:
            return TRUE;
        default:
            return FALSE;
    }
}
