
#include "ah.h"

#ifdef _WINDOWS
#ifdef AR9287DLL
		#define AR9287DLLSPEC __declspec(dllexport)
	#else
		#define AR9287DLLSPEC __declspec(dllimport)
	#endif
#else
	#define AR9287DLLSPEC
#endif


//
// clear all device control function pointers and set to default behavior
//
extern AR9287DLLSPEC int Ar9287DeviceSelect(void);

extern AR9287DLLSPEC int Ar9287SetCommand(int client);

extern AR9287DLLSPEC int Ar9287GetCommand(int client);
