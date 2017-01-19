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

const A_UCHAR rateValues[numRateCodes] = {
// 00-07   6   9  12  18 24  36 48  54
          11, 15, 10, 14, 9, 13, 8, 12,
// 08-14  1L   2L   2S   5.5L 5.5S 11L  11S
          0x1b,0x1a,0x1e,0x19,0x1d,0x18,0x1c,
// 15-23  0.25 0.5 1  2  3
          3,   7,  2, 6, 1, 0, 0, 0, 0,
// 24-31
    0, 0, 0, 0, 0, 0, 0, 0,
// 32-63  MCS 20 rates 0 - 23 (plus spare
          0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
          0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f,
		  0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97,
		  0, 0, 0, 0, 0, 0, 0, 0, //56-63 spare
// 64-95  MCS 40 rates 0 - 15
          0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
          0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f,
		  0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97,
		  0, 0, 0, 0, 0, 0, 0, 0, //88-95 spare
};

//change strings to map to the same array setup as the rateValues above
char *rateStrAll[numRateCodes] = {
        "6", "9", "12", "18", "24", "36", "48", "54",
        "1L", "2L", "2S", "5L", "5S", "11L", "11S",".25", 
		".5", "1XR", "2XR", "3XR", "", "", "", "",
		"", "", "", "", "", "", "", "",
        "t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7",
        "t8", "t9", "t10", "t11", "t12", "t13", "t14", "t15",
        "t16", "t17", "t18", "t19", "t20", "t21", "t22", "t23",
		"", "", "", "", "", "", "", "",
        "f0", "f1", "f2", "f3", "f4", "f5", "f6", "f7",
        "f8", "f9", "f10", "f11", "f12", "f13", "f14", "f15",
        "f16", "f17", "f18", "f19", "f20", "f21", "f22", "f23",
		"", "", "", "", "", "", "", "",
    };

static int rlegacy[]={0,1,2,3,4,5,6,7,8,9,10,11,12,13,14};
static int rht20[]={32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55};
static int rht40[]={64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87};
static int rall[]={0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,
		    32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,
		    64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87};

const A_UCHAR rateCodes[numRateCodes] =  {
    6,    9,    12,   18,  24,   36,   48,    54,
    0xb1, 0xb2, 0xd2, 0xb5, 0xd5, 0xbb, 0xdb,
//  XR0.25 XR0.5  XR1   XR2   XR3
    0xea, 0xeb,  0xe1, 0xe2, 0xe3, 0, 0, 0, 0,
    0, 0 ,0 ,0 ,0, 0, 0, 0,
//  MCS 20 rates 0 - 15
    0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f,
//  MCS 40 rates 0 - 15 - same rate encoding though 40 bit is also set in descriptor
    0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f
};

#ifdef UNUSED
A_UINT32 rate2bin(A_UINT32 rateCode) {
    A_UINT32 rateBin = UNKNOWN_RATE_CODE, i;

    if (rateCode == 0) {
        // provides access to generic stats bin for all rates
        return 0;
    }
    for(i = 0; i < numRateCodes; i++) {
        if(rateCode == rateCodes[i]) {
            rateBin = i;
            break;
        }
    }
    return rateBin;
}
#endif

A_UINT32 descRate2RateIndex(A_UINT32 descRateCode, A_BOOL ht40) {
    A_UINT32 rateBin = UNKNOWN_RATE_CODE, i;

    for(i = 0; i < numRateCodes; i++) {
        if(descRateCode == rateValues[i]) {
            rateBin = i;
            break;
        }
    }
    /* HT20 & 40 share same code, extra bit decides if it's a 40 rate */
    if ((i >= 32) && (i <= 63) && ht40) {
        rateBin += 32;
    }
    return rateBin;
}


