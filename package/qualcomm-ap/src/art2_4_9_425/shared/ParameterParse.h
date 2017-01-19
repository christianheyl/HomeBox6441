

#ifdef _WINDOWS
	#ifdef PARSEDLL
		#define PARSEDLLSPEC __declspec(dllexport)
	#else
		#define PARSEDLLSPEC __declspec(dllimport)
	#endif
#else
	#define PARSEDLLSPEC
#endif

struct _ParameterList;

extern PARSEDLLSPEC void ParseErrorInit();

extern PARSEDLLSPEC int ParseMacAddress(char *buffer, unsigned char *cmac);

extern PARSEDLLSPEC int ParseUnsigned(int input, char *name, int max, unsigned int *value);

extern PARSEDLLSPEC int ParseUnsignedList(int input, char *name, unsigned int *value, struct _ParameterList *list);

extern PARSEDLLSPEC int ParseInteger(int input, char *name, int max, int *value);

extern PARSEDLLSPEC int ParseIntegerList(int input, char *name, int *value, struct _ParameterList *list);

extern PARSEDLLSPEC int ParseFloat(int input, char *name, int max, float *value);

extern PARSEDLLSPEC int ParseFloatList(int input, char *name, float *value, struct _ParameterList *list);

extern PARSEDLLSPEC int ParseDouble(int input, char *name, int max, double *value);

extern PARSEDLLSPEC int ParseDoubleList(int input, char *name, double *value, struct _ParameterList *list);

extern PARSEDLLSPEC int ParseAddressList(int input, char *name, unsigned int *low, unsigned int *high, int *increment, int have, struct _ParameterList *list);

extern PARSEDLLSPEC int ParseHex(int input, char *name, int max, unsigned int *value);

extern PARSEDLLSPEC int ParseHexList(int input, char *name, unsigned int *value, struct _ParameterList *list);

extern PARSEDLLSPEC int ParseStringAndSetRates(int input, char *name, int max, int *value);

extern PARSEDLLSPEC void ParseParameterReplacement(char * (*f)(char *name, char *buffer, int max));

