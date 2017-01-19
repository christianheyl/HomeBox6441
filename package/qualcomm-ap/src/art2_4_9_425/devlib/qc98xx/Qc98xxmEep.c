/*
 * Copyright (c) 2002-2005 Atheros Communications, Inc.
 * All rights reserved.
 *
 */

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#ifdef __APPLE__
#include <sys/malloc.h>
#else
#include <malloc.h>
#endif //__APPLE__

#include "wlantype.h"

#include "qc98xx_eeprom.h"
#include "qc98xxtemplate.h"
#include "qc98xxtemplate_generic.h"
#include "qc98xxtemplate_1_1_cus223.h"
#include "qc98xxtemplate_1_1_wb342.h"
#include "qc98xxtemplate_1_1_xb340.h"
#include "qc98xxtemplate_1_1_cus226.h"
#include "qc98xxtemplate_1_1_xb141.h"
#include "qc98xxtemplate_1_1_xb143.h"
#include "qc98xxtemplate_1_1_xb140.h"
#include "qc98xxtemplate_1_2_xb140.h"

#include "Qc98xxEepromStructGet.h"
#include "Qc98xxEepromStruct.h"
#include "Qc98xxmEep.h"
#include "Qc9KDevice.h"
#include "Device.h"
#include "templatelist.h"
#include "crc.h"
#include "rate_constants.h"
#include "vrate_constants.h"
#include "DevSetConfig.h"
#include "UserPrint.h"

#ifndef ARRAY_LENGTH
#define ARRAY_LENGTH(a)         (sizeof(a) / sizeof((a)[0]))
#endif

QC98XX_EEPROM *pQc98xxtemplate_generic = (QC98XX_EEPROM *)qc98xxtemplate_generic;
//QC98XX_EEPROM *pQc98xxtemplate_1_1_cus220 = (QC98XX_EEPROM *)qc98xxtemplate_1_1_cus220;
QC98XX_EEPROM *pQc98xxtemplate_1_1_cus223 = (QC98XX_EEPROM *)qc98xxtemplate_1_1_cus223;
QC98XX_EEPROM *pQc98xxtemplate_1_1_wb342 = (QC98XX_EEPROM *)qc98xxtemplate_1_1_wb342;
QC98XX_EEPROM *pQc98xxtemplate_1_1_xb340 = (QC98XX_EEPROM *)qc98xxtemplate_1_1_xb340;
QC98XX_EEPROM *pQc98xxtemplate_1_1_cus226 = (QC98XX_EEPROM *)qc98xxtemplate_1_1_cus226;
//QC98XX_EEPROM *pQc98xxtemplate_1_1_cus226_030 = (QC98XX_EEPROM *)qc98xxtemplate_1_1_cus226_030;
QC98XX_EEPROM *pQc98xxtemplate_1_1_xb141 = (QC98XX_EEPROM *)qc98xxtemplate_1_1_xb141;
QC98XX_EEPROM *pQc98xxtemplate_1_1_xb143 = (QC98XX_EEPROM *)qc98xxtemplate_1_1_xb143;
QC98XX_EEPROM *pQc98xxtemplate_1_1_xb140 = (QC98XX_EEPROM *)qc98xxtemplate_1_1_xb140;
QC98XX_EEPROM *pQc98xxtemplate_1_2_xb140 = (QC98XX_EEPROM *)qc98xxtemplate_1_2_xb140;

QC98XX_EEPROM *Qc98xxEepromTemplatePtr[20];

A_UINT8 Qc98xxEepromArea[sizeof(QC98XX_EEPROM)];
A_UINT8 Qc98xxEepromBoardArea[sizeof(QC98XX_EEPROM)];

static int qc98xx_eeprom_template_preference = qc98xx_eeprom_template_generic;

static A_UINT32 numPier5G[WHAL_NUM_CHAINS], numPier2G[WHAL_NUM_CHAINS];

static A_UINT16 fbin2freq(A_UINT8 fbin, A_BOOL is2GHz);

