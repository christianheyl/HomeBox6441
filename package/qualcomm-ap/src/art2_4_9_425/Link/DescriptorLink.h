
#ifdef _WINDOWS
#ifdef LINKDLL
		#define LINKDLLSPEC __declspec(dllexport)
	#else
		#define LINKDLLSPEC __declspec(dllimport)
	#endif
#else
	#define LINKDLLSPEC
#endif


//
// clear all device control function pointers and set to default behavior
//
extern int LINKDLLSPEC LinkLinkSelect(void);


