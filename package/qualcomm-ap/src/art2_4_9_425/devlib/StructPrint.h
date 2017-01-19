
#ifdef _WINDOWS
#ifdef FIELDDLL
		#define FIELDDLLSPEC __declspec(dllexport)
	#else
		#define FIELDDLLSPEC __declspec(dllimport)
	#endif
#else
	#define FIELDDLLSPEC
#endif


//
// This fun structure contains the names, sizes, and types for all of the fields in the eeprom structure.
// It exists so that we can easily compare two structures and report the differences.
// It must match the definition of ar9300_eeprom_t in ar9300eep.h.
// Fields must be defined in the same order as they occur in the structure.
//

struct _StructPrint
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
};


FIELDDLLSPEC extern int StructGet(struct _StructPrint *list,
	unsigned char *mptr, int msize, 
	unsigned int *value, int max, int ix, int iy, int iz);

FIELDDLLSPEC extern int StructSet(struct _StructPrint *list,
	unsigned char *mptr, int msize, 
	unsigned int *value, int max, int ix, int iy, int iz);

FIELDDLLSPEC extern void StructDifferenceAnalyze(void (*print)(char *format, ...), 
    struct _StructPrint *list, int nt,
	unsigned char *mptr[], int msize, int mcount, int all);