#if AP_BUILD
void qc98xxInitTemplateTbl()
{

    Qc98xx_swap_eeprom((QC98XX_EEPROM *)pQc98xxtemplate_generic);
    Qc98xx_swap_eeprom((QC98XX_EEPROM *)pQc98xxtemplate_1_1_cus223);
    Qc98xx_swap_eeprom((QC98XX_EEPROM *)pQc98xxtemplate_1_1_xb140);
    Qc98xx_swap_eeprom((QC98XX_EEPROM *)pQc98xxtemplate_1_2_xb140);

	Qc98xxEepromTemplatePtr[0] = pQc98xxtemplate_generic;
	Qc98xxEepromTemplatePtr[1] = pQc98xxtemplate_1_1_cus223;
	Qc98xxEepromTemplatePtr[2] = pQc98xxtemplate_1_1_xb140;
	Qc98xxEepromTemplatePtr[3] = pQc98xxtemplate_1_2_xb140;
    Qc98xxEepromTemplatePtr[4] = NULL;
}
#else
void qc98xxInitTemplateTbl()
{
	Qc98xxEepromTemplatePtr[0] = pQc98xxtemplate_generic;
	//Qc98xxEepromTemplatePtr[1] = pQc98xxtemplate_1_1_cus220;
	Qc98xxEepromTemplatePtr[1] = pQc98xxtemplate_1_1_cus223;
	Qc98xxEepromTemplatePtr[2] = pQc98xxtemplate_1_1_wb342;
	Qc98xxEepromTemplatePtr[3] = pQc98xxtemplate_1_1_xb340;
	Qc98xxEepromTemplatePtr[4] = pQc98xxtemplate_1_1_cus226;
	//Qc98xxEepromTemplatePtr[6] = pQc98xxtemplate_1_1_cus226_030;
	Qc98xxEepromTemplatePtr[5] = pQc98xxtemplate_1_1_xb141;
	Qc98xxEepromTemplatePtr[6] = pQc98xxtemplate_1_1_xb143;
	Qc98xxEepromTemplatePtr[7] = pQc98xxtemplate_1_1_xb140;
	Qc98xxEepromTemplatePtr[8] = pQc98xxtemplate_1_2_xb140;
	Qc98xxEepromTemplatePtr[9] = NULL;
}
#endif

static A_BOOL qc98xxVerifyEepromChksum(QC98XX_EEPROM *pEeprom)
{
    A_UINT16 *pHalf;
    A_UINT16 sum = 0;
    A_INT32 i;

    // Verify checksums, 
    pHalf = (A_UINT16 *)pEeprom;
    for (i = 0; i < sizeof(QC98XX_EEPROM)/2; i++) 
    { 
        sum ^= *pHalf++; 
    }
    if (sum != 0xffff)
    {
	printf("qc98xxVerifyEepromChksum - error\n");
        return 0;
    }
    return 1;
}

/**************************************************************
 * qc98xxEepromAttach
 *
 * Attach either the provided data stream or EEPROM to the EEPROM data structure
 */
A_BOOL qc98xxEepromAttach()
{
    A_UINT16 sum = 0;

#ifdef AP_BUILD
    Qc98xx_swap_eeprom((QC98XX_EEPROM *)Qc98xxEepromArea);
#endif

    // Verify checksums, 
    if (!qc98xxVerifyEepromChksum((QC98XX_EEPROM *)Qc98xxEepromArea))
    {
		computeChecksum((QC98XX_EEPROM *)Qc98xxEepromArea);
    }
#ifdef AP_BUILD
    Qc98xx_swap_eeprom((QC98XX_EEPROM *)Qc98xxEepromArea);
#endif

    return 1;
}

/**************************************************************************
 * fbin2freq
 *
 * Get channel value from binary representation held in eeprom
 * RETURNS: the frequency in MHz
 */
static A_UINT16 fbin2freq(A_UINT8 fbin, A_BOOL is2GHz)
{
    /*
     * Reserved value 0xFF provides an empty definition both as
     * an fbin and as a frequency - do not convert
     */
    if (fbin == WHAL_BCHAN_UNUSED) {
        return fbin;
    }

    return (A_UINT16)((is2GHz) ? (2300 + fbin) : (4800 + 5 * fbin));
}


