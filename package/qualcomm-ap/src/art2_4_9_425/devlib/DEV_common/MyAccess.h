#ifndef	__INCMyAccessh
#define	__INCMyAccessh

#include "Device.h"

#ifdef _WINDOWS
#if (defined AR6KDLL) || (defined QC98XXDLL)
#define AR6KDLLSPEC __declspec(dllexport)
#else
#define AR6KDLLSPEC __declspec(dllimport)
#endif
#else
#define AR6KDLLSPEC
#endif


extern AR6KDLLSPEC int MyRegisterRead(unsigned int address, unsigned int *value);
extern AR6KDLLSPEC int MyRegisterWrite(unsigned int address, unsigned int value);
extern AR6KDLLSPEC unsigned int MaskCreate(int low, int high);
extern AR6KDLLSPEC int MyFieldRead(unsigned int address, int low, int high, unsigned int *value);
extern AR6KDLLSPEC int MyFieldWrite(unsigned int address, int low, int high, unsigned int value);
extern AR6KDLLSPEC int MyMemoryRead(unsigned int address, unsigned int *buffer, int many);
extern AR6KDLLSPEC int MyMemoryWrite(unsigned int address, unsigned int *buffer, int many);
//MANLIB_API unsigned int MyMemoryBase();
//MANLIB_API unsigned char *MyMemoryPtr(unsigned int address);


#endif //__INCMyAccessh
