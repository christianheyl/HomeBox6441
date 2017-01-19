#ifdef _WINDOWS
#ifdef ANWIDLL
		#define ANWIDLLSPEC __declspec(dllexport)
	#else
		#define ANWIDLLSPEC __declspec(dllimport)
	#endif
#else
	#define ANWIDLLSPEC
#include "ah_osdep.h"
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,2,0)
typedef unsigned int        uintptr_t;
#endif
#endif


extern ANWIDLLSPEC int AnwiDriverDetach(void);

//
// Grab the device id from the pci bus
// is there anything else we should check, like an Atheros id?
// how do we get this for other bus types?
//
extern ANWIDLLSPEC int AnwiDriverDeviceIdGet(void);

	
extern ANWIDLLSPEC int AnwiDriverAttach(int devid);

extern ANWIDLLSPEC uintptr_t AnwiDriverMemoryMap(void);


extern ANWIDLLSPEC int AnwiDriverMemorySize(void);


extern ANWIDLLSPEC uintptr_t AnwiDriverRegisterMap(void);

//
// returns 1 if the Anwi driver is successfully loaded
//
extern ANWIDLLSPEC int AnwiDriverValid(void);


extern ANWIDLLSPEC void MyRegisterDebug(int state);


extern ANWIDLLSPEC int MyRegisterRead(unsigned int address, unsigned int *value);


extern ANWIDLLSPEC int MyRegisterWrite(unsigned int address, unsigned int value);


extern ANWIDLLSPEC unsigned int MaskCreate(int low, int high);


extern ANWIDLLSPEC int MyFieldRead(unsigned int address, int low, int high, unsigned int *value);


extern ANWIDLLSPEC int MyFieldWrite(unsigned int address, int low, int high, unsigned int value);


extern ANWIDLLSPEC uintptr_t MyMemoryBase(void);


extern ANWIDLLSPEC unsigned char *MyMemoryPtr(unsigned int address);


extern ANWIDLLSPEC int MyMemoryRead(unsigned int address, unsigned int *buffer, int many);


extern ANWIDLLSPEC int MyMemoryWrite(unsigned int address, unsigned int *buffer, int many);

