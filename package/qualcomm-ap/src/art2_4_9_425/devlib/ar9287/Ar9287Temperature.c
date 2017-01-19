#include "ah.h"
#include "ar5416/ar5416.h"
#include "ar5416/ar5416phy.h"
#include "Field.h"
#include "wlantype.h"
#undef REGR
#undef REGW

extern ar9287_eeprom_t *Ar9287EepromStructGet(void);
extern int MyRegisterRead(unsigned long address, unsigned long *value);
extern int MyRegisterWrite(unsigned long address, unsigned long value);
extern struct ath_hal *AH;

static int deafMode = 0;
static int undeafThresh62 = 0;
static int undeafThresh62Ext = 0;
static int undeafForceAgcClear = 0;
static int undeafCycpwrThr1 = 0;
static int undeafCycpwrThr1Ext = 0;
static int undeafRssiThr1a = 0;

static unsigned long REGR(unsigned long devNum, unsigned long address)
{
	unsigned long value;

	devNum=0;

	MyRegisterRead(address,&value);

	return value;
}

static void REGW(unsigned long devNum,unsigned long address, unsigned long value)
{
    devNum =0;
    MyRegisterWrite(address, value);
}


#define TEMP_SENS_REG           (0xa264)
#define TEMP_MASK_OLPC          (0xFF)



int Ar9287TemperatureGet(int forceTempRead)
{
    unsigned int rddata;

    rddata=(OS_REG_READ(AH,TEMP_SENS_REG)>>1)& TEMP_MASK_OLPC;
   
    return(rddata);
}

int Ar9287VoltageGet(void)
{
    return 0;
}

int Ar9287Deaf(int deaf) 
{
    
    unsigned long value;    
    //
    //If not currently in deaf mode Store off the existing field values so that can go back to undeaf mode
    //
    if (deafMode == 0) {
        FieldRead("CCA.bb_thresh62", (unsigned long *)&undeafThresh62);
        FieldRead("CCA1.bb_thresh62_ext", (unsigned long *) &undeafThresh62Ext);
        FieldRead("TST_2.bb_force_agc_clear", (unsigned long *) &undeafForceAgcClear);
        FieldRead("TIMING5.bb_cycpwr_thr1", (unsigned long *) &undeafCycpwrThr1);
        FieldRead("AGC_EXT.bb_cycpwr_thr1_ext", (unsigned long *) &undeafCycpwrThr1Ext);
        FieldRead("TIMING5.bb_rssi_thr1a", (unsigned long *) &undeafRssiThr1a);

    }

    if(deaf) {
        FieldWrite("CCA.bb_thresh62", 0x7f);
        FieldWrite("CCA1.bb_thresh62_ext", 0x7f);
        FieldWrite("TST_2.bb_force_agc_clear", 1);
        FieldWrite("TIMING5.bb_cycpwr_thr1", 0x7f);
        FieldWrite("AGC_EXT.bb_cycpwr_thr1_ext", 0x7f);
        FieldWrite("TIMING5.bb_rssi_thr1a", 0x7f);
        
        deafMode=1;
    } else {
        FieldWrite("CCA.bb_thresh62", undeafThresh62);
        FieldWrite("CCA1.bb_thresh62_ext", undeafThresh62Ext);
        FieldWrite("TST_2.bb_force_agc_clear", undeafForceAgcClear);
        FieldWrite("TIMING5.bb_cycpwr_thr1", undeafCycpwrThr1);
        FieldWrite("AGC_EXT.bb_cycpwr_thr1_ext", undeafCycpwrThr1Ext);
        FieldWrite("TIMING5.bb_rssi_thr1a", undeafRssiThr1a);
        
        deafMode=0;
    }
    return 0;
}

#define NUM_ENTRIES_TX_GAIN_TABLE (22)

