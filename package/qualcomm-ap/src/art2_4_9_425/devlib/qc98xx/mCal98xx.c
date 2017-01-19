
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

//#include "AquilaNewmaMapping.h"

#include "wlantype.h"
#include "smatch.h"
#include "UserPrint.h"
#include "TimeMillisecond.h"
#include "Card.h"
#include "Field.h"
#include "Qc98xxDevice.h"
#include "qc98xx_eeprom.h"
#include "mCal98xx.h"
#ifndef __APPLE__
//#include "ar2413reg.h"
#endif
//include hal files
//#include "ah.h"
//#include "ah_internal.h"
//#include "qc98xx.h"
#include "Qc98xxmEep.h"
//#include "qc98xxmEep.h"

typedef struct qc98xx_txgain_tbl 
{
  unsigned int val;
  unsigned int total_gain;			//[31:24]
  unsigned char txbb1dbbiquadgain;		//[2:0]
  unsigned char txbb6dbbiquadgain;		  //[4:3]
  unsigned char txbbgc;					//[8:5]
  unsigned char padrv2gn2g;				//[12:9]
  unsigned char padrv3gn5g;				//[16:13]
  unsigned char padrv2gn5g;				//[19:17]
  unsigned char padrvhalfgn2g;			//[21]
 } QC98XX_TXGAIN_TBL; 

#define  TXGAIN_TABLE_STRING	  "tg_table" 
#define  TXGAIN_TABLE_TXBB1DBBIQUAD_LSB 	  0			// [2:0]
#define  TXGAIN_TABLE_TXBB1DBBIQUAD_MASK	  0x7
#define  TXGAIN_TABLE_TXBB6DBBIQUAD2_LSB	  3			// [4:3]
#define  TXGAIN_TABLE_TXBB6DBBIQUAD2_MASK	  0x3
#define  TXGAIN_TABLE_TXBBGC_LSB			  5			// [8:5]
#define  TXGAIN_TABLE_TXBBGC_MASK			  0xf
#define  TXGAIN_TABLE_PADRV2GN2_LSB 		  9			// [12:9]
#define  TXGAIN_TABLE_PADRV2GN2_MASK		  0xf
#define  TXGAIN_TABLE_PADRV3GN5_LSB 		  13			// [16:13]
#define  TXGAIN_TABLE_PADRV3GN5_MASK		  0xf
#define  TXGAIN_TABLE_PADRV2GN5_LSB 		  17			// [19:17]
#define  TXGAIN_TABLE_PADRV2GN5_MASK		  0x7
#define  TXGAIN_TABLE_PADRVHALFGN2G_LSB 	  21			// [21]
#define  TXGAIN_TABLE_PADRVHALFGN2G_MASK	  0x1
#define  TXGAIN_TABLE_TOTAL_GAIN_LSB		  24			// [31:24]
#define  TXGAIN_TABLE_TOTAL_GAIN_MASK		  0xff

#define MAX_SIZE_QC98XX_TX_GAIN_TABLE		 32

static int openLoopPwrCntl;			// WHO SETS THIS???
static int Qc98xxGainTableSize = 0;
static QC98XX_TXGAIN_TBL qc98xxGainTable[MAX_SIZE_QC98XX_TX_GAIN_TABLE];

#define GAIN_OVERRIDE		0xffff



#define  TXGAIN_TABLE_STRING_HIGH_POWER "high_power_bb_tx_gain_table_"
#define  TXGAIN_TABLE_FILENAME "qc98xx_tx_gain_2.tbl"
//#define  AR9287_PAL_ON		 "_pal_on"