//#undef N

/*
 * Returns the interpolated y value corresponding to the specified x value
 * from the np ordered pairs of data (px,py).
 * The pairs do not have to be in any order.
 * If the specified x value is less than any of the px,
 * the returned y value is equal to the py for the lowest px.
 * If the specified x value is greater than any of the px,
 * the returned y value is equal to the py for the highest px.
 */
static int interpolate(A_INT32 x, A_INT32 *px, A_INT32 *py, A_UINT16 np)
{
    int ip = 0;
    int lx = 0, ly = 0, lhave = 0;
    int hx = 0, hy = 0, hhave = 0;
    int dx = 0;
    int y = 0;
    int bf, factor, plus;

    lhave = 0;
    hhave = 0;
    /*
     * identify best lower and higher x calibration measurement
     */
    for (ip = 0; ip < np; ip++) {
        dx = x - px[ip];
        /* this measurement is higher than our desired x */
        if (dx <= 0) {
            if (!hhave || dx > (x - hx)) {
                /* new best higher x measurement */
                hx = px[ip];
                hy = py[ip];
                hhave = 1;
            }
        }
        /* this measurement is lower than our desired x */
        if (dx >= 0) {
            if (!lhave || dx < (x - lx)) {
                /* new best lower x measurement */
                lx = px[ip];
                ly = py[ip];
                lhave = 1;
            }
        }
    }
    /* the low x is good */
    if (lhave) {
        /* so is the high x */
        if (hhave) {
            /* they're the same, so just pick one */
            if (hx == lx) {
                y = ly;
            } else {
                /* interpolate with round off */
                bf = (2 * (hy - ly) * (x - lx)) / (hx - lx);
                plus = (bf % 2);
                factor = bf / 2;
                y = ly + factor + plus;
            }
        } else {
            /* only low is good, use it */
            y = ly;
        }
    } else if (hhave) {
        /* only high is good, use it */
        y = hy;
    } else {
        /* nothing is good,this should never happen unless np=0, ????  */
        y = -(1 << 30);
    }

    return y;
}

static A_BOOL deriveNumCalChans(QC98XX_EEPROM *eepromData, A_UINT32 *numPier5G, A_UINT32 *numPier2G, A_UINT32 iChain)
{
    A_UINT32 num5G=0, num2G=0, i;
    for (i=0; i<WHAL_NUM_11A_CAL_PIERS; i++) 
    {
        if ((WHAL_BCHAN_UNUSED == eepromData->calFreqPier5G[i]) || (0 == eepromData->calFreqPier5G[i])) 
        { 
            break; 
        } 
        else
        {
            num5G++;
        }
    }
    for (i=0; i<WHAL_NUM_11G_CAL_PIERS; i++) 
    {
        if ((WHAL_BCHAN_UNUSED == eepromData->calFreqPier2G[i]) || (0 == eepromData->calFreqPier2G[i])) 
        { 
            break; 
        } 
        else
        {
            num2G++;
        }
    }
    *numPier5G = num5G;
    *numPier2G = num2G;
    return(1);
}


void computeChecksum (QC98XX_EEPROM *pEepStruct)
{
    A_UINT16 sum, *pHalf;
    
    UserPrint("--computeChecksum old 0x%x\n", pEepStruct->baseEepHeader.checksum);
    pEepStruct->baseEepHeader.checksum = 0x0000;
    pHalf = (A_UINT16 *)pEepStruct;
    sum = computeChecksumOnly(pHalf, (sizeof(QC98XX_EEPROM))/2);
    sum = 0xFFFF ^ sum;
    memcpy(&pEepStruct->baseEepHeader.checksum, &sum, 2);
    UserPrint("--computeChecksum new 0x%x\n", pEepStruct->baseEepHeader.checksum);
}

extern A_UINT32 fwBoardDataAddress;

