

/*
 *
 *               Copyright (c) 1996-2000 Fantastic Data LLC
 *                           All Rights Reserved
 *
 */




/*
 * string.c
 *
 * routines to manipulate strings
 */



#include <string.h>
#include <ctype.h>
#ifndef __APPLE__
#include <malloc.h>
#endif
#include <stdlib.h>


#include "smatch.h"

#define Hallocate malloc
#define Hdeallocate free

#define MIN(a,b) ((a)<(b)?(a):(b))

/*
 * Returns the length of the string.
 */
PARSEDLLSPEC int Slength(char *text)
{
    if(text==0) return 0;
    else return strlen(text);
}


/*
 * Returns < 0 if the first string comes first in alphabetical order.
 * Returns = 0 if the two strings are the same.
 * Returns > 0 if the second string comes first in alphabetical order.
 */
PARSEDLLSPEC int Scompare(char *text1, char *text2)
{
    if(text1==0 && text2==0) 
    {
        return 0;
    }
    else if(text1==0)
    {
        return -1;
    }
    else if(text2==0) 
    {
        return 1;
    }

    return strcmp(text1,text2);
}


/*
 * Same as Scompare() except that the comparison is done from the end 
 * of the string. Thus CBA comes before BCA.
 *
 * Returns < 0 if the first string comes first in alphabetical order.
 * Returns = 0 if the two strings are the same.
 * Returns > 0 if the second string comes first in alphabetical order.
 */
PARSEDLLSPEC int ScompareBackwards(char *text1, char *text2)
{
    int ic;
    int length, length1, length2;

    if(text1==0 && text2==0) 
    {
        return 0;
    }
    else if(text1==0)
    {
        return -1;
    }
    else if(text2==0) 
    {
        return 1;
    }

    length1=Slength(text1);
    length2=Slength(text2);
    length=MIN(length1,length2);
    /*
     * Comapre the strings character by character until one of them ends.
     */
    for(ic=0; ic<length; ic++)
    {
        if(text1[length1-ic-1]<text2[length2-ic-1])
        {
            return -1;
        }
        if(text1[length1-ic-1]>text2[length2-ic-1])
        {
            return 1;
        }
    }
    /*
     * They're the same up until one of them ends. So the shorter one
     * comes first.
     */
    if(length1==length2) 
    {
        return 0;
    }
    else if(length1<length2) 
    {
        return -1;
    }
    else
    {
        return 1;
    }
}


/*
 * Returns 0 if the two input strings are different.
 * Returns 1 if they are the same.
 */
PARSEDLLSPEC int Sequal(char *text1, char *text2)
{
    if(text1==0 && text2==0) return 1;
    else if(text1==0 || text2==0) return 0;

    return strcmp(text1,text2)==0;
}


/*
 * Copies the source string to the destination.
 */
PARSEDLLSPEC void Scopy(char *destination, char *source)
{
    if(destination==0) 
    {
        return;
    }
    if(source==0) 
    {
        strcpy(destination,"");
        return;
    }

    (void)strcpy(destination,source);
}


PARSEDLLSPEC void Sdestroy(char *text)
{
    if(text!=0)
    {
        Hdeallocate(text);
    }
}


/*
 * Creates and copies an existing string.
 */
PARSEDLLSPEC char *Sduplicate(char *source)
{
    char *result = 0;

    if( source != 0 )
    {
	result = (char *)Hallocate(strlen(source) + 1);
	if( result != 0 )
        {
	    Scopy(result, source);
        }
    }
    return result;
}


/*
 * Makes the string empty.
 */
PARSEDLLSPEC void Sempty(char *text)
{
    if(text==0) return;

    Scopy(text,"");
}


/*
 * Copies the source string to the destination, converting uppercase letters
 * to lowercase letters in the process.
 */
PARSEDLLSPEC void Stolower(char *destination, char *source)
{
    int i;
	int length;
  
    if(destination==0) 
    {
        return;
    }
    if(source==0) 
    {
        Sempty(destination);
        return;
    }

    if(destination!=source)
    {
        (void)strcpy(destination,source);
    }
	length=strlen(destination);
    for (i=0; i<length; i++)
    {
        if (isupper(destination[i]))
            destination[i] = tolower(destination[i]);
    }
}