#if 0 
static void Qc98xxSearchNextTxGainIndex(int curTxGainIndex, double currentPwr, double targetPwr, int *analog)
{
	int newGainIndex = -1;
	if (currentPwr < targetPwr) // should we have a check as to say less than +/- 0.5 db 
	{
		// we might need to go to the next index
		// how do we predict what would be the new target power just by looking at the difference
		// in the value of the two indices
		// the current index might be more closer to target power than the next one.. so how do we
		// find out this one?

		if (curTxGainIndex >= Qc98xxGainTableSize)
		{
			*analog = newGainIndex;
			return;				
		}
		else
		{
			int curPwrIndexValue = qc98xxGainTable[curTxGainIndex];
			int nxtPwrIndexValue = (int)(targetPwr / 0.125);

			for (int i=curTxGainIndex; i<Qc98xxGainTableSize; i++)
			{
				if (qc98xxGainTable[i] == nxtPwrIndexValue ||
					(nxtPwrIndexValue - abs(qc98xxGainTable[i])* 0.125 < 1))
				{
					*analog = i;
					break;
				}
			}			
		}
	}
	else // currentPwr > targetPwr
	{
		int curPwrIndexValue = qc98xxGainTable[curTxGainIndex];
		int nxtPwrIndexValue = (int)(targetPwr / 0.125);

		for (int i=0; i<curTxGainIndex; i++)
		{
			if (qc98xxGainTable[i] == nxtPwrIndexValue ||
				(abs(qc98xxGainTable[i] - nxtPwrIndexValue)* 0.125 < 1))
			{
				*analog = i;
				break;
			}
		}		
	}	
}
#endif

static int generateQc98xxTxGainTblFromCfg(unsigned int modeMask, int *pGainTableSize)
{
	char fieldName[50];
	int currentMode;
	int i;
	int tab_1_16_lsb_ext = 0;
	int tab_17_32_lsb_ext = 0;
	int ext_bits = 0;

	//
	// we only compute the values for the current mode
	//
	currentMode=0;

	//read gain table size
	if(FieldRead("tx_gain_table_max", (unsigned int *)pGainTableSize) == -1) 
	{
		return(-1);
	}

	if (*pGainTableSize > MAX_SIZE_QC98XX_TX_GAIN_TABLE) {
		UserPrint("Bad gain table size = %d, must be less than %d\n", *pGainTableSize, MAX_SIZE_QC98XX_TX_GAIN_TABLE);
		return(-1);
	}

	Qc98xxGainTableSize = *pGainTableSize;
	
	if (FieldRead("tg_table1_16_lsb_ext", (unsigned int *)&tab_1_16_lsb_ext) == -1)
	{
		return (-1);
	}

	if (FieldRead("tg_table17_32_lsb_ext", (unsigned int *)&tab_17_32_lsb_ext) == -1)
	{
		return (-1);
	}

	for (i=0; i < *pGainTableSize; i++) 
	{
		//
		// FJC: Will need to add support for PAL_on table
		//
		SformatOutput(fieldName, 50-1, "%s%d", TXGAIN_TABLE_STRING, i+1); //gain table field names start from 1 	 

		// 
		// hal loads this value on reset
		// so we can fetch the value for the current mode from the register
		//
		FieldRead(fieldName,&qc98xxGainTable[i].val);

		
		UserPrint("fieldName %s, gain[%d]: %08x\n", fieldName, i,qc98xxGainTable[i].val);

		qc98xxGainTable[i].total_gain = 
			(unsigned int)((qc98xxGainTable[i].val >> TXGAIN_TABLE_TOTAL_GAIN_LSB)); // & TXGAIN_TABLE_TOTAL_GAIN_MASK); 
	
		qc98xxGainTable[i].total_gain = (qc98xxGainTable[i].total_gain << 2); // make space for putting 2 lsb ext bits

		if (i<16)
		{
			ext_bits = ((tab_1_16_lsb_ext & (3<<i*2)) >> (i*2)) & 0x03;
			qc98xxGainTable[i].total_gain |= ext_bits;			
		}
		else
		{
			ext_bits = ((tab_17_32_lsb_ext & (3<<(i-16)*2)) >> ((i-16)*2)) & 0x03;
			qc98xxGainTable[i].total_gain |= ext_bits;
		}

		qc98xxGainTable[i].txbb1dbbiquadgain = 
			(unsigned int)((qc98xxGainTable[i].val >> TXGAIN_TABLE_TXBB1DBBIQUAD_LSB) & TXGAIN_TABLE_TXBB1DBBIQUAD_MASK); 
		qc98xxGainTable[i].txbb6dbbiquadgain = 
			(unsigned int)((qc98xxGainTable[i].val >> TXGAIN_TABLE_TXBB6DBBIQUAD2_LSB) & TXGAIN_TABLE_TXBB6DBBIQUAD2_MASK); 
		qc98xxGainTable[i].txbbgc = 
			(unsigned int)((qc98xxGainTable[i].val >> TXGAIN_TABLE_TXBBGC_LSB) & TXGAIN_TABLE_TXBBGC_MASK); 
		qc98xxGainTable[i].padrv2gn2g = 
			(unsigned int)((qc98xxGainTable[i].val >> TXGAIN_TABLE_PADRV2GN2_LSB) & TXGAIN_TABLE_PADRV2GN2_MASK); 
		qc98xxGainTable[i].padrv3gn5g = 
			(unsigned int)((qc98xxGainTable[i].val >> TXGAIN_TABLE_PADRV3GN5_LSB) & TXGAIN_TABLE_PADRV3GN5_MASK); 
		qc98xxGainTable[i].padrv2gn5g = 
			(unsigned int)((qc98xxGainTable[i].val >> TXGAIN_TABLE_PADRVHALFGN2G_LSB) & TXGAIN_TABLE_PADRV2GN5_MASK); 
		qc98xxGainTable[i].padrvhalfgn2g = 
			(unsigned int)((qc98xxGainTable[i].val >> TXGAIN_TABLE_PADRVHALFGN2G_LSB) & TXGAIN_TABLE_PADRVHALFGN2G_MASK); 

	}

	for (i=0; i < *pGainTableSize; i++)
	{
		UserPrint("qc98xxGainTable[%d].val 0x%08x qc98xxGainTable[%d].total_gain 0x%08x == %d\n",
				   i, qc98xxGainTable[i].val, i, qc98xxGainTable[i].total_gain, qc98xxGainTable[i].total_gain);
	}

	return 0;
}