A_BOOL readCalDataFromFile(char *fileName, QC98XX_EEPROM *eepromData, A_UINT32 *bytes)
{
    FILE *fp;
    A_BOOL rc=1, recomputeChksum=0;
    size_t numBytes;
	A_UINT32 i, dataWord;
	A_UINT32 *pDataWord;
	char fullFileName[MAX_FILE_LENGTH];

	strcpy (fullFileName, configSetup.boardDataPath);
	strcat (fullFileName, fileName);

    UserPrint("readCalDataFromFile - reading EEPROM file %s\n",fullFileName);
    if( (fp = fopen(fullFileName, "rb")) == NULL) 
	{
		UserPrint("Could not open %s to read\n", fullFileName);
		if( (fp = fopen(fileName, "rb")) == NULL) 
		{
			UserPrint("Could not open %s to read\n", fileName);
			return 0;
		}
	}
    if (QC98XX_EEPROM_SIZE_LARGEST == (numBytes = fread((A_UCHAR *)eepromData, 1, QC98XX_EEPROM_SIZE_LARGEST, fp))) {
        UserPrint("Read %d from %s\n", numBytes, fileName);
        if (eepromData->baseEepHeader.eeprom_version < QC98XX_EEP_VER1)
        {
            UserPrint("Error - This eeprom bin file v%d is not supported by this NART\n", eepromData->baseEepHeader.eeprom_version);
            rc = 0;
        }
        else
        {
            rc = 1;
        }
    }
    else {
        if (feof(fp)) {
            UserPrint("Read %d from %s\n", numBytes, fileName);  
            rc = 1;
        } 
        else if (ferror(fp)) {
            UserPrint("Error reading %s\n", fileName);
            rc = 0;
        }
        else { UserPrint("Unknown fread rc\n"); rc = 0; }
    }
    if (fp) fclose(fp);

#ifdef AP_BUILD
    Qc98xx_swap_eeprom(eepromData);
#endif

    if (numBytes != eepromData->baseEepHeader.length)
    {
        UserPrint("WARNING - file size of %d NOT match eepromData->baseEepHeader.length %d\n", numBytes, eepromData->baseEepHeader.length);
    }
    if (rc) { 
        for (i = 0; i < WHAL_NUM_CHAINS; ++i)
        {
            rc = deriveNumCalChans(eepromData, &numPier5G[i], &numPier2G[i], i);
        }
        if (recomputeChksum) {
            eepromData->baseEepHeader.checksum = 0x0000;
            computeChecksum(eepromData);
        }

        *bytes = (A_UINT32)numBytes;
    }

    pDataWord = (A_UINT32 *)eepromData;

#define MAX_BLOCK_SIZE  512
    {
        int blockSize;
#ifdef AP_BUILD
    Qc98xx_swap_eeprom(eepromData);
#endif
        QC98XX_EEPROM tempEep;
        memcpy(&tempEep, eepromData, sizeof(QC98XX_EEPROM));
        pDataWord = (A_UINT32 *)&tempEep;

        for(i=0; i<sizeof(QC98XX_EEPROM); i=i+MAX_BLOCK_SIZE)
        {
            if((i+MAX_BLOCK_SIZE)> sizeof(QC98XX_EEPROM))
            {
                blockSize = sizeof(QC98XX_EEPROM) - i;
            }
            else
            {
                blockSize = MAX_BLOCK_SIZE;
            }
            if (Qc9KMemoryWrite(fwBoardDataAddress+i, &pDataWord[i/4], blockSize) != 0)
            {
                UserPrint("ERROR - loading golden caldata file into FW memory\n");
            }
        }
#ifdef AP_BUILD
    Qc98xx_swap_eeprom(eepromData);
#endif
    }



    return rc;
}

