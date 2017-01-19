#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

#include "wlantype.h"
#include "Card.h"
#include "rate_constants.h"
#include "Ar9287TargetPower.h"
//
// hal header files
//
#include "ah.h"
#include "ah_internal.h"
#include "ar5416/ar5416eep.h"
#include "ar5416/ar5416.h"

// 
// this is the hal pointer, 
// returned by ath_hal_attach
// used as the first argument by most (all?) HAL routines
//
struct ath_hal *AH;


//given the rate index as per rate_constant.c find and return the target power
int Ar9287TargetPowerGet(int freq, int rateIndex, double *powerOut) 
{
    //HAL_CHANNEL_INTERNAL chan;
    A_BOOL  is2GHz = 0;
    A_UINT8 powerT2 = 0;
    CAL_TARGET_POWER_LEG NewPower;
    CAL_TARGET_POWER_HT  targetPowerHt20, targetPowerHt40;
    struct ath_hal_5416 *ahp = AH5416(AH);
    //HAL_CHANNEL_INTERNAL *chan = AH_PRIVATE(AH)->ah_curchan;
    HAL_CHANNEL_INTERNAL curchan;
    HAL_CHANNEL_INTERNAL *chan = & curchan;

    //memset(&NewPower, 0x00, sizeof(CAL_TARGET_POWER_LEG));
    memset(&curchan, 0x00, sizeof(HAL_CHANNEL_INTERNAL));
    if(freq < 4000) {
        is2GHz = 1;
        chan->channel_flags |= CHANNEL_2GHZ;
    }
    chan->channel = freq;
    
    //call the relevant get target power file based on rateIndex.  
    //Rate index will be used to find the appropriate target power index
    switch (rateIndex) {
        //Legacy
        case RATE_INDEX_6:
        case RATE_INDEX_9:
        case RATE_INDEX_12:
        case RATE_INDEX_18:
        case RATE_INDEX_24:            
            ar5416GetTargetPowersLeg(AH, chan, ahp->ah_eeprom.map.mapAr9287.calTargetPower2G,
                                     AR5416_EEPDEF_NUM_2G_CCK_TARGET_POWERS, &NewPower, 4, AH_FALSE);
            powerT2 = NewPower.tPow2x[0];           
            break;

        case RATE_INDEX_36:            
            ar5416GetTargetPowersLeg(AH, chan, ahp->ah_eeprom.map.mapAr9287.calTargetPower2G,
                                     AR5416_EEPDEF_NUM_2G_CCK_TARGET_POWERS, &NewPower, 4, AH_FALSE);
            powerT2 = NewPower.tPow2x[1];
            break;
        case RATE_INDEX_48:            
            ar5416GetTargetPowersLeg(AH, chan, ahp->ah_eeprom.map.mapAr9287.calTargetPower2G,
                                     AR5416_EEPDEF_NUM_2G_CCK_TARGET_POWERS, &NewPower, 4, AH_FALSE);
            powerT2 = NewPower.tPow2x[2];
            break;
        case RATE_INDEX_54:           
            ar5416GetTargetPowersLeg(AH, chan, ahp->ah_eeprom.map.mapAr9287.calTargetPower2G,
                                     AR5416_EEPDEF_NUM_2G_CCK_TARGET_POWERS, &NewPower, 4, AH_FALSE);
            powerT2 = NewPower.tPow2x[3];
            break;

        //Legacy CCK
        case RATE_INDEX_1L:
            ar5416GetTargetPowersLeg(AH, chan, ahp->ah_eeprom.map.mapAr9287.calTargetPowerCck,
                                     AR5416_EEPDEF_NUM_2G_CCK_TARGET_POWERS, &NewPower, 4, AH_FALSE);
            powerT2 = NewPower.tPow2x[0];                         
            break;
        case RATE_INDEX_2L:            
        case RATE_INDEX_2S:
            ar5416GetTargetPowersLeg(AH, chan, ahp->ah_eeprom.map.mapAr9287.calTargetPowerCck,
                                     AR5416_EEPDEF_NUM_2G_CCK_TARGET_POWERS, &NewPower, 4, AH_FALSE);
            powerT2 = NewPower.tPow2x[1];
            break;
        case RATE_INDEX_5L:            
        case RATE_INDEX_5S:
            ar5416GetTargetPowersLeg(AH, chan, ahp->ah_eeprom.map.mapAr9287.calTargetPowerCck,
                                     AR5416_EEPDEF_NUM_2G_CCK_TARGET_POWERS, &NewPower, 4, AH_FALSE);
            powerT2 = NewPower.tPow2x[2];
            break;
        case RATE_INDEX_11L:            
        case RATE_INDEX_11S:
            ar5416GetTargetPowersLeg(AH, chan, ahp->ah_eeprom.map.mapAr9287.calTargetPowerCck,
                                     AR5416_EEPDEF_NUM_2G_CCK_TARGET_POWERS, &NewPower, 4, AH_FALSE);
            powerT2 = NewPower.tPow2x[3];
            break;
       
        //HT20
        case RATE_INDEX_HT20_MCS0:
        case RATE_INDEX_HT20_MCS8:    
            ar5416GetTargetPowers(AH, chan, ahp->ah_eeprom.map.mapAr9287.calTargetPower2GHT20,
                                  AR9287_NUM_2G_20_TARGET_POWERS, &targetPowerHt20, 8, AH_FALSE);
            powerT2 = targetPowerHt20.tPow2x[0];
            break;
        case RATE_INDEX_HT20_MCS1:
        case RATE_INDEX_HT20_MCS9:
            ar5416GetTargetPowers(AH, chan, ahp->ah_eeprom.map.mapAr9287.calTargetPower2GHT20,
                                  AR9287_NUM_2G_20_TARGET_POWERS, &targetPowerHt20, 8, AH_FALSE);
            powerT2 = targetPowerHt20.tPow2x[1];
            break;
        case RATE_INDEX_HT20_MCS2:
        case RATE_INDEX_HT20_MCS10:
            ar5416GetTargetPowers(AH, chan, ahp->ah_eeprom.map.mapAr9287.calTargetPower2GHT20,
                                  AR9287_NUM_2G_20_TARGET_POWERS, &targetPowerHt20, 8, AH_FALSE);
            powerT2 = targetPowerHt20.tPow2x[2];
            break;
        case RATE_INDEX_HT20_MCS3:
        case RATE_INDEX_HT20_MCS11:
            ar5416GetTargetPowers(AH, chan, ahp->ah_eeprom.map.mapAr9287.calTargetPower2GHT20,
                                  AR9287_NUM_2G_20_TARGET_POWERS, &targetPowerHt20, 8, AH_FALSE);
            powerT2 = targetPowerHt20.tPow2x[3];
            break;
        case RATE_INDEX_HT20_MCS4:
        case RATE_INDEX_HT20_MCS12: 
            ar5416GetTargetPowers(AH, chan, ahp->ah_eeprom.map.mapAr9287.calTargetPower2GHT20,
                                  AR9287_NUM_2G_20_TARGET_POWERS, &targetPowerHt20, 8, AH_FALSE);
            powerT2 = targetPowerHt20.tPow2x[4];
            break;
        case RATE_INDEX_HT20_MCS5:
        case RATE_INDEX_HT20_MCS13:
            ar5416GetTargetPowers(AH, chan, ahp->ah_eeprom.map.mapAr9287.calTargetPower2GHT20,
                                  AR9287_NUM_2G_20_TARGET_POWERS, &targetPowerHt20, 8, AH_FALSE);
            powerT2 = targetPowerHt20.tPow2x[5];
            break;
        case RATE_INDEX_HT20_MCS6:
        case RATE_INDEX_HT20_MCS14:
            ar5416GetTargetPowers(AH, chan, ahp->ah_eeprom.map.mapAr9287.calTargetPower2GHT20,
                                  AR9287_NUM_2G_20_TARGET_POWERS, &targetPowerHt20, 8, AH_FALSE);
            powerT2 = targetPowerHt20.tPow2x[6];
            break;
        case RATE_INDEX_HT20_MCS7:
        case RATE_INDEX_HT20_MCS15:
            ar5416GetTargetPowers(AH, chan, ahp->ah_eeprom.map.mapAr9287.calTargetPower2GHT20,
                                  AR9287_NUM_2G_20_TARGET_POWERS, &targetPowerHt20, 8, AH_FALSE);
            powerT2 = targetPowerHt20.tPow2x[7];
            break;
                 
        //HT40
        case RATE_INDEX_HT40_MCS0:
        case RATE_INDEX_HT40_MCS8:
            ar5416GetTargetPowers(AH, chan, ahp->ah_eeprom.map.mapAr9287.calTargetPower2GHT40,
                                  AR9287_NUM_2G_40_TARGET_POWERS, &targetPowerHt40, 8, AH_TRUE);
            powerT2 = targetPowerHt40.tPow2x[0];
            break;
        case RATE_INDEX_HT40_MCS1:
        case RATE_INDEX_HT40_MCS9:
            ar5416GetTargetPowers(AH, chan, ahp->ah_eeprom.map.mapAr9287.calTargetPower2GHT40,
                                  AR9287_NUM_2G_40_TARGET_POWERS, &targetPowerHt40, 8, AH_TRUE);
            powerT2 = targetPowerHt40.tPow2x[1];
            break;
        case RATE_INDEX_HT40_MCS2:
        case RATE_INDEX_HT40_MCS10:
            ar5416GetTargetPowers(AH, chan, ahp->ah_eeprom.map.mapAr9287.calTargetPower2GHT40,
                                  AR9287_NUM_2G_40_TARGET_POWERS, &targetPowerHt40, 8, AH_TRUE);
            powerT2 = targetPowerHt40.tPow2x[2];
            break;
        case RATE_INDEX_HT40_MCS3:
        case RATE_INDEX_HT40_MCS11:
            ar5416GetTargetPowers(AH, chan, ahp->ah_eeprom.map.mapAr9287.calTargetPower2GHT40,
                                  AR9287_NUM_2G_40_TARGET_POWERS, &targetPowerHt40, 8, AH_TRUE);
            powerT2 = targetPowerHt40.tPow2x[3];
            break;
        case RATE_INDEX_HT40_MCS4:
        case RATE_INDEX_HT40_MCS12:
            ar5416GetTargetPowers(AH, chan, ahp->ah_eeprom.map.mapAr9287.calTargetPower2GHT40,
                                  AR9287_NUM_2G_40_TARGET_POWERS, &targetPowerHt40, 8, AH_TRUE);
            powerT2 = targetPowerHt40.tPow2x[4];
            break;
        case RATE_INDEX_HT40_MCS5:
        case RATE_INDEX_HT40_MCS13:
            ar5416GetTargetPowers(AH, chan, ahp->ah_eeprom.map.mapAr9287.calTargetPower2GHT40,
                                  AR9287_NUM_2G_40_TARGET_POWERS, &targetPowerHt40, 8, AH_TRUE);
            powerT2 = targetPowerHt40.tPow2x[5];
            break;
        case RATE_INDEX_HT40_MCS6:
        case RATE_INDEX_HT40_MCS14:
            ar5416GetTargetPowers(AH, chan, ahp->ah_eeprom.map.mapAr9287.calTargetPower2GHT40,
                                  AR9287_NUM_2G_40_TARGET_POWERS, &targetPowerHt40, 8, AH_TRUE);
            powerT2 = targetPowerHt40.tPow2x[6];
            break;
        case RATE_INDEX_HT40_MCS7:
        case RATE_INDEX_HT40_MCS15:
            ar5416GetTargetPowers(AH, chan, ahp->ah_eeprom.map.mapAr9287.calTargetPower2GHT40,
                                  AR9287_NUM_2G_40_TARGET_POWERS, &targetPowerHt40, 8, AH_TRUE);
            powerT2 = targetPowerHt40.tPow2x[7];
            break;
    }
    *powerOut = ((double)powerT2)/2;
    return 0;
}