static int ForceSingleGainTableQc98xx(int mode, unsigned int requestedGain)
{
	unsigned short i;
	unsigned int dac_gain = 0;
	QC98XX_TXGAIN_TBL *pGainTbl = NULL;
	int gainTableSize;
	int offset=0;

	generateQc98xxTxGainTblFromCfg(0x3, &gainTableSize);

	pGainTbl = qc98xxGainTable;

	i = 0;
	while ((requestedGain > pGainTbl[i].total_gain) && (i < (gainTableSize -1)) ) {i++;}  // Find entry closest

	UserPrint("using gain entry %d\n",i);
	
	if (pGainTbl[i].total_gain > requestedGain) 
	{
		dac_gain = pGainTbl[i].total_gain - requestedGain;
	}

	FieldWrite("force_dac_gain", 1);
	FieldWrite("forced_dac_gain", dac_gain);
	FieldWrite("force_tx_gain", 1);
	FieldWrite("forced_txbb1dbbiquadgain", pGainTbl[i].txbb1dbbiquadgain);
	FieldWrite("forced_txbb6dbbiquadgain", pGainTbl[i].txbb6dbbiquadgain);
	FieldWrite("forced_txbbgc", pGainTbl[i].txbbgc);
	FieldWrite("forced_padrv2gn2g", pGainTbl[i].padrv2gn2g);
	FieldWrite("forced_padrv3gn5g", pGainTbl[i].padrv3gn5g);
	FieldWrite("forced_padrv2gn5g", pGainTbl[i].padrv2gn5g);
	FieldWrite("forced_padrvhalfgn2g", pGainTbl[i].padrvhalfgn2g);

	//FJC: set PAL to off for now. Will need to make this selectable.
	FieldWrite("forced_enable_PAL", 0);

	return i;
}

#ifdef UNUSED

