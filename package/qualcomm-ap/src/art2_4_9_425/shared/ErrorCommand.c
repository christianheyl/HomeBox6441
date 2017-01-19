

#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

#include "ParseError.h"
#include "CommandParse.h"
#include "ParameterSelect.h"
#include "ParameterParse.h"
#include "smatch.h"
#include "ErrorPrint.h"
#include "ErrorCommand.h"

#define MBUFFER 1024


#define ErrorResponseAll 100
#define ErrorResponseNone 101
#define ErrorResponseNormal 102

static struct _ParameterList ResponseParameter[]=
{
	{ErrorResponseShowCode,{"code",0,0},"show the 4 digit message code",0,0,0,0,0,0,0,0,0,0},
	{ErrorResponseShowType,{"type",0,0},"show the message type severity",0,0,0,0,0,0,0,0,0,0},
	{ErrorResponseShowMessage,{"message",0,0},"show the actual message",0,0,0,0,0,0,0,0,0,0},
	{ErrorResponsePause,{"pause",0,0},"pause and wait for user response",0,0,0,0,0,0,0,0,0,0},
	{ErrorResponseBell,{"bell",0,0},"ring the bell",0,0,0,0,0,0,0,0,0,0},
	{ErrorResponseLog,{"log","file",0},"append message to the current log file",0,0,0,0,0,0,0,0,0,0},
	{ErrorResponseAll,{"all",0,0},"same as code+type+message",0,0,0,0,0,0,0,0,0,0},
	{ErrorResponseNone,{"none",0,0},"ignore error message",0,0,0,0,0,0,0,0,0,0},
	{ErrorResponseNormal,{"normal",0,0},"do the normal response",0,0,0,0,0,0,0,0,0,0},
};


static struct _ParameterList TypeParameter[]=
{
	{ErrorDebug,{ErrorDebugText,0,0},0,0,0,0,0,0,0,0,0,0,0},
	{ErrorControl,{ErrorControlText,0,0},0,0,0,0,0,0,0,0,0,0,0},
	{ErrorInformation,{ErrorInformationText,0,0},0,0,0,0,0,0,0,0,0,0,0},
	{ErrorWarning,{ErrorWarningText,0,0},0,0,0,0,0,0,0,0,0,0,0},
	{ErrorFatal,{ErrorFatalText,0,0},0,0,0,0,0,0,0,0,0,0,0},
};


enum
{
    ErrorCode=0,
    ErrorType,
    ErrorResponse,
	ErrorList,
	ErrorShort,
};


static struct _ParameterList LogicalParameter[]=
{
	{0,{"no",0,0},0,0,0,0,0,0,0,0,0,0,0},
	{1,{"yes",0,0},0,0,0,0,0,0,0,0,0,0,0},
};

#define MCODE 100
#define MTYPE 10

static struct _ParameterList ErrorParameter[]=
{
    {ErrorCode,{"code","number",0},"the individual message code or number",0,0,MCODE,1,1,0,0,0,0,0},
    {ErrorType,{"type",0,0},"the message type",'z',0,1,1,1,0,0,0,
	    sizeof(TypeParameter)/sizeof(TypeParameter[0]),TypeParameter},
    {ErrorResponse,{"response",0,0},"the response",'z',0,MTYPE,1,1,0,0,0, 
	    sizeof(ResponseParameter)/sizeof(ResponseParameter[0]),ResponseParameter},
    {ErrorList,{"list",0,0},"list all of the matching error messages",'z',0,MTYPE,1,1,0,0,0, 
	    sizeof(LogicalParameter)/sizeof(LogicalParameter[0]),LogicalParameter},
    {ErrorShort,{"short",0,0},"use short format?",'z',0,1,1,1,0,0,0, 
	    sizeof(LogicalParameter)/sizeof(LogicalParameter[0]),LogicalParameter},
};


PARSEDLLSPEC void ErrorParameterSplice(struct _ParameterList *list)
{
    list->nspecial=sizeof(ErrorParameter)/sizeof(ErrorParameter[0]);
    list->special=ErrorParameter;
}