/*
 * Copies the source string to the destination, converting lowercase letters
 * to uppercase letters in the process.
 */
PARSEDLLSPEC void Stoupper(char *destination, char *source)
{
    int i;
	int length;
  
    if(destination==0) 
    {
        return;
    }
    if(source==0) 
    {
        Sempty(destination);
        return;
    }
    if(destination!=source)
    {
        (void)strcpy(destination,source);
    }
	length=strlen(destination);
    for (i=0; i<length; i++)
    {
        if (islower(destination[i]))
            destination[i] = toupper(destination[i]);
    }
}


/*
 * Truncates the string to the specified length.
 */
PARSEDLLSPEC void Struncate(char *text, int length)
{
    if(text==0) return;

    if(Slength(text)>length) text[length]=0;
}


/*
 * Appends the second string to the end of the first.
 */
PARSEDLLSPEC void Sappend(char *text1, char *text2)
{
    if(text1==0 || text2==0) return;

    (void)strcat(text1,text2);
}


/*
 * Appends the character the end of the text.
 */
PARSEDLLSPEC void SappendChar(char *text1, char char1)
{
    char text2[10];

    if(text1==0) return;

    text2[0]=char1;
    text2[1]=0;

    (void)strcat(text1,text2);
}


static int space(char character)
{
    if(character==' ' || character=='\t') return 1;
    else return 0;
}


/*
 * Skip over spaces and tabs.
 */
static int skip(char *text, int itext)
{
    while(space(text[itext])) itext++;
    return itext;
}


//
// removes eol, cr, space, and tab from end of input line
// alters the input buffer
// returns the new length of the buffer
//
PARSEDLLSPEC int StrimEnd(char *buffer)
{
	int length;

	length=strlen(buffer);
	while(length>0)
	{
		if(buffer[length-1]=='\n' ||
			buffer[length-1]=='\r' ||
			buffer[length-1]==' ' ||
			buffer[length-1]=='\t')
		{
			buffer[length-1]=0;
			length--;
		}
		else
		{
			break;
		}
	}
	return length;
}


/*
 * Removes blanks from both ends of the string.
 */
PARSEDLLSPEC void Strim(char *source)
{
    char * dest;
    char * first;

    if(source==0) 
    {
        return;
    }
    dest=source;
    /*
     * Skip over the initial spaces.
     */
    for( ; (*source)!=0; source++)
    {
        if(space(*source)==0)
        {
            break;
        }
    }
    /*
     * Copy characters until the end.  Remember the first space
     * we see.  If we don't see any other characters, at the end we'll
     * zero the space.
     */
    first=0;
    for( ; (*source)!=0; source++)
    {
        if(space(*source)==0)
        {
            first=0;
        }
        else if(first==0)
        {
            first=dest;
        }
        (*dest) = (*source);
        dest++;
    }

    if(first!=0)
    {
        (*first) = 0;
    }
    else
    {
        (*dest) = 0;
    }
}


/*
 * Compare two strings in a more liberal manner than Sequal().
 * Spaces and tabs are equivalent.  Spaces at the beginning and end of
 * the strings are ignored.  Any number of spaces are equivalent.
 * Upper and lower case are equivalent.  Returns 1 if the first
 * string is a subset of the second.  Returns zero otherwise.
 */
PARSEDLLSPEC int Ssub(char *sub, char *line)
{
    int isub,lsub;
    int iline,lline;

    lsub=Slength(sub);
    lline=Slength(line);

    if((lsub==0) && (lline==0)) return 1;
    if((lsub==0) && (lline!=0)) return 0;
    if((lsub!=0) && (lline==0)) return 0;

    /*
     * skip leading spaces
     */
    isub=0;
    isub=skip(sub,isub);
    iline=0;
    iline=skip(line,iline);
    /*
     * compare characters until
     *    1.  a space is seen
     *    2.  they don't match
     *    3.  we run out of characters in sub
     *    4.  we run out of characters in line
     */
    for( ; iline<lline && isub<lsub; iline++, isub++)
    {
        if(space(sub[isub]))
        {
            if(space(line[iline]))
            {
                isub=skip(sub,isub)-1;
                iline=skip(line,iline)-1;
            }
            else return 0;
        }
        else if(tolower(sub[isub])!=tolower(line[iline])) return 0;
    }
    /*
     * see if there are trailing spaces on sub
     */
    if(isub<lsub && iline>=lline) isub=skip(sub,isub);

    if(isub>=lsub && iline<=lline) return 1; 
    else return 0;
}


