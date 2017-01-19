
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <math.h>
#include <ctype.h>


#include "smatch.h"
#include "UserPrint.h"
#include "TimeMillisecond.h"
#include "NewArt.h"
#include "ParameterSelect.h"
#include "Card.h"
#include "Field.h"

#include "mCal5416.h"
#include "ar2413reg.h"
#include "wlantype.h"
#include "Ar9287EepromStructSet.h"

#include "AnwiDriverInterface.h"

//++JC++
typedef struct AR2413_txgain_tbl {
  unsigned char desired_gain ;
  unsigned char bb1_gain ;
  unsigned char bb2_gain ;
  unsigned char if_gain ;
  unsigned char rf_gain ;
 } AR2413_TXGAIN_TBL; 
//++JC++

typedef struct merlin_txgain_tbl {
  unsigned int val;		// this is the value compressed into one 32 bit thingee
  unsigned char desired_gain ;
  unsigned char paout2gn;
  unsigned char padrvgn;
  unsigned char pabuf5gn;
  unsigned char txV2Igain;
  unsigned char txldBloqgain;
 } MERLIN_TXGAIN_TBL; 

#define  MERLIN_TX_GAIN_TBL_SIZE  22
#define  AR2413_TX_GAIN_TBL_SIZE  26

#define  TXGAIN_TABLE_STRING      "bb_tx_gain_table_"
#define  TXGAIN_TABLE_STRING_OPEN_LOOP "high_power_bb_tx_gain_table_"
#define  TXGAIN_TABLE_TX1DBLOQGAIN_LSB           0
#define  TXGAIN_TABLE_TX1DBLOQGAIN_MASK          0x7
#define  TXGAIN_TABLE_TXV2IGAIN_LSB              3
#define  TXGAIN_TABLE_TXV2IGAIN_MASK             0x3
#define  TXGAIN_TABLE_PABUF5GN_LSB               5
#define  TXGAIN_TABLE_PABUF5GN_MASK              0x1
#define  TXGAIN_TABLE_PADRVGN_LSB                6
#define  TXGAIN_TABLE_PADRVGN_MASK               0x7
#define  TXGAIN_TABLE_PAOUT2GN_LSB               9
#define  TXGAIN_TABLE_PAOUT2GN_MASK              0x7
#define  TXGAIN_TABLE_GAININHALFDB_LSB           12
#define  TXGAIN_TABLE_GAININHALFDB_MASK          0x7F

static int openLoopPwrCntl;			// WHO SETS THIS???

#define GAIN_OVERRIDE       0xffff


#ifdef UNUSED

static AR2413_TXGAIN_TBL griffin_tx_gain_tbl[] =
{
#include  "AR2413_tx_gain.tbl"
} ;

static AR2413_TXGAIN_TBL spider_tx_gain_tbl[] =
{
#include  "spider2_0.tbl"
} ;

static AR2413_TXGAIN_TBL eagle_tx_gain_tbl_2[] =
{
#include  "ar5413_tx_gain_2.tbl"
} ;
static AR2413_TXGAIN_TBL eagle_tx_gain_tbl_5[] =
{
#include  "ar5413_tx_gain_5.tbl"
} ;
#define  AR2413_TX_GAIN_TBL_SIZE  26
#endif

static AR2413_TXGAIN_TBL owl_tx_gain_tbl_5[] =
{
#include  "AR5416_tx_gain_5.tbl"
} ;
#define  AR5416_TX_GAIN_TBL_5_SIZE  20


static AR2413_TXGAIN_TBL owl_tx_gain_tbl_2[] =
{
#include  "AR5416_tx_gain_2.tbl"
} ;
#define  AR5416_TX_GAIN_TBL_2_SIZE  19

#define  TXGAIN_TABLE_STRING_HIGH_POWER "high_power_bb_tx_gain_table_"
#define  TXGAIN_TABLE_FILENAME "merlin_tx_gain_2.tbl"
#define  AR9287_PAL_ON         "_pal_on"

MERLIN_TXGAIN_TBL merlin_tx_gain_table[MERLIN_TX_GAIN_TBL_SIZE];


