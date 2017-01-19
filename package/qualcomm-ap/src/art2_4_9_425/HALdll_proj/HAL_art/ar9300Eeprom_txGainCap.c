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

#include "Ar9300EepromParameter.h"
#include "ar9300EepromPrint.h"

#include "ParameterConfigDef.h"

#define MBUFFER 1024

int Ar9300Eeprom_txGainCapGet(ar9300_eeprom_t *ahp_Eeprom, int iBand)
{
	int  value=0;
    return value; 
}

int Ar9300Eeprom_txGainCapSet(ar9300_eeprom_t *ahp_Eeprom, int value, int iBand)
{
	return 0;
}

int Ar9300Eeprom_txGainCapEnableGet(ar9300_eeprom_t *ahp_Eeprom)
{
	int  value=0;
    return value; 
}

int Ar9300Eeprom_txGainCapEnableSet(ar9300_eeprom_t *ahp_Eeprom, int value)
{
    return 0;
}

int Ar9300Eeprom_CalibrationTxgainCAPSet(struct ath_hal *AH, int *txgainmax)
{
	return 0;
}

int get_num_txGainCap()
{
	return 0;
}

void get_txGainCap(char *itemName, char *itemValue, int itemIndex, const ar9300_eeprom_t *ahp_Eeprom, int iBand)
{
	return;
}

void Ar9300EepromDifferenceAnalyze_txGainCap(void (*print)(char *format, ...), ar9300_eeprom_t *mptr, int mcount, int all)
{
	return;
}