int RateCount(A_UINT32 rateMask, A_UINT32 rateMaskMcs20, A_UINT32 rateMaskMcs40, int *Rate)
{
	int ir;
	int numRates;

	numRates=0;
    for(ir = RATE_INDEX_6; ir <= RATE_INDEX_11S; ir++)
	{
        if(rateMask & (1<<ir)) 
		{
            Rate[numRates] = ir;
			numRates++;
        }
    }
    for(ir = RATE_INDEX_HT20_MCS0; ir < RATE_INDEX_HT40_MCS0; ir++)
	{
        if(rateMaskMcs20 & (1 << (ir - RATE_INDEX_HT20_MCS0))) 
		{
            Rate[numRates] = ir;
			numRates++;
        }
    }
    for(ir = RATE_INDEX_HT40_MCS0; ir < 96; ir++)
	{
        if(rateMaskMcs40 & (1 << (ir - RATE_INDEX_HT40_MCS0))) 
		{
            Rate[numRates] = ir;
			numRates++;
        }
    }
	return numRates;
}

static void RateInsert(int *list, int many, int *nlist, int nmany)
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

int RateExpand(int *rate, int nrate)
{
	int ir;
	int nmove;
	//
	// expand any of the special codes
	//
	for(ir=0; ir<nrate; ir++)
	{
		if(rate[ir]==RateAll)
		{
			nmove=sizeof(rall)/sizeof(rall[0]);
			RateInsert(&rate[ir],nrate-ir,rall,nmove);
			nrate+=(nmove-1);
		}
		else if(rate[ir]==RateLegacy)
		{
			nmove=sizeof(rlegacy)/sizeof(rlegacy[0]);
			RateInsert(&rate[ir],nrate-ir,rlegacy,nmove);
			nrate+=(nmove-1);
		}
		else if(rate[ir]==RateHt20)
		{
			nmove=sizeof(rht20)/sizeof(rht20[0]);
			RateInsert(&rate[ir],nrate-ir,rht20,nmove);
			nrate+=(nmove-1);
		}
		else if(rate[ir]==RateHt40)
		{
			nmove=sizeof(rht40)/sizeof(rht40[0]);
			RateInsert(&rate[ir],nrate-ir,rht40,nmove);
			nrate+=(nmove-1);
		}
	}
	return nrate;
}

void RateMaskGet (A_UINT32 *rateMask, A_UINT32 *rateMaskMcs20, A_UINT32 *rateMaskMcs40, int *Rate, int nrate)
{
    int i, j;

    *rateMask = 0;
    *rateMaskMcs20 = 0;
    *rateMaskMcs40 = 0;

    for (i = 0; i < nrate; ++i)
    {
        // legacy
        if (Rate[i] < rht20[0])
        {
            for (j = 0; j < (sizeof(rlegacy)/sizeof(int)); ++j)
            {
                if (Rate[i] == rlegacy[j])
                {
                    *rateMask |= (1 << j);
                    break;
                }
            }
        }
        // ht20
        else if (Rate[i] < rht40[0])
        {
            for (j = 0; j < (sizeof(rht20)/sizeof(int)); ++j)
            {
                if (Rate[i] == rht20[j])
                {
                    *rateMaskMcs20 |= (1 << j);
                    break;
                }
            }
        }
        // ht40
        else
        {
            for (j = 0; j < (sizeof(rht40)/sizeof(int)); ++j)
            {
                if (Rate[i] == rht40[j])
                {
                    *rateMaskMcs40 |= (1 << j);
                    break;
                }
            }
        }
    }
}

#ifdef UNUSED
A_UINT32 descRate2bin(A_UINT32 descRateCode) {
	A_UINT32 rateBin = 0, i;
	//A_UCHAR rateValues[]={6, 10};
	for(i = 0; i < NUM_RATES; i++) {
		if(descRateCode == rateValues[i]) {
			rateBin = i;
			break;
		}
	}
	return rateBin;
}

#endif //UNUSED

