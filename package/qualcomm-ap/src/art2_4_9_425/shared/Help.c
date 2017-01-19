
// "ACI $Id: //depot/sw/branches/art2_main_per_cs/src/art2/shared/Help.c#1 $, $Header: //depot/sw/branches/art2_main_per_cs/src/art2/shared/Help.c#1 $"

#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>


#include "smatch.h"
#include "ErrorPrint.h"
#include "ParseError.h"
#include "Help.h"

#define MBUFFER 1024

static FILE *HelpFile;


PARSEDLLSPEC void HelpClose()
{
	if(HelpFile!=0)
	{
		fclose(HelpFile);
		HelpFile=0;
	}
}


static void HelpRewind()
{
	if(HelpFile!=0)
	{
		fseek(HelpFile,0,SEEK_SET);
	}
}



PARSEDLLSPEC int HelpOpen(char *filename)
{
	if(HelpFile==0)
	{
		HelpFile=fopen(filename,"r");
	    if(HelpFile!=0)
		{
			return 0;
		}
        else
        {
            return -1;
        }
    }
    return 0;
}


static int HelpRead(char *buffer, int maxlen)
{
	char *eof;
	int length;

	if(HelpFile!=0)
	{
		buffer[0]=0;
		eof=fgets(buffer,maxlen,HelpFile);
		if(eof!=buffer)
		{
			return -1;
		}
		else
		{
			length=StrimEnd(buffer);
			return length;
		}
	}
	return -1;
}


PARSEDLLSPEC int Help(char *name, void (*print)(char *buffer))
{
	char buffer[MBUFFER];
	char start[MBUFFER];
	int count;
    int found;

	if(print!=0)
	{
		count=0;
        found=0;

		if(HelpFile==0)
		{
			HelpOpen("nart.help");
		}

		if(HelpFile!=0)
		{
			SformatOutput(start,MBUFFER-1,"<tag=%s>",name);
			buffer[MBUFFER-1]=0;

			HelpRewind();
			//
			// look for start of documentation section, <tag=name>
			//
			while(HelpRead(buffer,MBUFFER-1)>=0)
			{
				if(Smatch(buffer,start))
				{
                    found=1;
					break;
				}
			}
			//
			// print all lines up to the next <tag=anything>
			//
            if(found)
            {
                ErrorPrint(ParseHelpDescriptionStart);

			    while(HelpRead(buffer,MBUFFER-1)>=0)
			    {
				    if(strncmp(buffer,"<tag=",5)==0)
				    {
					    break;
				    }
				    ErrorPrint(ParseHelp,buffer);
				    count++;
			    }
				ErrorPrint(ParseHelpDescriptionEnd);
			    return count;
            }
		}
	}

	return -1;
}


PARSEDLLSPEC int HelpIndex(void (*print)(char *buffer))
{
	char buffer[MBUFFER];
	int count;
	char *end;

	if(print!=0)
	{
		count=0;
 
		if(HelpFile==0)
		{
			HelpOpen("nart.help");
		}

		if(HelpFile!=0)
		{
			HelpRewind();
			//
			// look for start of documentation section, <tag=name>
			//
			while(HelpRead(buffer,MBUFFER-1)>=0)
			{
			    if(strncmp(buffer,"<tag=",5)==0)
			    {
					end=strchr(buffer,'>');
					if(end!=0)
					{
						*end=0;
					}
					ErrorPrint(ParseHelp,&buffer[5]);
					count++;
			    }
            }
		}
	}

	return count;
}

