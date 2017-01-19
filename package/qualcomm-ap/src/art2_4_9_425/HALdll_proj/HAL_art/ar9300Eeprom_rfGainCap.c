#include <string.h>
#include <math.h>
#include <ctype.h>

#include "wlantype.h"
#include "NewArt.h"
#include "smatch.h"
#include "ErrorPrint.h"
#include "NartError.h"

#include "ah.h"
#include "ah_internal.h"
#include "ar9300eep.h"
#include "ar9300.h"

#include "AquilaNewmaMapping.h"
#include "Ar9300EepromParameter.h"
#include "ar9300EepromPrint.h"

#include "ParameterConfigDef.h"

#define MBUFFER 1024

extern struct ath_hal *AH;

int Ar9300Eeprom_rfGainCapGet(ar9300_eeprom_t *ahp_Eeprom, int iBand)
{
	int  value=0;

	if (iBand==band_BG) {
		value = ahp_Eeprom->modal_header_2g.rf_gain_cap;
	} else {
		value = ahp_Eeprom->modal_header_5g.rf_gain_cap;
	}	
    return value;
}

int Ar9300Eeprom_rfGainCapSet(ar9300_eeprom_t *ahp_Eeprom, int value, int iBand)
{ 
	if (iBand==band_BG) {
		ahp_Eeprom->modal_header_2g.rf_gain_cap = (u_int8_t)value;
		ar9300_rf_gain_cap_apply(AH, 1);
	} else {
		ahp_Eeprom->modal_header_5g.rf_gain_cap = (u_int8_t)value;
		ar9300_rf_gain_cap_apply(AH, 0);
	}   
	return 0;
} 

static _EepromPrintStruct _Ar9300EepromList_rfGainCap[]=
{
	{Ar9300Eeprom2GHzRFGainCAP,offsetof(ar9300_eeprom_t,modal_header_2g.rf_gain_cap),sizeof(u_int8_t),1,1,1,'x',1,-1,-1,0},
	{Ar9300Eeprom5GHzRFGainCAP,offsetof(ar9300_eeprom_t,modal_header_5g.rf_gain_cap),sizeof(u_int8_t),1,1,1,'x',1,-1,-1,0},
};

int get_num_rfGainCap()
{
	return 1;
}

void get_rfGainCap(char *itemName, char *itemValue, int itemIndex, const ar9300_eeprom_t *ahp_Eeprom, int iBand)
{
	sprintf(itemName, "rf_gain_cap");
	if (iBand==band_BG) {
		sprintf(itemValue, "0x%2x", ahp_Eeprom->modal_header_2g.rf_gain_cap);
	} else {
		sprintf(itemValue, "0x%2x", ahp_Eeprom->modal_header_5g.rf_gain_cap);
	}
}

void Ar9300EepromDifferenceAnalyze_rfGainCap(void (*print)(char *format, ...), ar9300_eeprom_t *mptr, int mcount, int all)
{
	int nt;
	nt=sizeof(_Ar9300EepromList_rfGainCap)/sizeof(_Ar9300EepromList_rfGainCap[0]);
	Ar9300EepromDifferenceAnalyze_List(print, mptr, mcount, all, _Ar9300EepromList_rfGainCap, nt, 0);
}

