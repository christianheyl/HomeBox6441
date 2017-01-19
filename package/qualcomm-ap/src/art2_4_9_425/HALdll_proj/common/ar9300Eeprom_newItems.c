
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

#include "ar9300Eeprom_switchcomspdt.h"
#include "ar9300Eeprom_xLNA.h"
#include "ar9300Eeprom_tempslopextension.h"
#include "ar9300Eeprom_rfGainCap.h"
#include "ar9300Eeprom_txGainCap.h"

#define MBUFFER 1024
#define TOTAL_SPACE 35

char  buffer[MBUFFER];
int nc=0, lc=0;

void print9300_lineEnd(int emptyRight)
{
	int i=0;
	// fill the space for right side of the line
	if (emptyRight==1) {
		nc = SformatOutput(&buffer[lc],MBUFFER-lc-1,"|");
		lc+=nc;
		for (i=0; i<TOTAL_SPACE; i++) {
			nc = SformatOutput(&buffer[lc],MBUFFER-lc-1," ");
			lc+=nc;
		}
	} 
	nc = SformatOutput(&buffer[lc],MBUFFER-lc-1,"|\n");
	ErrorPrint(NartOther, buffer);
	lc=0;
}

void print9300_half_Line(char *eepItemName, char *eepItemValue, int isLeft)
{
	int spaceNumb=1; 
	int eepItemNameSize = strlen(eepItemName);
	int eepItemValueSize = strlen(eepItemValue);
	int i=0;
	spaceNumb = TOTAL_SPACE -1 -2 -eepItemNameSize - eepItemValueSize;

	if (isLeft)
		nc = SformatOutput(&buffer[lc],MBUFFER-lc-1," | %s", eepItemName);
	else {
		nc = SformatOutput(&buffer[lc],MBUFFER-lc-1,"|  %s", eepItemName);
		spaceNumb--;
	}
	lc+=nc;
	// fill the space between the name and value
	for (i=0; i<spaceNumb; i++) {
		nc = SformatOutput(&buffer[lc],MBUFFER-lc-1," ");
		lc+=nc;
	}
    nc = SformatOutput(&buffer[lc],MBUFFER-lc-1,"%s  ", eepItemValue);
	lc+=nc;
}

void print9300_whole_Line(char *eepItemName, char *eepItemValue)
{
	int spaceNumb=1; 
	int eepItemNameSize = strlen(eepItemName);
	int eepItemValueSize = strlen(eepItemValue);
	int i=0;
	spaceNumb = TOTAL_SPACE*2 -eepItemNameSize - eepItemValueSize;

	nc = SformatOutput(&buffer[lc],MBUFFER-lc-1," | %s", eepItemName);
		
	lc+=nc;
    nc = SformatOutput(&buffer[lc],MBUFFER-lc-1,"%s", eepItemValue);
	lc+=nc;
	// fill the space 
	for (i=0; i<spaceNumb; i++) {
		nc = SformatOutput(&buffer[lc],MBUFFER-lc-1," ");
		lc+=nc;
	}
	nc = SformatOutput(&buffer[lc],MBUFFER-lc-1,"|\n");
	ErrorPrint(NartOther, buffer);
}

void print9300_Header_newItems(int client, const ar9300_eeprom_t *ahp_Eeprom, int iBand)
{
	char eepItemName[100], eepItemValue[100];
	int itemNum;
	int emptyRight = 1;
	
	// print for ar9300Eeprom_switchcomspdt
	lc=0;
	itemNum = get_num_switchcomspdt();
	if (itemNum>0) {
		get_switchcomspdt(eepItemName, eepItemValue, itemNum-1, ahp_Eeprom, iBand);
		print9300_half_Line(eepItemName, eepItemValue, 1);
//		print9300_lineEnd(emptyRight);
	//	for second item	
//		print9300_half_Line(eepItemName, eepItemValue, 0);
//		print9300_lineEnd(0);
	}

	itemNum = get_num_xLNABiasStrength();
	if (itemNum>0) {
		get_xLNABiasStrength(eepItemName, eepItemValue, itemNum-1, ahp_Eeprom, iBand);
		print9300_half_Line(eepItemName, eepItemValue, 0);
		print9300_lineEnd(0);
	}

	itemNum = get_num_rfGainCap();
	if (itemNum>0) {
		get_rfGainCap(eepItemName, eepItemValue, itemNum-1, ahp_Eeprom, iBand);
		print9300_half_Line(eepItemName, eepItemValue, 1);
	}

	itemNum = get_num_txGainCap();
	if (itemNum>0) {
		get_txGainCap(eepItemName, eepItemValue, itemNum-1, ahp_Eeprom, iBand);
		print9300_half_Line(eepItemName, eepItemValue, 0);
		print9300_lineEnd(0);
	}

	if(iBand == band_A){
		// print for ar9300Eeprom_tempslopextension
		lc=0;
		itemNum = get_num_tempslopextension();
		if (itemNum>0) {
			get_tempslopextension(eepItemName, eepItemValue, itemNum-1, ahp_Eeprom);
			print9300_whole_Line(eepItemName, eepItemValue);
		}
	}


	// print a space line at the end of header print
	SformatOutput(buffer, MBUFFER-1," |                                   |                                   |");
	ErrorPrint(NartOther,buffer);
} 

void Ar9300EepromDifferenceAnalyze_newItems(void (*print)(char *format, ...), ar9300_eeprom_t *mptr, int mcount, int all)
{
	Ar9300EepromDifferenceAnalyze_switchcomspdt(print, mptr, mcount, all);
	Ar9300EepromDifferenceAnalyze_xLNABiasStrength(print, mptr, mcount, all);
	Ar9300EepromDifferenceAnalyze_tempslopextension(print, mptr, mcount, all);
	Ar9300EepromDifferenceAnalyze_rfGainCap(print, mptr, mcount, all);
	Ar9300EepromDifferenceAnalyze_txGainCap(print, mptr, mcount, all);
}
