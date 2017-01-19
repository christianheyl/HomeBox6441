#include "ah.h"
#include "ah_internal.h"
#include "ar9300eep.h"

#include "ParameterConfigDef.h"

#include "ar9300Eeprom_tempslopextension.h"

int Ar9300Eeprom_tempslopeextensionGet(ar9300_eeprom_t *ahp_Eeprom, int *value, int ix, int *num)
{
    return 0; 
}


int Ar9300Eeprom_tempslopeextensionSet(ar9300_eeprom_t *ahp_Eeprom, int *value, int ix, int iy, int iz, int num, int iBand)
{
    return 0; 
}


int get_num_tempslopextension()
{
	return 0;
}

void get_tempslopextension(char *itemName, char *itemValue, int itemIndex, const ar9300_eeprom_t *ahp_Eeprom)
{
	return;
}

void Ar9300EepromDifferenceAnalyze_tempslopextension(void (*print)(char *format, ...), ar9300_eeprom_t *mptr, int mcount, int all)
{
	return;
}

