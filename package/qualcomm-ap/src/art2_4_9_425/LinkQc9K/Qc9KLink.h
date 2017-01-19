#ifndef _QC9K_LINK_H_
#define _QC9K_LINK_H_

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
extern int LINKDLLSPEC Qc9KLinkSelect(void);

#endif //_QC9K_LINK_H_
