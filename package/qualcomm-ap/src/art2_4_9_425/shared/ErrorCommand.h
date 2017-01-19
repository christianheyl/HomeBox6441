


#ifdef _WINDOWS
	#ifdef PARSEDLL
		#define PARSEDLLSPEC __declspec(dllexport)
	#else
		#define PARSEDLLSPEC __declspec(dllimport)
	#endif
#else
	#define PARSEDLLSPEC
#endif


//
// Adjust which error messages are shown to the user.
//
extern PARSEDLLSPEC void ErrorCommand();

extern PARSEDLLSPEC void ErrorParameterSplice(struct _ParameterList *list);

