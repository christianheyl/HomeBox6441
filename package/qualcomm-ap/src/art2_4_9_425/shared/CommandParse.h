

#ifdef _WINDOWS
	#ifdef PARSEDLL
		#define PARSEDLLSPEC __declspec(dllexport)
	#else
		#define PARSEDLLSPEC __declspec(dllimport)
	#endif
#else
	#define PARSEDLLSPEC
#endif


extern PARSEDLLSPEC void CommandReplacement(char * (*f)(char *name, char *buffer, int max));


extern PARSEDLLSPEC void CommandParameterReplacement(char * (*f)(char *name, char *buffer, int max));


//
// Returns a pointer to the original command input.
//
extern PARSEDLLSPEC char *CommandInput();

//
// Returns a pointer to the command input after variable translation.
//
extern PARSEDLLSPEC char *CommandInputTranslated();

//
// Returns a pointer to the command word.
// Valid after a call to ComandParse().
//
extern PARSEDLLSPEC char *CommandWord();


//
// Returns the number of parameters.
// Valid after a call to ComandParse().
//
extern PARSEDLLSPEC int CommandParameterMany();


//
// Returns a pointer to the parameter name.
// Valid after a call to ComandParse().
//
extern PARSEDLLSPEC char *CommandParameterName(int arg);

extern PARSEDLLSPEC int CommandParameterNameIsSelected(char *name);
//
// Returns the number of values for the specified parameter.
// Valid after a call to ComandParse().
//
extern PARSEDLLSPEC int CommandParameterValueMany(int arg);


//
// Returns a pointer to the specified value for the specified parameter.
// Valid after a call to ComandParse().
//
// WARNING: the returned value points to a static memory area. It will change
// on the next call to this function. Copy or consume the value immediately.
//
extern PARSEDLLSPEC char *CommandParameterValue(int arg, int value);


//
// Returns a pointer to the specified value for the specified parameter.
// Valid after a call to ComandParse().
//
extern PARSEDLLSPEC char *CommandParameterValueNoReplacement(int arg, int value);


//
// Parses the input buffer into a command word and parameter name value pairs.
// Alters the input buffer
// Format is the following:
//
//    command parameter=value; parameter=value; ... 
//
// Equal signs and semicolons are required. 
// Command and parameter names may not include spaces.
// Values may contain anything except leading spaces and semicolons.
//
extern PARSEDLLSPEC int CommandParse(char *buffer);


//
// Pushes the current set of parameter values on to the parameter stack. 
// These values are returned in advance of any new values typed on 
// the next command line. In essence these values become default values
// that may be overwritten by later values.
//
extern PARSEDLLSPEC int CommandPush();


//
// Pops the last set of parameter values off the parameter stack. 
//
extern PARSEDLLSPEC int CommandPop();


