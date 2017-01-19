
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

#include "AquilaNewmaMapping.h"

#include "wlantype.h"
#include "smatch.h"
#include "UserPrint.h"
#include "TimeMillisecond.h"
#include "Card.h"
#include "Field.h"
#include "Ar9300Device.h"

#include "mCal9300.h"
#ifndef __APPLE__
//#include "ar2413reg.h"
#endif
//include hal files
#include "ah.h"
#include "ah_internal.h"
#include "ar9300.h"
#include "ar9300eep.h"


enum {TpcmTxGain=0, TpcmTxPower, TpcmTargetPower,TpcmTxGainIndex};

typedef struct osprey_txgain_tbl {
	unsigned int val;
  unsigned char total_gain;			//[31:24]
  unsigned char txbb1dbgain;		//[2:0]
  unsigned char txbb6dbgain;        //[4:3]
  unsigned char txmxrgain;			//[8:5]
  unsigned char padrvgnA;           //[12:9]
  unsigned char padrvgnB;           //[16:13]
  unsigned char padrvgnC;           //[20:17]
  unsigned char padrvgnD;            //[22:21]
 } OSPREY_TXGAIN_TBL; 



#define  TXGAIN_TABLE_STRING      "tg_table" 
#define  TXGAIN_TABLE_TXBB1DBGAIN_LSB        0			// [2:0]
#define  TXGAIN_TABLE_TXBB1DBGAIN_MASK       0x7
#define  TXGAIN_TABLE_TXBB6DBGAIN_LSB        3			// [4:3]
#define  TXGAIN_TABLE_TXBB6DBGAIN_MASK       0x3
#define  TXGAIN_TABLE_TXMXRGAIN_LSB          5			// [8:5]
#define  TXGAIN_TABLE_TXMXRGAIN_MASK         0xf
#define  TXGAIN_TABLE_PADRVGNA_LSB           9			// [12:9]
#define  TXGAIN_TABLE_PADRVGNA_MASK          0xf
#define  TXGAIN_TABLE_PADRVGNB_LSB           13			// [16:13]
#define  TXGAIN_TABLE_PADRVGNB_MASK          0xf
#define  TXGAIN_TABLE_PADRVGNC_LSB           17			// [20:17]
#define  TXGAIN_TABLE_PADRVGNC_MASK          0xf
#define  TXGAIN_TABLE_PADRVGND_LSB           21			// [22:21]
#define  TXGAIN_TABLE_PADRVGND_MASK          0x3
#define  TXGAIN_TABLE_TOTAL_GAIN_LSB         24			// [31:24]
#define  TXGAIN_TABLE_TOTAL_GAIN_MASK        0xff

#define MAX_SIZE_OSPREY_TX_GAIN_TABLE        32

static int openLoopPwrCntl;			// WHO SETS THIS???
static OSPREY_TXGAIN_TBL ospreyGainTable[MAX_SIZE_OSPREY_TX_GAIN_TABLE];

#define GAIN_OVERRIDE       0xffff



#define  TXGAIN_TABLE_STRING_HIGH_POWER "high_power_bb_tx_gain_table_"
#define  TXGAIN_TABLE_FILENAME "merlin_tx_gain_2.tbl"
#define  AR9287_PAL_ON         "_pal_on"




