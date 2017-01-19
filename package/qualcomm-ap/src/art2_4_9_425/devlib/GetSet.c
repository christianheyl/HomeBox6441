#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdarg.h>
#include <string.h>
#include <fcntl.h>



#include "smatch.h"
#include "TimeMillisecond.h"
#include "CommandParse.h"
#include "ParameterSelect.h"

#include "Device.h"

#include "ParameterParse.h"

#include "ErrorPrint.h"
#include "UserPrint.h"
#include "ParseError.h"
#include "NartError.h"

#include "StructPrint.h"
#include "ConfigurationCommand.h"

#include "GetSet.h"


#define MBUFFER 1024

//#ifndef max
//#define max(a,b)	(((a) > (b)) ? (a) : (b))
//#endif

//#define MALLOWED 100




static void SuccessMessage(char *command, char *name, char type, unsigned int *uvalue, int nvalue)
{
	char buffer[MBUFFER];
	int lc, nc;
	int it;

    int *dvalue; 
	unsigned char *cvalue;
	double *fvalue;

	fvalue=(double *)uvalue;
	dvalue=(int *)uvalue;
	cvalue=(unsigned char *)uvalue;

	lc=0;
	nc=SformatOutput(&buffer[lc],MBUFFER-lc-1,"|%s|%s|",command,name);
	if(nc>0)
	{
		lc+=nc;
	}
	for(it=0; it<nvalue; it++)
	{
		switch(type)
		{
		case 'p':
			case 'f':
				nc=SformatOutput(&buffer[lc],MBUFFER-lc-1,"%lf,",fvalue[it]);
				break;
			case 'd':
				nc=SformatOutput(&buffer[lc],MBUFFER-lc-1,"%+d,",dvalue[it]);
				break;
			case '2':
			case '5':
			case 'u':
			case 'z':
				nc=SformatOutput(&buffer[lc],MBUFFER-lc-1,"%u,",uvalue[it]);
				break;
			case 'x':
				nc=SformatOutput(&buffer[lc],MBUFFER-lc-1,"0x%x,",uvalue[it]);
				break;
			case 'm':
				nc=SformatOutput(&buffer[lc],MBUFFER-lc-1,"%02x:%02x:%02x:%02x:%02x:%02x,",
					cvalue[0],cvalue[1],cvalue[2],cvalue[3],cvalue[4],cvalue[5]);
				nvalue=1;
				break;
			default:
				nc=SformatOutput(&buffer[lc],MBUFFER-lc-1,"%s,",cvalue);
				nvalue=1;
				break;
		}
		if(nc>0)
		{
			lc+=nc;
		}
	}
	//
	// overwrite last comma with bar
	//
	if(lc>0)
	{
		SformatOutput(&buffer[lc-1],MBUFFER-lc,"|");
	}

	ErrorPrint(NartData,buffer);
}

static struct _ParameterList *GetParameter=0;
static int GetParameterMany=0;

static struct _StructPrint *EepromList=0;
static int EepromListMany=0;

static char *EepromData=0;
static int EepromDataMany=0;

static void GetParameterMake()
{
	int il;
	int iw;

    GetParameterMany=EepromListMany;	
    GetParameter=(struct _ParameterList *)malloc((GetParameterMany+1)*sizeof(struct _ParameterList));
	if(GetParameter!=0)
	{
		//
		// start with an entry for "all" at the end of the list
		//
		GetParameter[GetParameterMany].code=GetParameterMany;
        GetParameter[GetParameterMany].word[0]="all";
		for(iw=1; iw<MPWORD; iw++)
		{
			GetParameter[GetParameterMany].word[iw]=0;
		}
        GetParameter[GetParameterMany].help=0;
        GetParameter[GetParameterMany].type='z';			
        GetParameter[GetParameterMany].units=0;		
        GetParameter[GetParameterMany].nx=1;		
        GetParameter[GetParameterMany].ny=1;
        GetParameter[GetParameterMany].nz=1;
        GetParameter[GetParameterMany].minimum=0;		
        GetParameter[GetParameterMany].maximum=0;		
        GetParameter[GetParameterMany].def=0;			
        GetParameter[GetParameterMany].nspecial=0;
        GetParameter[GetParameterMany].special=0;
		//
		// add each of the eeprom entries
		//
		for(il=0; il<GetParameterMany; il++)
		{
			GetParameter[il].code=il;
            GetParameter[il].word[0]=EepromList[il].name;
			for(iw=1; iw<MPWORD; iw++)
			{
				GetParameter[il].word[iw]=0;
			}
	        GetParameter[il].help=0;
			if(EepromList[il].type=='p')
			{
				GetParameter[il].type='f';
			}
			else if(EepromList[il].type=='2' || EepromList[il].type=='5')
			{
				GetParameter[il].type='u';
			}
			else
			{
				GetParameter[il].type=EepromList[il].type;
			}
	        GetParameter[il].units=0;		
	        GetParameter[il].nx=EepromList[il].nx;		
	        GetParameter[il].ny=EepromList[il].ny;
	        GetParameter[il].nz=EepromList[il].nz;
	        GetParameter[il].minimum=0;		
	        GetParameter[il].maximum=0;		
	        GetParameter[il].def=0;			
	        GetParameter[il].nspecial=0;
	        GetParameter[il].special=0;
		}
	}			
}