void ForceSinglePCDACTableGriffin(int mode, int pcdac, int offset)
{
	unsigned short i;
	unsigned int dac_gain = 0;
	AR2413_TXGAIN_TBL *pGainTbl = NULL;
	unsigned int gainTableSize = AR2413_TX_GAIN_TBL_SIZE;

#ifdef UNUSED
	if(isSpider(pLibDev->swDevID)) {
		pGainTbl = spider_tx_gain_tbl;
		gainTableSize = 15;
		offset -= 20;
UserPrint("SNOOP: using spider gain table\n");
	}
	else if(isGriffin(pLibDev->swDevID)) {
		if (isNala(pLibDev->swDevID))
		{
			pGainTbl = nala_tx_gain_tbl;
			gainTableSize = sizeof(nala_tx_gain_tbl) / sizeof(AR2413_TXGAIN_TBL);
		}
		else
		{
			pGainTbl = griffin_tx_gain_tbl;
			gainTableSize = sizeof(griffin_tx_gain_tbl)/sizeof(AR2413_TXGAIN_TBL);
		}
	}
	else if(isEagle(pLibDev->swDevID)) {
		if(pLibDev->mode == MODE_11A) {
			pGainTbl = eagle_tx_gain_tbl_5;
			offset += 10;
		} else {
			pGainTbl = eagle_tx_gain_tbl_2;
		}
	}
	else if(isOwl(pLibDev->swDevID))
#endif
	
	{
		if(mode) 
		{
			pGainTbl = owl_tx_gain_tbl_5;
			gainTableSize = AR5416_TX_GAIN_TBL_5_SIZE;
//			offset += 10;
		} 
		else 
		{
			pGainTbl = owl_tx_gain_tbl_2;
			gainTableSize = AR5416_TX_GAIN_TBL_2_SIZE;
			if(offset != GAIN_OVERRIDE) 
			{
				offset += 10;
			}
		}
	}
#ifdef UNUSED
	if(isDragon(devNum)) {
		offset += 20;
	}
#endif

	if(pGainTbl == NULL) 
	{
		UserPrint("Error: unable to initialize gainTable in ForceSinglePCDACTableGriffin\n");
	}
	i = 0;
	if(offset != GAIN_OVERRIDE) 
	{
		if (mode) 
		{
			offset = (unsigned short)(offset + 10);	// Up the offset for 11b mode
		}
		pcdac = (unsigned short)(pcdac + offset);		// Offset pcdac to get in a reasonable range
	}
	
	if(pcdac > pGainTbl[gainTableSize - 1].desired_gain) 
	{
		i = (unsigned short)(gainTableSize - 1);
	} 
	else 
	{
	  while ((pcdac > pGainTbl[i].desired_gain) &&
			(i < gainTableSize) ) {i++;}  // Find entry closest
	}

	if (pGainTbl[i].desired_gain > pcdac) 
	{
		dac_gain = pGainTbl[i].desired_gain - pcdac;
	}	
	FieldWrite( "bb_force_dac_gain", 1);
	FieldWrite( "bb_forced_dac_gain", dac_gain);
	FieldWrite( "bb_force_tx_gain", 1);
	FieldWrite( "bb_forced_txgainbb1", pGainTbl[i].bb1_gain);
	FieldWrite( "bb_forced_txgainbb2", pGainTbl[i].bb2_gain);
	FieldWrite( "bb_forced_txgainif", pGainTbl[i].if_gain);
	FieldWrite( "bb_forced_txgainrf", pGainTbl[i].rf_gain);

//	 UserPrint("\nSNOOP: offset = %d, i=%d, dac_gain = %d, bb1 = %d, bb2 = %d, gainif = %d, gainrf = %d\n",offset,i,
//	 dac_gain, pGainTbl[i].bb1_gain, pGainTbl[i].bb2_gain, pGainTbl[i].if_gain,
//	 pGainTbl[i].rf_gain);

#ifdef DEBUG_
	UserPrint("SNOOP: dac_gain = %d, bb1 = %d, bb2 = %d, gainif = %d, gainrf = %d\n",
	   dac_gain, pGainTbl[i].bb1_gain, pGainTbl[i].bb2_gain, pGainTbl[i].if_gain,
	   pGainTbl[i].rf_gain);
#endif
	return;
}