static int generateOspreyTxGainTblFromCfg(unsigned int modeMask, int *pGainTableSize)
{
    char fieldName[50];
    int currentMode;
	int i;

    //
	// we only compute the values for the current mode
	//
    currentMode=0;

	//read gain table size
	if(FieldRead("tx_gain_table_max", (unsigned int *)pGainTableSize) == -1) 
	{
		return(-1);
	}

	if (*pGainTableSize > MAX_SIZE_OSPREY_TX_GAIN_TABLE) {
		UserPrint("Bad gain table size = %d, must be less than %d\n", *pGainTableSize, MAX_SIZE_OSPREY_TX_GAIN_TABLE);
		return(-1);
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
		FieldRead(fieldName,&ospreyGainTable[i].val);
		
//		UserPrint("gain[%d]: %x\n",i,ospreyGainTable[i].val);

		ospreyGainTable[i].total_gain = 
			(unsigned char)((ospreyGainTable[i].val >> TXGAIN_TABLE_TOTAL_GAIN_LSB) & TXGAIN_TABLE_TOTAL_GAIN_MASK); 
		ospreyGainTable[i].txbb1dbgain = 
			(unsigned char)((ospreyGainTable[i].val >> TXGAIN_TABLE_TXBB1DBGAIN_LSB) & TXGAIN_TABLE_TXBB1DBGAIN_MASK); 
		ospreyGainTable[i].txbb6dbgain = 
			(unsigned char)((ospreyGainTable[i].val >> TXGAIN_TABLE_TXBB6DBGAIN_LSB) & TXGAIN_TABLE_TXBB6DBGAIN_MASK); 
		ospreyGainTable[i].txmxrgain = 
			(unsigned char)((ospreyGainTable[i].val >> TXGAIN_TABLE_TXMXRGAIN_LSB) & TXGAIN_TABLE_TXMXRGAIN_MASK); 
		ospreyGainTable[i].padrvgnA = 
			(unsigned char)((ospreyGainTable[i].val >> TXGAIN_TABLE_PADRVGNA_LSB) & TXGAIN_TABLE_PADRVGNA_MASK); 
		ospreyGainTable[i].padrvgnB = 
			(unsigned char)((ospreyGainTable[i].val >> TXGAIN_TABLE_PADRVGNB_LSB) & TXGAIN_TABLE_PADRVGNB_MASK); 
		ospreyGainTable[i].padrvgnC = 
			(unsigned char)((ospreyGainTable[i].val >> TXGAIN_TABLE_PADRVGNC_LSB) & TXGAIN_TABLE_PADRVGNC_MASK); 
		ospreyGainTable[i].padrvgnD = 
			(unsigned char)((ospreyGainTable[i].val >> TXGAIN_TABLE_PADRVGND_LSB) & TXGAIN_TABLE_PADRVGND_MASK); 

	}
    return 0;
}


