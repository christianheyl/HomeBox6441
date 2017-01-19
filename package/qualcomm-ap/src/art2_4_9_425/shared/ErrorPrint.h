

#ifdef _WINDOWS
	#ifdef PARSEDLL
		#define PARSEDLLSPEC __declspec(dllexport)
	#else
		#define PARSEDLLSPEC __declspec(dllimport)
	#endif
#else
	#define PARSEDLLSPEC
#endif



struct _Error
{
    int code;
    int type;
    char *format;
};


enum ErrorType
{
    ErrorDebug=0,
    ErrorControl,
    ErrorInformation,
    ErrorWarning,
    ErrorFatal,
    ErrorMany,          // put this last, so that we can get a count of the number of types
};

#define ErrorDebugText "DEBUG"
#define ErrorControlText "CONTROL"
#define ErrorInformationText "INFO"
#define ErrorWarningText "WARNING"
#define ErrorFatalText "ERROR"

#define ErrorDebugAbbreviation "d"
#define ErrorControlAbbreviation "c"
#define ErrorInformationAbbreviation "i"
#define ErrorWarningAbbreviation "w"
#define ErrorFatalAbbreviation "e"

//
// response codes
// if ErrorResponseSpecial is set in type, then the individual message is displayed according to the high byte in type
// if not, then the default respons for the type is used.
// 
enum ErrorResponse
{
    ErrorResponseShowCode=0,
    ErrorResponseShowType,
    ErrorResponseShowMessage,
    ErrorResponsePause,
    ErrorResponseBell,
	ErrorResponseLog,
    ErrorResponseSpecial=7,
};


enum
{
    ErrorNone=0,
    ErrorUnknown,
};

//
// add a set of error codes and message to the hash table
//
extern PARSEDLLSPEC int ErrorHashCreate(struct _Error *m, int many);

//
// adds the standard message codes, ErrorNone and ErrorUnknown
//
extern PARSEDLLSPEC void ErrorInit();

//
// Gets the response field for a whole class of messages. see enum ErrorResponse
//
extern PARSEDLLSPEC int ErrorTypeResponseGet(int type);

//
// Sets the response field for a whole class of messages. see enum ErrorResponse
//
extern PARSEDLLSPEC int ErrorTypeResponseSet(int type, int response);

//
// Gets the response field for a single message code. see enum ErrorResponse
//
extern PARSEDLLSPEC int ErrorResponseGet(int code);

//
// Gets the response field for a single message code. see enum ErrorResponse
//
extern PARSEDLLSPEC int ErrorResponseSet(int code, int response);

//
// Handle the error. Print it or whatever.
//
extern PARSEDLLSPEC int ErrorPrint(int code, ...);

//
// Forward an error message from art.
// Similar to ErrorPrint() exceptthe message is already formatted.
//
extern PARSEDLLSPEC void ErrorForward(int art, int code, char *type, char *message);

//
// set the function called to execute a pause
//
extern PARSEDLLSPEC void ErrorPauseFunction(void (*pause)(void));

//
// set the function called to ring the bell
//
extern PARSEDLLSPEC void ErrorBellFunction(void (*bell)(void));

//
// set the function used to log messages to a file
//
extern PARSEDLLSPEC void ErrorLogFunction(void (*log)(char *buffer));

//
// set the function used to show messages to the user
//
extern PARSEDLLSPEC void ErrorPrintFunction(void (*print)(char *buffer));


extern PARSEDLLSPEC void ErrorListType(int type);


extern PARSEDLLSPEC void ErrorListCode(int code);


extern PARSEDLLSPEC void ErrorListAll(void);


extern PARSEDLLSPEC int ErrorTypeParse(char *text);


extern PARSEDLLSPEC void ErrorTypeAbbreviation(int onoff);