void ForceSinglePCDACTable(unsigned short pcdac)
{
	unsigned short temp16, i;
	unsigned int temp32;
	unsigned int regoffset ;
	unsigned int pcdac_shift = 8;
	unsigned int predist_scale = 0x00FF;
	unsigned char	   falconTrue;
	falconTrue = isFalcon(devNum);

	if (falconTrue) {
		pcdac_shift = 0;
		predist_scale = 0; // no predist in falcon
	}

//++JC++

	if(isGriffin(pLibDev->swDevID) || isEagle(pLibDev->swDevID) || isOwl(pLibDev->swDevID))  {	  // For Griffin
		ForceSinglePCDACTableGriffin(pcdac, 30);  // By default, offset of 30
		return;
	}
//++JC++

	temp16 = (unsigned short) (0x0000 | (pcdac << pcdac_shift) | predist_scale);
	temp32 = (temp16 << 16) | temp16 ;

	regoffset = 0x9800 + (608 << 2) ;
	for (i=0; i<32; i++)
	{
		REGW(devNum, regoffset, temp32);
		if (falconTrue) {
			REGW(devNum, regoffset + 0x1000, temp32);
		}
		regoffset += 4;
	}

	if (!falconTrue) {
		FieldWrite( "rf_xpdbias", 1);
	}

	return;
}


//
// Set tx power registers to array of values passed in
//
Qc98xxTransmitPowerRegWrite(A_UINT8 *pPwrArray) 
{	
	/* make sure forced gain is not set */
	FieldWrite("force_dac_gain", 0);
	FieldWrite("force_tx_gain", 0);

	/* Write the OFDM power per rate set */
	/* 6 (LSB), 9, 12, 18 (MSB) */
	MyRegisterWrite(0xa3c0,
		POW_SM(pPwrArray[ALL_TARGET_LEGACY_6_24], 24)
		  | POW_SM(pPwrArray[ALL_TARGET_LEGACY_6_24], 16)
		  | POW_SM(pPwrArray[ALL_TARGET_LEGACY_6_24],  8)
		  | POW_SM(pPwrArray[ALL_TARGET_LEGACY_6_24],  0)
	);
	/* 24 (LSB), 36, 48, 54 (MSB) */
	MyRegisterWrite(0xa3c4,
		POW_SM(pPwrArray[ALL_TARGET_LEGACY_54], 24)
		  | POW_SM(pPwrArray[ALL_TARGET_LEGACY_48], 16)
		  | POW_SM(pPwrArray[ALL_TARGET_LEGACY_36],  8)
		  | POW_SM(pPwrArray[ALL_TARGET_LEGACY_6_24],  0)
	);

	/* Write the CCK power per rate set */
	/* 1L (LSB), reserved, 2L, 2S (MSB) */	
	MyRegisterWrite(0xa3c8,
		POW_SM(pPwrArray[ALL_TARGET_LEGACY_1L_5L], 24)
		  | POW_SM(pPwrArray[ALL_TARGET_LEGACY_1L_5L],	16)
//		  | POW_SM(txPowerTimes2,  8) /* this is reserved for Osprey */
		  | POW_SM(pPwrArray[ALL_TARGET_LEGACY_1L_5L],	 0)
	);
	/* 5.5L (LSB), 5.5S, 11L, 11S (MSB) */
	MyRegisterWrite(0xa3cc,
		POW_SM(pPwrArray[ALL_TARGET_LEGACY_11S], 24)
		  | POW_SM(pPwrArray[ALL_TARGET_LEGACY_11L], 16)
		  | POW_SM(pPwrArray[ALL_TARGET_LEGACY_5S],  8)
		  | POW_SM(pPwrArray[ALL_TARGET_LEGACY_1L_5L],	0)
	);

	/* Write the HT20 power per rate set */
	/* 0/8/16 (LSB), 1-3/9-11/17-19, 4, 5 (MSB) */
	MyRegisterWrite(0xa3d0,
		POW_SM(pPwrArray[ALL_TARGET_HT20_5], 24)
		  | POW_SM(pPwrArray[ALL_TARGET_HT20_4],  16)
		  | POW_SM(pPwrArray[ALL_TARGET_HT20_1_3_9_11_17_19],  8)
		  | POW_SM(pPwrArray[ALL_TARGET_HT20_0_8_16],	0)
	);
	
	/* 6 (LSB), 7, 12, 13 (MSB) */
	MyRegisterWrite(0xa3d4,
		POW_SM(pPwrArray[ALL_TARGET_HT20_13], 24)
		  | POW_SM(pPwrArray[ALL_TARGET_HT20_12],  16)
		  | POW_SM(pPwrArray[ALL_TARGET_HT20_7],  8)
		  | POW_SM(pPwrArray[ALL_TARGET_HT20_6],   0)
	);

	/* 14 (LSB), 15, 20, 21 */
	MyRegisterWrite(0xa3e4,
		POW_SM(pPwrArray[ALL_TARGET_HT20_21], 24)
		  | POW_SM(pPwrArray[ALL_TARGET_HT20_20],  16)
		  | POW_SM(pPwrArray[ALL_TARGET_HT20_15],  8)
		  | POW_SM(pPwrArray[ALL_TARGET_HT20_14],	0)
	);

	/* Mixed HT20 and HT40 rates */
	/* HT20 22 (LSB), HT20 23, HT40 22, HT40 23 (MSB) */
	MyRegisterWrite(0xa3e8,
		POW_SM(pPwrArray[ALL_TARGET_HT40_23], 24)
		  | POW_SM(pPwrArray[ALL_TARGET_HT40_22],  16)
		  | POW_SM(pPwrArray[ALL_TARGET_HT20_23],  8)
		  | POW_SM(pPwrArray[ALL_TARGET_HT20_22],	0)
	);
	
	/* Write the HT40 power per rate set */
	// correct PAR difference between HT40 and HT20/LEGACY
	/* 0/8/16 (LSB), 1-3/9-11/17-19, 4, 5 (MSB) */
	MyRegisterWrite(0xa3d8,
		POW_SM(pPwrArray[ALL_TARGET_HT40_5], 24)
		  | POW_SM(pPwrArray[ALL_TARGET_HT40_4],  16)
		  | POW_SM(pPwrArray[ALL_TARGET_HT40_1_3_9_11_17_19],  8)
		  | POW_SM(pPwrArray[ALL_TARGET_HT40_0_8_16],	0)
	);

	/* 6 (LSB), 7, 12, 13 (MSB) */
	MyRegisterWrite(0xa3dc,
		POW_SM(pPwrArray[ALL_TARGET_HT40_13], 24)
		  | POW_SM(pPwrArray[ALL_TARGET_HT40_12],  16)
		  | POW_SM(pPwrArray[ALL_TARGET_HT40_7], 8)
		  | POW_SM(pPwrArray[ALL_TARGET_HT40_6], 0)
	);

	/* 14 (LSB), 15, 20, 21 */
	MyRegisterWrite(0xa3ec,
		POW_SM(pPwrArray[ALL_TARGET_HT40_21], 24)
		  | POW_SM(pPwrArray[ALL_TARGET_HT40_20],  16)
		  | POW_SM(pPwrArray[ALL_TARGET_HT40_15],  8)
		  | POW_SM(pPwrArray[ALL_TARGET_HT40_14],	0)
	);

	return 0;
}