//
// Adjust which error messages are shown to the user.
//
PARSEDLLSPEC void ErrorCommand()
{
    int ip, iv;
    int nparam;
    int error;
    char *name;
	int ncode,code[MCODE];
    int ntype,type[MCODE];
    int nresponse,response[MCODE];
    unsigned short rcode,rtype,rdebug;
    int selection;
	int nlist,dolist;
	int index;
	int nshort,doshort;
    //
	// Parse the user input
	//
	nlist=0;
	dolist=0;
    ncode= 0;
    ntype= 0;
    error=0;
    rcode=0;
	doshort=0;
	nshort=0;
    rtype=((1<<ErrorResponseShowCode)|(1<<ErrorResponseShowType)|(1<<ErrorResponseShowMessage));
    rdebug=0;
	nparam=CommandParameterMany();
	for(ip=0; ip<nparam; ip++)
	{
		name=CommandParameterName(ip);
        index=ParameterSelectIndex(name,ErrorParameter,sizeof(ErrorParameter)/sizeof(struct _ParameterList));
		if(index>=0)
		{
			selection=ErrorParameter[index].code;
			switch(selection)
			{
				case ErrorList:
 					nlist=ParseIntegerList(ip,name,&dolist,&ErrorParameter[index]);
					if(nlist<=0)
					{
						ErrorPrint(ParseBadParameter,name);
						error++;
					}
					break;
				case ErrorShort:
 					nshort=ParseIntegerList(ip,name,&doshort,&ErrorParameter[index]);
					if(nshort<=0)
					{
						ErrorPrint(ParseBadParameter,name);
						error++;
						break;
					}
					break;
				case ErrorCode:
 					ncode=ParseIntegerList(ip,name,code,&ErrorParameter[index]);
					if(ncode<=0)
					{
						ErrorPrint(ParseBadParameter,name);
						error++;
						break;
					}
					break;
				case ErrorType:
 					ntype=ParseIntegerList(ip,name,type,&ErrorParameter[index]);
					if(ntype<=0)
					{
						ErrorPrint(ParseBadParameter,name);
						error++;
					}
					break;
				case ErrorResponse:
 					nresponse=ParseIntegerList(ip,name,response,&ErrorParameter[index]);
					if(nresponse<=0)
					{
						ErrorPrint(ParseBadParameter,name);
						error++;
					}
					break;
				default:
					ErrorPrint(ParseBadParameter,name);
					error++;
					break;
			}
		}
		else
		{
			ErrorPrint(ParseBadParameter,name);
			error++;
			break;
		}
    }
        
    if(error<=0)
    {
		if(nshort>0)
		{
			ErrorTypeAbbreviation(doshort);
		}

		if(dolist)
		{
			if(ntype>=0)
			{
				for(iv=0; iv<ntype; iv++)
				{
					ErrorListType(type[iv]);
				}
			}
			if(ncode>=0)
			{
				for(iv=0; iv<ncode; iv++)
				{
					ErrorListCode(code[iv]);
				}
			}
			if(ncode<=0&& ntype<=0)
			{
				ErrorListAll();
			}
		}

		if(nresponse>0)
		{
			rcode=0;
			rtype=0;
			rdebug=0;
			for(iv=0; iv<nresponse; iv++)
			{
				selection=response[iv];
				if(selection==ErrorResponseNone)
				{
					rcode=(1<<ErrorResponseSpecial);
					rtype=(1<<ErrorResponseSpecial);
					rdebug=(1<<ErrorResponseSpecial);
				}
				else if(selection==ErrorResponseNormal)
				{
					rcode=0;
					rtype=((1<<ErrorResponseShowCode)|(1<<ErrorResponseShowType)|(1<<ErrorResponseShowMessage));
					rdebug=0;
				}
				else if(selection==ErrorResponseAll)
				{
					rcode|=((1<<ErrorResponseShowCode)|(1<<ErrorResponseShowType)|(1<<ErrorResponseShowMessage));
					rtype|=((1<<ErrorResponseShowCode)|(1<<ErrorResponseShowType)|(1<<ErrorResponseShowMessage));
					rdebug|=((1<<ErrorResponseShowCode)|(1<<ErrorResponseShowType)|(1<<ErrorResponseShowMessage));
				}
				else
				{
					rcode|=(1<<selection);
					rtype|=(1<<selection);
					rdebug|=(1<<selection);
				}
			}

			if(ntype>=0)
			{
				for(iv=0; iv<ntype; iv++)
				{
					if(type[iv]==ErrorDebug || type[iv]==ErrorControl)
					{
						ErrorTypeResponseSet(type[iv],rdebug);
					}
					else
					{
						ErrorTypeResponseSet(type[iv],rtype);
					}
				}
			}
			if(ncode>=0)
			{
				for(iv=0; iv<ncode; iv++)
				{
					ErrorResponseSet(code[iv],rcode);
				}
			}
		}
    }
}


