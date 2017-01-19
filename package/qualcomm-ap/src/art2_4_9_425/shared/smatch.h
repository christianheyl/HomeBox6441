

/*
 *
 *               Copyright (c) 1996-2000 Fantastic Data LLC
 *                           All Rights Reserved
 *
 */




/*
 * smatch.h
 *
 * routines to manipulate strings
 */


#ifdef __cplusplus
extern "C" {
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
// our versions of snprintf and sscanf
// defined as macros since the calling sequences are the same on windows and linux, just the names are different
//
#if defined(LINUX) || defined(__APPLE__)
#define SformatOutput snprintf
#define SformatInput sscanf
#else
#define SformatOutput(buffer, size, format, ...) _snprintf_s(buffer, size, _TRUNCATE, format, __VA_ARGS__) 
#define SformatInput sscanf
#endif


/*
 * Returns the length of the string.
 */
extern PARSEDLLSPEC int Slength(char *text);


/*
 * Returns < 0 if the first string comes first in alphabetical order.
 * Returns = 0 if the two strings are the same.
 * Returns > 0 if the second string comes first in alphabetical order.
 */
extern PARSEDLLSPEC int Scompare(char *text1, char *text2);


/*
 * Same as Scompare() except that the comparison is done from the end 
 * of the string. Thus CBA comes before BCA.
 *
 * Returns < 0 if the first string comes first in alphabetical order.
 * Returns = 0 if the two strings are the same.
 * Returns > 0 if the second string comes first in alphabetical order.
 */
extern PARSEDLLSPEC int ScompareBackwards(char *text1, char *text2);


/*
 * Returns 0 if the two input strings are different.
 * Returns 1 if they are the same.
 */
extern PARSEDLLSPEC int Sequal(char *text1, char *text2);


/*
 * Copies the source string to the destination.
 */
extern PARSEDLLSPEC void Scopy(char *destination, char *source);


extern PARSEDLLSPEC void Sdestroy(char *text);


/*
 * Creates and copies an existing string.
 */
extern PARSEDLLSPEC char *Sduplicate(char *source);


/*
 * Makes the string empty.
 */
extern PARSEDLLSPEC void Sempty(char *text);


/*
 * Copies the source string to the destination, converting uppercase letters
 * to lowercase letters in the process.
 */
extern PARSEDLLSPEC void Stolower(char *destination, char *source);


/*
 * Copies the source string to the destination, converting lowercase letters
 * to uppercase letters in the process.
 */
extern PARSEDLLSPEC void Stoupper(char *destination, char *source);


/*
 * Truncates the string to the specified length.
 */
extern PARSEDLLSPEC void Struncate(char *text, int length);


/*
 * Appends the second string to the end of the first.
 */
extern PARSEDLLSPEC void Sappend(char *text1, char *text2);


/*
 * Appends the character the end of the text.
 */
extern PARSEDLLSPEC void SappendChar(char *text1, char char1);


/*
 * Removes blanks from both ends of the string.
 */
extern PARSEDLLSPEC void Strim(char *source);


/*
 * Compare two strings in a more liberal manner than Sequal().
 * Spaces and tabs are equivalent.  Spaces at the beginning and end of
 * the strings are ignored.  Any number of spaces are equivalent.
 * Upper and lower case are equivalent.  Returns 1 if the first
 * string is a subset of the second.  Returns zero otherwise.
 */
extern PARSEDLLSPEC int Ssub(char *sub, char *line);


/*
 * Compare two strings in a more liberal manner than Sequal().
 * Spaces and tabs are equivalent.  Spaces at the beginning and end of
 * the strings are ignored.  Any number of spaces are equivalent.
 * Upper and lower case are equivalent.  Returns 1 if the two
 * strings are the same.  Returns zero otherwise.
 */
extern PARSEDLLSPEC int Smatch(char *text1, char *text2);


/*
 * Returns index if the character is a separator.
 * Otherwise returns -1.
 */
extern PARSEDLLSPEC int Sisoneof(char *separator, char character);


/*
 * Strips quotation marks from around the word.
 * Returns 0 if it does it;  -1 if not.
 */
extern PARSEDLLSPEC int Sunquote(char *word);
        
    
/*
 * Chop the input string into words.
 * Words are separated by the specified separator character.
 * Words are grouped by the specified grouping characters.
 * Returns with words in the specified array "word".
 * Returns the number of words found.  
 */
extern PARSEDLLSPEC int Swords(char *text, 
    int condense, char *separator, char *grouper, char *escaper, char **word);

    
/*
 * Counts the words in the input string.
 * Words are separated by the specified separator character.
 * Words are grouped by the specified grouping characters.
 * Returns the number of words found.  
 */
extern PARSEDLLSPEC int ScountWords(char *text, 
    int condense, char *separator, char *grouper, char *escaper);
    
  
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
extern PARSEDLLSPEC int SwordsNoNest(char *text,
    int condense, char *separator, char *grouper, char *escaper, char **word);
    
  
/*
 * Extract the word in the specified position.
 * Uses the same parameters as Swords() and follows
 * the same parsing rules.
 */
extern PARSEDLLSPEC int SextractNoNest(char *text,
    int condense, char *separator, char *grouper, char *escaper, int position,
    char *word);

    
/*
 * Extract the word in the specified position.
 * Uses the same parameters as Swords() and follows
 * the same parsing rules.
 */
extern PARSEDLLSPEC int Sextract(char *text,
    int condense, char *separator, char *grouper, char *escaper, int position,
    char *word);
    

/*
 * Blanks the word in the specified position.
 * Uses the same parameters as Swords() and follows
 * the same parsing rules.
 */
extern PARSEDLLSPEC int SblankWord(char *text,
    int condense, char *separator, char *grouper, char *escaper, int position);
    

/*
 * Blanks the separator in the specified position.
 * Uses the same parameters as Swords() and follows
 * the same parsing rules.
 */
extern PARSEDLLSPEC int SblankSeparator(char *text,
    char *separator, char *grouper, char *escaper, int position);
    

extern PARSEDLLSPEC int SnoBlank(char *result, char *source);


extern PARSEDLLSPEC int Sgreater(char *string1, char *string2);

//
// removes eol, cr, space, and tab from end of input line
// alters the input buffer
// returns the new length of the buffer
//
extern PARSEDLLSPEC int StrimEnd(char *buffer);


#ifdef __cplusplus
}
#endif