void RateMask2UtfRateMask (A_UINT32 rateMask, A_UINT32 rateMaskMcs20, A_UINT32 rateMaskMcs40,
						   A_UINT32 *utfRateMask)
{
    // UTF's utfRateMask[2] format:
    // A_UINT32 utfRateMask[0] - byte 0 is CCK mask
    // A_UINT32 utfRateMask[0] - byte 1 is Legacy mask
    // A_UINT32 utfRateMask[0] - byte 2 is HT20 mask (stream 1)
    // A_UINT32 utfRateMask[0] - byte 3 is HT40 mask (stream 1)
    // A_UINT32 utfRateMask[1] - byte 0 is HT20 mask (stream 2)
    // A_UINT32 utfRateMask[1] - byte 1 is HT40 mask (stream 2)
    // A_UINT32 utfRateMask[1] - byte 2 is HT20 mask (stream 3)
    // A_UINT32 utfRateMask[1] - byte 3 is HT40 mask (stream 3)

    utfRateMask[0] = (((rateMask >> 8) & 0x000000FF) |         // CCK
                          ((rateMask << 8) & 0x0000FF00) |         // legacy
                          ((rateMaskMcs20 << 16) & 0x00FF0000) |   // HT20 (0 - 7)
                          ((rateMaskMcs40 << 24) & 0xFF000000));    // HT40 (0 - 7)
    utfRateMask[1] = (((rateMaskMcs20 >> 8) & 0x000000FF) |    // HT20 (8 - 15)
                          (rateMaskMcs40 & 0x0000FF00) |           // HT40 (8 - 15)
                          (rateMaskMcs20 & 0x00FF0000) |           // HT20 (16 - 23)
                          ((rateMaskMcs40 << 8) & 0xFF000000));     // HT40 (16 - 23)
}

int UtfRateBit2RateIndx(A_UINT32 rateBit)
{
	int rateIndx = -1;

	// CCK
    if (rateBit < 8)
    {
        rateIndx = rateBit + 8;
    }
	// Legacy
    else if (rateBit < 16)
    {
        rateIndx = rateBit - 8;
    }
	// HT20 MCS0 - MSC7
    else if (rateBit < 24)
    {
        rateIndx = rateBit + RATE_INDEX_HT20_MCS0 - 16;
    }
	// HT40 MCS0 - MCS7
    else if (rateBit < 32)
    {
        rateIndx = rateBit + RATE_INDEX_HT40_MCS0 - 24;
	}
	// HT20 MCS8 - MSC15
    else if (rateBit < 40)
    {
        rateIndx = rateBit + RATE_INDEX_HT20_MCS8 - 32;
	}
	// HT40 MCS8 - MSC15
    else if (rateBit < 48)
    {
        rateIndx = rateBit + RATE_INDEX_HT40_MCS8 - 40;
	}
	// HT20 MCS16 - MSC23
    else if (rateBit < 56)
    {
        rateIndx = rateBit + RATE_INDEX_HT20_MCS16 - 48;
	}
	// HT40 MCS16 - MSC23
    else if (rateBit < 64)
	{
        rateIndx = rateBit + RATE_INDEX_HT40_MCS16 - 56;
	}
	return rateIndx;
}

int RateIndx2UtfRateBit(int rateIndx)
{
    int rateBit = -1;

    if (rateIndx <= RATE_INDEX_54)
    {
        rateBit = rateIndx - RATE_INDEX_6 + 8;
    }
    else if (rateIndx <= RATE_INDEX_11S)
    {
        rateBit = rateIndx - RATE_INDEX_1L;
    }
    else if (rateIndx <= RATE_INDEX_HT20_MCS7)
    {
        rateBit = rateIndx - RATE_INDEX_HT20_MCS0 + 16;
    }
    else if (rateIndx <= RATE_INDEX_HT20_MCS15)
    {
        rateBit = rateIndx - RATE_INDEX_HT20_MCS8 + 32;
    }
    else if (rateIndx <= RATE_INDEX_HT20_MCS23)
    {
        rateBit = rateIndx - RATE_INDEX_HT20_MCS16 + 48;
    }
    else if (rateIndx <= RATE_INDEX_HT40_MCS7)
    {
        rateBit = rateIndx - RATE_INDEX_HT40_MCS0 + 24;
    }
    else if (rateIndx <= RATE_INDEX_HT40_MCS15)
    {
        rateBit = rateIndx - RATE_INDEX_HT40_MCS8 + 40;
    }
    else if (rateIndx <= RATE_INDEX_HT40_MCS23)
    {
        rateBit = rateIndx - RATE_INDEX_HT40_MCS16 + 56;
    }
    return rateBit;
}

enum { 
	BW_VHT80_0 = 80, 
	BW_VHT80_1, 
	BW_VHT80_2, 
	BW_VHT80_3 
}; 
 