/*
 * Compare two strings in a more liberal manner than Sequal().
 * Spaces and tabs are equivalent.  Spaces at the beginning and end of
 * the strings are ignored.  Any number of spaces are equivalent.
 * Upper and lower case are equivalent.  Returns 1 if the two
 * strings are the same.  Returns zero otherwise.
 */
PARSEDLLSPEC int Smatch(char *text1, char *text2)
{
    return (Ssub(text1,text2) && Ssub(text2,text1));
}


#define MAX_PAREN 20

/*
 * Returns index into the grouper array if the specified character
 * is a grouper.  Otherwise returns -1.
 */
static int GrouperStart(int ngrouper, char *grouper, char gstart)
{
    int igrouper;

    for(igrouper=0; igrouper<ngrouper; igrouper++)
    {
        if(grouper[igrouper*2]==gstart) return igrouper;
    }
    return -1;
}


/*
 * Returns index of the 
 */
static int GrouperStop(int ngrouper, char *grouper, char gstart, char gstop)
{
    int igrouper;

    for(igrouper=0; igrouper<ngrouper; igrouper++)
    {
        if(grouper[igrouper*2]==gstart && grouper[igrouper*2+1]==gstop)
            return igrouper;
    }
    return -1;
}


/*
 * Returns index if the character is a separator.
 * Otherwise returns -1.
 */
PARSEDLLSPEC int Sisoneof(char *separator, char character)
{
    int iseparator;
    int nseparator;

    nseparator=Slength(separator);
    for(iseparator=0; iseparator<nseparator; iseparator++)
    {
        if(separator[iseparator]==character) return iseparator;
    }
    return -1;
}


/*
 * Strips quotation marks from around the word.
 * Returns 0 if it does it;  -1 if not.
 */
PARSEDLLSPEC int Sunquote(char *word)
{
    int ichar;
    int nchar;

    if(word==0) return -1;

    nchar=Slength(word);
    if(nchar<2) return -1;

    if((word[0]=='"') && (word[nchar-1]=='"'))
    {
        for(ichar=0; ichar<nchar-2; ichar++) word[ichar]=word[ichar+1];
        word[nchar-2]=0;
        return 0;
    }
    if((word[0]=='\'') && (word[nchar-1]=='\''))
    {
        for(ichar=0; ichar<nchar-2; ichar++) word[ichar]=word[ichar+1];
        word[nchar-2]=0;
        return 0;
    }
    return -1;
}
        
    
/*
 * Chop the input string into words.
 * Words are separated by the specified separator character.
 * Words are grouped by the specified grouping characters.
 * Returns with words in the specified array "word".
 * Returns the number of words found.  
 */
PARSEDLLSPEC int Swords(char *text, 
    int condense, char *separator, char *grouper, char *escaper, char **word)
{
    int nword;			/* number of words already found */
    int wplace;			/* set if we are in the middle of a word */
    int tplace;
    int tlength;
    int nparen;
    char paren[MAX_PAREN];				// changed from int, 090714
    int group;
    int ngrouper;
    int nescaper;
    int nseparator;

    if(text==0) return -1;

    tplace=0;
    wplace=0;
    nword=0;
    nparen=0;
    tlength=Slength(text);
    ngrouper=Slength(grouper)/2;
    nseparator=Slength(separator);
    nescaper=Slength(escaper);

    for(tplace=0; tplace<tlength; tplace++)
    {
        /*
         * are we inside a group?  If so, see if this is the end.
         */
        if(nparen>0)
        {
            if(GrouperStop(ngrouper,grouper,paren[nparen-1],text[tplace])>=0)
            {
                word[nword][wplace]=text[tplace];
                wplace++;
                nparen--;
                continue;
            }
        }
        /*
         * Is this the start of a group?
         */
        group=GrouperStart(ngrouper,grouper,text[tplace]);
        if(nparen<MAX_PAREN && group>=0)
        {
            word[nword][wplace]=text[tplace];
            wplace++;
            paren[nparen]=text[tplace];
            nparen++;
            continue;
        }
        /*
         * Is this an escape character?
         */
        if(nparen<=0 && Sisoneof(escaper,text[tplace])>=0)
        {
            tplace++;
            word[nword][wplace]=text[tplace]; 
            wplace++;
            continue;
        }
        /*
         * Is this a separator character?
         */
        if(nparen<=0 && Sisoneof(separator,text[tplace])>=0)
        {
            if((!condense) || (wplace!=0))
            {
                word[nword][wplace]=0;
                nword++;
                wplace=0;
            }
            continue;
        }
        /*
         * Just a regular old character.  Put it in the output word.
         */
        word[nword][wplace]=text[tplace];
        wplace++;
    }

    if(wplace>0)
    {
        word[nword][wplace]=0;
        nword++;
    }
    return nword; 
}
    