//
// Set tx power registers to array of values passed in
//
int nartQc98xxTransmitPowerRegWrite(A_UINT8 *pPwrArray) 
{	
	/* make sure forced gain is not set */
	FieldWrite("force_dac_gain", 0);
	FieldWrite("force_tx_gain", 0);
	
	//call function within HAL
	ar9300_transmit_power_reg_write(AH, pPwrArray);
	return 0;
}
//set power registers all to same value
 int Qc98xxTransmitPowerSet(int mode, double txp)
{
	A_INT8	  ht40PowerIncForPdadc = 0; //hard code for now, need to get from eeprom struct
	A_INT8	  powerOffsetFromZero  = 0; //hard code for now, need to get from eeprom struct
	A_UINT8    txPowerTimes2;
	A_UINT8 targetPowerValT2[NUM_TRGT_PWR_REGISTERS];
	A_UINT16 i;
	
	//convert power from double to integer (power * 2)
	txPowerTimes2=(int)((2*txp)+0.5) - powerOffsetFromZero;

	//Fill target powers
	for(i=0; i < NUM_TRGT_PWR_REGISTERS; i++) {
		targetPowerValT2[i] = txPowerTimes2;

		if(i >= ALL_TARGET_HT40_0_8_16) {
			targetPowerValT2[i] += ht40PowerIncForPdadc;
		}
	}
	
	//Call function to write power array to register
	nartQc98xxTransmitPowerRegWrite(targetPowerValT2);
	return 0;
}