int Qc98xxRateGroupIndex2Stream (A_UINT16 rateGroupIndex, A_UINT16 neighborRateIndex)
{
	int stream;
	switch (rateGroupIndex)
	{
		case WHAL_VHT_TARGET_POWER_RATES_MCS0_10_20:	//not sure what stream these 3 belong to
		case WHAL_VHT_TARGET_POWER_RATES_MCS1_2_11_12_21_22:
		case WHAL_VHT_TARGET_POWER_RATES_MCS3_4_13_14_23_24:
			if (neighborRateIndex <= WHAL_VHT_TARGET_POWER_RATES_MCS9)
			{
				stream = 0;
			}
			else if (neighborRateIndex <= WHAL_VHT_TARGET_POWER_RATES_MCS19)
			{
				stream = 1;
			}
			else
			{
				stream = 2;
			}
		case WHAL_VHT_TARGET_POWER_RATES_MCS5:
		case WHAL_VHT_TARGET_POWER_RATES_MCS6:
		case WHAL_VHT_TARGET_POWER_RATES_MCS7:
		case WHAL_VHT_TARGET_POWER_RATES_MCS8:
		case WHAL_VHT_TARGET_POWER_RATES_MCS9:
			stream = 0;
			break;

		case WHAL_VHT_TARGET_POWER_RATES_MCS15:
		case WHAL_VHT_TARGET_POWER_RATES_MCS16:
		case WHAL_VHT_TARGET_POWER_RATES_MCS17:
		case WHAL_VHT_TARGET_POWER_RATES_MCS18:
		case WHAL_VHT_TARGET_POWER_RATES_MCS19:
			stream = 1;
			break;

		case WHAL_VHT_TARGET_POWER_RATES_MCS25:
		case WHAL_VHT_TARGET_POWER_RATES_MCS26:
		case WHAL_VHT_TARGET_POWER_RATES_MCS27:
		case WHAL_VHT_TARGET_POWER_RATES_MCS28:
		case WHAL_VHT_TARGET_POWER_RATES_MCS29:
			stream = 2;
			break;

		default:
			stream = -1;
			UserPrint("Qc98xxRateIndex2Stream - ERROR invalid rate group index\n");
			break;
	}
	return stream;
}

int Qc98xxUserRateIndex2Stream (A_UINT16 userRateGroupIndex)
{
	int stream;
	
	stream = IS_1STREAM_TARGET_POWER_VHT_RATES(userRateGroupIndex) ? 0 :
			(IS_2STREAM_TARGET_POWER_VHT_RATES(userRateGroupIndex) ? 1 :
			(IS_3STREAM_TARGET_POWER_VHT_RATES(userRateGroupIndex) ? 2 : -1));
	
	return stream;
}

int Qc98xxRateIndex2Stream (A_UINT16 rateIndex)
{
	int stream;

	if (IS_1STREAM_RATE_INDEX(rateIndex) || IS_1STREAM_vRATE_INDEX(rateIndex))
	{
		stream = 0;
	}
	else if (IS_2STREAM_RATE_INDEX(rateIndex) || IS_2STREAM_vRATE_INDEX(rateIndex))
	{
		stream = 1;
	}
	else if (IS_3STREAM_RATE_INDEX(rateIndex) || IS_3STREAM_vRATE_INDEX(rateIndex))
	{
		stream = 2;
	}
	else
	{
		stream = -1;
		UserPrint("Qc98xxRateIndex2Stream - ERROR invalid rate group index\n");
	}
	return stream;
}