/*
 * Counts the words in the input string.
 * Words are separated by the specified separator character.
 * Words are grouped by the specified grouping characters.
 * Returns the number of words found.  
 */
PARSEDLLSPEC int ScountWords(char *text, 
    int condense, char *separator, char *grouper, char *escaper) 
{
    int nword;			/* number of words already found */
    int wplace;			/* set if we are in the middle of a word */
    int tplace;
    int tlength;
    int nparen;
    char paren[MAX_PAREN];			// changed from int, 090712
    int group;
    int ngrouper;
    int nescaper;
    int nseparator;

    if(text==0) return -1;

    tplace=0;
    wplace=0;
    nword=0;
    nparen=0;
    tlength=Slength(text);
    ngrouper=Slength(grouper)/2;
    nseparator=Slength(separator);
    nescaper=Slength(escaper);

    for(tplace=0; tplace<tlength; tplace++)
    {
        /*
         * are we inside a group?  If so, see if this is the end.
         */
        if(nparen>0)
        {
            if(GrouperStop(ngrouper,grouper,paren[nparen-1],text[tplace])>=0)
            {
                wplace++;
                nparen--;
                continue;
            }
        }
        /*
         * Is this the start of a group?
         */
        group=GrouperStart(ngrouper,grouper,text[tplace]);
        if(nparen<MAX_PAREN && group>=0)
        {
            wplace++;
            paren[nparen]=text[tplace];
            nparen++;
            continue;
        }
        /*
         * Is this an escape character?
         */
        if(nparen<=0 && Sisoneof(escaper,text[tplace])>=0)
        {
            tplace++;
            wplace++;
            continue;
        }
        /*
         * Is this a separator character?
         */
        if(nparen<=0 && Sisoneof(separator,text[tplace])>=0)
        {
            if((!condense) || (wplace!=0))
            {
                nword++;
                wplace=0;
            }
            continue;
        }
        /*
         * Just a regular old character.  Put it in the output word.
         */
        wplace++;
    }

    if(wplace>0)
    {
        nword++;
    }
    return --nword;  /* I don't know why it was off?  PKB */
}
    
  
/*
 * Chop the input string into words.
 * Words are separated by the specified separator character.
 * Words are grouped by the specified grouping characters.
 * Returns with words in the specified array "word".
 * Returns the number of words found.  
 *
 * Grouping is performed to only one level.
 *
 * Example: "(I'm a string)" with () and '' as groupers ignores
 *          the single quote within the parenthesized group.
 */
