

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#if defined(LINUX) || defined(__APPLE__)
#include <unistd.h>
#endif

#include "smatch.h"
#include "CommandParse.h"

#define MBUFFER 1024
#define MFRAME 20
#ifdef LINUX
#define MARG 100
#else
#define MARG 1024
#endif
#define MVALUE 100
#define MWORD 100

struct _Parameter
{
    char name[MWORD];
    int nvalue;
    char value[MVALUE][MWORD];
};


//
// the latest command word
//
static char Word[MWORD];

//
// the number of saved parameter sets
//
static int FrameMany=0;

// 
// the last parameter in each frame
//
static int FrameTop[MFRAME];

//
// the total number of parameters
// the total is the sum of all of the saved sets plus the current set
//
static int ArgMany=0;

//
// the list of all of the parameters
//
static struct _Parameter Arg[MARG];

//
// a copy of the original command input, used in done messages
//
static char *_CommandInput;

//
// a copy of the original command input after variable translation
//
static char *_CommandInputTranslated;


//
// Scans the input string searching for $.
// Replaces $$ with single $.
// Replaces $name with corresponding value.
// Does not do any substitution inside "" or ''.
//
// Returns pointer to buffer.
//
// Actualy the supplied function can do anything it wants. 
// The above description is what is expected and done by cart.
//
static char * (*_Replacement)(char *name, char *buffer, int max);

//
// this function massages the input by replacing any occurence of $NAME with the value
//
static char * (*_ValueReplacement)(char *name, char *buffer, int max);


PARSEDLLSPEC void CommandReplacement(char * (*f)(char *name, char *buffer, int max))
{
    _Replacement=f;
}


PARSEDLLSPEC void CommandParameterReplacement(char * (*f)(char *name, char *buffer, int max))
{
    _ValueReplacement=f;
}


PARSEDLLSPEC char *CommandInput()
{
	return _CommandInput;
}


PARSEDLLSPEC char *CommandInputTranslated()
{
	return _CommandInputTranslated;
}


PARSEDLLSPEC int CommandPush()
{
    if(FrameMany<MFRAME-1)
    {
        FrameTop[FrameMany]=ArgMany;
        FrameMany++;
        return FrameMany;
    }
    return -1;
}


PARSEDLLSPEC int CommandPop()
{
    if(FrameMany>0)
    {
        FrameMany--;
        if(FrameMany<=0)
        {
            FrameMany=0;
            ArgMany=0;
        }
        else
        {
            ArgMany=FrameTop[FrameMany-1];
        }
    }
    return FrameMany;
}


PARSEDLLSPEC char *CommandWord()
{
    return Word;
}


PARSEDLLSPEC int CommandParameterMany()
{
    return ArgMany;
}


PARSEDLLSPEC char *CommandParameterName(int arg)
{
    if(arg>=0 && arg<ArgMany)
    {
        return Arg[arg].name;
    }
    return 0;
}

PARSEDLLSPEC int CommandParameterNameIsSelected(char *name)
{
    char argNameL[256];
	int arg;

    for (arg=0; arg<ArgMany; arg++)
    {
		Stolower(argNameL, Arg[arg].name);
        if (Sequal(argNameL, name))
			return 1;
    }
    return 0;
}

PARSEDLLSPEC int CommandParameterValueMany(int arg)
{
    if(arg>=0 && arg<ArgMany)
    {
        return Arg[arg].nvalue;
    }
    return 0;
}


static char *CommandParameterValueInternal(int arg, int value, char * (*rf)(char *name, char *buffer, int max))
{
	static char buffer[MBUFFER];

    if(arg>=0 && arg<ArgMany)
    {
        if (Arg[arg].nvalue==0) // in case of param=""; nvalue=0, but value is whatever last time set.
            return 0;
        if(rf!=0)
        {
            rf(Arg[arg].value[value],buffer,MBUFFER);
            return buffer;
        }
        else
        {
            return Arg[arg].value[value];
        }
    }
    return 0;
}

PARSEDLLSPEC char *CommandParameterValue(int arg, int value)
{
	return CommandParameterValueInternal(arg,value,_ValueReplacement);
}

PARSEDLLSPEC char *CommandParameterValueNoReplacement(int arg, int value)
{
	return CommandParameterValueInternal(arg,value,0);
}

