
#ifdef _WINDOWS
#ifdef FIELDDLL
		#define FIELDDLLSPEC __declspec(dllexport)
	#else
		#define FIELDDLLSPEC __declspec(dllimport)
	#endif
#else
	#define FIELDDLLSPEC
#endif

#define STICKY_FLAG_SENT_MASK		0x00000001		// 0 -> not sent; 1 -> sent
#define STICKY_FLAG_SENT_SHIFT		0
#define STICKY_FLAG_PREPOST_MASK	0x00000002		// 0 -> write sticky pre hw cal; 1 -> write sticky post hw cal
#define STICKY_FLAG_PREPOST_SHIFT	1
// default: not sent, write sticky post hw cal
#define STICKY_FLAG_DEFAULT			(~STICKY_FLAG_SENT_MASK	& STICKY_FLAG_PREPOST_MASK)

#define DEF_LINKLIST_IDX    0     // Maintain backward compatibility
#define ARN_LINKLIST_IDX    0     // after reset and no deletion on link-list, same as default
#define ARD_LINKLIST_IDX    1     // after reset and delete link-list
#define MRN_LINKLIST_IDX    2     // middle reset and no deletion on link-list
#define MRD_LINKLIST_IDX    3     // middle reset and delete link-list

struct _Sticky
{
	unsigned int address;
	unsigned int value[8];
	int low;
	int high;
    int flag;
    int numVal;
	struct _Sticky *next;
	struct _Sticky *prev;
};

//
// execute all of the sticky register and field writes
//
extern FIELDDLLSPEC int StickyExecute(int idx);
extern FIELDDLLSPEC int StickyExecuteDut(int idx);

//
// clear the list of sticky registers
//
extern FIELDDLLSPEC int StickyClear(int idx, int toDut);

//
// find a sticky register on the list
//
extern FIELDDLLSPEC struct _Sticky *StickyInternalFind(int idx,unsigned int address, int low, int high);

//
// clear one register from the list
//
extern FIELDDLLSPEC int StickyInternalClear(int idx,unsigned int address, int low, int high);

//
// change the value of a sticky register, create it if it does not exist
//
extern FIELDDLLSPEC int StickyInternalChange(int idx,unsigned int address, int low, int high, unsigned int value, int prepost);
extern FIELDDLLSPEC int StickyInternalChangeArray(int idx,unsigned int address, int low, int high, unsigned int *value, int numVal, int prepost);

//
// add a sticky register and value to the list
//
extern FIELDDLLSPEC int StickyInternalAdd(int idx,unsigned int address, int low, int high, unsigned int value, int prepost);
extern FIELDDLLSPEC int StickyInternalAddArray(int idx,unsigned int address, int low, int high, unsigned int *value, int numVal, int prepost);

//
// find a sticky register on the list
//
extern FIELDDLLSPEC struct _Sticky *StickyRegisterFind(int idx,unsigned int address);

//
// clear one register from the list
//
extern FIELDDLLSPEC int StickyRegisterClear(int idx,unsigned int address);

//
// add a sticky register and value to the list
//
extern FIELDDLLSPEC int StickyRegisterAdd(int idx,unsigned int address, unsigned int value);

//
// find a sticky register on the list
//
extern FIELDDLLSPEC struct _Sticky *StickyFieldFind(int idx,char *name);

//
// clear one field from the list
//
extern FIELDDLLSPEC int StickyFieldClear(int idx,char *name);

//
// add one sticky field and value to the list
//
extern FIELDDLLSPEC int StickyFieldAdd(int idx,char *name, unsigned int value);

//
// return the values of the first sticky thing on the list
// return value is 0 if successful, non zero if not
//
extern FIELDDLLSPEC int StickyHead(int idx,unsigned int *address, int *low, int *high, unsigned int *value, int *numVal);

//
// return the values of the next sticky thing on the list
// return value is 0 if successful, non zero if not
//
extern FIELDDLLSPEC int StickyNext(int idx,unsigned int *address, int *low, int *high, unsigned int *value, int *numVal);


extern FIELDDLLSPEC int StickyListToEeprom(int idx);