PARSEDLLSPEC int SwordsNoNest(char *text,
    int condense, char *separator, char *grouper, char *escaper, char **word)
{
    int nword;			/* number of words already found */
    int wplace;			/* set if we are in the middle of a word */
    int tplace;
    int tlength;
    int nparen;
    char paren[MAX_PAREN];
    int group;
    int ngrouper;
    int nescaper;
    int nseparator;

    if(text==0) return -1;

    tplace=0;
    wplace=0;
    nword=0;
    nparen=0;
    tlength=Slength(text);
    ngrouper=Slength(grouper)/2;
    nseparator=Slength(separator);
    nescaper=Slength(escaper);

    for(tplace=0; tplace<tlength; tplace++)
    {
        /*
         * are we inside a group?  If so, see if this is the end.
         */
        if(nparen>0)
        {
            if(GrouperStop(ngrouper,grouper,paren[nparen-1],text[tplace])>=0)
            {
                word[nword][wplace]=text[tplace];
                wplace++;
                nparen--;
                continue;
            }
        }
        /*
         * Is this the start of a group?
         */
	if(nparen<=0)
	{
	    group=GrouperStart(ngrouper,grouper,text[tplace]);
	    if(nparen<MAX_PAREN && group>=0)
	    {
		word[nword][wplace]=text[tplace];
		wplace++;
		paren[nparen]=text[tplace];
		nparen++;
		continue;
	    }
	}
        /*
         * Is this an escape character?
         */
        if(nparen<=0 && Sisoneof(escaper,text[tplace])>=0)
        {
            tplace++;
            word[nword][wplace]=text[tplace]; 
            wplace++;
            continue;
        }
        /*
         * Is this a separator character?
         */
        if(nparen<=0 && Sisoneof(separator,text[tplace])>=0)
        {
            if((!condense) || (wplace!=0))
            {
                word[nword][wplace]=0;
                nword++;
                wplace=0;
            }
            continue;
        }
        /*
         * Just a regular old character.  Put it in the output word.
         */
        word[nword][wplace]=text[tplace];
        wplace++;
    }

    if(wplace>0)
    {
        word[nword][wplace]=0;
        nword++;
    }
    return nword; 
}
    
  
/*
 * Extract the word in the specified position.
 * Uses the same parameters as Swords() and follows
 * the same parsing rules.
 */
PARSEDLLSPEC int SextractNoNest(char *text,
    int condense, char *separator, char *grouper, char *escaper, int position,
    char *word)
{
    int nword;			/* number of words already found */
    int wplace;			/* set if we are in the middle of a word */
    int tplace;
    int tlength;
    int nparen;
    char paren[MAX_PAREN];
    int group;
    int ngrouper;
    int nescaper;
    int nseparator;

    Sempty(word);

    if(text==0) return -1;

    tplace=0;
    wplace=0;
    nword=0;
    nparen=0;
    tlength=Slength(text);
    ngrouper=Slength(grouper)/2;
    nseparator=Slength(separator);
    nescaper=Slength(escaper);

    for(tplace=0; tplace<tlength; tplace++)
    {
        /*
         * are we inside a group?  If so, see if this is the end.
         */
        if(nparen>0)
        {
            if(GrouperStop(ngrouper,grouper,paren[nparen-1],text[tplace])>=0)
            {
                if(nword==position) {
		    word[wplace]=text[tplace];
		}
		wplace++;
                nparen--;
                continue;
            }
        }
        /*
         * Is this the start of a group?
         */
        if(nparen<=0)
        {
            group=GrouperStart(ngrouper,grouper,text[tplace]);
            if(nparen<MAX_PAREN && group>=0)
            {
                if(nword==position) {
		    word[wplace]=text[tplace];
		}
                wplace++;
                paren[nparen]=text[tplace];
                nparen++;
                continue;
            }
        }
        /*
         * Is this an escape character?
         */
        if(nparen<=0 && Sisoneof(escaper,text[tplace])>=0)
        {
            tplace++;
            if(nword==position) {
		word[wplace]=text[tplace]; 
	    }
            wplace++;
            continue;
        }
        /*
         * Is this a separator character?
         */
        if(nparen<=0 && Sisoneof(separator,text[tplace])>=0)
        {
            if((!condense) || (wplace!=0))
            {
                if(nword==position) word[wplace]=0;
                nword++;
                if(nword>position) break;
                wplace=0;
            }
            continue;
        }
        /*
         * Just a regular old character.  Put it in the output word.
         */
        if(nword==position) {
	    word[wplace]=text[tplace];
	}
        wplace++;
    }

    if(wplace>0)
    {
        if(nword==position) word[wplace]=0;
        nword++;
    }
    if(nword>position) return 0;
    else return -1;
}
    
/*
 * Extract the word in the specified position.
 * Uses the same parameters as Swords() and follows
 * the same parsing rules.
 */
