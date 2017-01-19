#ifndef _QC9K_EEPROM_PRINT_H_
#define _QC9K_EEPROM_PRINT_H_

#include "Qc9KEeprom.h"

typedef struct _EepromPrint
{
	char *name;
	short offset;
	short size;
	short nx,ny,nz;
	char type;	// f -- floating point
				// d -- decimal
				// x -- hexadecimal
				// u -- unsigned
				// c -- character
				// t -- text
				// p -- transmit power, 0.5*value as floating point 
				// 2 -- compressed 2GHz frequency
				// 5 -- compressed 5GHz frequency
				// m -- mac address
	int interleave;
	int high;
	int low;
	int voff;
} _EepromPrintStruct;

extern void Qc9KEepromPrintEntry(void (*print)(char *format, ...), 
	char *name, int offset, int size, int high, int low, int voff,
	int nx, int ny, int nz, int interleave,int interleave2, int interleave3,
	char type, QC98XX_EEPROM *mptr, int mcount, int all);

extern void Qc9KEepromDifferenceAnalyze_List(void (*print)(char *format, ...), QC98XX_EEPROM *mptr, int mcount, int all,
										_EepromPrintStruct *_EepromList, int nt, int checkUnknown);

extern void Qc9KEepromDifferenceAnalyze(void (*print)(char *format, ...), QC98XX_EEPROM *mptr, int mcount, int all);

#endif //_QC9K_EEPROM_PRINT_H_
