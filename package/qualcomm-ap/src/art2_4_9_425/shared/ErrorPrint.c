

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "smatch.h"
#include "UserPrint.h"
#include "ErrorPrint.h"



#ifdef WINDOWS
#include <windows.h>

DWORD GetTickCount(void);
#endif

#define MBUFFER 1024

#define EHASH 101                   // should be prime number
#define EENTRY 10

struct _ErrorHash
{
    int max;
    int many;
    struct _Error **fptr;
};

static struct _ErrorHash EH[EHASH];

// 
// bit mask of classes that we will print. may be changed by user command.
//
static int _ErrorResponse[]=
{
    0,                                                              // ErrorDebug
    0,                                                              // ErrorControl
    (1<<ErrorResponseShowCode)|(1<<ErrorResponseShowType)|(1<<ErrorResponseShowMessage),     // ErrorInformation
    (1<<ErrorResponseShowCode)|(1<<ErrorResponseShowType)|(1<<ErrorResponseShowMessage),     // ErrorWarning
    (1<<ErrorResponseShowCode)|(1<<ErrorResponseShowType)|(1<<ErrorResponseShowMessage)|(1<<ErrorResponseLog),     // ErrorFatal
};           

static int _ErrorFirst=1;

static struct _Error _ErrorStandard[]=
{
    {ErrorNone,ErrorDebug,"%s"},
    {ErrorUnknown,ErrorWarning,"Unknown error %d."},
};


static void (*_ErrorPause)(void);
static void (*_ErrorBell)(void);
static void (*_ErrorLog)(char *buffer);
static void (*_ErrorPrint)(char *buffer)=UserPrintIt;


PARSEDLLSPEC void ErrorPrintFunction(void (*print)(char *buffer))
{
	_ErrorPrint=print;
}


PARSEDLLSPEC void ErrorLogFunction(void (*log)(char *buffer))
{
	_ErrorLog=log;
}


static int ErrorHashKey(int code)
{
    int key;

    key=code%EHASH;
    if(key<0)
    {
        key+=EHASH;
    }

    return key;
}


static void ErrorHashDestroy()
{
    int it;

    for(it=0; it<EHASH; it++)
    {
        if(EH[it].fptr!=0)
        {
            free(EH[it].fptr);
            EH[it].fptr=0;
            EH[it].max=0;
            EH[it].many=0;
        }
    }
}


PARSEDLLSPEC int ErrorHashCreate(struct _Error *m, int many)
{
    struct _Error **fpnew;
    int it, ip;
    int key;

    if(_ErrorFirst)
    {
        ErrorInit();
    }

    for(it=0; it<many; it++)
    {
        //
        // compute hash key
        //
        key=ErrorHashKey(m[it].code);
        //
        // see if we need to create more room
        //
        if(EH[key].many>=EH[key].max)
        {
            fpnew=(struct _Error **)malloc((EH[key].max+EENTRY)*sizeof(struct _Error *));
            if(fpnew!=0)
            {
                if(EH[key].many>0 && EH[key].fptr!=0)
                {
                    for(ip=0; ip<EH[key].many; ip++)
                    {
                        fpnew[ip]=EH[key].fptr[ip];
                    }
                    free(EH[key].fptr);
                }
                EH[key].fptr=fpnew;
                EH[key].max+=EENTRY;
            }
            else
            {
                ErrorHashDestroy();
                return -1;
            }
        }
        //
        // now we should be able to stick the new one in the structure
        //
        EH[key].fptr[EH[key].many]= &m[it];
        EH[key].many++;
    }

    return 0;
}


PARSEDLLSPEC void ErrorInit()
{
    _ErrorFirst=0;
    ErrorHashCreate(_ErrorStandard,sizeof(_ErrorStandard)/sizeof(_ErrorStandard[0]));
}


static struct _Error *ErrorFind(int code)
{
	int it;
    int key;
 
    key=ErrorHashKey(code);
    for(it=0; it<EH[key].many; it++)
    {
	    if(EH[key].fptr[it]->code==code)
		{
            return EH[key].fptr[it];
		}
    }
    return 0;
}


PARSEDLLSPEC int ErrorTypeResponseGet(int type)
{
    if(type<sizeof(_ErrorResponse)/sizeof(_ErrorResponse[0]))
    {
        return _ErrorResponse[type];
    }
    return 0;
}


PARSEDLLSPEC int ErrorTypeResponseSet(int type, int response)
{
    if(type<sizeof(_ErrorResponse)/sizeof(_ErrorResponse[0]))
    {
        response&=(~(1<<ErrorResponseSpecial));
        _ErrorResponse[type]=response;
        return _ErrorResponse[type];
    }
    return 0;
}


PARSEDLLSPEC int ErrorResponseGet(int code)
{
    struct _Error *mptr;

    mptr=ErrorFind(code);
    if(mptr!=0)
    {
        return (mptr->type>>8)&0xff;
    }
    return 0;
}