PARSEDLLSPEC int Sextract(char *text,
    int condense, char *separator, char *grouper, char *escaper, int position,
    char *word)
{
    int nword;			/* number of words already found */
    int wplace;			/* set if we are in the middle of a word */
    int tplace;
    int tlength;
    int nparen;
    char paren[MAX_PAREN];
    int group;
    int ngrouper;
    int nescaper;
    int nseparator;

    Sempty(word);

    if(text==0) return -1;

    tplace=0;
    wplace=0;
    nword=0;
    nparen=0;
    tlength=Slength(text);
    ngrouper=Slength(grouper)/2;
    nseparator=Slength(separator);
    nescaper=Slength(escaper);

    for(tplace=0; tplace<tlength; tplace++)
    {
        /*
         * are we inside a group?  If so, see if this is the end.
         */
        if(nparen>0)
        {
            if(GrouperStop(ngrouper,grouper,paren[nparen-1],text[tplace])>=0)
            {
                if(nword==position) {
		    word[wplace]=text[tplace];
		}
		wplace++;
                nparen--;
                continue;
            }
        }
        /*
         * Is this the start of a group?
         */
        group=GrouperStart(ngrouper,grouper,text[tplace]);
        if(nparen<MAX_PAREN && group>=0)
        {
            if(nword==position) {
		word[wplace]=text[tplace];
	    }
            wplace++;
            paren[nparen]=text[tplace];
            nparen++;
            continue;
        }
        /*
         * Is this an escape character?
         */
        if(nparen<=0 && Sisoneof(escaper,text[tplace])>=0)
        {
            tplace++;
            if(nword==position) {
		word[wplace]=text[tplace]; 
	    }
            wplace++;
            continue;
        }
        /*
         * Is this a separator character?
         */
        if(nparen<=0 && Sisoneof(separator,text[tplace])>=0)
        {
            if((!condense) || (wplace!=0))
            {
                if(nword==position) word[wplace]=0;
                nword++;
                if(nword>position) break;
                wplace=0;
            }
            continue;
        }
        /*
         * Just a regular old character.  Put it in the output word.
         */
        if(nword==position) {
	    word[wplace]=text[tplace];
	}
        wplace++;
    }

    if(wplace>0)
    {
        if(nword==position) word[wplace]=0;
        nword++;
    }
    if(nword>position) return 0;
    else return -1;
}
    

/*
 * Blanks the word in the specified position.
 * Uses the same parameters as Swords() and follows
 * the same parsing rules.
 */
PARSEDLLSPEC int SblankWord(char *text,
    int condense, char *separator, char *grouper, char *escaper, int position)
{
    int nword;			/* number of words already found */
    int wplace;			/* set if we are in the middle of a word */
    int tplace;
    int tlength;
    int nparen;
    char paren[MAX_PAREN];
    int group;
    int ngrouper;
    int nescaper;
    int nseparator;

    if(text==0) return -1;

    tplace=0;
    wplace=0;
    nword=0;
    nparen=0;
    tlength=Slength(text);
    ngrouper=Slength(grouper)/2;
    nseparator=Slength(separator);
    nescaper=Slength(escaper);

    for(tplace=0; tplace<tlength; tplace++)
    {
        /*
         * are we inside a group?  If so, see if this is the end.
         */
        if(nparen>0)
        {
            if(GrouperStop(ngrouper,grouper,paren[nparen-1],text[tplace])>=0)
            {
                if(nword==position) {
		    text[tplace] = ' '; /* blank out this word in the text */
		}
		wplace++;
                nparen--;
                continue;
            }
        }
        /*
         * Is this the start of a group?
         */
        group=GrouperStart(ngrouper,grouper,text[tplace]);
        if(nparen<MAX_PAREN && group>=0)
        {
            paren[nparen]=text[tplace];
            nparen++;
            if(nword==position) {
		text[tplace] = ' '; /* blank out this word in the text */
	    }
            wplace++;
            continue;
        }
        /*
         * Is this an escape character?
         */
        if(nparen<=0 && Sisoneof(escaper,text[tplace])>=0)
        {
            tplace++;
            if(nword==position) {
		text[tplace] = ' '; /* blank out this word in the text */
	    }
            wplace++;
            continue;
        }
        /*
         * Is this a separator character?
         */
        if(nparen<=0 && Sisoneof(separator,text[tplace])>=0)
        {
            if((!condense) || (wplace!=0))
            {
                nword++;
                if(nword>position) break;
                wplace=0;
            }
            continue;
        }
        /*
         * Just a regular old character.  Put it in the output word.
         */
        if(nword==position) {
	    text[tplace] = ' '; /* blank out this word in the text */
	}
        wplace++;
    }

    if(wplace>0)
    {
        nword++;
    }
    if(nword>position) return 0;
    else return -1;
}
    

