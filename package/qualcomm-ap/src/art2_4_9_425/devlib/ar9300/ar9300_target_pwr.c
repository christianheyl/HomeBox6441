#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

#include "AquilaNewmaMapping.h"

#include "wlantype.h"
#include "rate_constants.h"
#include "mEepStruct9300.h"
#include "ar9300_target_pwr.h"
#include "Ar9300Device.h"

//
// hal header files
//
#include "ah.h"
#include "ah_internal.h"
#include "ar9300eep.h"



//given the rate index as per rate_constant.c find and return the target power
AR9300DLLSPEC int Ar9300TargetPowerGet(int freq, int rateIndex, double *powerOut) 
{
    A_BOOL  is2GHz = 0;
    A_UINT8 powerT2 = 0;

    if(freq < 4000) {
        is2GHz = 1;
    }
    //call the relevant get target power file based on rateIndex.  
    //Rate index will be used to find the appropriate target power index
    switch (rateIndex) {
        //Legacy
        case RATE_INDEX_6:
        case RATE_INDEX_9:
        case RATE_INDEX_12:
        case RATE_INDEX_18:
        case RATE_INDEX_24:
            powerT2 = ar9300_eeprom_get_legacy_trgt_pwr(AH, LEGACY_TARGET_RATE_6_24, freq, is2GHz);            
            break;
        case RATE_INDEX_36:
            powerT2 = ar9300_eeprom_get_legacy_trgt_pwr(AH, LEGACY_TARGET_RATE_36, freq, is2GHz);            
            break;
        case RATE_INDEX_48:
            powerT2 = ar9300_eeprom_get_legacy_trgt_pwr(AH, LEGACY_TARGET_RATE_48, freq, is2GHz);            
            break;
        case RATE_INDEX_54:
            powerT2 = ar9300_eeprom_get_legacy_trgt_pwr(AH, LEGACY_TARGET_RATE_54, freq, is2GHz);            
            break;

        //Legacy CCK
        case RATE_INDEX_1L:
        case RATE_INDEX_2L:
        case RATE_INDEX_2S:
        case RATE_INDEX_5L:
            powerT2 = ar9300_eeprom_get_cck_trgt_pwr(AH, LEGACY_TARGET_RATE_1L_5L, freq);
            break;
        case RATE_INDEX_5S:
            powerT2 = ar9300_eeprom_get_cck_trgt_pwr(AH, LEGACY_TARGET_RATE_5S, freq);
            break;
        case RATE_INDEX_11L:
            powerT2 = ar9300_eeprom_get_cck_trgt_pwr(AH, LEGACY_TARGET_RATE_11L, freq);
            break;
        case RATE_INDEX_11S:
            powerT2 = ar9300_eeprom_get_cck_trgt_pwr(AH, LEGACY_TARGET_RATE_11S, freq);
            break;
        
        //HT20
        case RATE_INDEX_HT20_MCS0:
        case RATE_INDEX_HT20_MCS8:
        case RATE_INDEX_HT20_MCS16:
            powerT2 = ar9300_eeprom_get_ht20_trgt_pwr(AH, HT_TARGET_RATE_0_8_16, freq, is2GHz);
            break;
        case RATE_INDEX_HT20_MCS1:
        case RATE_INDEX_HT20_MCS2:
        case RATE_INDEX_HT20_MCS3:
        case RATE_INDEX_HT20_MCS9:
        case RATE_INDEX_HT20_MCS10:
        case RATE_INDEX_HT20_MCS11:
        case RATE_INDEX_HT20_MCS17:
        case RATE_INDEX_HT20_MCS18:
        case RATE_INDEX_HT20_MCS19:
            powerT2 = ar9300_eeprom_get_ht20_trgt_pwr(AH, HT_TARGET_RATE_1_3_9_11_17_19, freq, is2GHz);     
            break;
        case RATE_INDEX_HT20_MCS4:
            powerT2 = ar9300_eeprom_get_ht20_trgt_pwr(AH, HT_TARGET_RATE_4, freq, is2GHz);
            break;
        case RATE_INDEX_HT20_MCS5:
            powerT2 = ar9300_eeprom_get_ht20_trgt_pwr(AH, HT_TARGET_RATE_5, freq, is2GHz);
            break;
        case RATE_INDEX_HT20_MCS6:
            powerT2 = ar9300_eeprom_get_ht20_trgt_pwr(AH, HT_TARGET_RATE_6, freq, is2GHz);
            break;
        case RATE_INDEX_HT20_MCS7:
            powerT2 = ar9300_eeprom_get_ht20_trgt_pwr(AH, HT_TARGET_RATE_7, freq, is2GHz);
            break;
        case RATE_INDEX_HT20_MCS12:
            powerT2 = ar9300_eeprom_get_ht20_trgt_pwr(AH, HT_TARGET_RATE_12, freq, is2GHz);
            break;
        case RATE_INDEX_HT20_MCS13:
            powerT2 = ar9300_eeprom_get_ht20_trgt_pwr(AH, HT_TARGET_RATE_13, freq, is2GHz);
            break;
        case RATE_INDEX_HT20_MCS14:
            powerT2 = ar9300_eeprom_get_ht20_trgt_pwr(AH, HT_TARGET_RATE_14, freq, is2GHz);
            break;
        case RATE_INDEX_HT20_MCS15:
            powerT2 = ar9300_eeprom_get_ht20_trgt_pwr(AH, HT_TARGET_RATE_15, freq, is2GHz);
            break;
        case RATE_INDEX_HT20_MCS20:
            powerT2 = ar9300_eeprom_get_ht20_trgt_pwr(AH, HT_TARGET_RATE_20, freq, is2GHz);
            break;
        case RATE_INDEX_HT20_MCS21:
            powerT2 = ar9300_eeprom_get_ht20_trgt_pwr(AH, HT_TARGET_RATE_21, freq, is2GHz);
            break;
        case RATE_INDEX_HT20_MCS22:
            powerT2 = ar9300_eeprom_get_ht20_trgt_pwr(AH, HT_TARGET_RATE_22, freq, is2GHz);
            break;
        case RATE_INDEX_HT20_MCS23:
            powerT2 = ar9300_eeprom_get_ht20_trgt_pwr(AH, HT_TARGET_RATE_23, freq, is2GHz);
            break;
        
        //HT40
        case RATE_INDEX_HT40_MCS0:
        case RATE_INDEX_HT40_MCS8:
        case RATE_INDEX_HT40_MCS16:
            powerT2 = ar9300_eeprom_get_ht40_trgt_pwr(AH, HT_TARGET_RATE_0_8_16, freq, is2GHz);
            break;
        case RATE_INDEX_HT40_MCS1:
        case RATE_INDEX_HT40_MCS2:
        case RATE_INDEX_HT40_MCS3:
        case RATE_INDEX_HT40_MCS9:
        case RATE_INDEX_HT40_MCS10:
        case RATE_INDEX_HT40_MCS11:
        case RATE_INDEX_HT40_MCS17:
        case RATE_INDEX_HT40_MCS18:
        case RATE_INDEX_HT40_MCS19:
            powerT2 = ar9300_eeprom_get_ht40_trgt_pwr(AH, HT_TARGET_RATE_1_3_9_11_17_19, freq, is2GHz);
            break;
        case RATE_INDEX_HT40_MCS4:
            powerT2 = ar9300_eeprom_get_ht40_trgt_pwr(AH, HT_TARGET_RATE_4, freq, is2GHz);
            break;
        case RATE_INDEX_HT40_MCS5:
            powerT2 = ar9300_eeprom_get_ht40_trgt_pwr(AH, HT_TARGET_RATE_5, freq, is2GHz);
            break;
        case RATE_INDEX_HT40_MCS6:
            powerT2 = ar9300_eeprom_get_ht40_trgt_pwr(AH, HT_TARGET_RATE_6, freq, is2GHz);
            break;
        case RATE_INDEX_HT40_MCS7:
            powerT2 = ar9300_eeprom_get_ht40_trgt_pwr(AH, HT_TARGET_RATE_7, freq, is2GHz);
            break;
        case RATE_INDEX_HT40_MCS12:
            powerT2 = ar9300_eeprom_get_ht40_trgt_pwr(AH, HT_TARGET_RATE_12, freq, is2GHz);
            break;
        case RATE_INDEX_HT40_MCS13:
            powerT2 = ar9300_eeprom_get_ht40_trgt_pwr(AH, HT_TARGET_RATE_13, freq, is2GHz);
            break;
        case RATE_INDEX_HT40_MCS14:
            powerT2 = ar9300_eeprom_get_ht40_trgt_pwr(AH, HT_TARGET_RATE_14, freq, is2GHz);
            break;
        case RATE_INDEX_HT40_MCS15:
            powerT2 = ar9300_eeprom_get_ht40_trgt_pwr(AH, HT_TARGET_RATE_15, freq, is2GHz);
            break;
        case RATE_INDEX_HT40_MCS20:
            powerT2 = ar9300_eeprom_get_ht40_trgt_pwr(AH, HT_TARGET_RATE_20, freq, is2GHz);
            break;
        case RATE_INDEX_HT40_MCS21:
            powerT2 = ar9300_eeprom_get_ht40_trgt_pwr(AH, HT_TARGET_RATE_21, freq, is2GHz);
            break;
        case RATE_INDEX_HT40_MCS22:
            powerT2 = ar9300_eeprom_get_ht40_trgt_pwr(AH, HT_TARGET_RATE_22, freq, is2GHz);
            break;
        case RATE_INDEX_HT40_MCS23:
            powerT2 = ar9300_eeprom_get_ht40_trgt_pwr(AH, HT_TARGET_RATE_23, freq, is2GHz);
            break;
    }
    *powerOut = ((double)powerT2)/2;
    return 0;
}