PARSEDLLSPEC int ErrorResponseSet(int code, int response)
{
    struct _Error *mptr;

    mptr=ErrorFind(code);
    if(mptr!=0)
    {
        if(response!=0)
        {
            response|=(1<<ErrorResponseSpecial);
        }
        mptr->type=(mptr->type&0xff)|((response&0xff)<<8);
        return (mptr->type>>8)&0xff;
    }
    return 0;
}


static int UseAbbreviation=0;


PARSEDLLSPEC char *ErrorTypePrint(int type)
{
	if(UseAbbreviation)
	{
		switch(type)
		{
			case ErrorDebug:
				return ErrorDebugAbbreviation;
			case ErrorControl:
				return ErrorControlAbbreviation;
			case ErrorInformation:
				return ErrorInformationAbbreviation;
			case ErrorWarning:
				return ErrorWarningAbbreviation;
			case ErrorFatal:
				return ErrorFatalAbbreviation;
			default:
				return "U";
		}
	}
	else
	{
		switch(type)
		{
			case ErrorDebug:
				return ErrorDebugText;
			case ErrorControl:
				return ErrorControlText;
			case ErrorInformation:
				return ErrorInformationText;
			case ErrorWarning:
				return ErrorWarningText;
			case ErrorFatal:
				return ErrorFatalText;
			default:
				return "UNKNOWN";
		}
	}
}


PARSEDLLSPEC void ErrorTypeAbbreviation(int onoff)
{
	UseAbbreviation=onoff;
}


PARSEDLLSPEC int ErrorTypeParse(char *text)
{
	if(Smatch(text,ErrorFatalText)||Smatch(text,ErrorFatalAbbreviation))
	{
        return ErrorFatal;
	}
	else if(Smatch(text,ErrorWarningText)||Smatch(text,ErrorWarningAbbreviation))
	{
        return ErrorWarning;
	}
	else if(Smatch(text,ErrorInformationText)||Smatch(text,ErrorInformationAbbreviation))
	{
        return ErrorInformation;
	}
	else if(Smatch(text,ErrorControlText)||Smatch(text,ErrorControlText))
	{
        return ErrorControl;
	}
	else 
	{
        return ErrorDebug;
	}
}


//
// set the function called to execute a pause
//
PARSEDLLSPEC void ErrorPauseFunction(void (*pause)(void))
{
    _ErrorPause=pause;
}

//
// set the function called to ring the bell
//
PARSEDLLSPEC void ErrorBellFunction(void (*bell)(void))
{
    _ErrorBell=bell;
}

static void ErrorPrintIt(int art, int code, int type, int response, char *buffer)
{
	char full[MBUFFER];
	char *eol="\n",*blank="",*use;
	int length;
	int lc;
	// DWORD ticks = 0;

    //
    // suppress end of line if alraedy included in the message
    //
    length=Slength(buffer);
    if(buffer[length-1]=='\n')
    {
		use=blank;
    }
	else
	{
		use=eol;
	}
	//
	// prepend the art number
	//
	if(art>=0)
	{
		if(UseAbbreviation)
		{
			lc=SformatOutput(full,MBUFFER-1,"%d-",art);
			full[MBUFFER-1]=0;
		}
		else
		{
			lc=SformatOutput(full,MBUFFER-1,"[%d] ",art);
			full[MBUFFER-1]=0;
		}
	}
	else
	{
		lc=0;
	}

#ifdef WINDOWS	
	//ticks = GetTickCount();
	//printf("%d.%d\n", ticks/1000, ticks%1000);
#endif
	
	//
    // put code and type on the front and print it
    //
	if(UseAbbreviation)
	{
		if((response&(1<<ErrorResponseShowCode)) && (response&(1<<ErrorResponseShowType)))
		{
			SformatOutput(&full[lc],MBUFFER-lc-1,"%04d%s %s%s",code,ErrorTypePrint(type),buffer,use);
		}
		else if((response&(1<<ErrorResponseShowCode)))
		{
			SformatOutput(&full[lc],MBUFFER-lc-1,"%04d %s%s",code,buffer,use);
		}
		else if((response&(1<<ErrorResponseShowType)))
		{
			SformatOutput(&full[lc],MBUFFER-lc-1,"%s %s%s",ErrorTypePrint(type),buffer,use);
		}
		else
		{
			SformatOutput(&full[lc],MBUFFER-lc-1,"%s%s",buffer,use);
		}
	}
	else
	{
		if((response&(1<<ErrorResponseShowCode)) && (response&(1<<ErrorResponseShowType)))
		{
			SformatOutput(&full[lc],MBUFFER-lc-1,"%04d %s %s%s",code,ErrorTypePrint(type),buffer,use);
		}
		else if((response&(1<<ErrorResponseShowCode)))
		{
			SformatOutput(&full[lc],MBUFFER-lc-1,"%04d %s%s",code,buffer,use);
		}
		else if((response&(1<<ErrorResponseShowType)))
		{
			SformatOutput(&full[lc],MBUFFER-lc-1,"%s %s%s",ErrorTypePrint(type),buffer,use);
		}
		else
		{
			SformatOutput(&full[lc],MBUFFER-lc-1,"%s%s",buffer,use);
		}
	}
    full[MBUFFER-1]=0;
    //
    // decide what to do with it
    //
	if(_ErrorPrint!=0)
	{
		(_ErrorPrint)(full);
	}
    //
    // should we put it in a file
    //
    if((response&(1<<ErrorResponseLog)) && _ErrorLog!=0)
    {
        (*_ErrorLog)(full);
    }
    //
    // should we ring the bell
    //
    if((response&(1<<ErrorResponseBell)) && _ErrorBell!=0)
    {
        (*_ErrorBell)();
    }
    //
    // should we pause for user input
    //
    if((response&(1<<ErrorResponsePause)) && _ErrorPause!=0)
    {
        (*_ErrorPause)();
    }

}


