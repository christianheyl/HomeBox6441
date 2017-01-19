

#ifdef _WINDOWS
	#ifdef PARSEDLL
		#define PARSEDLLSPEC __declspec(dllexport)
	#else
		#define PARSEDLLSPEC __declspec(dllimport)
	#endif
#else
	#define PARSEDLLSPEC
#endif


extern PARSEDLLSPEC int KeyboardHasInput(void);

extern PARSEDLLSPEC void KeyboardBeep(void);

//
// get line of input from keyboard
//
extern PARSEDLLSPEC int KeyboardReadWait(char *buffer, int maxlen);

//
// get line of input from keyboard
//
extern PARSEDLLSPEC int KeyboardRead(char *buffer, int maxlen);