/*
 * Blanks the separator in the specified position.
 * Uses the same parameters as Swords() and follows
 * the same parsing rules.
 */
PARSEDLLSPEC int SblankSeparator(char *text,
    char *separator, char *grouper, char *escaper, int position)
{
    int nword;			/* number of words already found */
    int wplace;			/* set if we are in the middle of a word */
    int tplace;
    int tlength;
    int nparen;
    char paren[MAX_PAREN];
    int group;
    int ngrouper;
    int nescaper;
    int nseparator;

    if(text==0) return -1;

    tplace=0;
    wplace=0;
    nword=0;
    nparen=0;
    tlength=Slength(text);
    ngrouper=Slength(grouper)/2;
    nseparator=Slength(separator);
    nescaper=Slength(escaper);

    for(tplace=0; tplace<tlength; tplace++)
    {
        /*
         * are we inside a group?  If so, see if this is the end.
         */
        if(nparen>0)
        {
            if(GrouperStop(ngrouper,grouper,paren[nparen-1],text[tplace])>=0)
            {
		wplace++;
                nparen--;
                continue;
            }
        }
        /*
         * Is this the start of a group?
         */
        group=GrouperStart(ngrouper,grouper,text[tplace]);
        if(nparen<MAX_PAREN && group>=0)
        {
            paren[nparen]=text[tplace];
            nparen++;
            wplace++;
            continue;
        }
        /*
         * Is this an escape character?
         */
        if(nparen<=0 && Sisoneof(escaper,text[tplace])>=0)
        {
            tplace++;
            wplace++;
            continue;
        }
        /*
         * Is this a separator character?
         */
        if(nparen<=0 && Sisoneof(separator,text[tplace])>=0)
        {
            if(wplace!=0)
            {
		if(nword==position) {
		    text[tplace] = ' '; /* blank out this
					   separator in the text */
		}
                nword++;
                if(nword>position) break;
                wplace=0;
            }
            continue;
        }
        wplace++;
    }

    if(wplace>0)
    {
        nword++;
    }
    if(nword>position) return 0;
    else return -1;
}
    

PARSEDLLSPEC int SnoBlank(char *result, char *source)
{
    int got;

    if(result==0) return -1;
    else if(source==0)
    {
        Sempty(result);
        return 0;
    }

    got=Sextract(source,1," \t","","",0,result);
    if(got==1) return 0;
    else return -1;
}



/***************************************************************
 *
 * Sgreater() - indicate if string is greater than another
 *
 * Accepts string1 and string2, returns TRUE if string1 > string2,
 * otherwise returns FALSE. The check is strictly with regard
 * to ASCII order of the characters!
 *
 ***************************************************************/

#define TRUE 1
#define FALSE 0

PARSEDLLSPEC int Sgreater(char *string1, char *string2)
{
	/* if string1 is NULL, it's clearly not greater than anything, but
	 * if string2 is NULL and string1 isn't, then we'll say it's greater
	 */
	if (!string1)
		return FALSE;
	if (!string2)
		return TRUE;

	/* now we loop through while they're equal, until they're unequal
	 * and a decision is made, or one of them reaches the end
	 */
	while (*string1 && *string2)
	{
		if (*string1 > *string2)	/* we are triumphant */
			return TRUE;
		if (*string1 < *string2)	/* we are desolate and defeated */
			return FALSE;
		++string1;
		++string2;
	}

	/* we reached the end; if string1 is zero, it's not greater, but
	 * otherwise string2 must have become zero, and we win the prize
	 */
	if (*string1 == '\0')
		return FALSE;
	else
		return TRUE;
}

