


#ifdef _WINDOWS
	#ifdef PARSEDLL
		#define PARSEDLLSPEC __declspec(dllexport)
	#else
		#define PARSEDLLSPEC __declspec(dllimport)
	#endif
#else
	#define PARSEDLLSPEC
#endif



extern PARSEDLLSPEC void HelpClose();

extern PARSEDLLSPEC int HelpOpen(char *filename);

//
// find the specified topic and print the description
//
extern PARSEDLLSPEC int Help(char *name, void (*print)(char *buffer));

//
// print a list of topics in the help file
//
extern PARSEDLLSPEC int HelpIndex(void (*print)(char *buffer));
