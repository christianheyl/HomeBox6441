#ifndef _PARAMETER_SELECT_H_
#define _PARAMETER_SELECT_H_

#ifdef _WINDOWS
	#ifdef PARSEDLL
		#define PARSEDLLSPEC __declspec(dllexport)
	#else
		#define PARSEDLLSPEC __declspec(dllimport)
	#endif
#else
	#define PARSEDLLSPEC
#endif

#define MPWORD 3

struct _ParameterList
{
    int code;			// code returned by ParameterSelect()
    char *word[MPWORD];	// keyword and aliases
	char *help;
	char type;			// d(ecimal), u(nsigned), h(ex) also x, f(loat), t(text) also s
	char *units;		// only used in help message
	int nx;				// MAXIMUM NUMBER OF VALUES
	int ny;
	int nz;
	void *minimum;		// ptr to minimum value of the correct type, 0 if none
	void *maximum;		// ptr to maximum value of the correct type, 0 if none
	void *def;			// ptr to default value of the correct type, 0 if none
	int nspecial;
	struct _ParameterList *special;
};

//
// returns 1 if the paramter type is valid. returns 0 otherwise.
//
extern PARSEDLLSPEC int ParameterTypeValid(char type);

//
// prints the description of the specified parameter from the structure.
//
extern PARSEDLLSPEC void ParameterHelpSingle(struct _ParameterList *cl, void (*print)(char *buffer));

extern PARSEDLLSPEC void ParameterHelpLine(struct _ParameterList *cl, char *buffer,int max);

//
// prints the list of parameter names from the structure.
// the function (*print)() is called once for each command.
//
extern PARSEDLLSPEC void ParameterHelp(struct _ParameterList *cl, int nl, void (*print)(char *buffer));

//
// parse the input text and returns the index for the best match in the parameter structure.
// also return array indexes if the user typed name[it,jt,kt]=value. it=jt=kt=1 if no index supplied by user.
// return -1 if no match is found.
//
extern PARSEDLLSPEC int ParameterSelectIndexArray(char *buffer, struct _ParameterList *cl, int nl, int *it, int *jt, int *kt);

//
// parse the input text and returns the index for the best match in the parameter structure.
// return -1 if no match is found.
//
extern PARSEDLLSPEC int ParameterSelectIndex(char *buffer, struct _ParameterList *cl, int nl);

//
// parse the input text and returns the code for the best match in the parameter structure.
// return -1 if no match is found.
//
extern PARSEDLLSPEC int ParameterSelect(char *buffer, struct _ParameterList *cl, int nl);

//
// returns the first name that matches the code
//
extern PARSEDLLSPEC char *ParameterName(int code, struct _ParameterList *cl, int nl);

//
// returns the maximum number of parameter values
//
extern PARSEDLLSPEC int ParameterValueMaximum(struct _ParameterList *cl);


#endif //_PARAMETER_SELECT_H_