static int ForceSingleGainTableOsprey(int mode, int requestedGain)
{
	unsigned short i;
	unsigned int dac_gain = 0;
	OSPREY_TXGAIN_TBL *pGainTbl = NULL;
	int gainTableSize;
	int offset=0;

    generateOspreyTxGainTblFromCfg(0x3, &gainTableSize);

    pGainTbl = ospreyGainTable;
    
    if(mode==TpcmTxGainIndex){
	
	UserPrint("Using gain entry %d\n",requestedGain);
	FieldWrite("force_dac_gain", 1);
	FieldWrite("forced_dac_gain", 0);
	FieldWrite("force_tx_gain", 1);
	FieldWrite("forced_txbb1dbgain", pGainTbl[requestedGain-1].txbb1dbgain);
	FieldWrite("forced_txbb6dbgain", pGainTbl[requestedGain-1].txbb6dbgain);
	FieldWrite("forced_txmxrgain", pGainTbl[requestedGain-1].txmxrgain);
	FieldWrite("forced_padrvgnA", pGainTbl[requestedGain-1].padrvgnA);
	FieldWrite("forced_padrvgnB", pGainTbl[requestedGain-1].padrvgnB);
	FieldWrite("forced_padrvgnC", pGainTbl[requestedGain-1].padrvgnC);
	FieldWrite("forced_padrvgnD", pGainTbl[requestedGain-1].padrvgnD);

	//FJC: set PAL to off for now. Will need to make this selectable.
	FieldWrite("forced_enable_PAL", 0);
	return requestedGain;
    }

	i = 0;
	while ((requestedGain > pGainTbl[i].total_gain) && (i < (gainTableSize -1)) ) {i++;}  // Find entry closest

	UserPrint("using gain entry %d\n",i+1);
	
    if (pGainTbl[i].total_gain > requestedGain) 
	{
        dac_gain = pGainTbl[i].total_gain - requestedGain;
	}

	FieldWrite("force_dac_gain", 1);
	FieldWrite("forced_dac_gain", dac_gain);
	FieldWrite("force_tx_gain", 1);
	FieldWrite("forced_txbb1dbgain", pGainTbl[i].txbb1dbgain);
	FieldWrite("forced_txbb6dbgain", pGainTbl[i].txbb6dbgain);
	FieldWrite("forced_txmxrgain", pGainTbl[i].txmxrgain);
	FieldWrite("forced_padrvgnA", pGainTbl[i].padrvgnA);
	FieldWrite("forced_padrvgnB", pGainTbl[i].padrvgnB);
	FieldWrite("forced_padrvgnC", pGainTbl[i].padrvgnC);
	FieldWrite("forced_padrvgnD", pGainTbl[i].padrvgnD);

	//FJC: set PAL to off for now. Will need to make this selectable.
	FieldWrite("forced_enable_PAL", 0);

	return i+1;
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

//   UserPrint("\nSNOOP: offset = %d, i=%d, dac_gain = %d, bb1 = %d, bb2 = %d, gainif = %d, gainrf = %d\n",offset,i,
//   dac_gain, pGainTbl[i].bb1_gain, pGainTbl[i].bb2_gain, pGainTbl[i].if_gain,
//   pGainTbl[i].rf_gain);

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
	unsigned char      falconTrue;
	falconTrue = isFalcon(devNum);

	if (falconTrue) {
		pcdac_shift = 0;
		predist_scale = 0; // no predist in falcon
	}

//++JC++

	if(isGriffin(pLibDev->swDevID) || isEagle(pLibDev->swDevID) || isOwl(pLibDev->swDevID))  {    // For Griffin
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
Ar9300TransmitPowerRegWrite(A_UINT8 *pPwrArray) 
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
		  | POW_SM(pPwrArray[ALL_TARGET_LEGACY_1L_5L],  16)
//		  | POW_SM(txPowerTimes2,  8) /* this is reserved for Osprey */
		  | POW_SM(pPwrArray[ALL_TARGET_LEGACY_1L_5L],   0)
	);
    /* 5.5L (LSB), 5.5S, 11L, 11S (MSB) */
	MyRegisterWrite(0xa3cc,
	    POW_SM(pPwrArray[ALL_TARGET_LEGACY_11S], 24)
		  | POW_SM(pPwrArray[ALL_TARGET_LEGACY_11L], 16)
		  | POW_SM(pPwrArray[ALL_TARGET_LEGACY_5S],  8)
		  | POW_SM(pPwrArray[ALL_TARGET_LEGACY_1L_5L],  0)
	);

    /* Write the HT20 power per rate set */
    /* 0/8/16 (LSB), 1-3/9-11/17-19, 4, 5 (MSB) */
    MyRegisterWrite(0xa3d0,
        POW_SM(pPwrArray[ALL_TARGET_HT20_5], 24)
          | POW_SM(pPwrArray[ALL_TARGET_HT20_4],  16)
          | POW_SM(pPwrArray[ALL_TARGET_HT20_1_3_9_11_17_19],  8)
          | POW_SM(pPwrArray[ALL_TARGET_HT20_0_8_16],   0)
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
          | POW_SM(pPwrArray[ALL_TARGET_HT20_14],   0)
    );

    /* Mixed HT20 and HT40 rates */
    /* HT20 22 (LSB), HT20 23, HT40 22, HT40 23 (MSB) */
    MyRegisterWrite(0xa3e8,
        POW_SM(pPwrArray[ALL_TARGET_HT40_23], 24)
          | POW_SM(pPwrArray[ALL_TARGET_HT40_22],  16)
          | POW_SM(pPwrArray[ALL_TARGET_HT20_23],  8)
          | POW_SM(pPwrArray[ALL_TARGET_HT20_22],   0)
    );
    
    /* Write the HT40 power per rate set */
    // correct PAR difference between HT40 and HT20/LEGACY
    /* 0/8/16 (LSB), 1-3/9-11/17-19, 4, 5 (MSB) */
    MyRegisterWrite(0xa3d8,
        POW_SM(pPwrArray[ALL_TARGET_HT40_5], 24)
          | POW_SM(pPwrArray[ALL_TARGET_HT40_4],  16)
          | POW_SM(pPwrArray[ALL_TARGET_HT40_1_3_9_11_17_19],  8)
          | POW_SM(pPwrArray[ALL_TARGET_HT40_0_8_16],   0)
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
	      | POW_SM(pPwrArray[ALL_TARGET_HT40_14],   0)
	);

	return 0;
}
#endif