//
// set transmit power gain (pcdac) to a specified value
//	
 int Qc98xxTransmitGainSet(int mode, int txgain)
{	
	return ForceSingleGainTableQc98xx(mode, txgain);
}

#endif

 int Qc98xxTransmitGainRead(int entry, unsigned int *rvalue, int *value, int max)
{
	unsigned int dac_gain = 0;
	QC98XX_TXGAIN_TBL *pGainTbl = NULL;
	int offset=0;
	int pGainTableSize;

	generateQc98xxTxGainTblFromCfg(0x3,&pGainTableSize);


	if(entry>=0 && entry<pGainTableSize && max>=8)
	{
		pGainTbl = &qc98xxGainTable[entry];

		*rvalue=pGainTbl->val;
		value[0]=pGainTbl->total_gain;
		value[1]=pGainTbl->txbb1dbbiquadgain;
		value[2]=pGainTbl->txbb6dbbiquadgain;
		value[3]=pGainTbl->txbbgc;
		value[4]=pGainTbl->padrv2gn2g;
		value[5]=pGainTbl->padrv3gn5g;
		value[6]=pGainTbl->padrv2gn5g;
		value[7]=pGainTbl->padrvhalfgn2g;
		return 8;
	}
	return -1;
}

int Qc98xxTransmitINIGainGet(int *total_gain)
{
	QC98XX_TXGAIN_TBL *pGainTbl = NULL;
	int entry;
	int pGainTableSize;

	generateQc98xxTxGainTblFromCfg(0x3,&pGainTableSize);

	for (entry=0; entry<pGainTableSize; entry++) 
	{
		pGainTbl = &qc98xxGainTable[entry];
		total_gain[entry] = pGainTbl->total_gain;
	}
	return pGainTableSize;
}

//Function - Qc98xxTxGainTableRead_AddressGainTable
//Purpose  - Retrive loaded in memory gain table address and number of entries
//Parameter- addr : retunred adress of Gain Table array
//			 row  : return number of entries
//			 col  : return size of each of entries
//Return   - None
#ifdef UNUSED
 int Qc98xxTxGainTableRead_AddressGainTable(unsigned int **addr, unsigned int *row, unsigned int *col)
{
	extern struct ath_hal *AH;
	struct ath_hal_9300 *ahp = AH9300(AH);

	*addr = (unsigned int *)ahp->ah_ini_modes_txgain.ia_array;
	*row=ahp->ah_ini_modes_txgain.ia_rows;
	*col=ahp->ah_ini_modes_txgain.ia_columns;
	return (0);
}
#endif
#define MAX_FIELD_ENTRIES	100   // Wild guess on unknown # of fields

