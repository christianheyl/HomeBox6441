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

int Ar9300Eeprom_xLNABiasStrengthGet(ar9300_eeprom_t *ahp_Eeprom, int iBand)
{
	int  value=0;

	if (iBand==band_BG) {
		value = ahp_Eeprom->modal_header_2g.xLNA_bias_strength;
	} else {
		value = ahp_Eeprom->modal_header_5g.xLNA_bias_strength;
	}	
    return value; 
}

int Ar9300Eeprom_xLNABiasStrengthSet(ar9300_eeprom_t *ahp_Eeprom, int value, int iBand, int ix, int num)
{
	u_int8_t  value2;
	// bit0,1 for chain0, bit2,3 for chain1, bit4,5 for chain2
	if (ix<0 || ix>=OSPREY_MAX_CHAINS)
		value2 = (u_int8_t)(value & 0x3f) ;
	else {
		if (iBand==band_BG) 
			value2 = ahp_Eeprom->modal_header_2g.xLNA_bias_strength;
		else
			value2 = ahp_Eeprom->modal_header_5g.xLNA_bias_strength;
		// only change for chain ix.
		value2= (value2 & (~((0x03)<<(ix*2)))) | (value <<(ix*2));
	}

	if (iBand==band_BG) {
		ahp_Eeprom->modal_header_2g.xLNA_bias_strength = (u_int8_t)value2;
	} else {
		ahp_Eeprom->modal_header_5g.xLNA_bias_strength = (u_int8_t)value2;
	}
	return 0;
}

static _EepromPrintStruct _Ar9300EepromList_xLNABiasStrength[]=
{
	{Ar9300Eeprom2GHzXLNABiasStrength,offsetof(ar9300_eeprom_t,modal_header_2g.xLNA_bias_strength),sizeof(u_int8_t),1,1,1,'x',1,-1,-1,0},
	{Ar9300Eeprom5GHzXLNABiasStrength,offsetof(ar9300_eeprom_t,modal_header_5g.xLNA_bias_strength),sizeof(u_int8_t),1,1,1,'x',1,-1,-1,0},
};

int get_num_xLNABiasStrength()
{
	return 1;
}

void get_xLNABiasStrength(char *itemName, char *itemValue, int itemIndex, const ar9300_eeprom_t *ahp_Eeprom, int iBand)
{
	sprintf(itemName, "xLNA_bias_strength");
	if (iBand==band_BG) {
		sprintf(itemValue, "%2x", ahp_Eeprom->modal_header_2g.xLNA_bias_strength);
	} else {
		sprintf(itemValue, "%2x", ahp_Eeprom->modal_header_5g.xLNA_bias_strength);
	}
}

void Ar9300EepromDifferenceAnalyze_xLNABiasStrength(void (*print)(char *format, ...), ar9300_eeprom_t *mptr, int mcount, int all)
{
	int nt;
	nt=sizeof(_Ar9300EepromList_xLNABiasStrength)/sizeof(_Ar9300EepromList_xLNABiasStrength[0]);
	Ar9300EepromDifferenceAnalyze_List(print, mptr, mcount, all, _Ar9300EepromList_xLNABiasStrength, nt, 0);
}