static A_BOOL InVHT80CenterFreq(int frequency) 
{ 
  switch (frequency) { 
	case 5210: 
	case 5290: 
	case 5530: 
	case 5610: 
	case 5690: 
	case 5775: return 1; 
	default  : return 0; 
  } 
} 
 
static A_BOOL InVHT40CenterFreq(int frequency) 
{ 
  switch (frequency) { 
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
	case 5795: return 1; 
	default  : return 0; 
  } 
} 
 
int AdjustCenterFreqBasedOnRateAndBandwidth(int frequency, int *rate, int bandwidth) 
{ 
	int offFreq = 0; 
 
	if (rate[2]>0) {  //  HT40 
	  if (InVHT40CenterFreq(frequency)) 
	    if (bandwidth<0) 
		  offFreq = -10; 
	    else 
		  offFreq =  10; 
	} else 
	if (rate[4]>0) {  // VHT40 
	  if (InVHT40CenterFreq(frequency)) 
	    if (bandwidth<=0) 
		  offFreq = -10; 
	    else 
		  offFreq =  10; 
	} else 
	if (rate[5]>0) {  // VHT80 
		if (InVHT80CenterFreq(frequency)) { 
			switch (bandwidth) { 
				case BW_VHT80_0: offFreq = -30; 
					 break; 
				case BW_VHT80_1: offFreq = -10; 
					 break; 
			 	case BW_VHT80_2: offFreq =  10; 
					 break; 
			 	case BW_VHT80_3: offFreq =  30; 
					 break; 
			} 
		} 
	} 
	return (frequency+offFreq); 
} 
 
enum { 
	HT_MINUS = -1, 
    HT_PLUS = 1 
}; 
 
static int InVHT40Freq(int frequency, A_BOOL ht40) 
{ 
  switch (frequency) { 
	case 5700: //Plus 
			   if (ht40) 
			     return 0;  // HT40 does not allow 5700 per spec. 
	case 5180: 
	case 5220: 
	case 5260: 
	case 5300: 
	case 5500: 
	case 5540: 
	case 5580: 
	case 5620: 
	case 5660: 
	case 5745: 
	case 5785: return HT_PLUS; 
	// 
	case 5720: // Minus 
			   if (ht40) 
			     return 0;  // HT40 does not allow 5720 per spec. 
	case 5200: 
	case 5240: 
	case 5280: 
	case 5320: 
	case 5520: 
	case 5560: 
	case 5600: 
	case 5640: 
	case 5680: 
	case 5765: 
	case 5805: return HT_MINUS; 
	// 
	default  : return 0; 
  } 
} 
 
static int InVHT80Freq(int frequency) 
{ 
  switch (frequency) { 
	case 5180: 
	case 5260: 
	case 5500: 
	case 5580: 
	case 5660: 
	case 5745: return BW_VHT80_0; 
	// 
	case 5200: 
	case 5280: 
	case 5520: 
	case 5600: 
	case 5680: 
	case 5765: return BW_VHT80_1; 
	// 
	case 5220: 
	case 5300: 
	case 5540: 
	case 5620: 
	case 5700: 
	case 5785: return BW_VHT80_2; 
	// 
	case 5240: 
	case 5320: 
	case 5560: 
	case 5640: 
	case 5720: 
	case 5805: return BW_VHT80_3; 
	// 
	default  : return 0; 
  } 
} 
 
int AdjustFreqBasedOnRateAndBandwidth(int frequency, int *rate, int bandwidth, int bCenFreqUsed) 
{ 
#define BW_HT40_MINUS  (-40)
	int offFreq = 0; 
	//printf("===Freq=[%4d] cf=[%d] bw=[%3d] HT40[%d] VHT40[%d]\n",frequency,bCenFreqUsed,bandwidth,rate[2],rate[4]);
	if (bCenFreqUsed)  // DO NOT shift freq if center frequency is used
		return (frequency);

	if ((rate[2]>0) || (rate[4]>0)) {  //  HT40, VHT40 
	  if (bandwidth == BW_HT40_MINUS)
	    offFreq = -10; 
	  else
	    offFreq = 10; 
	} 
	//printf("===Returned Freq=[%4d]\n",frequency+offFreq);
	return (frequency+offFreq); 
}
