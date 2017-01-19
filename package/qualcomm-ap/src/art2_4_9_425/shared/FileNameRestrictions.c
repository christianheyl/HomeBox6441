

//  "ACI $Id: //depot/sw/branches/art2_main_per_cs/src/art2/shared/FileNameRestrictions.c#1 $, $Header: //depot/sw/branches/art2_main_per_cs/src/art2/shared/FileNameRestrictions.c#1 $"




#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>


//
// this is the microsoft windows version
//

#include "FileNameRestrictions.h" 

//
// returns the character used between directories in filenames
//
PARSEDLLSPEC char FilenameDirectoryDelimiter()
{
#ifndef __APPLE__
    return '\\';
#else
	return '/';
#endif
}


//
// return 1 if the character is allowed in file names
//
PARSEDLLSPEC int FilenameCharacterAllowed(char c, int position)
{
	if(position==1)
	{
		return (c!='/' && c!='*' && c!='?' && c!='\"' && c!='<' && c!='>' && c!='|');	// allow c:filename
	}
	else
	{
#ifndef __APPLE__
		return (c!='/' && c!=':' && c!='*' && c!='?' && c!='\"' && c!='<' && c!='>' && c!='|');
#else
		return (c!=':' && c!='*' && c!='?' && c!='\"' && c!='<' && c!='>' && c!='|');
#endif
	}
}


//
// returns 1 if the filename specifies an absolute path that should not be modified
//
PARSEDLLSPEC int FilenameAbsolutePath(char *name)
{
    //
    // THIS TEST NEEDS TO BE BETTER
    //
    //
    // check for something starting with ./ or ../
    //
    if(name[0]=='.')
    {
        return 1;
    }
    //
    // check for something starting with C: or similar
    //
    if(name[1]==':')
    {
        return 1;
    }
    return 0;
}