A_BOOL Qc98xxIsRateInStream (A_UINT32 stream, A_UINT16 rateGroupIndex)
{
	if ((stream == 0 && (rateGroupIndex == WHAL_VHT_TARGET_POWER_RATES_MCS0_10_20 || 
						rateGroupIndex == WHAL_VHT_TARGET_POWER_RATES_MCS1_2_11_12_21_22 ||
						rateGroupIndex == WHAL_VHT_TARGET_POWER_RATES_MCS3_4_13_14_23_24 ||
						rateGroupIndex == WHAL_VHT_TARGET_POWER_RATES_MCS5 || 
						rateGroupIndex == WHAL_VHT_TARGET_POWER_RATES_MCS6 ||
						rateGroupIndex == WHAL_VHT_TARGET_POWER_RATES_MCS7 || 
						rateGroupIndex == WHAL_VHT_TARGET_POWER_RATES_MCS8 ||
						rateGroupIndex == WHAL_VHT_TARGET_POWER_RATES_MCS9)) ||
		(stream == 1 && (rateGroupIndex == WHAL_VHT_TARGET_POWER_RATES_MCS15 || 
						rateGroupIndex == WHAL_VHT_TARGET_POWER_RATES_MCS16 ||
						rateGroupIndex == WHAL_VHT_TARGET_POWER_RATES_MCS17 || 
						rateGroupIndex == WHAL_VHT_TARGET_POWER_RATES_MCS18 ||
						rateGroupIndex == WHAL_VHT_TARGET_POWER_RATES_MCS19)) ||

		(stream == 2 && (rateGroupIndex == WHAL_VHT_TARGET_POWER_RATES_MCS25 || 
						rateGroupIndex == WHAL_VHT_TARGET_POWER_RATES_MCS26 ||
						rateGroupIndex == WHAL_VHT_TARGET_POWER_RATES_MCS27 || 
						rateGroupIndex == WHAL_VHT_TARGET_POWER_RATES_MCS28 ||
						rateGroupIndex == WHAL_VHT_TARGET_POWER_RATES_MCS29)))
	{
		return 1;
	}
	return 0;
}

int Qc98xxEepromTemplatePreference(int templateId)
{
    qc98xx_eeprom_template_preference = templateId;
	return 0;
}

int Qc98xxGetEepromTemplatePreference()
{
	return qc98xx_eeprom_template_preference;
}

int Qc98xxEepromStructDefaultMany(void)
{
    return ARRAY_LENGTH(Qc98xxEepromTemplatePtr);
}

int Qc98xxEepromTemplateVersionValid (int templateVersion)
{
    int it;

    for (it = 0; it < ARRAY_LENGTH(Qc98xxEepromTemplatePtr); it++) 
	{
		if (Qc98xxEepromTemplatePtr[it] == NULL)
		{
			break;
		}
        if (Qc98xxEepromTemplatePtr[it]->baseEepHeader.template_version == templateVersion)
		{
            return 1;
        }
    }
    return 0;
}

QC98XX_EEPROM *Qc98xxEepromStructDefault(int index) 
{
    if (index >= 0 && index < ARRAY_LENGTH(Qc98xxEepromTemplatePtr))
    {
        return Qc98xxEepromTemplatePtr[index];
    } 
	else 
	{
        return NULL;
    }
}

QC98XX_EEPROM *Qc98xxEepromStructDefaultFindById(int templateId) 
{
    int it;

    for (it = 0; it < ARRAY_LENGTH(Qc98xxEepromTemplatePtr); it++) 
	{
		if (Qc98xxEepromTemplatePtr[it] == NULL)
		{
			break;
		}
        if (QC98XX_TEMPLATEVERSION_TO_TEMPLATEID(Qc98xxEepromTemplatePtr[it]->baseEepHeader.template_version) == templateId)
		{
            return Qc98xxEepromTemplatePtr[it];
        }
    }
    return NULL;
}

QC98XX_EEPROM *Qc98xxEepromStructDefaultFindByTemplateVersion(int templateVer) 
{
    int it;

    for (it = 0; it < ARRAY_LENGTH(Qc98xxEepromTemplatePtr); it++) 
	{
		if (Qc98xxEepromTemplatePtr[it] == NULL)
		{
			break;
		}
        if (Qc98xxEepromTemplatePtr[it]->baseEepHeader.template_version == templateVer)
		{
            return Qc98xxEepromTemplatePtr[it];
        }
    }
    return NULL;
}

char *Qc98xxGetTemplateNameGivenVersion(int templateVer)
{
	int i;
	int templateId = QC98XX_TEMPLATEVERSION_TO_TEMPLATEID(templateVer);

	for (i = 0; i < ARRAY_LENGTH(TemplatePreferenceParameter); ++i)
	{
		if (TemplatePreferenceParameter[i].code == templateId)
		{
			return (TemplatePreferenceParameter[i].word[0]);
		}
	}
	return NULL;
}
