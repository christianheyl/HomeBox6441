

// "ACI $Id: //depot/sw/branches/art2_main_per_cs/src/art2/shared/FileNameRestrictions.h#1 $, $Header: //depot/sw/branches/art2_main_per_cs/src/art2/shared/FileNameRestrictions.h#1 $"



#ifdef UNUSED
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#endif



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
// returns the character used between directories in filenames
//
extern PARSEDLLSPEC char FilenameDirectoryDelimiter();


//
// return 1 if the character is allowed in file names
//
extern PARSEDLLSPEC int FilenameCharacterAllowed(char c, int position);


//
// returns 1 if the filename specifies an absolute path that should not be modified
//
extern PARSEDLLSPEC int FilenameAbsolutePath(char *name);
