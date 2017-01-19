
//#ident  "ACI $Id: //depot/sw/branches/art2_main_per_cs/src/art2/shared/TimeMillisecond.h#1 $, $Header: //depot/sw/branches/art2_main_per_cs/src/art2/shared/TimeMillisecond.h#1 $"



#ifdef _WINDOWS
	#ifdef PARSEDLL
		#define PARSEDLLSPEC __declspec(dllexport)
	#else
		#define PARSEDLLSPEC __declspec(dllimport)
	#endif
#else
	#define PARSEDLLSPEC
#endif


extern PARSEDLLSPEC int TimeMillisecond();