static int generateTxGainTblFromCfg(unsigned long modeMask)
{
    char fieldName[50];
    int currentMode;
	int i;
    //
	// we only compute the values for the current mode
	//
    currentMode=0;

			for (i=0; i<MERLIN_TX_GAIN_TBL_SIZE; i++) 
			{
				if( openLoopPwrCntl ) 
				{ /* open loop power control */
	#ifdef UNUSED
					if( isKiwi(pLibDev->swDevID) ) 
					{
						if( pLibDev->libCfgParams.pal_on ) /* pal on */
						{
							SformatOutput(fieldName, 50-1, "%s%d%s", TXGAIN_TABLE_STRING, i, AR9287_PAL_ON);
						}
						else /* pal off */
						{ 
							SformatOutput(fieldName, 50-1, "%s%d", TXGAIN_TABLE_STRING, i);
						}
					}
					else 
	#endif
					{
						SformatOutput(fieldName, 50-1, "%s%d", TXGAIN_TABLE_STRING_OPEN_LOOP, i);
					}
				}
				else 
				{
	#ifdef UNUSED
					if((isKiteSW(pLibDev->swDevID))  && (pLibDev->libCfgParams.txGainType==1))
					{ // Kite based High_power design (CUS128)
						SformatOutput(fieldName, 50-1, "%s%d", TXGAIN_TABLE_STRING_HIGH_POWER, i);
					}
					else
	#endif
					{
						SformatOutput(fieldName, 50-1, "%s%d", TXGAIN_TABLE_STRING, i);      
					}
				}

				// 
				// hal loads this value on reset
				// so we can fetch the value for the current mode from the register
				//
				FieldRead(fieldName,&merlin_tx_gain_table[i].val);
				
				merlin_tx_gain_table[i].txldBloqgain = 
					(unsigned char)((merlin_tx_gain_table[i].val >> TXGAIN_TABLE_TX1DBLOQGAIN_LSB) & TXGAIN_TABLE_TX1DBLOQGAIN_MASK); 
				merlin_tx_gain_table[i].txV2Igain = 
					(unsigned char)((merlin_tx_gain_table[i].val >> TXGAIN_TABLE_TXV2IGAIN_LSB) & TXGAIN_TABLE_TXV2IGAIN_MASK); 
				merlin_tx_gain_table[i].pabuf5gn = 
					(unsigned char)((merlin_tx_gain_table[i].val >> TXGAIN_TABLE_PABUF5GN_LSB) & TXGAIN_TABLE_PABUF5GN_MASK); 
				merlin_tx_gain_table[i].padrvgn = 
					(unsigned char)((merlin_tx_gain_table[i].val >> TXGAIN_TABLE_PADRVGN_LSB) & TXGAIN_TABLE_PADRVGN_MASK); 
				merlin_tx_gain_table[i].paout2gn = 
					(unsigned char)((merlin_tx_gain_table[i].val >> TXGAIN_TABLE_PAOUT2GN_LSB) & TXGAIN_TABLE_PAOUT2GN_MASK); 
				merlin_tx_gain_table[i].desired_gain = 
					(unsigned char)((merlin_tx_gain_table[i].val >> TXGAIN_TABLE_GAININHALFDB_LSB) & TXGAIN_TABLE_GAININHALFDB_MASK); 

			}

    return 0;
}


static int ForceSinglePCDACTableMerlin(int mode, int pcdac)
{
	unsigned short i;
	unsigned long dac_gain = 0;
	MERLIN_TXGAIN_TBL *pGainTbl = NULL;
	int offset=0;

    generateTxGainTblFromCfg(0x3);

    pGainTbl = merlin_tx_gain_table;

	i = 0;
	while ((pcdac > pGainTbl[i].desired_gain) && (i < (MERLIN_TX_GAIN_TBL_SIZE -1)) ) {i++;}  // Find entry closest

	UserPrint("using gain entry %d\n",i);
	
    if (pGainTbl[i].desired_gain > pcdac) 
	{
        dac_gain = pGainTbl[i].desired_gain - pcdac;
	}
	FieldWrite("bb_force_dac_gain", 1);
	FieldWrite("bb_forced_dac_gain", dac_gain);
	FieldWrite("bb_force_tx_gain", 1);
	FieldWrite("bb_forced_paout2gn", pGainTbl[i].paout2gn);
	FieldWrite("bb_forced_padrvgn", pGainTbl[i].padrvgn);
	FieldWrite("bb_forced_txV2Igain", pGainTbl[i].txV2Igain);
	FieldWrite("bb_forced_txldBloqgain", pGainTbl[i].txldBloqgain);
#ifdef UNUSED
    if( isKiwi(pLibDev->swDevID) ) 
	{
        FieldWrite("bb_forced_pabuf5gn", pGainTbl[i].pabuf5gn);
    }
    else 
#endif
	{
        if( mode ) 
		{
            FieldWrite("bb_forced_pabuf5gn", pGainTbl[i].pabuf5gn);
        }
    }

	FieldWrite("bb_use_per_packet_powertx_max", 0);

	return i;
}


#ifdef UNUSED