//Function - Qc98xxTxGainTableRead_AddressHeader
//Purpose  - Retrive all field names associated with register name from input address
//Parameter- address : adress of entry
//			 header  : buffer returned with regsiter name and associated field names for NART to display
//			 max	 : maximum size of input buffer for header
//Return   - number of fields
 int Qc98xxTxGainTableRead_AddressHeader(unsigned int address, char *header, char *subheader, int max)
{
  char	*rName, *fName;
  int low, high, i, lc=0, lc2=0, nc, nc2;

  for (i=0; i<MAX_FIELD_ENTRIES; i++)
  {
	if (FieldFindByAddressOnly(address, i, &high, &low, &rName, &fName) == -1)
	  break;
	if (lc == 0) {
	  nc=SformatOutput(header,max-1, "|%s|32regValue|%s|",rName, fName);
	  nc2=SformatOutput(subheader,max-1, "|Bit|31..0|%d..%d|",high,low);
	}
	else {
	  nc=SformatOutput((header+lc),max-lc-1, "%s|",fName);
	  nc2=SformatOutput((subheader+lc2),max-lc2-1, "%d..%d|",high,low);
	}
	lc+=nc;
	lc2+=nc2;
  }

  return (i);
}
//Function - Qc98xxTxGainTableRead_AddressValue
//Purpose  - Retrive field value from address along with nth field in case of multiple fields with same register name
//Parameter- address : adress of entry
//			 idx	 : index of field wish to retrive
//					 : -1 means getting whole 32 bit value regardless low and high bit address
//			 rName	 : returned register name for subsequent fucntion FiledRead call
//			 fName	 : returned field name for subsequent fucntion FiledRead call
//			 value	 : returned value of field value with low and high bit mask
//			 low	 : low address bit
//			 high	 : hgih address bit
//			 max	 : maximum size of input buffer for header
//Return   -  0 suceed
//			 -1 failed
 int Qc98xxTxGainTableRead_AddressValue(unsigned int address, int idx, char *rName, char *fName, int *value, int *low, int *high)
{
  char rfName[256], *rPtr, *fPtr;

  if (FieldFindByAddressOnly(address, idx == -1 ? 0: idx, high, low, &rPtr, &fPtr) == -1)
	return (-1);
  strcpy(rName,rPtr);
  strcpy(fName,fPtr);
  SformatOutput(rfName,256-1, "%s.%s",rName, fName);
  if (idx == -1)
	  return(FieldReadNoMask(rfName,value));
  else
	return(FieldRead(rfName,value));
}

 int Qc98xxTransmitGainWrite(int entry, int *value, int nvalue)
{
	char fieldName[50];
	QC98XX_TXGAIN_TBL pGainTbl;

	int pGainTableSize;

	//read gain table size
	if(FieldRead("tx_gain_table_max", (unsigned int *)&pGainTableSize) == -1) 
	{
		return(-1);
	}

	if(entry>=0 && entry<pGainTableSize && nvalue==8)
	{
		pGainTbl.total_gain=value[0] ;
		pGainTbl.txbb1dbbiquadgain=value[1];
		pGainTbl.txbb6dbbiquadgain=value[2];
		pGainTbl.txbbgc=value[3];
		pGainTbl.padrv2gn2g=value[4];
		pGainTbl.padrv3gn5g=value[5];
		pGainTbl.padrv2gn5g=value[6];
		pGainTbl.padrvhalfgn2g=value[7];
		//
		// pack up the fields
		//


		pGainTbl.val=0;
		pGainTbl.val |= ((pGainTbl.txbb1dbbiquadgain&TXGAIN_TABLE_TXBB1DBBIQUAD_MASK)<<TXGAIN_TABLE_TXBB1DBBIQUAD_LSB);
		pGainTbl.val |= ((pGainTbl.txbb6dbbiquadgain&TXGAIN_TABLE_TXBB6DBBIQUAD2_MASK)<<TXGAIN_TABLE_TXBB6DBBIQUAD2_LSB);
		pGainTbl.val |= ((pGainTbl.txbbgc&TXGAIN_TABLE_TXBBGC_MASK)<<TXGAIN_TABLE_TXBBGC_LSB);
		pGainTbl.val |= ((pGainTbl.padrv2gn2g&TXGAIN_TABLE_PADRV2GN2_MASK)<<TXGAIN_TABLE_PADRV2GN2_LSB);
		pGainTbl.val |= ((pGainTbl.padrv3gn5g&TXGAIN_TABLE_PADRV3GN5_MASK)<<TXGAIN_TABLE_PADRV3GN5_LSB);
		pGainTbl.val |= ((pGainTbl.padrv2gn5g&TXGAIN_TABLE_PADRV2GN5_MASK)<<TXGAIN_TABLE_PADRV2GN5_LSB);
		pGainTbl.val |= ((pGainTbl.padrvhalfgn2g&TXGAIN_TABLE_PADRVHALFGN2G_MASK)<<TXGAIN_TABLE_PADRVHALFGN2G_LSB);
		pGainTbl.val |= ((pGainTbl.total_gain&TXGAIN_TABLE_TOTAL_GAIN_MASK)<<TXGAIN_TABLE_TOTAL_GAIN_LSB);
				
		//
		// figure out where we are supposed to write this
		//
		//
		// FJC: Will need to add support for PAL_on table
		//
		SformatOutput(fieldName, 50-1, "%s%d", TXGAIN_TABLE_STRING, entry+1); //field table names are 1 based	   

		// 
		// hal loads this value on reset
		// so we can fetch the value for the current mode from the register
		//
		FieldWrite(fieldName,pGainTbl.val);
	}

	return -1;
}