PARSEDLLSPEC void backwardSetCmdProcess(char *buffer)
{
	int iIndex=0;
	char *pt, *pt1;
	char buff[MBUFFER], tmp[MBUFFER];
	strcpy(buff, buffer);
	if (strstr(buff, "set")>0) {	// for set cmd
		if (strstr(buff, "caltgtpwr")>0 ||															// target power set
			strstr(buff, "CtlFreq")>0 || strstr(buff, "ctlflag")>0 || strstr(buff, "ctlpwr")>0) {	// ctl set
			if (strstr(buff, "v.(")>0) {		
			// yes this is old set target power arr format.(set caltgtpwr2g=v.(18,18,17,16),f.2;)
				// find freq index, if no f.N defined take iChann=0
				if (strstr(buff, "caltgtpwr")>0) {	// target power set
					if ((pt = strstr(buff, "f."))) {
						SformatOutput(tmp,MBUFFER-1,"%s",pt+2);
						*pt=0;
						if ((pt=strstr(tmp, ";")))	// if there is ; at the end, take it out.
							*pt=0;
						iIndex=atoi(tmp);
					} 
				} else {	// ctl set
					if ((pt = strstr(buff, "ctl."))) {
						SformatOutput(tmp,MBUFFER-1,"%s",pt+4);
						*pt=0;
						if ((pt=strstr(tmp, ";")))	// if there is ; at the end, take it out.
							*pt=0;
						iIndex=atoi(tmp);
					} 
				}
				// set buffer to new format
				strcpy(tmp, buffer);
				if ((pt=strstr(buff, ")"))) {	// cut all after ) in buff
					*pt=0;
					pt1=strstr(buff, "(");	// get all the values between()in pt1
					if ((pt=strstr(tmp, "="))) {	// get all before = int tmp
						*pt=0;
						if (!strstr(buffer, "ctlpwr")>0)
							SformatOutput(buffer,MBUFFER-1,"%s[%i]=%s", tmp,iIndex,pt1+1);
						else {	// for ctlpwr, need to convert hex to float and /2.0 for old format (set ctlpwr2g=v.(0x3a,0x3a,0x3a,0x3a),ctl.2)
							//ngot=sscanf(tmin, " %x %1c",&vmin,&extra);
							int ngot=0, i;
							int value[8];
							float fvalue[8];
							if (strstr(buffer, "ctlpwr2g")>0) {
								strcpy(buff, pt1+1);
								ngot=sscanf(buff, " 0x%x,0x%x,0x%x,0x%x", &value[0], &value[1], &value[2], &value[3]);
							} else {
								ngot=sscanf(buff, " 0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x", 
									&value[0], &value[1], &value[2], &value[3],&value[4], &value[5], &value[6], &value[7]);
							}
							for (i=0; i<8; i++)
								fvalue[i] = (float)(value[i]/2.0);
							if (strstr(buffer, "ctlpwr2g")>0) 
								SformatOutput(buffer,MBUFFER-1,"%s[%i]=%.1f,%.1f,%.1f,%.1f", 
									tmp,iIndex,fvalue[0],fvalue[1],fvalue[2],fvalue[3]);
							else
								SformatOutput(buffer,MBUFFER-1,"%s[%i]=%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f", 
									tmp,iIndex,fvalue[0],fvalue[1],fvalue[2],fvalue[3],fvalue[4],fvalue[5],fvalue[6],fvalue[7]);
						}
					}
				}
			}
		}
	}
}
//
// Parses the input buffer into a command Word and parameter name value pairs.
// Format is the following:
//
//    command parameter=value; parameter=value; ... 
//
// Equal signs and semicolons are required. 
// Command and parameter names may not include spaces.
// Values may contain anything except leading spaces and semicolons.
//
PARSEDLLSPEC int CommandParse(char *input)
{
    char *ptr,*word,*pname,*pvalue,*sptr;
    int equal;
    int done;
	int group;
	int gstart[1000];
    int gskip;
	int noEqualSign = 0;
	char changed[MBUFFER];
	char *buffer;
	//
	// Save the input
	//
	if(_CommandInput!=0)
	{
		Sdestroy(_CommandInput);
	}
	_CommandInput=Sduplicate(input);
    //
	// Replace any $name with value.
	//
	if(_CommandInputTranslated!=0)
	{
		Sdestroy(_CommandInputTranslated);
	}
	_CommandInputTranslated=0;
	if(_Replacement!=0)
	{
		buffer=(*_Replacement)(input,changed,MBUFFER);
		_CommandInputTranslated=Sduplicate(buffer);
	}
	else
	{
		buffer=input;
	}

	//
	// THIS SHOULDN"T BE HERE SINCE IT IS NART SPECIFIC, NOT A GENERAL PURPOSE FUNCTION.
	//
	backwardSetCmdProcess(buffer);

    //
    // Make sure frame stack is initialized.
    // First frame is unused.
    //
    if(FrameMany<=0)
    {
        FrameMany=0;
        FrameTop[0]=0;
        ArgMany=0;
    }
    else
    {
        ArgMany=FrameTop[FrameMany-1];
    }
    //
    // find beginning of command Word
    //
	group=0;
    word=Word;
    *word=0;
    for(ptr=buffer; *ptr!=0; ptr++)
    {
        if(!(*ptr==' ' || *ptr=='\t'))
        {
//x            *word= *ptr;
//x            word++;
//x            *word=0;
//x            ptr++;
            break;
        }
    }
    //
    // find end of command Word and put a NULL there
    //
    for( ; *ptr!=0 ; ptr++)
    {
        if(*ptr==' ' || *ptr=='\t')
        {
            *word=0;
            ptr++;
            break;
        }
        *word= *ptr;
        word++;
        *word=0;
    }
    //
    // now we go looking for parameter=value pairs
    // they end with semicolons
    //
    for( ; ArgMany<MARG && *ptr!=0; ArgMany++)
    {
		noEqualSign = 0;	// reset it for parameter has equalSign and previous paramter has noEqualSign.
        //
        // find beginning of parameter name
        //
        pname=Arg[ArgMany].name;
        *pname=0;
        Arg[ArgMany].nvalue=0;
        for(; *ptr!=0; ptr++)
        {
            if(!(*ptr==' ' || *ptr=='\t'))
            {
//x                *pname= *ptr;
//x                pname++;
//x                *pname=0;
//x                ptr++;
                break;
            }
        }
        //
        // find end of name and put a NULL there.
        // no spaces allowed in names.
        //
        equal=0;
        for( ; *ptr!=0 ; ptr++)
        {
#ifdef UNUSED
            if(*ptr==' ' || *ptr=='\t')
            {
                equal=0;
                *pname=0;
                ptr++;
                break;
            }
#endif
  //        if(*ptr=='=')	// get sometime doesn't have =
            if(*ptr=='=' || *ptr==';')
            {
				if (*ptr==';')
					noEqualSign = 1;
                equal=1;
                *pname=0;
                //
                // trim spaces off the back end of the command word
                //
                for(sptr=ptr-1; *sptr!=0 ; sptr--)
                {
                    if(*sptr==' ' || *sptr=='\t')
                    {
                        *sptr=0;
                    }
                    else
                    {
                        break;
                    }
                }
                ptr++;
                break;
            }
            *pname= *ptr;
            pname++;
            *pname=0;
        }
		if (noEqualSign)
			continue;
#ifdef UNUSED
        //
        // now go looking for an equal sign if
        // we didn't find it up above
        //
        if(!equal)
        {
            for( ; *ptr!=0 ; ptr++)
            {
                if(*ptr=='=')
                {
                    ptr++;
                    break;
                }
            }
        }
#endif
        //
        // skip leading spaces on parameter values
        //
        for(Arg[ArgMany].nvalue=0; Arg[ArgMany].nvalue<MARG && *ptr!=0; )
        {
            pvalue=Arg[ArgMany].value[Arg[ArgMany].nvalue];
            for( ; *ptr!=0; ptr++)
            {
                if(!(*ptr==' ' || *ptr=='\t'))
                {
//x                    *pvalue= *ptr;
//x                    pvalue++;
//x                    *pvalue=0;
                    Arg[ArgMany].nvalue++;
//x                    ptr++;
                    break;
                }
            }
            //
            // now go looking for a semicolon or comma
            //
            for( ; *ptr!=0 ; ptr++)
            {
 				//
				// end of a group?
				//
				if((*ptr==')' && gstart[group-1]=='(') ||
				    (*ptr==']' && gstart[group-1]=='[') ||
				    (*ptr=='"' && gstart[group-1]=='"') ||
				    (*ptr=='\'' && gstart[group-1]=='\''))
				{
					group--;
					//
					// and skip the closer for the first group WHY DO WE DO THIS?
					//
					if(group<=0 && gskip)
					{
						group=0;
						continue;
					}
				}
				//
				// start of a group?
				//
				else if(*ptr=='(' || *ptr=='[' || *ptr=='"' || *ptr=='\'')
				{
					gstart[group]=*ptr;
					group++;
					//
					// skip the first grouping character
					//
					if(group==1)
                    {
                        if((gstart[0]=='\'' || gstart[0]=='"') && pvalue==Arg[ArgMany].value[Arg[ArgMany].nvalue-1])
					    {
                            gskip=1;
						    continue;
					    }
                        else
                        {
                            gskip=0;
                        }
                    }
				}
				//
				// if not in a group, look for , or ;
				//
				if(group<=0)
				{
                    if(*ptr==',')
					{
                        *pvalue=0;
                        ptr++;
                        done=0;
                        break;
					}
                    else if(*ptr==';')
					{
                        *pvalue=0;
                        ptr++;
                        done=1;
                        break;
					}
				}
                *pvalue= *ptr;
                pvalue++;
                *pvalue=0;
            }
            if(done || *ptr==0)
            {
                break;
            }
        }
        //
        // we're done with that one
        // maybe there are some more
        //
    }
    return ArgMany;
}