void ForceSinglePCDACTableGriffin(int mode, int pcdac, int offset)
{
	unsigned short i;
	unsigned long dac_gain = 0;
	AR2413_TXGAIN_TBL *pGainTbl = NULL;
	unsigned long gainTableSize = AR2413_TX_GAIN_TBL_SIZE;

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
	unsigned long temp32;
	unsigned long regoffset ;
	unsigned long pcdac_shift = 8;
	unsigned long predist_scale = 0x00FF;
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

#endif

//
// Set tx power to a specific value
//
int Ar5416TransmitPowerSet(int mode, double txp)
{
    unsigned short    ht40PowerIncForPdadc = 0; //hard code for now, need to get from eeprom struct
    unsigned short    powerOffsetFromZero  = -10; //hard code for now, need to get from eeprom struct
    unsigned short    txPowerTimes2;
#define POW_SM(_r, _s)     (((_r) & 0x3f) << (_s))

    FieldWrite("bb_use_per_packet_powertx_max", 0);
    FieldWrite("bb_force_dac_gain", 0);
    FieldWrite("bb_force_tx_gain", 0);

    //convert power from double to integer (power * 2)
    txPowerTimes2=(int)((2*txp)+0.5) - powerOffsetFromZero;

    /* Write the OFDM power per rate set */
    MyRegisterWrite(0x9934,
        POW_SM(txPowerTimes2, 24)
          | POW_SM(txPowerTimes2, 16)
          | POW_SM(txPowerTimes2,  8)
          | POW_SM(txPowerTimes2,  0)
    );
    MyRegisterWrite(0x9938,
        POW_SM(txPowerTimes2, 24)
          | POW_SM(txPowerTimes2, 16)
          | POW_SM(txPowerTimes2,  8)
          | POW_SM(txPowerTimes2,  0)
    );

	/* Write the CCK power per rate set */
	MyRegisterWrite(0xa234,
	    POW_SM(txPowerTimes2, 24)
		  | POW_SM(txPowerTimes2,  16)
		  | POW_SM(txPowerTimes2,  8) /* XR target power */
		  | POW_SM(txPowerTimes2,   0)
	);
	MyRegisterWrite(0xa238,
	    POW_SM(txPowerTimes2, 24)
		  | POW_SM(txPowerTimes2, 16)
		  | POW_SM(txPowerTimes2,  8)
		  | POW_SM(txPowerTimes2,  0)
	);

    /* Write the HT20 power per rate set */
    MyRegisterWrite(0xa38C,
        POW_SM(txPowerTimes2, 24)
          | POW_SM(txPowerTimes2,  16)
          | POW_SM(txPowerTimes2,  8)
          | POW_SM(txPowerTimes2,   0)
    );
    MyRegisterWrite(0xa390,
        POW_SM(txPowerTimes2, 24)
          | POW_SM(txPowerTimes2,  16)
          | POW_SM(txPowerTimes2,  8)
          | POW_SM(txPowerTimes2,   0)
    );

    /* Write the HT40 power per rate set */
    // correct PAR difference between HT40 and HT20/LEGACY
    MyRegisterWrite(0xa3CC,
        POW_SM(txPowerTimes2 + ht40PowerIncForPdadc, 24)
          | POW_SM(txPowerTimes2 + ht40PowerIncForPdadc,  16)
          | POW_SM(txPowerTimes2 + ht40PowerIncForPdadc,  8)
          | POW_SM(txPowerTimes2 + ht40PowerIncForPdadc,   0)
    );
    MyRegisterWrite(0xa3D0,
        POW_SM(txPowerTimes2 + ht40PowerIncForPdadc, 24)
          | POW_SM(txPowerTimes2 + ht40PowerIncForPdadc,  16)
          | POW_SM(txPowerTimes2 + ht40PowerIncForPdadc,  8)
          | POW_SM(txPowerTimes2 + ht40PowerIncForPdadc,   0)
    );

	MyRegisterWrite(0xa3D4,
	    POW_SM(txPowerTimes2, 24)
	      | POW_SM(txPowerTimes2,  16)
	      | POW_SM(txPowerTimes2,  8)
	      | POW_SM(txPowerTimes2,   0)
	);

	MyRegisterWrite(0x9933,0);

	return 0;
}


//
// set transmit power gain (pcdac) to a specified value
//	
int Ar5416TransmitGainSet(int mode, int pcdac)
{	
	unsigned int value;
#ifdef UNUSED
    if (isGriffin(swDeviceID) || isEagle(swDeviceID) || isOwl(swDeviceID)) 
#endif
	{
        MyRegisterRead(TPCRG1_REG, &value);
		MyRegisterWrite(TPCRG1_REG,  value | BB_FORCE_DAC_GAIN_SET);

        MyRegisterRead(TPCRG7_REG, &value);
        MyRegisterWrite( TPCRG7_REG, value | BB_FORCE_TX_GAIN_SET);
    }
#ifdef UNUSED
    if(isMerlinPowerControl(pLibDev->swDevID)) 
#endif
	{
        return ForceSinglePCDACTableMerlin(mode, pcdac);
    }
#ifdef UNUSED
	else
	{
        return ForceSinglePCDACTableGriffin(mode, pcdac, 30);
	}
#endif
 
    return -1;
}


int Ar5416TransmitGainRead(int entry, unsigned int *rvalue, int *value, int max)
{
	unsigned long dac_gain = 0;
	MERLIN_TXGAIN_TBL *pGainTbl = NULL;
	int offset=0;

    generateTxGainTblFromCfg(0x3);

	if(entry>=0 && entry<MERLIN_TX_GAIN_TBL_SIZE && max>=6)
	{
        pGainTbl = &merlin_tx_gain_table[entry];

		*rvalue=pGainTbl->val;
        value[0]=pGainTbl->desired_gain ;
        value[1]=pGainTbl->paout2gn;
        value[2]=pGainTbl->padrvgn;
        value[3]=pGainTbl->pabuf5gn;
        value[4]=pGainTbl->txV2Igain;
        value[5]=pGainTbl->txldBloqgain;
	    return 6;
	}
	return -1;
}


int Ar5416TransmitGainWrite(int entry, int *value, int nvalue)
{
    char fieldName[50];
 	MERLIN_TXGAIN_TBL pGainTbl;

	if(entry>=0 && entry<MERLIN_TX_GAIN_TBL_SIZE && nvalue==6)
	{
        pGainTbl.desired_gain=value[0] ;
        pGainTbl.paout2gn=value[1];
        pGainTbl.padrvgn=value[2];
        pGainTbl.pabuf5gn=value[3];
        pGainTbl.txV2Igain=value[4];
        pGainTbl.txldBloqgain=value[5];
        //
		// pack up the fields
		//
		pGainTbl.val=0;
		pGainTbl.val |= ((pGainTbl.txldBloqgain&TXGAIN_TABLE_TX1DBLOQGAIN_MASK)<<TXGAIN_TABLE_TX1DBLOQGAIN_LSB);
		pGainTbl.val |= ((pGainTbl.txV2Igain&TXGAIN_TABLE_TXV2IGAIN_MASK)<<TXGAIN_TABLE_TXV2IGAIN_LSB);
		pGainTbl.val |= ((pGainTbl.pabuf5gn&TXGAIN_TABLE_PABUF5GN_MASK)<<TXGAIN_TABLE_PABUF5GN_LSB);
		pGainTbl.val |= ((pGainTbl.padrvgn&TXGAIN_TABLE_PADRVGN_MASK)<<TXGAIN_TABLE_PADRVGN_LSB);
		pGainTbl.val |= ((pGainTbl.paout2gn&TXGAIN_TABLE_PAOUT2GN_MASK)<<TXGAIN_TABLE_PAOUT2GN_LSB);
		pGainTbl.val |= ((pGainTbl.desired_gain&TXGAIN_TABLE_GAININHALFDB_MASK)<<TXGAIN_TABLE_GAININHALFDB_LSB);
				
        //
		// figure out where we are supposed to write this
		//
				if( openLoopPwrCntl ) 
				{ /* open loop power control */
	#ifdef UNUSED
					if( isKiwi(pLibDev->swDevID) ) 
					{
						if( pLibDev->libCfgParams.pal_on ) /* pal on */
						{
							SformatOutput(fieldName, 50-1, "%s%d%s", TXGAIN_TABLE_STRING, entry, AR9287_PAL_ON);
						}
						else /* pal off */
						{ 
							SformatOutput(fieldName, 50-1, "%s%d", TXGAIN_TABLE_STRING, entry);
						}
					}
					else 
	#endif
					{
						SformatOutput(fieldName, 50-1, "%s%d", TXGAIN_TABLE_STRING_OPEN_LOOP, entry);
					}
				}
				else 
				{
	#ifdef UNUSED
					if((isKiteSW(pLibDev->swDevID))  && (pLibDev->libCfgParams.txGainType==1))
					{ // Kite based High_power design (CUS128)
						SformatOutput(fieldName, 50-1, "%s%d", TXGAIN_TABLE_STRING_HIGH_POWER, entry);
					}
					else
	#endif
					{
						SformatOutput(fieldName, 50-1, "%s%d", TXGAIN_TABLE_STRING, entry);      
					}
				}

				

				// 
				// overwrite the value
				//
				FieldWrite(fieldName,pGainTbl.val);
	}

	return -1;
}

extern struct ath_hal *AH;

extern ar5416SetTargetPowerFromEeprom(struct ath_hal *ah);
int Ar5416TargetPowerApply(int frequency)
{
	/* make sure forced gain is not set - HAL function will not do this */
	FieldWrite("bb_use_per_packet_powertx_max", 0);
	FieldWrite("bb_force_dac_gain", 0);
	FieldWrite("bb_force_tx_gain", 0);

	ar5416SetTargetPowerFromEeprom(AH);

    Ar9287_RefPwrSet(frequency);

    return 0;
}

