
#include <stdio.h>


#ifdef _WINDOWS
	#ifdef PARSEDLL
		#define PARSEDLLSPEC __declspec(dllexport)
	#else
		#define PARSEDLLSPEC __declspec(dllimport)
	#endif
#else
	#define PARSEDLLSPEC
#endif


#include "Socket.h"

//
// print a message for the user
//
extern PARSEDLLSPEC void UserPrintIt(char *buffer);

//
// print a message for the user
//
extern PARSEDLLSPEC void UserPrint(char *format, ...);


//
// print user messages to the console
//
extern PARSEDLLSPEC void UserPrintConsole(int onoff);
extern PARSEDLLSPEC int UserPrintConsoleGet();

//
// print user messages to the specified file.
// the file must be opened before calling this function.
// if file==0, messages are no longer added to the file
//
extern PARSEDLLSPEC void UserPrintFile(char *filename);

//
// print user messages to the specified socket.
// the socket must be opened before calling this function.
// if socket==0, messages are no longer added to the file
//
extern PARSEDLLSPEC void UserPrintSocket(struct _Socket *socket);

//
// print user messages with an application defined function
//
extern PARSEDLLSPEC void UserPrintFunction(void (*print)(char *buffer));

