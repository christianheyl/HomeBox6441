typedef struct _EepromPrint
{
	char *name;
	short offset;
	char size;
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

extern void Ar9300EepromPrintEntry(void (*print)(char *format, ...), 
	char *name, int offset, int size, int high, int low, int voff,
	int nx, int ny, int nz, int interleave,
	char type, ar9300_eeprom_t *mptr, int mcount, int all, int jw, int jx, int jy, int jz);

extern void Ar9300EepromDifferenceAnalyze_List(void (*print)(char *format, ...), ar9300_eeprom_t *mptr, int mcount, int all,
										_EepromPrintStruct *_EepromList, int nt, int checkUnknown);

extern void Ar9300EepromDifferenceAnalyze(void (*print)(char *format, ...), ar9300_eeprom_t *mptr, int mcount, int all);