//
// Set tx power registers to array of values passed in
//
int nartAr9300TransmitPowerRegWrite(A_UINT8 *pPwrArray) 
{   
	/* make sure forced gain is not set */
    FieldWrite("force_dac_gain", 0);
	FieldWrite("force_tx_gain", 0);
	
	//call function within HAL
	ar9300_transmit_power_reg_write(AH, pPwrArray);
	return 0;
}
//set power registers all to same value
AR9300DLLSPEC int Ar9300TransmitPowerSet(int mode, double txp)
{
    A_INT8    ht40PowerIncForPdadc = 0; //hard code for now, need to get from eeprom struct
    A_INT8    powerOffsetFromZero  = 0; //hard code for now, need to get from eeprom struct
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
    nartAr9300TransmitPowerRegWrite(targetPowerValT2);
    return 0;
}


//
// set transmit power gain (pcdac) to a specified value
//	
AR9300DLLSPEC int Ar9300TransmitGainSet(int mode, int txgain)
{	
    return ForceSingleGainTableOsprey(mode, txgain);
}

AR9300DLLSPEC int Ar9300TransmitGainRead(int entry, unsigned int *rvalue, int *value, int max)
{
	unsigned int dac_gain = 0;
	OSPREY_TXGAIN_TBL *pGainTbl = NULL;
	int offset=0;
    int pGainTableSize;

    generateOspreyTxGainTblFromCfg(0x3,&pGainTableSize);

	if(entry>=0 && entry<pGainTableSize && max>=8)
	{
        pGainTbl = &ospreyGainTable[entry];

		*rvalue=pGainTbl->val;
        value[0]=pGainTbl->total_gain;
        value[1]=pGainTbl->txbb1dbgain;
        value[2]=pGainTbl->txbb6dbgain;
        value[3]=pGainTbl->txmxrgain;
        value[4]=pGainTbl->padrvgnA;
        value[5]=pGainTbl->padrvgnB;
        value[6]=pGainTbl->padrvgnC;
        value[7]=pGainTbl->padrvgnD;
	    return 8;
	}
	return -1;
}

AR9300DLLSPEC int Ar9300TransmitINIGainGet(int *total_gain)
{
	OSPREY_TXGAIN_TBL *pGainTbl = NULL;
	int entry;
    int pGainTableSize;

    generateOspreyTxGainTblFromCfg(0x3,&pGainTableSize);

	for (entry=0; entry<pGainTableSize; entry++) 
	{
        pGainTbl = &ospreyGainTable[entry];
		total_gain[entry] = pGainTbl->total_gain;
	}
	return pGainTableSize;
}

//Function - Ar9300TxGainTableRead_AddressGainTable
//Purpose  - Retrive loaded in memory gain table address and number of entries
//Parameter- addr : retunred adress of Gain Table array
//           row  : return number of entries
//           col  : return size of each of entries
//Return   - None
AR9300DLLSPEC int Ar9300TxGainTableRead_AddressGainTable(unsigned int **addr, unsigned int *row, unsigned int *col)
{
	extern struct ath_hal *AH;
	struct ath_hal_9300 *ahp = AH9300(AH);

	*addr = (unsigned int *)ahp->ah_ini_modes_txgain.ia_array;
	*row=ahp->ah_ini_modes_txgain.ia_rows;
	*col=ahp->ah_ini_modes_txgain.ia_columns;
	return (0);
}
#define MAX_FIELD_ENTRIES   100   // Wild guess on unknown # of fields