int Ar9287CalibrationSetting(void)
{
    unsigned int i;
    unsigned int rddata, value, temp;
    static unsigned int originalGain[22];
    static unsigned int initPDADC = 0;
	static unsigned int txGainMax_old = 0;
    unsigned int currPDADC = 1;
    unsigned int calPDADC, currTxGainMax;
    int  delta=0;
    static int  delta_old = 0;
    int slope = 0;
	unsigned char slope_palon = 0;
    unsigned int tempValue;

    ar9287_eeprom_t *peep9287;  
    peep9287 = (ar9287_eeprom_t *)Ar9287EepromStructGet();   // prints the Current EEPROM structure

	slope=peep9287->baseEepHeader.tempSensSlope;
    calPDADC=peep9287->calPierData2G[0][0].calDataOpen.vpdPdg[0][0];
    
    MyRegisterRead(0xa264, &rddata);

    /*
        MyRegisterRead(0x7898,&tempValue);
        tempValue=(tempValue& 0xFFFF3FFF)|(0x2<<14);
        MyRegisterWrite(0x7898,tempValue);
        MyRegisterRead(0xa274,&tempValue);
        tempValue=tempValue|0x1<31;
        MyRegisterWrite(0xa274,tempValue);

        */

    MyRegisterRead(0xa274,&tempValue);
    currTxGainMax=(tempValue>>13)&0x3f;
    
    if((initPDADC != calPDADC) || (txGainMax_old != currTxGainMax)){ 
        /* read the tx gain pcdacs */
        for( i = 0; i < NUM_ENTRIES_TX_GAIN_TABLE; i++ ) {
            MyRegisterRead(0xa300 + i * 4,&tempValue);
            originalGain[i]=(tempValue>>12)&0x7f;
        }
        //printf(" index %d = 0x%x\n", i, originalGain[i]);

        initPDADC = calPDADC;
		txGainMax_old = currTxGainMax;
        delta_old = 0;
    }

    currPDADC = (rddata >> 1) & 0xff;

    if ( currPDADC && initPDADC ) { // both values should be non-zero
		if(slope == 0) { // Divide-by-0 protection
			delta = 0;
		} else {
			delta = ((A_INT32)(currPDADC - initPDADC)*4/slope);
		}
        }

#if (0)
    printf("slope = %d, calPDADC = %d, initPDADC=%d, currPDADC=%d, delta=%d, delta_old=%d, originalGain=0x%x, currgain=0x%x\n",
           slope, calPDADC, initPDADC, currPDADC, delta, delta_old, originalGain[18], (REGR(devNum, 0xa300 + 18 * 4) >> 12) & 0x7F);
#endif      

		// set bb_ch0_olpc_temp_compensation 
		MyRegisterRead(0xa398,&temp);
		temp = temp & 0xFFFF03FF;
		temp = temp | ((delta & 0x3f) << 10);
		MyRegisterWrite(0xa398, temp);
		// set bb_ch1_olpc_temp_compensation 
		MyRegisterRead(0xb398,&temp);
		temp = temp & 0xFFFF03FF;
		temp = temp | ((delta & 0x3f) << 10);
		MyRegisterWrite(0xb398, temp);
//		printf("REG 0xa398 is 0x%x\n", REGR(devNum, 0xa398));
//		printf("REG 0xb398 is 0x%x\n", REGR(devNum, 0xb398));

/*

               MyRegisterWrite (0x9800,0x10000007);
MyRegisterWrite (0x984c,0x170233c);

               MyRegisterWrite (0x9c0c,0xb991);
MyRegisterWrite (0x9c3c,0xff);

               MyRegisterWrite (0xa234,0x20202020);
MyRegisterWrite (0xa238,0x20202020);


               MyRegisterWrite (0xa2b4,0x20202020);
MyRegisterWrite (0xa2b8,0x20202020);
               MyRegisterWrite (0xa2d8,0x6cc11381);
MyRegisterWrite (0xa398,0xddce);

*/
		return 0;
}
