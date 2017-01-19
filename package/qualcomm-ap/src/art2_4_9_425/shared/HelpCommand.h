
#ifdef _WINDOWS
	#ifdef PARSEDLL
		#define PARSEDLLSPEC __declspec(dllexport)
	#else
		#define PARSEDLLSPEC __declspec(dllimport)
	#endif
#else
	#define PARSEDLLSPEC
#endif



extern PARSEDLLSPEC void HelpCommand(struct _ParameterList *list, int nlist);


extern PARSEDLLSPEC void HelpParameterSplice(struct _ParameterList *list);