//Function - Ar9300TxGainTableRead_AddressHeader
//Purpose  - Retrive all field names associated with register name from input address
//Parameter- address : adress of entry
//           header  : buffer returned with regsiter name and associated field names for NART to display
//           max     : maximum size of input buffer for header
//Return   - number of fields
AR9300DLLSPEC int Ar9300TxGainTableRead_AddressHeader(unsigned int address, char *header, char *subheader, int max)
{
  char  *rName, *fName;
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
//Function - Ar9300TxGainTableRead_AddressValue
//Purpose  - Retrive field value from address along with nth field in case of multiple fields with same register name
//Parameter- address : adress of entry
//           idx     : index of field wish to retrive
//                   : -1 means getting whole 32 bit value regardless low and high bit address
//           rName   : returned register name for subsequent fucntion FiledRead call
//           fName   : returned field name for subsequent fucntion FiledRead call
//           value   : returned value of field value with low and high bit mask
//           low     : low address bit
//           high    : hgih address bit
//           max     : maximum size of input buffer for header
//Return   -  0 suceed
//           -1 failed
AR9300DLLSPEC int Ar9300TxGainTableRead_AddressValue(unsigned int address, int idx, char *rName, char *fName, int *value, int *low, int *high)
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

AR9300DLLSPEC int Ar9300TransmitGainWrite(int entry, int *value, int nvalue)
{
    char fieldName[50];
 	OSPREY_TXGAIN_TBL pGainTbl;

    int pGainTableSize;

	//read gain table size
	if(FieldRead("tx_gain_table_max", (unsigned int *)&pGainTableSize) == -1) 
	{
		return(-1);
	}

	if(entry>=0 && entry<pGainTableSize && nvalue==8)
	{
        pGainTbl.total_gain=value[0] ;
        pGainTbl.txbb1dbgain=value[1];
        pGainTbl.txbb6dbgain=value[2];
        pGainTbl.txmxrgain=value[3];
        pGainTbl.padrvgnA=value[4];
        pGainTbl.padrvgnB=value[5];
        pGainTbl.padrvgnC=value[6];
        pGainTbl.padrvgnD=value[7];
        //
		// pack up the fields
		//
		pGainTbl.val=0;
		pGainTbl.val |= ((pGainTbl.txbb1dbgain&TXGAIN_TABLE_TXBB1DBGAIN_MASK)<<TXGAIN_TABLE_TXBB1DBGAIN_LSB);
		pGainTbl.val |= ((pGainTbl.txbb6dbgain&TXGAIN_TABLE_TXBB6DBGAIN_MASK)<<TXGAIN_TABLE_TXBB6DBGAIN_LSB);
		pGainTbl.val |= ((pGainTbl.txmxrgain&TXGAIN_TABLE_TXMXRGAIN_MASK)<<TXGAIN_TABLE_TXMXRGAIN_LSB);
		pGainTbl.val |= ((pGainTbl.padrvgnA&TXGAIN_TABLE_PADRVGNA_MASK)<<TXGAIN_TABLE_PADRVGNA_LSB);
		pGainTbl.val |= ((pGainTbl.padrvgnB&TXGAIN_TABLE_PADRVGNB_MASK)<<TXGAIN_TABLE_PADRVGNB_LSB);
		pGainTbl.val |= ((pGainTbl.padrvgnC&TXGAIN_TABLE_PADRVGNC_MASK)<<TXGAIN_TABLE_PADRVGNC_LSB);
		pGainTbl.val |= ((pGainTbl.padrvgnD&TXGAIN_TABLE_PADRVGND_MASK)<<TXGAIN_TABLE_PADRVGND_LSB);
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