FIELDDLLSPEC int GetSetDataSetup(struct _StructPrint *list, int nlist, unsigned char *data, int ndata)
{
    EepromList=list;
    EepromListMany=nlist;

    EepromData=data;
    EepromDataMany=ndata;

	GetParameterMake();

	return 0;
}

FIELDDLLSPEC int GetParameterSplice(struct _ParameterList *list)
{
	if(GetParameter==0)
	{
		GetParameterMake();
	}
    list->nspecial=GetParameterMany+1;
    list->special=GetParameter;
	return 0;
}

FIELDDLLSPEC int SetParameterSplice(struct _ParameterList *list)
{
	if(GetParameter==0)
	{
		GetParameterMake();
	}
    list->nspecial=GetParameterMany;
    list->special=GetParameter;
	return 0;
}


#define MAXVALUE 1000



static void ConfigSet(int ip, struct _ParameterList *menu, int ix, int iy, int iz, 
    struct _StructPrint *list, unsigned char *mptr, int msize)
{
	int status;
	int ngot=0;
	char *name;	
	int lc,nc;
	int it;
	int nset;

    int dvalue[MAXVALUE]; 
	unsigned int uvalue[MAXVALUE]; 
	char cvalue[MBUFFER];
	unsigned int *value;
	double fvalue[MAXVALUE];

	char atext[MBUFFER];

	//
	// format the full name of the parameter
	//
	if(ix<0)
	{
		SformatOutput(atext,MBUFFER-1,"%s",menu->word[0]);
	}
	else if(iy<0)
	{
		SformatOutput(atext,MBUFFER-1,"%s[%d]",menu->word[0],ix);
	}
	else if(iz<0)
	{
		SformatOutput(atext,MBUFFER-1,"%s[%d][%d]",menu->word[0],ix,iy);
	}
	else
	{
		SformatOutput(atext,MBUFFER-1,"%s[%d][%d][%d]",menu->word[0],ix,iy,iz);
	}
    //
	// parse the parameter value input
	//
	name=CommandParameterName(ip);
	switch(menu->type)
	{
		case '2':
		case '5':
		case 'u':
		case 'z':
			ngot=ParseIntegerList(ip,name,uvalue,menu);
			value=(unsigned int *)&uvalue;
			break;
		case 'd': 
			ngot=ParseIntegerList(ip,name,dvalue,menu);
			value=(unsigned int *)&dvalue;
			break;
		case 'x': 
			ngot=ParseHexList(ip,name,uvalue,menu);
			value=(unsigned int *)&uvalue;
			break;
		case 'p':
		case 'f': 
			ngot=ParseDoubleList(ip,name,fvalue,menu);
			value=(unsigned int *)&fvalue;
			break;
		case 'm': 
			status=ParseMacAddress(CommandParameterValue(ip,0),cvalue);
			if(status == 0) 
			{   
				ngot=1;
			}
			else
			{
				ngot=0;
			}
			value=(unsigned int *)&cvalue;
			break;
		default:
			lc=SformatOutput(cvalue,MBUFFER-1,"%s",CommandParameterValue(ip,0));
			for(it=1; it<CommandParameterValueMany(ip); it++)
			{
			    nc=SformatOutput(&cvalue[lc],MBUFFER-lc-1,", %s",CommandParameterValue(ip,it));
				if(nc>0)
				{
					lc+=nc;
				}
			}
			cvalue[MBUFFER-1]=0;
			ngot=Slength(cvalue)+1;
			value=(unsigned int *)&cvalue;
			break;
	}
    //
	// update the structure
	//
	if(ngot>0)
	{
        nset=StructSet(list, mptr, msize, value, ngot, ix, iy, iz);
		if(nset>0) 
		{
			//
			// return success message
			//
            SuccessMessage("set", atext, menu->type, value, nset);
		}
		else
		{
			//
			// return error message
			//
		}
	}
	else
	{
		//
		// return error message
		//
	}
}


