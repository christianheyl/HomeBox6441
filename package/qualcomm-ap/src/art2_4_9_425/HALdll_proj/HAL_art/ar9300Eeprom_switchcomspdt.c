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

#include "AquilaNewmaMapping.h"
#include "Ar9300EepromParameter.h"
#include "ar9300EepromPrint.h"

#include "ParameterConfigDef.h"

#define MBUFFER 1024

int Ar9300Eeprom_switchcomspdtGet(ar9300_eeprom_t *ahp_Eeprom, int iBand)
{
	int  value=0;

	if (iBand==band_BG) {
		value = ahp_Eeprom->modal_header_2g.switchcomspdt;
	} else {
		value = ahp_Eeprom->modal_header_5g.switchcomspdt;
	}	
    return value; 
}

int Ar9300Eeprom_switchcomspdtSet(ar9300_eeprom_t *ahp_Eeprom, int value, int iBand)
{
	if (iBand==band_BG) {
		ahp_Eeprom->modal_header_2g.switchcomspdt = (u_int16_t)value;
	} else {
		ahp_Eeprom->modal_header_5g.switchcomspdt = (u_int16_t)value;
	}
	return 0;
}

static _EepromPrintStruct _Ar9300EepromList_switchcomspdt[]=
{
	{Ar9300Eeprom2GHzWlanSpdtSwitchGlobalControl,offsetof(ar9300_eeprom_t,modal_header_2g.switchcomspdt),sizeof(u_int16_t),1,1,1,'x',1,-1,-1,0},
	{Ar9300Eeprom5GHzWlanSpdtSwitchGlobalControl,offsetof(ar9300_eeprom_t,modal_header_5g.switchcomspdt),sizeof(u_int16_t),1,1,1,'x',1,-1,-1,0},
};

int get_num_switchcomspdt()
{
	return 1;
}

void get_switchcomspdt(char *itemName, char *itemValue, int itemIndex, const ar9300_eeprom_t *ahp_Eeprom, int iBand)
{
	sprintf(itemName, "switchcomspdt");
	if (iBand==band_BG) {
		sprintf(itemValue, "%4x", ahp_Eeprom->modal_header_2g.switchcomspdt);
	} else {
		sprintf(itemValue, "%4x", ahp_Eeprom->modal_header_5g.switchcomspdt);
	}
}

void Ar9300EepromDifferenceAnalyze_switchcomspdt(void (*print)(char *format, ...), ar9300_eeprom_t *mptr, int mcount, int all)
{
	int nt;
	nt=sizeof(_Ar9300EepromList_switchcomspdt)/sizeof(_Ar9300EepromList_switchcomspdt[0]);
	Ar9300EepromDifferenceAnalyze_List(print, mptr, mcount, all, _Ar9300EepromList_switchcomspdt, nt, 0);
}