PARSEDLLSPEC int ErrorPrint(int code, ...)
{
    va_list ap;
    char buffer[MBUFFER];
    struct _Error *mptr;
    int type;
    int response;
	char *format;
	char uformat[MBUFFER];

    if(_ErrorFirst)
    {
        ErrorInit();
    }

    mptr=ErrorFind(code);
    if(mptr==0)
    {
		type=ErrorWarning;
		response=(1<<ErrorResponseShowCode)|(1<<ErrorResponseShowType)|(1<<ErrorResponseShowMessage);
		SformatOutput(uformat,MBUFFER-1,"Unknown error.");
		uformat[MBUFFER-1]=0;
		format=uformat;
    }
    else
    {
        type=mptr->type&0xff;
        response=(mptr->type>>8)&0xff;
        if(response==0)
        {
            if(type>=0 && type<sizeof(_ErrorResponse)/sizeof(_ErrorResponse[0]))
            {
                response=_ErrorResponse[type];
            }
            else
            {
                response=0;
            }
        }
		format=mptr->format;
	}
    response&=(~(1<<ErrorResponseSpecial));
    //
    // here is where we format the message
    //
    if(response!=0)
    {
        if(response&(1<<ErrorResponseShowMessage))
        {
            va_start(ap, code);
#if defined(LINUX) || defined(__APPLE__)
    		vsnprintf(buffer,MBUFFER-1,format,ap);
#else
    		_vsnprintf(buffer,MBUFFER-1,format,ap);
#endif
            va_end(ap);
            buffer[MBUFFER-1]=0;
        }
        else
        {
            buffer[0]=0;
        }

		ErrorPrintIt(-1,code,type,response,buffer);

    }
    return code;
}

PARSEDLLSPEC void ErrorForward(int art, int code, char *ttype, char *message)
{
    struct _Error *mptr;
    int response;
	int type;

    if(_ErrorFirst)
    {
        ErrorInit();
    }

	type=ErrorTypeParse(ttype);
    mptr=ErrorFind(code);
    if(mptr==0)
    {
        if(type>=0 && type<sizeof(_ErrorResponse)/sizeof(_ErrorResponse[0]))
        {
            response=_ErrorResponse[type];
        }
        else
        {
            response=0;
        }
	}
    else
    {
        type=mptr->type&0xff;
        response=(mptr->type>>8)&0xff;
        if(response==0)
        {
            if(type>=0 && type<sizeof(_ErrorResponse)/sizeof(_ErrorResponse[0]))
            {
                response=_ErrorResponse[type];
            }
            else
            {
                response=0;
            }
        }
	}
    response&=(~(1<<ErrorResponseSpecial));
    //
    // here is where we format the message
    //
    if(response!=0)
    {
        ErrorPrintIt(art,code,type,response,message);
	}
}

static void ErrorList(struct _Error *mptr)
{
	char full[MBUFFER];
    char eol[20];
	int length;
	int type;

	if(mptr!=0)
	{
		//
		// suppress end of line if alraedy included in the message
		//
		Scopy(eol,"\n");
		length=Slength(mptr->format);
		if(mptr->format[length-1]=='\n')
		{
			eol[0]=0;
		}
        type=mptr->type&0xff;
		SformatOutput(full,MBUFFER-1,"%04d %s %s%s",mptr->code,ErrorTypePrint(type),mptr->format,eol);
		full[MBUFFER-1]=0;
		UserPrintIt(full);
	}
}


PARSEDLLSPEC void ErrorListType(int type)
{
	int ik;
	int it;

	for(ik=0; ik<EHASH; ik++)
	{
		for(it=0; it<EH[ik].many; it++)
		{
			if(EH[ik].fptr[it]->type==type)
			{
				ErrorList(EH[ik].fptr[it]);
			}
		}
	}
}


PARSEDLLSPEC void ErrorListCode(int code)
{
	int ik;
	int it;

	for(ik=0; ik<EHASH; ik++)
	{
		for(it=0; it<EH[ik].many; it++)
		{
			if(EH[ik].fptr[it]->code==code)
			{
				ErrorList(EH[ik].fptr[it]);
			}
		}
	}
}


PARSEDLLSPEC void ErrorListAll(void)
{
	int ik;
	int it;

	for(ik=0; ik<EHASH; ik++)
	{
		for(it=0; it<EH[ik].many; it++)
		{
			ErrorList(EH[ik].fptr[it]);
		}
	}
}