static void ConfigGet(int ip, struct _ParameterList *menu, int ix, int iy, int iz, 
    struct _StructPrint *list, unsigned char *mptr, int msize)
{
	int ngot=0;

	unsigned int value[2*MAXVALUE];
	char atext[MBUFFER];

	//
	// format the full name of the parameter
	//
	if(ix<0)
	{
		SformatOutput(atext,MBUFFER-1,"%s",menu->word[0]);
	}
	else if(iy<0)
	{
		SformatOutput(atext,MBUFFER-1,"%s[%d]",menu->word[0],ix);
	}
	else if(iz<0)
	{
		SformatOutput(atext,MBUFFER-1,"%s[%d][%d]",menu->word[0],ix,iy);
	}
	else
	{
		SformatOutput(atext,MBUFFER-1,"%s[%d][%d][%d]",menu->word[0],ix,iy,iz);
	}
    //
	// fetch the values from the structure
	//
    ngot=StructGet(list, mptr, msize, value, MAXVALUE, ix, iy, iz);
	if(ngot>0) 
	{
		//
		// return success message
		//
        SuccessMessage("get", atext, menu->type, value, ngot);
	}
	else
	{
		//
		// return error message
		//
	}
}


//
// parse and then set a configuration parameter in the internal structure
//
FIELDDLLSPEC int SetCommand(int client)
{
	int np, ip;
	char *name;	
	int error;
	int done;
	int index;
	int ix, iy, iz;
	struct _StructPrint *list;
	struct _ParameterList *menu;

	error=0;
	done=0;
	//
	// parse arguments and do it
	//
	np=CommandParameterMany();
	for(ip=0; ip<np; ip++)
	{
		name=CommandParameterName(ip);
		index=ParameterSelectIndexArray(name,GetParameter,GetParameterMany, &ix, &iy, &iz);
		if(index>=0)
		{
			list= &EepromList[index];	
			menu= &GetParameter[index];
			if(list!=0)
			{
				ConfigSet(ip, menu, ix, iy, iz, list, EepromData, EepromDataMany);
			}
			else
			{
				error++;
				ErrorPrint(ParseBadParameter,name);
			}
			break;
		}
		else
		{
			error++;
			ErrorPrint(ParseBadParameter,name);
		}
	}
	return 0;
}


static void PrintIt(char *format, ...)
{
    va_list ap;
    char buffer[MBUFFER];

	va_start(ap, format);

	#if defined(Linux) || defined(__APPLE__)
		vsnprintf(buffer,MBUFFER,format,ap);
	#else
		_vsnprintf(buffer,MBUFFER,format,ap);
	#endif

	va_end(ap);

	ErrorPrint(NartData,buffer);
}

FIELDDLLSPEC int GetAll(void (*print)(char *format, ...), int all)
{
	char *block[10];

	if(print!=0)
	{
		block[0]=EepromData;
		StructDifferenceAnalyze(print, 
			EepromList, EepromListMany,
			block, EepromDataMany, 1, all);
	}
	return EepromDataMany;
}

//
// parse and then get a configuration parameter in the internal structure
//
FIELDDLLSPEC int GetCommand(int client)
{
	int np, ip;
	char *name;	
	int error;
	int done;
	int index;
	int ix, iy, iz;
	struct _StructPrint *list;
	struct _ParameterList *menu;

	error=0;
	done=0;
	//
	// parse arguments and do it
	//
	np=CommandParameterMany();
	if(np<=0)
	{
		GetAll(PrintIt,1);
	}
	
	for(ip=0; ip<np; ip++)
	{
		name=CommandParameterName(ip);
		index=ParameterSelectIndexArray(name,GetParameter,GetParameterMany+1, &ix, &iy, &iz);
		if(index>=GetParameterMany)
		{
			GetAll(PrintIt,1);
		}
		else if(index>=0)
		{
			list= &EepromList[index];		
			menu= &GetParameter[index];
			if(list!=0)
			{
				ConfigGet(ip, menu, ix, iy, iz, list, EepromData, EepromDataMany);
			}
			else
			{
				error++;
				ErrorPrint(ParseBadParameter,name);
			}
			break;
		}
		else
		{
			error++;
			ErrorPrint(ParseBadParameter,name);
		}
	}
	return 0;
}	

