#ifndef __INCConfigDiffh
#define __INCConfigDiffh

#define CONFIG_DIFF_MIN_APART       8
#define CONFIG_DIFF_MAX_BUFFER      255-8
#define CONFIG_DIFF_MAX_SIZE        CONFIG_DIFF_MAX_BUFFER-4

typedef struct _ConfigDiff
{
	A_UINT16 offset;
	A_UINT8  size;
	A_UINT8  *pData;
	struct _ConfigDiff *next;
	struct _ConfigDiff *prev;
} CONFIG_DIFF;

//
// initialize the list
//
void ConfigDiffInit();

//
// execute all of the diff config register and field writes
//
extern int ConfigDiffExecute();

//
// clear the list of diff config registers
//
extern int ConfigDiffClearAll();

//
// find a diff config register on the list
//
extern struct _ConfigDiff *ConfigDiffFind(A_UINT16 offset);

//
// clear one entry from the list
//
extern int ConfigDiffClear(A_UINT16 offset);

//
// change the value of a diff config register, create it if it does not exist
//
extern int ConfigDiffChange(A_UINT16 offset, A_UINT8 size, A_UINT8 *pData);

//
// add a diff config entry and value to the list
//
extern int ConfigDiffAdd(A_UINT16 offset, A_UINT8 size, A_UINT8 *pData);

//
// Create diff config list between 2 area location
// The list will contains the values of the pEeprom1
//
extern int ConfigDiffCreateDiffList (A_UINT8 *pArea1, A_UINT8 *pArea2, A_UINT32 numBytes); 

//
// initialize the cal info list
//
extern void CalInfoInit();

//
// add/change a cal info entry to the list
//
extern int CalInfoChange(A_UINT16 offset, A_UINT8 size, A_UINT8 *pData);

//
// execute cal info list
//
extern int CalInfoExecute();

//
// clear cal list
//
extern int CalInfoClearAll();


#endif //__INCConfigDiffh