
//  "ACI $Id: //depot/sw/branches/art2_main_per_cs/src/art2/shared/ParameterParse.c#1 $, $Header: //depot/sw/branches/art2_main_per_cs/src/art2/shared/ParameterParse.c#1 $"


#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

#include "smatch.h"
#include "ErrorPrint.h"

#include "ParseError.h"
#include "CommandParse.h"
#include "ParameterSelect.h"
#include "ParameterParse.h"

#define MBUFFER 1024

static struct _Error _ParseError[]=
{
    {ParseBadParameter,ErrorFatal,ParseBadParameterFormat},
    {ParseBadValue,ErrorFatal,ParseBadValueFormat},
    {ParseTooMany,ErrorFatal,ParseTooManyFormat},
    {ParseNegativeIncrement,ErrorFatal,ParseNegativeIncrementFormat},
    {ParsePositiveIncrement,ErrorFatal,ParsePositiveIncrementFormat},
    {ParseMinimumDecimal,ErrorFatal,ParseMinimumDecimalFormat},
    {ParseMaximumDecimal,ErrorFatal,ParseMaximumDecimalFormat},
    {ParseMinimumHex,ErrorFatal,ParseMinimumHexFormat},
    {ParseMaximumHex,ErrorFatal,ParseMaximumHexFormat},
    {ParseMinimumDouble,ErrorFatal,ParseMinimumDoubleFormat},
    {ParseMaximumDouble,ErrorFatal,ParseMaximumDoubleFormat},
    {ParseError,ErrorFatal,ParseErrorFormat},
    {ParseHelp,ErrorInformation,ParseHelpFormat},
    {ParseHelpStart,ErrorControl,ParseHelpStartFormat},
    {ParseHelpEnd,ErrorControl,ParseHelpEndFormat},
    {ParseMinimumUnsigned,ErrorFatal,ParseMinimumUnsignedFormat},
    {ParseMaximumUnsigned,ErrorFatal,ParseMaximumUnsignedFormat},
	{ParseMinimumMac,ErrorFatal,ParseMinimumMacFormat},
    {ParseMaximumMac,ErrorFatal,ParseMaximumMacFormat},
	{ParseHelpSynopsisStart,ErrorControl,ParseHelpSynopsisStartFormat},
	{ParseHelpSynopsisEnd,ErrorControl,ParseHelpSynopsisEndFormat},
	{ParseHelpParametersStart,ErrorControl,ParseHelpParametersStartFormat},
	{ParseHelpParametersEnd,ErrorControl,ParseHelpParametersEndFormat},
	{ParseHelpDescriptionStart,ErrorControl,ParseHelpDescriptionStartFormat},
	{ParseHelpDescriptionEnd,ErrorControl,ParseHelpDescriptionEndFormat},
	{ParseHelpUnknown,ErrorInformation,ParseHelpUnknownFormat},
    {ParseBadCommand,ErrorFatal,ParseBadCommandFormat},
	{ParseBadArrayIndex,ErrorFatal,ParseBadArrayIndexFormat},
	{ParseArrayIndexBound,ErrorFatal,ParseArrayIndexBoundFormat},
	{ParseArrayIndexBound2,ErrorFatal,ParseArrayIndexBound2Format},
	{ParseArrayIndexBound3,ErrorFatal,ParseArrayIndexBound3Format},
    {ParseCenterFrequencyUsed,ErrorFatal,ParseCenterFrequencyUsedFormat},

};

static int _ErrorFirst=1;

PARSEDLLSPEC void ParseErrorInit()
{
    if(_ErrorFirst)
    {
        ErrorHashCreate(_ParseError,sizeof(_ParseError)/sizeof(_ParseError[0]));
    }
    _ErrorFirst=0;
}

enum 
{
	 code_all=0,
	 code_all_legacy,
	 code_all_mcs_ht_20,  
	 code_all_mcs_ht_40,  
	 code_all_dvt,  
	 code_all_vmcs_ht_20,  
	 code_all_vmcs_ht_40,  
	 code_all_vmcs_ht_80,  
	 code_all_vdvt,  
	 code_11s,  
	 code_11l,  
	 code_5s,   
	 code_5l,
	 code_2s,
	 code_2l,
	 code_1l,
	 code_6,
	 code_9,
	 code_12,
	 code_18,
	 code_24,
	 code_36,
	 code_48,
	 code_54,
	 code_t0,
	 code_t1,
	 code_t2,
	 code_t3,
	 code_t4,
	 code_t5,
	 code_t6,
	 code_t7,
	 code_t8,
	 code_t9,
	 code_t10,
	 code_t11,
	 code_t12,
	 code_t13,
	 code_t14,
	 code_t15,
	 code_t16,
	 code_t17,
	 code_t18,
	 code_t19,
	 code_t20,
	 code_t21,
	 code_t22,
	 code_t23,
	 code_vt0,
	 code_vt1,
	 code_vt2,
	 code_vt3,
	 code_vt4,
	 code_vt5,
	 code_vt6,
	 code_vt7,
	 code_vt8,
	 code_vt9,
	 code_vt10,
	 code_vt11,
	 code_vt12,
	 code_vt13,
	 code_vt14,
	 code_vt15,
	 code_vt16,
	 code_vt17,
	 code_vt18,
	 code_vt19,
	 code_vt20,
	 code_vt21,
	 code_vt22,
	 code_vt23,
	 code_vt24,
	 code_vt25,
	 code_vt26,
	 code_vt27,
	 code_vt28,
	 code_vt29,
	 code_f0,
	 code_f1,
	 code_f2,
	 code_f3,
	 code_f4,
	 code_f5,
	 code_f6,
	 code_f7,
	 code_f8,
	 code_f9,
	 code_f10,
	 code_f11,
	 code_f12,
	 code_f13,
	 code_f14,
	 code_f15,
	 code_f16,
	 code_f17,
	 code_f18,
	 code_f19,
	 code_f20,
	 code_f21,
	 code_f22,
	 code_f23,
	 code_vf0,
	 code_vf1,
	 code_vf2,
	 code_vf3,
	 code_vf4,
	 code_vf5,
	 code_vf6,
	 code_vf7,
	 code_vf8,
	 code_vf9,
	 code_vf10,
	 code_vf11,
	 code_vf12,
	 code_vf13,
	 code_vf14,
	 code_vf15,
	 code_vf16,
	 code_vf17,
	 code_vf18,
	 code_vf19,
	 code_vf20,
	 code_vf21,
	 code_vf22,
	 code_vf23,
	 code_vf24,
	 code_vf25,
	 code_vf26,
	 code_vf27,
	 code_vf28,
	 code_vf29,
	 code_ve0,
	 code_ve1,
	 code_ve2,
	 code_ve3,
	 code_ve4,
	 code_ve5,
	 code_ve6,
	 code_ve7,
	 code_ve8,
	 code_ve9,
	 code_ve10,
	 code_ve11,
	 code_ve12,
	 code_ve13,
	 code_ve14,
	 code_ve15,
	 code_ve16,
	 code_ve17,
	 code_ve18,
	 code_ve19,
	 code_ve20,
	 code_ve21,
	 code_ve22,
	 code_ve23,
	 code_ve24,
	 code_ve25,
	 code_ve26,
	 code_ve27,
	 code_ve28,
	 code_ve29,
};


static struct _ParameterList rates[]=
{
	{code_all, "all"},
	{code_all_legacy, "all_legacy","legacy_all"},
	{code_all_mcs_ht_20, "all_mcs_ht_20","all_mcs_ht20","ht20_mcs_all"},
	{code_all_mcs_ht_40, "all_mcs_ht_40","all_mcs_ht40","ht40_mcs_all"},
	{code_all_dvt, "all_dvt","dvt_all"},
	{code_all_vmcs_ht_20, "all_vmcs_ht_20","all_vmcs_ht20","ht20_vmcs_all"},
	{code_all_vmcs_ht_40, "all_vmcs_ht_40","all_vmcs_ht40","ht40_vmcs_all"},
	{code_all_vmcs_ht_80, "all_vmcs_ht_80","all_vmcs_ht80","ht80_vmcs_all"},
	{code_all_vdvt, "all_vdvt","vdvt_all"},
	{code_11s, "11s"},
	{code_11l, "11l"},
	{code_5s, "5s"},
	{code_5l, "5l"},
	{code_2s, "2s"},
	{code_2l, "2l"},
	{code_1l, "1l"},
	{code_6, "6"},
	{code_9, "9"},
	{code_12, "12"},
	{code_18, "18"},
	{code_24, "24"},
	{code_36, "36"},
	{code_48, "48"},
	{code_54, "54"},
	{code_t0, "t0", "mcs0"},
	{code_t1, "t1", "mcs1"},
	{code_t2, "t2", "mcs2"},
	{code_t3, "t3", "mcs3"},
	{code_t4, "t4", "mcs4"},
	{code_t5, "t5", "mcs5"},
	{code_t6, "t6", "mcs6"},
	{code_t7, "t7", "mcs7"},
	{code_t8, "t8", "mcs8"},
	{code_t9, "t9", "mcs9"},
	{code_t10, "t10", "mcs10"},
	{code_t11, "t11", "mcs11"},
	{code_t12, "t12", "mcs12"},
	{code_t13, "t13", "mcs13"},
	{code_t14, "t14", "mcs14"},
	{code_t15, "t15", "mcs15"},
	{code_t16, "t16", "mcs16"},
	{code_t17, "t17", "mcs17"},
	{code_t18, "t18", "mcs18"},
	{code_t19, "t19", "mcs19"},
	{code_t20, "t20", "mcs20"},
	{code_t21, "t21", "mcs21"},
	{code_t22, "t22", "mcs22"},
	{code_t23, "t23", "mcs23"},
	{code_vt0, "vt0", "vmcs0"},
	{code_vt1, "vt1", "vmcs1"},
	{code_vt2, "vt2", "vmcs2"},
	{code_vt3, "vt3", "vmcs3"},
	{code_vt4, "vt4", "vmcs4"},
	{code_vt5, "vt5", "vmcs5"},
	{code_vt6, "vt6", "vmcs6"},
	{code_vt7, "vt7", "vmcs7"},
	{code_vt8, "vt8", "vmcs8"},
	{code_vt9, "vt9", "vmcs9"},
	{code_vt10, "vt10", "vmcs10"},
	{code_vt11, "vt11", "vmcs11"},
	{code_vt12, "vt12", "vmcs12"},
	{code_vt13, "vt13", "vmcs13"},
	{code_vt14, "vt14", "vmcs14"},
	{code_vt15, "vt15", "vmcs15"},
	{code_vt16, "vt16", "vmcs16"},
	{code_vt17, "vt17", "vmcs17"},
	{code_vt18, "vt18", "vmcs18"},
	{code_vt19, "vt19", "vmcs19"},
	{code_vt20, "vt20", "vmcs20"},
	{code_vt21, "vt21", "vmcs21"},
	{code_vt22, "vt22", "vmcs22"},
	{code_vt23, "vt23", "vmcs23"},
	{code_vt24, "vt24", "vmcs24"},
	{code_vt25, "vt25", "vmcs25"},
	{code_vt26, "vt26", "vmcs26"},
	{code_vt27, "vt27", "vmcs27"},
	{code_vt28, "vt28", "vmcs28"},
	{code_vt29, "vt29", "vmcs29"},
	{code_f0, "f0", "mcs0/40"},
	{code_f1, "f1", "mcs1/40"},
	{code_f2, "f2", "mcs2/40"},
	{code_f3, "f3", "mcs3/40"},
	{code_f4, "f4", "mcs4/40"},
	{code_f5, "f5", "mcs5/40"},
	{code_f6, "f6", "mcs6/40"},
	{code_f7, "f7", "mcs7/40"},
	{code_f8, "f8", "mcs8/40"},
	{code_f9, "f9", "mcs9/40"},
	{code_f10, "f10", "mcs10/40"},
	{code_f11, "f11", "mcs11/40"},
	{code_f12, "f12", "mcs12/40"},
	{code_f13, "f13", "mcs13/40"},
	{code_f14, "f14", "mcs14/40"},
	{code_f15, "f15", "mcs15/40"},
	{code_f16, "f16", "mcs16/40"},
	{code_f17, "f17", "mcs17/40"},
	{code_f18, "f18", "mcs18/40"},
	{code_f19, "f19", "mcs19/40"},
	{code_f20, "f20", "mcs20/40"},
	{code_f21, "f21", "mcs21/40"},
	{code_f22, "f22", "mcs22/40"},
	{code_f23, "f23", "mcs23/40"},
	{code_vf0, "vf0", "vmcs0/40"},
	{code_vf1, "vf1", "vmcs1/40"},
	{code_vf2, "vf2", "vmcs2/40"},
	{code_vf3, "vf3", "vmcs3/40"},
	{code_vf4, "vf4", "vmcs4/40"},
	{code_vf5, "vf5", "vmcs5/40"},
	{code_vf6, "vf6", "vmcs6/40"},
	{code_vf7, "vf7", "vmcs7/40"},
	{code_vf8, "vf8", "vmcs8/40"},
	{code_vf9, "vf9", "vmcs9/40"},
	{code_vf10, "vf10", "vmcs10/40"},
	{code_vf11, "vf11", "vmcs11/40"},
	{code_vf12, "vf12", "vmcs12/40"},
	{code_vf13, "vf13", "vmcs13/40"},
	{code_vf14, "vf14", "vmcs14/40"},
	{code_vf15, "vf15", "vmcs15/40"},
	{code_vf16, "vf16", "vmcs16/40"},
	{code_vf17, "vf17", "vmcs17/40"},
	{code_vf18, "vf18", "vmcs18/40"},
	{code_vf19, "vf19", "vmcs19/40"},
	{code_vf20, "vf20", "vmcs20/40"},
	{code_vf21, "vf21", "vmcs21/40"},
	{code_vf22, "vf22", "vmcs22/40"},
	{code_vf23, "vf23", "vmcs23/40"},
	{code_vf24, "vf24", "vmcs24/40"},
	{code_vf25, "vf25", "vmcs25/40"},
	{code_vf26, "vf26", "vmcs26/40"},
	{code_vf27, "vf27", "vmcs27/40"},
	{code_vf28, "vf28", "vmcs28/40"},
	{code_vf29, "vf29", "vmcs29/40"},
	{code_ve0, "ve0", "vmcs0/80"},
	{code_ve1, "ve1", "vmcs1/80"},
	{code_ve2, "ve2", "vmcs2/80"},
	{code_ve3, "ve3", "vmcs3/80"},
	{code_ve4, "ve4", "vmcs4/80"},
	{code_ve5, "ve5", "vmcs5/80"},
	{code_ve6, "ve6", "vmcs6/80"},
	{code_ve7, "ve7", "vmcs7/80"},
	{code_ve8, "ve8", "vmcs8/80"},
	{code_ve9, "ve9", "vmcs9/80"},
	{code_ve10, "ve10", "vmcs10/80"},
	{code_ve11, "ve11", "vmcs11/80"},
	{code_ve12, "ve12", "vmcs12/80"},
	{code_ve13, "ve13", "vmcs13/80"},
	{code_ve14, "ve14", "vmcs14/80"},
	{code_ve15, "ve15", "vmcs15/80"},
	{code_ve16, "ve16", "vmcs16/80"},
	{code_ve17, "ve17", "vmcs17/80"},
	{code_ve18, "ve18", "vmcs18/80"},
	{code_ve19, "ve19", "vmcs19/80"},
	{code_ve20, "ve20", "vmcs20/80"},
	{code_ve21, "ve21", "vmcs21/80"},
	{code_ve22, "ve22", "vmcs22/80"},
	{code_ve23, "ve23", "vmcs23/80"},
	{code_ve24, "ve24", "vmcs24/80"},
	{code_ve25, "ve25", "vmcs25/80"},
	{code_ve26, "ve26", "vmcs26/80"},
	{code_ve27, "ve27", "vmcs27/80"},
	{code_ve28, "ve28", "vmcs28/80"},
	{code_ve29, "ve29", "vmcs29/80"},
};


//
// this function massages the input by replacing any occurence of $NAME with the value
//
static char * (*_ValueReplacement)(char *name, char *buffer, int max);


PARSEDLLSPEC void ParseParameterReplacement(char * (*f)(char *name, char *buffer, int max))
{
    _ValueReplacement=f;
}

static int TryOtherFormats(char *text, int *value)
{
	int extra;
	int ngot;

	ngot=0;
	if(ngot!=1)
	{
		ngot=sscanf(text, " 0x%x %1c",value,&extra);
	}
	if(ngot!=1)
	{
		ngot=sscanf(text, " x%x %1c",value,&extra);
	}
	if(ngot!=1)
	{
		ngot=sscanf(text, " h%x %1c",value,&extra);
	}
	if(ngot!=1)
	{
		ngot=sscanf(text, " d%d %1c",value,&extra);
	}
	if(ngot!=1)
	{
		ngot=sscanf(text, " t%d %1c",value,&extra);
	}
	if(ngot!=1)
	{
		ngot=sscanf(text, " +%d %1c",value,&extra);
	}
	if(ngot!=1)
	{
		ngot=sscanf(text, " -%d %1c",value,&extra);
		if(ngot==1)
		{
			*value = -(*value);
		}
	}
	if(ngot!=1)
	{
		ngot=sscanf(text, " u%u %1c",value,&extra);
	}

	return ngot;
}


PARSEDLLSPEC int ParseIntegerList(int input, char *name, int *value, struct _ParameterList *list)
{
	int ip, np;
	int ngot;
	int nvalue;
	int vmin,vmax,vinc,iv;
	char *text, tcopy[MBUFFER];
	char buffer[MBUFFER];
	char *tmin, *tmax, *tinc;
	int index;
	int extra;

    ParseErrorInit();

    nvalue=0;		
	
	np=CommandParameterValueMany(input);
	for(ip=0; ip<np; ip++)
	{
		if(ParameterValueMaximum(list)>0 && nvalue>ParameterValueMaximum(list))
		{
			ErrorPrint(ParseTooMany,name,ParameterValueMaximum(list));
            nvalue= -1;
			break;
		}

        text=CommandParameterValue(input,ip);
		if(_ValueReplacement!=0)
		{
			(*_ValueReplacement)(text,buffer,MBUFFER);
			text=buffer;
		}

		SformatOutput(tcopy,MBUFFER-1,"%s",text);
		tcopy[MBUFFER-1]=0;
		tinc=0;
		tmax=0;
		tmin=tcopy;
		tmax=strchr(tcopy,':');
		if(tmax!=0)
		{
			*tmax=0;
			tmax++;
			tinc=strchr(tmax,':');
			if(tinc!=0)
			{
				*tinc=0;
				tinc++;
			}
		}
        //
		// parse the minimum value
		//
		ngot= -1;
		if(list->nspecial>0)
		{
			index=ParameterSelectIndex(tmin,list->special,list->nspecial);
			if(index>=0)
			{
				vmin=list->special[index].code;
				ngot=1;
			}
		}
		if(ngot<=0)
		{
			ngot=sscanf(tmin, " %d %1c",&vmin,&extra);
			//
			// also try hex
			//
			if(ngot!=1)
			{
				ngot=TryOtherFormats(tmin,&vmin);
			}
			if(ngot!=1) 
			{
				ErrorPrint(ParseBadValue,text,name);
				break;
			}
			if(list!=0 && list->minimum!=0 && vmin< *((int *)list->minimum))
			{
				ErrorPrint(ParseMinimumDecimal,vmin,*((int *)list->minimum),name);
				break;
			}
			if(list!=0 && list->maximum!=0 && vmin> *((int *)list->maximum))
			{
				ErrorPrint(ParseMaximumDecimal,vmin,*((int *)list->maximum),name);
				break;
			}
		}
		//
		// now parse the maximum value
		//
		if(tmax!=0)
		{
			ngot= -1;
			if(list!=0 && list->nspecial>0)
			{
				index=ParameterSelectIndex(tmin,list->special,list->nspecial);
				if(index>=0)
				{
					vmax=list->special[index].code;
					ngot=1;
				}
			}
			if(ngot<=0)
			{
				ngot=sscanf(tmax, " %d %1c",&vmax,&extra);
				if(ngot!=1)
				{
					ngot=TryOtherFormats(tmax,&vmax);
				}
				if(ngot!=1) 
				{
					ErrorPrint(ParseBadValue,text,name);
					break;
				}
				if(list!=0 && list->minimum!=0 && vmax< *((int *)list->minimum))
				{
					ErrorPrint(ParseMinimumDecimal,vmax, *((int *)list->minimum),name);
					break;
				}
				if(list!=0 && list->maximum!=0 && vmax> *((int *)list->maximum))
				{
					ErrorPrint(ParseMaximumDecimal,vmax, *((int *)list->maximum),name);
					break;
				}
			}
		}
		else
		{
			vmax=vmin;
		}
		//
		// parse the increment
		//
		if(tinc!=0)
		{
			ngot=sscanf(tinc, " %d %1c",&vinc,&extra);
			if(ngot!=1)
			{
				ngot=TryOtherFormats(tinc,&vinc);
			}
			if(ngot!=1) 
			{
				ErrorPrint(ParseBadValue,text,name);
				break;
			}
		}
		else
		{
			vinc=1;
		}

		if(vinc>0)
		{
			if(vmax>=vmin)
			{
		        for(iv=vmin; iv<=vmax; iv+=vinc)
				{
			        if(ParameterValueMaximum(list)>0 && nvalue>=ParameterValueMaximum(list))
					{
				        ErrorPrint(ParseTooMany,name,ParameterValueMaximum(list));
                        nvalue= -1;
				        break;
					}
		            value[nvalue]=iv;
		            nvalue++;
				}
			}
			else
			{
				ErrorPrint(ParsePositiveIncrement,name);
			}
		}
		else if(vinc<0)
		{
			if(vmax<=vmin)
			{
		        for(iv=vmin; iv>=vmax; iv+=vinc)
				{
			        if(ParameterValueMaximum(list)>0 && nvalue>=ParameterValueMaximum(list))
					{
				        ErrorPrint(ParseTooMany,name,ParameterValueMaximum(list));
                        nvalue= -1;
				        break;
					}
		            value[nvalue]=iv;
		            nvalue++;
				}
			}
			else
			{
				ErrorPrint(ParseNegativeIncrement,name);
				continue;
			}
		}
		else
		{
		    value[nvalue]=vmin;
		    nvalue++;
		}
	}

	return nvalue;
}


PARSEDLLSPEC int ParseInteger(int input, char *name, int max, int *value)
{
	struct _ParameterList list;

	list.nspecial=0;
	list.nx=max;
	list.ny=1;
	list.nz=1;
	list.minimum=0;
	list.maximum=0;

	return ParseIntegerList(input,name,value,&list);
}


PARSEDLLSPEC int ParseFloatList(int input, char *name, float *value, struct _ParameterList *list)
{
	int ip, np;
	int ngot;
	int nvalue;
	double vmin,vmax,vinc,iv;
	char *text, tcopy[MBUFFER];
	char buffer[MBUFFER];
	char *tmin, *tmax, *tinc;
	int index;
	int extra;

    ParseErrorInit();

    nvalue=0;		
	
	np=CommandParameterValueMany(input);
	for(ip=0; ip<np; ip++)
	{
		if(ParameterValueMaximum(list)>0 && nvalue>ParameterValueMaximum(list))
		{
			ErrorPrint(ParseTooMany,name,ParameterValueMaximum(list));
            nvalue= -1;
			break;
		}

        text=CommandParameterValue(input,ip);
		if(_ValueReplacement!=0)
		{
			(*_ValueReplacement)(text,buffer,MBUFFER);
			text=buffer;
		}

		SformatOutput(tcopy,MBUFFER-1,"%s",text);
		tcopy[MBUFFER-1]=0;
		tinc=0;
		tmax=0;
		tmin=tcopy;
		tmax=strchr(tcopy,':');
		if(tmax!=0)
		{
			*tmax=0;
			tmax++;
			tinc=strchr(tmax,':');
			if(tinc!=0)
			{
				*tinc=0;
				tinc++;
			}
		}
        //
		// parse the minimum value
		//
		ngot= -1;
		if(list->nspecial>0)
		{
			index=ParameterSelectIndex(tmin,list->special,list->nspecial);
			if(index>=0)
			{
				vmin=list->special[index].code;
				ngot=1;
			}
		}
		if(ngot<=0)
		{
			ngot=sscanf(tmin, " %lg %1c",&vmin,&extra);
			if(ngot!=1) 
			{
				ErrorPrint(ParseBadValue,text,name);
				break;
			}
			if(list!=0 && list->minimum!=0 && vmin< *((float *)list->minimum))
			{
				ErrorPrint(ParseMinimumDouble,(double)vmin, (double)*((float *)list->minimum),name);
				break;
			}
			if(list!=0 && list->maximum!=0 && vmin> *((float *)list->maximum))
			{
				ErrorPrint(ParseMaximumDouble,(double)vmin, (double)*((float *)list->maximum),name);
				break;
			}
		}
		//
		// now parse the maximum value
		//
		if(tmax!=0)
		{
			ngot= -1;
			if(list!=0 && list->nspecial>0)
			{
				index=ParameterSelectIndex(tmin,list->special,list->nspecial);
				if(index>=0)
				{
					vmax=(float)list->special[index].code;
					ngot=1;
				}
			}
			if(ngot<=0)
			{
				ngot=sscanf(tmax, " %lg %1c",&vmax,&extra);
				if(ngot!=1) 
				{
					ErrorPrint(ParseBadValue,text,name);
					break;
				}
				if(list!=0 && list->minimum!=0 && vmax< *((float *)list->minimum))
				{
					ErrorPrint(ParseMinimumDouble,(double)vmax, (double)*((float *)list->minimum),name);
					break;
				}
				if(list!=0 && list->maximum!=0 && vmax> *((float *)list->maximum))
				{
					ErrorPrint(ParseMaximumDouble,(double)vmax, (double)*((float *)list->maximum),name);
					break;
				}
			}
		}
		else
		{
			vmax=vmin;
		}
		//
		// parse the increment
		//
		if(tinc!=0)
		{
			ngot=sscanf(tinc, " %lg %1c",&vinc,&extra);
			if(ngot!=1) 
			{
				ErrorPrint(ParseBadValue,text,name);
				break;
			}
		}
		else
		{
			vinc=1;
		}

		if(vinc>0)
		{
			if(vmax>=vmin)
			{
		        for(iv=vmin; iv<=vmax; iv+=vinc)
				{
			        if(ParameterValueMaximum(list)>0 && nvalue>=ParameterValueMaximum(list))
					{
				        ErrorPrint(ParseTooMany,name,ParameterValueMaximum(list));
                        nvalue= -1;
				        break;
					}
		            value[nvalue]=(float)iv;
		            nvalue++;
				}
			}
			else
			{
				ErrorPrint(ParsePositiveIncrement,name);
			}
		}
		else if(vinc<0)
		{
			if(vmax<=vmin)
			{
		        for(iv=vmin; iv>=vmax; iv+=vinc)
				{
			        if(ParameterValueMaximum(list)>0 && nvalue>=ParameterValueMaximum(list))
					{
				        ErrorPrint(ParseTooMany,name,ParameterValueMaximum(list));
                        nvalue= -1;
				        break;
					}
		            value[nvalue]=(float)iv;
		            nvalue++;
				}
			}
			else
			{
				ErrorPrint(ParseNegativeIncrement,name);
				continue;
			}
		}
		else
		{
		    value[nvalue]=(float)vmin;
		    nvalue++;
		}
	}

	return nvalue;
}


PARSEDLLSPEC int ParseFloat(int input, char *name, int max, float *value)
{
	struct _ParameterList list;

	list.nspecial=0;
	list.nx=max;
	list.ny=1;
	list.nz=1;
	list.minimum=0;
	list.maximum=0;

	return ParseFloatList(input,name,value,&list);
}


PARSEDLLSPEC int ParseDoubleList(int input, char *name, double *value, struct _ParameterList *list)
{
	int ip, np;
	int ngot;
	int nvalue;
	double vmin,vmax,vinc,iv;
	char *text, tcopy[MBUFFER];
	char buffer[MBUFFER];
	char *tmin, *tmax, *tinc;
	int index;
	int extra;

    ParseErrorInit();

    nvalue=0;		
	
	np=CommandParameterValueMany(input);
	for(ip=0; ip<np; ip++)
	{
		if(ParameterValueMaximum(list)>0 && nvalue>ParameterValueMaximum(list))
		{
			ErrorPrint(ParseTooMany,name,ParameterValueMaximum(list));
            nvalue= -1;
			break;
		}

        text=CommandParameterValue(input,ip);
		if(_ValueReplacement!=0)
		{
			(*_ValueReplacement)(text,buffer,MBUFFER);
			text=buffer;
		}

		SformatOutput(tcopy,MBUFFER-1,"%s",text);
		tcopy[MBUFFER-1]=0;
		tinc=0;
		tmax=0;
		tmin=tcopy;
		tmax=strchr(tcopy,':');
		if(tmax!=0)
		{
			*tmax=0;
			tmax++;
			tinc=strchr(tmax,':');
			if(tinc!=0)
			{
				*tinc=0;
				tinc++;
			}
		}
        //
		// parse the minimum value
		//
		ngot= -1;
		if(list->nspecial>0)
		{
			index=ParameterSelectIndex(tmin,list->special,list->nspecial);
			if(index>=0)
			{
				vmin=(double)list->special[index].code;
				ngot=1;
			}
		}
		if(ngot<=0)
		{
			ngot=sscanf(tmin, " %lg %1c",&vmin,&extra);
			if(ngot!=1) 
			{
				ErrorPrint(ParseBadValue,text,name);
				break;
			}
			if(list!=0 && list->minimum!=0 && vmin< *((double *)list->minimum))
			{
				ErrorPrint(ParseMinimumDouble,vmin, *((double *)list->minimum),name);
				break;
			}
			if(list!=0 && list->maximum!=0 && vmin> *((double *)list->maximum))
			{
				ErrorPrint(ParseMaximumDouble,vmin, *((double *)list->maximum),name);
				break;
			}
		}
		//
		// now parse the maximum value
		//
		if(tmax!=0)
		{
			ngot= -1;
			if(list!=0 && list->nspecial>0)
			{
				index=ParameterSelectIndex(tmin,list->special,list->nspecial);
				if(index>=0)
				{
					vmax=(double)list->special[index].code;
					ngot=1;
				}
			}
			if(ngot<=0)
			{
				ngot=sscanf(tmax, " %lg %1c",&vmax,&extra);
				if(ngot!=1) 
				{
					ErrorPrint(ParseBadValue,text,name);
					break;
				}
				if(list!=0 && list->minimum!=0 && vmax< *((double *)list->minimum))
				{
					ErrorPrint(ParseMinimumDouble,vmax, *((double *)list->minimum),name);
					break;
				}
				if(list!=0 && list->maximum!=0 && vmax> *((double *)list->maximum))
				{
					ErrorPrint(ParseMaximumDouble,vmax, *((double *)list->maximum),name);
					break;
				}
			}
		}
		else
		{
			vmax=vmin;
		}
		//
		// parse the increment
		//
		if(tinc!=0)
		{
			ngot=sscanf(tinc, " %lg %1c",&vinc,&extra);
			if(ngot!=1) 
			{
				ErrorPrint(ParseBadValue,text,name);
				break;
			}
		}
		else
		{
			vinc=1;
		}

		if(vinc>0)
		{
			if(vmax>=vmin)
			{
		        for(iv=vmin; iv<=vmax; iv+=vinc)
				{
			        if(ParameterValueMaximum(list)>0 && nvalue>=ParameterValueMaximum(list))
					{
				        ErrorPrint(ParseTooMany,name,ParameterValueMaximum(list));
                        nvalue= -1;
				        break;
					}
		            value[nvalue]=iv;
		            nvalue++;
				}
			}
			else
			{
				ErrorPrint(ParsePositiveIncrement,name);
			}
		}
		else if(vinc<0)
		{
			if(vmax<=vmin)
			{
		        for(iv=vmin; iv>=vmax; iv+=vinc)
				{
			        if(ParameterValueMaximum(list)>0 && nvalue>=ParameterValueMaximum(list))
					{
				        ErrorPrint(ParseTooMany,name,ParameterValueMaximum(list));
                        nvalue= -1;
				        break;
					}
		            value[nvalue]=iv;
		            nvalue++;
				}
			}
			else
			{
				ErrorPrint(ParseNegativeIncrement,name);
				continue;
			}
		}
		else
		{
		    value[nvalue]=vmin;
		    nvalue++;
		}
	}

	return nvalue;
}


PARSEDLLSPEC int ParseDouble(int input, char *name, int max, double *value)
{
	struct _ParameterList list;

	list.nspecial=0;
	list.nx=max;
	list.ny=1;
	list.nz=1;
	list.minimum=0;
	list.maximum=0;

	return ParseDoubleList(input,name,value,&list);
}

PARSEDLLSPEC int ParseAddressList(int input, char *name, unsigned int *low, unsigned int *high, int *increment, int have, struct _ParameterList *list)
{
	int ip, np;
	int ngot;
	int nvalue;
	unsigned int vmin,vmax,vinc;
	char *text, tcopy[MBUFFER];
	char buffer[MBUFFER];
	char *tmin, *tmax, *tinc;
	int index;
	int extra;
	char *tplus;

    ParseErrorInit();

    nvalue=0;		
	
	np=CommandParameterValueMany(input);
	for(ip=0; ip<np; ip++)
	{
		if(ParameterValueMaximum(list)>0 && nvalue+have>ParameterValueMaximum(list))
		{
			ErrorPrint(ParseTooMany,name,ParameterValueMaximum(list));
            nvalue= -1;
			break;
		}

        text=CommandParameterValue(input,ip);
		if(_ValueReplacement!=0)
		{
			(*_ValueReplacement)(text,buffer,MBUFFER);
			text=buffer;
		}

		SformatOutput(tcopy,MBUFFER-1,"%s",text);
		tcopy[MBUFFER-1]=0;
		tinc=0;
		tmax=0;
		tmin=tcopy;
		tmax=strchr(tcopy,':');
		if(tmax!=0)
		{
			*tmax=0;
			tmax++;
			tinc=strchr(tmax,':');
			if(tinc!=0)
			{
				*tinc=0;
				tinc++;
			}
		}
        //
		// parse the minimum value
		//
		ngot= -1;
		if(list->nspecial>0)
		{
			index=ParameterSelectIndex(tmin,list->special,list->nspecial);
			if(index>=0)
			{
				vmin=(unsigned int)list->special[index].code;
				ngot=1;
			}
		}
		if(ngot<=0)
		{
			//
			// scan text for + or -. we don't allow those in hex although sscanf does.
			//
			for(tplus=tmin; *tplus!=0 && *tplus!='+' && *tplus!='-'; tplus++);
			if(*tplus==0)
			{
				ngot=sscanf(tmin, " %x %1c",&vmin,&extra);
			}
			else
			{
				ngot= -1;
			}
			if(ngot!=1)
			{
				ngot=TryOtherFormats(tmin,&vmin);
			}
			if(ngot!=1) 
			{
				ErrorPrint(ParseBadValue,text,name);
				break;
			}
			if(list!=0 && list->minimum!=0 && vmin< *((unsigned int *)list->minimum))
			{
				ErrorPrint(ParseMinimumHex,vmin, *((unsigned int *)list->minimum),name);
				break;
			}
			if(list!=0 && list->maximum!=0 && vmin> *((unsigned int *)list->maximum))
			{
				ErrorPrint(ParseMaximumHex,vmin, *((unsigned int *)list->maximum),name);
				break;
			}
		}
		//
		// now parse the maximum value
		//
		if(tmax!=0)
		{
			ngot= -1;
			if(list!=0 && list->nspecial>0)
			{
				index=ParameterSelectIndex(tmin,list->special,list->nspecial);
				if(index>=0)
				{
					vmax=(unsigned int)list->special[index].code;
					ngot=1;
				}
			}
			if(ngot<=0)
			{
				//
				// scan text for + or -. we don't allow those in hex although sscanf does.
				//
				for(tplus=tmax; *tplus!=0 && *tplus!='+' && *tplus!='-'; tplus++);
				if(*tplus==0)
				{
					ngot=sscanf(tmax, " %x %1c",&vmax,&extra);
				}
				else
				{
					ngot= -1;
				}
				if(ngot!=1)
				{
					ngot=TryOtherFormats(tmax,&vmax);
				}
				if(ngot!=1) 
				{
					ErrorPrint(ParseBadValue,text,name);
					break;
				}
				if(list!=0 && list->minimum!=0 && vmax< *((unsigned int *)list->minimum))
				{
					ErrorPrint(ParseMinimumHex,vmax, *((unsigned int *)list->minimum),name);
					break;
				}
				if(list!=0 && list->maximum!=0 && vmax> *((unsigned int *)list->maximum))
				{
					ErrorPrint(ParseMaximumHex,vmax, *((unsigned int *)list->maximum),name);
					break;
				}
			}
		}
		else
		{
			vmax=vmin;
		}
		//
		// parse the increment
		//
		if(tinc!=0)
		{
			//
			// scan text for + or -. we don't allow those in hex although sscanf does.
			//
			for(tplus=tinc; *tplus!=0 && *tplus!='+' && *tplus!='-'; tplus++);
			if(*tplus==0)
			{
				ngot=sscanf(tinc, " %x %1c",&vinc,&extra);
			}
			else
			{
				ngot= -1;
			}
			if(ngot!=1)
			{
				ngot=TryOtherFormats(tinc,&vinc);
			}
			if(ngot!=1) 
			{
				ErrorPrint(ParseBadValue,text,name);
				break;
			}
		}
		else
		{
			vinc=0;
		}

		low[nvalue]=vmin;
		high[nvalue]=vmax;
		increment[nvalue]=vinc;
		nvalue++;
	}

	return nvalue;
}


PARSEDLLSPEC int ParseHexList(int input, char *name, unsigned int *value, struct _ParameterList *list)
{
	int ip, np;
	int ngot;
	int nvalue;
	unsigned int vmin,vmax,vinc,iv;
	char *text, tcopy[MBUFFER];
	char buffer[MBUFFER];
	char *tmin, *tmax, *tinc;
	int index;
	int extra;
	char *tplus;

    ParseErrorInit();

    nvalue=0;		
	
	np=CommandParameterValueMany(input);
	for(ip=0; ip<np; ip++)
	{
		if(ParameterValueMaximum(list)>0 && nvalue>ParameterValueMaximum(list))
		{
			ErrorPrint(ParseTooMany,name,ParameterValueMaximum(list));
            nvalue= -1;
			break;
		}

        text=CommandParameterValue(input,ip);
		if(_ValueReplacement!=0)
		{
			(*_ValueReplacement)(text,buffer,MBUFFER);
			text=buffer;
		}

		SformatOutput(tcopy,MBUFFER-1,"%s",text);
		tcopy[MBUFFER-1]=0;
		tinc=0;
		tmax=0;
		tmin=tcopy;
		tmax=strchr(tcopy,':');
		if(tmax!=0)
		{
			*tmax=0;
			tmax++;
			tinc=strchr(tmax,':');
			if(tinc!=0)
			{
				*tinc=0;
				tinc++;
			}
		}
        //
		// parse the minimum value
		//
		ngot= -1;
		if(list->nspecial>0)
		{
			index=ParameterSelectIndex(tmin,list->special,list->nspecial);
			if(index>=0)
			{
				vmin=(unsigned int)list->special[index].code;
				ngot=1;
			}
		}
		if(ngot<=0)
		{
			//
			// scan text for + or -. we don't allow those in hex although sscanf does.
			//
			for(tplus=tmin; *tplus!=0 && *tplus!='+' && *tplus!='-'; tplus++);
			if(*tplus==0)
			{
				ngot=sscanf(tmin, " %x %1c",&vmin,&extra);
			}
			else
			{
				ngot= -1;
			}
			if(ngot!=1)
			{
				ngot=TryOtherFormats(tmin,&vmin);
			}
			if(ngot!=1) 
			{
				ErrorPrint(ParseBadValue,text,name);
				break;
			}
			if(list!=0 && list->minimum!=0 && vmin< *((unsigned int *)list->minimum))
			{
				ErrorPrint(ParseMinimumHex,vmin, *((unsigned int *)list->minimum),name);
				break;
			}
			if(list!=0 && list->maximum!=0 && vmin> *((unsigned int *)list->maximum))
			{
				ErrorPrint(ParseMaximumHex,vmin, *((unsigned int *)list->maximum),name);
				break;
			}
		}
		//
		// now parse the maximum value
		//
		if(tmax!=0)
		{
			ngot= -1;
			if(list!=0 && list->nspecial>0)
			{
				index=ParameterSelectIndex(tmin,list->special,list->nspecial);
				if(index>=0)
				{
					vmax=(unsigned int)list->special[index].code;
					ngot=1;
				}
			}
			if(ngot<=0)
			{
				//
				// scan text for + or -. we don't allow those in hex although sscanf does.
				//
				for(tplus=tmax; *tplus!=0 && *tplus!='+' && *tplus!='-'; tplus++);
				if(*tplus==0)
				{
					ngot=sscanf(tmax, " %x %1c",&vmax,&extra);
				}
				else
				{
					ngot= -1;
				}
				if(ngot!=1)
				{
					ngot=TryOtherFormats(tmax,&vmax);
				}
				if(ngot!=1) 
				{
					ErrorPrint(ParseBadValue,text,name);
					break;
				}
				if(list!=0 && list->minimum!=0 && vmax< *((unsigned int *)list->minimum))
				{
					ErrorPrint(ParseMinimumHex,vmax, *((unsigned int *)list->minimum),name);
					break;
				}
				if(list!=0 && list->maximum!=0 && vmax> *((unsigned int *)list->maximum))
				{
					ErrorPrint(ParseMaximumHex,vmax, *((unsigned int *)list->maximum),name);
					break;
				}
			}
		}
		else
		{
			vmax=vmin;
		}
		//
		// parse the increment
		//
		if(tinc!=0)
		{
			//
			// scan text for + or -. we don't allow those in hex although sscanf does.
			//
			for(tplus=tinc; *tplus!=0 && *tplus!='+' && *tplus!='-'; tplus++);
			if(*tplus==0)
			{
				ngot=sscanf(tinc, " %x %1c",&vinc,&extra);
			}
			else
			{
				ngot= -1;
			}
			if(ngot!=1)
			{
				ngot=TryOtherFormats(tinc,&vinc);
			}
			if(ngot!=1) 
			{
				ErrorPrint(ParseBadValue,text,name);
				break;
			}
		}
		else
		{
			vinc=1;
		}

		if(vinc>0)
		{
			if(vmax>=vmin)
			{
		        for(iv=vmin; iv>=vmin && iv<=vmax; iv+=vinc)
				{
			        if(ParameterValueMaximum(list)>0 && nvalue>=ParameterValueMaximum(list))
					{
				        ErrorPrint(ParseTooMany,name,ParameterValueMaximum(list));
                        nvalue= -1;
				        break;
					}
		            value[nvalue]=iv;
		            nvalue++;
				}
			}
			else
			{
				ErrorPrint(ParsePositiveIncrement,name);
			}
		}
		else if(vinc<0)
		{
			if(vmax<=vmin)
			{
		        for(iv=vmin; iv<=vmin && iv>=vmax; iv+=vinc)
				{
			        if(ParameterValueMaximum(list)>0 && nvalue>=ParameterValueMaximum(list))
					{
				        ErrorPrint(ParseTooMany,name,ParameterValueMaximum(list));
                        nvalue= -1;
				        break;
					}
		            value[nvalue]=iv;
		            nvalue++;
				}
			}
			else
			{
				ErrorPrint(ParseNegativeIncrement,name);
				continue;
			}
		}
		else
		{
		    value[nvalue]=vmin;
		    nvalue++;
		}
	}

	return nvalue;
}


PARSEDLLSPEC int ParseHex(int input, char *name, int max, unsigned int *value)
{
	struct _ParameterList list;

	list.nspecial=0;
	list.nx=max;
	list.ny=1;
	list.nz=1;
	list.minimum=0;
	list.maximum=0;

	return ParseHexList(input,name,value,&list);
}

PARSEDLLSPEC int ParseUnsignedList(int input, char *name, unsigned int *value, struct _ParameterList *list)
{
	int ip, np;
	int ngot;
	int nvalue;
	unsigned int vmin,vmax,vinc,iv;
	char *text, tcopy[MBUFFER];
	char buffer[MBUFFER];
	char *tmin, *tmax, *tinc;
	int index;
	int extra;

    ParseErrorInit();

    nvalue=0;		
	
	np=CommandParameterValueMany(input);
	for(ip=0; ip<np; ip++)
	{
		if(ParameterValueMaximum(list)>0 && nvalue>ParameterValueMaximum(list))
		{
			ErrorPrint(ParseTooMany,name,ParameterValueMaximum(list));
            nvalue= -1;
			break;
		}

        text=CommandParameterValue(input,ip);
		if(_ValueReplacement!=0)
		{
			(*_ValueReplacement)(text,buffer,MBUFFER);
			text=buffer;
		}

		SformatOutput(tcopy,MBUFFER-1,"%s",text);
		tcopy[MBUFFER-1]=0;
		tinc=0;
		tmax=0;
		tmin=tcopy;
		tmax=strchr(tcopy,':');
		if(tmax!=0)
		{
			*tmax=0;
			tmax++;
			tinc=strchr(tmax,':');
			if(tinc!=0)
			{
				*tinc=0;
				tinc++;
			}
		}
        //
		// parse the minimum value
		//
		ngot= -1;
		if(list->nspecial>0)
		{
			index=ParameterSelectIndex(tmin,list->special,list->nspecial);
			if(index>=0)
			{
				vmin=(unsigned int)list->special[index].code;
				ngot=1;
			}
		}
		if(ngot<=0)
		{
			ngot=sscanf(tmin, " %u %1c",&vmin,&extra);
			if(ngot!=1)
			{
				ngot=TryOtherFormats(tmin,&vmin);
			}
			if(ngot!=1) 
			{
				ErrorPrint(ParseBadValue,text,name);
				break;
			}
			if(list!=0 && list->minimum!=0 && vmin< *((unsigned int *)list->minimum))
			{
				ErrorPrint(ParseMinimumUnsigned,vmin, *((unsigned int *)list->minimum),name);
				break;
			}
			if(list!=0 && list->maximum!=0 && vmin> *((unsigned int *)list->maximum))
			{
				ErrorPrint(ParseMaximumUnsigned,vmin, *((unsigned int *)list->maximum),name);
				break;
			}
		}
		//
		// now parse the maximum value
		//
		if(tmax!=0)
		{
			ngot= -1;
			if(list!=0 && list->nspecial>0)
			{
				index=ParameterSelectIndex(tmin,list->special,list->nspecial);
				if(index>=0)
				{
					vmax=(unsigned int)list->special[index].code;
					ngot=1;
				}
			}
			if(ngot<=0)
			{
				ngot=sscanf(tmax, " %u %1c",&vmax,&extra);
				if(ngot!=1)
				{
					ngot=TryOtherFormats(tmax,&vmax);
				}
				if(ngot!=1) 
				{
					ErrorPrint(ParseBadValue,text,name);
					break;
				}
				if(list!=0 && list->minimum!=0 && vmax< *((unsigned int *)list->minimum))
				{
					ErrorPrint(ParseMinimumUnsigned,vmax, *((unsigned int *)list->minimum),name);
					break;
				}
				if(list!=0 && list->maximum!=0 && vmax> *((unsigned int *)list->maximum))
				{
					ErrorPrint(ParseMaximumUnsigned,vmax, *((unsigned int *)list->maximum),name);
					break;
				}
			}
		}
		else
		{
			vmax=vmin;
		}
		//
		// parse the increment
		//
		if(tinc!=0)
		{
			ngot=sscanf(tinc, " %u %1c",&vinc,&extra);
			if(ngot!=1)
			{
				ngot=TryOtherFormats(tinc,&vinc);
			}
			if(ngot!=1) 
			{
				ErrorPrint(ParseBadValue,text,name);
				break;
			}
		}
		else
		{
			vinc=1;
		}

		if(vinc>0)
		{
			if(vmax>=vmin)
			{
		        for(iv=vmin; iv>=vmin && iv<=vmax; iv+=vinc)
				{
			        if(ParameterValueMaximum(list)>0 && nvalue>=ParameterValueMaximum(list))
					{
				        ErrorPrint(ParseTooMany,name,ParameterValueMaximum(list));
                        nvalue= -1;
				        break;
					}
		            value[nvalue]=iv;
		            nvalue++;
				}
			}
			else
			{
				ErrorPrint(ParsePositiveIncrement,name);
			}
		}
		else if(vinc<0)
		{
			if(vmax<=vmin)
			{
		        for(iv=vmin; iv<=vmin && iv>=vmax; iv+=vinc)
				{
			        if(ParameterValueMaximum(list)>0 && nvalue>=ParameterValueMaximum(list))
					{
				        ErrorPrint(ParseTooMany,name,ParameterValueMaximum(list));
                        nvalue= -1;
				        break;
					}
		            value[nvalue]=iv;
		            nvalue++;
				}
			}
			else
			{
				ErrorPrint(ParseNegativeIncrement,name);
				continue;
			}
		}
		else
		{
		    value[nvalue]=vmin;
		    nvalue++;
		}
	}

	return nvalue;
}


PARSEDLLSPEC int ParseUnsigned(int input, char *name, int max, unsigned int *value)
{
	struct _ParameterList list;

	list.nspecial=0;
	list.nx=max;
	list.ny=1;
	list.nz=1;
	list.minimum=0;
	list.maximum=0;

	return ParseIntegerList(input,name,value,&list);
}


PARSEDLLSPEC int ParseStringAndSetRates(int input, char *name, int max, int *value)
{
	int ip, np;
	int ngot=0;
	int nvalue;
	int code;
	char *text;
	char buffer[MBUFFER];

    ParseErrorInit();

    nvalue=0;		
	for (ip=0; ip<max; ip++) {
		value[ip]=0;
	}

	np=CommandParameterValueMany(input);
	for(ip=0; ip<np; ip++)
	{
		if(nvalue>max)
		{
			ErrorPrint(ParseTooMany,name,max);
            nvalue= -1;
			break;
		}

        text=CommandParameterValue(input,ip);
		if(_ValueReplacement!=0)
		{
			(*_ValueReplacement)(text,buffer,MBUFFER);
			text=buffer;
		}

        code=ParameterSelect(text,rates,sizeof(rates)/sizeof(struct _ParameterList));

		switch (code)
		{
			case code_all:
				value[0]=0x7fff;
				value[1]=0xffffff;
				value[2]=0x3fffffff;
				value[3]=0x3fffffff;
				value[4]=0x3fffffff;
				value[5]=0x3fffffff;
				ngot=1;
			    break;
			case code_all_legacy:
				value[0]=0x7fff;
				ngot=1;
			    break;
			case code_all_mcs_ht_20:  
				value[1]=0xffffff;
				ngot=1;
			    break;
			case code_all_mcs_ht_40:  
				value[2]=0xffffff;
				ngot=1;
			    break;
			case code_all_vmcs_ht_20:  
				value[3]=0x3fffffff;
				ngot=1;
			    break;
			case code_all_vmcs_ht_40:  
				value[4]=0x3fffffff;
				ngot=1;
			    break;
			case code_all_vmcs_ht_80:  
				value[5]=0x3fffffff;
				ngot=1;
			    break;
			case code_all_dvt:  
				value[0]|=0x4181;
				value[1]|=0x8181;
				value[2]|=0x8181;
				ngot=1;
			    break;
			case code_all_vdvt:  
				value[3]|=0x20180601;
				value[4]|=0x20180601;
				value[5]|=0x20180601;
				ngot=1;
			    break;
			case code_11s:  
				value[0]|=0x4000;
				ngot=1;
			    break;
			case code_11l:  
				value[0]|=0x2000;
				ngot=1;
			    break;
			case code_5s:  
				value[0]|=0x1000;
				ngot=1;
			    break;
			case code_5l:  
				value[0]|=0x0800;
				ngot=1;
			    break;
			case code_2s:  
				value[0]|=0x0400;
				ngot=1;
			    break;
			case code_2l:  
				value[0]|=0x0200;
				ngot=1;
			    break;
			case code_1l:  
				value[0]|=0x0100;
				ngot=1;
			    break;
			case code_6:  
				value[0]|=0x0001;
				ngot=1;
			    break;
			case code_9:  
				value[0]|=0x0002;
				ngot=1;
			    break;
			case code_12:  
				value[0]|=0x0004;
				ngot=1;
			    break;
			case code_18:  
				value[0]|=0x0008;
				ngot=1;
			    break;
			case code_24:  
				value[0]|=0x0010;
				ngot=1;
			    break;
			case code_36:  
				value[0]|=0x0020;
				ngot=1;
			    break;
			case code_48:  
				value[0]|=0x0040;
				ngot=1;
			    break;
			case code_54:  
				value[0]|=0x0080;
				ngot=1;
			    break;
			case code_t0:  
				value[1]|=0x0001;
				ngot=1;
			    break;
			case code_t1:				
				value[1]|=0x0002;
				ngot=1;
			    break;
			case code_t2:  
				value[1]|=0x0004;
				ngot=1;
			    break;
			case code_t3:  
				value[1]|=0x0008;
				ngot=1;
			    break;
			case code_t4:  
				value[1]|=0x0010;
				ngot=1;
			    break;
			case code_t5:  
				value[1]|=0x0020;
				ngot=1;
			    break;
			case code_t6:  
				value[1]|=0x0040;
				ngot=1;
			    break;
			case code_t7:  
				value[1]|=0x0080;
				ngot=1;
			    break;
			case code_t8:  
				value[1]|=0x0100;
				ngot=1;
			    break;
			case code_t9:  
				value[1]|=0x0200;
				ngot=1;
			    break;
			case code_t10:  
				value[1]|=0x0400;
				ngot=1;
			    break;
			case code_t11:  
				value[1]|=0x0800;
				ngot=1;
			    break;
			case code_t12:  
				value[1]|=0x1000;
				ngot=1;
			    break;
			case code_t13:  
				value[1]|=0x2000;
				ngot=1;
			    break;
			case code_t14:  
				value[1]|=0x4000;
				ngot=1;
			    break;
			case code_t15:  
				value[1]|=0x8000;
				ngot=1;
			    break;
			case code_t16:  
				value[1]|=0x10000;
				ngot=1;
			    break;
			case code_t17:  
				value[1]|=0x20000;
				ngot=1;
			    break;
			case code_t18:  
				value[1]|=0x40000;
				ngot=1;
			    break;
			case code_t19:  
				value[1]|=0x80000;
				ngot=1;
			    break;
			case code_t20:  
				value[1]|=0x100000;
				ngot=1;
			    break;
			case code_t21:  
				value[1]|=0x200000;
				ngot=1;
			    break;
			case code_t22:  
				value[1]|=0x400000;
				ngot=1;
			    break;
			case code_t23:  
				value[1]|=0x800000;
				ngot=1;
			    break;
			case code_vt0:  
				value[3]|=0x0001;
				ngot=1;
			    break;
			case code_vt1:				
				value[3]|=0x0002;
				ngot=1;
			    break;
			case code_vt2:  
				value[3]|=0x0004;
				ngot=1;
			    break;
			case code_vt3:  
				value[3]|=0x0008;
				ngot=1;
			    break;
			case code_vt4:  
				value[3]|=0x0010;
				ngot=1;
			    break;
			case code_vt5:  
				value[3]|=0x0020;
				ngot=1;
			    break;
			case code_vt6:  
				value[3]|=0x0040;
				ngot=1;
			    break;
			case code_vt7:  
				value[3]|=0x0080;
				ngot=1;
			    break;
			case code_vt8:  
				value[3]|=0x0100;
				ngot=1;
			    break;
			case code_vt9:  
				value[3]|=0x0200;
				ngot=1;
			    break;
			case code_vt10:  
				value[3]|=0x0400;
				ngot=1;
			    break;
			case code_vt11:  
				value[3]|=0x0800;
				ngot=1;
			    break;
			case code_vt12:  
				value[3]|=0x1000;
				ngot=1;
			    break;
			case code_vt13:  
				value[3]|=0x2000;
				ngot=1;
			    break;
			case code_vt14:  
				value[3]|=0x4000;
				ngot=1;
			    break;
			case code_vt15:  
				value[3]|=0x8000;
				ngot=1;
			    break;
			case code_vt16:  
				value[3]|=0x10000;
				ngot=1;
			    break;
			case code_vt17:  
				value[3]|=0x20000;
				ngot=1;
			    break;
			case code_vt18:  
				value[3]|=0x40000;
				ngot=1;
			    break;
			case code_vt19:  
				value[3]|=0x80000;
				ngot=1;
			    break;
			case code_vt20:  
				value[3]|=0x100000;
				ngot=1;
			    break;
			case code_vt21:  
				value[3]|=0x200000;
				ngot=1;
			    break;
			case code_vt22:  
				value[3]|=0x400000;
				ngot=1;
			    break;
			case code_vt23:  
				value[3]|=0x800000;
				ngot=1;
			    break;
			case code_vt24:  
				value[3]|=0x1000000;
				ngot=1;
			    break;
			case code_vt25:  
				value[3]|=0x2000000;
				ngot=1;
			    break;
			case code_vt26:  
				value[3]|=0x4000000;
				ngot=1;
			    break;
			case code_vt27:  
				value[3]|=0x8000000;
				ngot=1;
			    break;
			case code_vt28:  
				value[3]|=0x10000000;
				ngot=1;
			    break;
			case code_vt29:  
				value[3]|=0x20000000;
				ngot=1;
			    break;
			case code_f0:  
				value[2]|=0x0001;
				ngot=1;
			    break;
			case code_f1:  
				value[2]|=0x0002;
				ngot=1;
			    break;
			case code_f2:  
				value[2]|=0x0004;
				ngot=1;
			    break;
			case code_f3:  
				value[2]|=0x0008;
				ngot=1;
			    break;
			case code_f4:  
				value[2]|=0x0010;
				ngot=1;
			    break;
			case code_f5:  
				value[2]|=0x0020;
				ngot=1;
			    break;
			case code_f6:  
				value[2]|=0x0040;
				ngot=1;
			    break;
			case code_f7:  
				value[2]|=0x0080;
				ngot=1;
			    break;
			case code_f8:  
				value[2]|=0x0100;
				ngot=1;
			    break;
			case code_f9:  
				value[2]|=0x0200;
				ngot=1;
			    break;
			case code_f10:  
				value[2]|=0x0400;
				ngot=1;
			    break;
			case code_f11:  
				value[2]|=0x0800;
				ngot=1;
			    break;
			case code_f12:  
				value[2]|=0x1000;
				ngot=1;
			    break;
			case code_f13:  
				value[2]|=0x2000;
				ngot=1;
			    break;
			case code_f14:  
				value[2]|=0x4000;
				ngot=1;
			    break;
			case code_f15:  
				value[2]|=0x8000;
				ngot=1;
			    break;
			case code_f16:  
				value[2]|=0x10000;
				ngot=1;
			    break;
			case code_f17:  
				value[2]|=0x20000;
				ngot=1;
			    break;
			case code_f18:  
				value[2]|=0x40000;
				ngot=1;
			    break;
			case code_f19:  
				value[2]|=0x80000;
				ngot=1;
			    break;
			case code_f20:  
				value[2]|=0x100000;
				ngot=1;
			    break;
			case code_f21:  
				value[2]|=0x200000;
				ngot=1;
			    break;
			case code_f22:  
				value[2]|=0x400000;
				ngot=1;
			    break;
			case code_f23:  
				value[2]|=0x800000;
				ngot=1;
			    break;
			case code_vf0:  
				value[4]|=0x0001;
				ngot=1;
			    break;
			case code_vf1:  
				value[4]|=0x0002;
				ngot=1;
			    break;
			case code_vf2:  
				value[4]|=0x0004;
				ngot=1;
			    break;
			case code_vf3:  
				value[4]|=0x0008;
				ngot=1;
			    break;
			case code_vf4:  
				value[4]|=0x0010;
				ngot=1;
			    break;
			case code_vf5:  
				value[4]|=0x0020;
				ngot=1;
			    break;
			case code_vf6:  
				value[4]|=0x0040;
				ngot=1;
			    break;
			case code_vf7:  
				value[4]|=0x0080;
				ngot=1;
			    break;
			case code_vf8:  
				value[4]|=0x0100;
				ngot=1;
			    break;
			case code_vf9:  
				value[4]|=0x0200;
				ngot=1;
			    break;
			case code_vf10:  
				value[4]|=0x0400;
				ngot=1;
			    break;
			case code_vf11:  
				value[4]|=0x0800;
				ngot=1;
			    break;
			case code_vf12:  
				value[4]|=0x1000;
				ngot=1;
			    break;
			case code_vf13:  
				value[4]|=0x2000;
				ngot=1;
			    break;
			case code_vf14:  
				value[4]|=0x4000;
				ngot=1;
			    break;
			case code_vf15:  
				value[4]|=0x8000;
				ngot=1;
			    break;
			case code_vf16:  
				value[4]|=0x10000;
				ngot=1;
			    break;
			case code_vf17:  
				value[4]|=0x20000;
				ngot=1;
			    break;
			case code_vf18:  
				value[4]|=0x40000;
				ngot=1;
			    break;
			case code_vf19:  
				value[4]|=0x80000;
				ngot=1;
			    break;
			case code_vf20:  
				value[4]|=0x100000;
				ngot=1;
			    break;
			case code_vf21:  
				value[4]|=0x200000;
				ngot=1;
			    break;
			case code_vf22:  
				value[4]|=0x400000;
				ngot=1;
			    break;
			case code_vf23:  
				value[4]|=0x800000;
				ngot=1;
			    break;
			case code_vf24:  
				value[4]|=0x1000000;
				ngot=1;
			    break;
			case code_vf25:  
				value[4]|=0x2000000;
				ngot=1;
			    break;
			case code_vf26:  
				value[4]|=0x4000000;
				ngot=1;
			    break;
			case code_vf27:  
				value[4]|=0x8000000;
				ngot=1;
			    break;
			case code_vf28:  
				value[4]|=0x10000000;
				ngot=1;
			    break;
			case code_vf29:  
				value[4]|=0x20000000;
				ngot=1;
			    break;
			case code_ve0:  
				value[5]|=0x0001;
				ngot=1;
			    break;
			case code_ve1:  
				value[5]|=0x0002;
				ngot=1;
			    break;
			case code_ve2:  
				value[5]|=0x0004;
				ngot=1;
			    break;
			case code_ve3:  
				value[5]|=0x0008;
				ngot=1;
			    break;
			case code_ve4:  
				value[5]|=0x0010;
				ngot=1;
			    break;
			case code_ve5:  
				value[5]|=0x0020;
				ngot=1;
			    break;
			case code_ve6:  
				value[5]|=0x0040;
				ngot=1;
			    break;
			case code_ve7:  
				value[5]|=0x0080;
				ngot=1;
			    break;
			case code_ve8:  
				value[5]|=0x0100;
				ngot=1;
			    break;
			case code_ve9:  
				value[5]|=0x0200;
				ngot=1;
			    break;
			case code_ve10:  
				value[5]|=0x0400;
				ngot=1;
			    break;
			case code_ve11:  
				value[5]|=0x0800;
				ngot=1;
			    break;
			case code_ve12:  
				value[5]|=0x1000;
				ngot=1;
			    break;
			case code_ve13:  
				value[5]|=0x2000;
				ngot=1;
			    break;
			case code_ve14:  
				value[5]|=0x4000;
				ngot=1;
			    break;
			case code_ve15:  
				value[5]|=0x8000;
				ngot=1;
			    break;
			case code_ve16:  
				value[5]|=0x10000;
				ngot=1;
			    break;
			case code_ve17:  
				value[5]|=0x20000;
				ngot=1;
			    break;
			case code_ve18:  
				value[5]|=0x40000;
				ngot=1;
			    break;
			case code_ve19:  
				value[5]|=0x80000;
				ngot=1;
			    break;
			case code_ve20:  
				value[5]|=0x100000;
				ngot=1;
			    break;
			case code_ve21:  
				value[5]|=0x200000;
				ngot=1;
			    break;
			case code_ve22:  
				value[5]|=0x400000;
				ngot=1;
			    break;
			case code_ve23:  
				value[5]|=0x800000;
				ngot=1;
			    break;
			case code_ve24:  
				value[5]|=0x1000000;
				ngot=1;
			    break;
			case code_ve25:  
				value[5]|=0x2000000;
				ngot=1;
			    break;
			case code_ve26:  
				value[5]|=0x4000000;
				ngot=1;
			    break;
			case code_ve27:  
				value[5]|=0x8000000;
				ngot=1;
			    break;
			case code_ve28:  
				value[5]|=0x10000000;
				ngot=1;
			    break;
			case code_ve29:  
				value[5]|=0x20000000;
				ngot=1;
			    break;
		}

	}
	return ngot;
}


//
// parses mac address value.
// returns 0 on success. nonzero error on failure
//
PARSEDLLSPEC int ParseMacAddress(char *buffer, unsigned char *cmac)
{
	int ngot;
	int mac[6];
	int it;

	for(it=0; it<6; it++)
	{
		cmac[it]=0;
	}
    //
	// try with dots
	//
	ngot=SformatInput(buffer," %x.%x.%x.%x.%x.%x ",
		&mac[0],&mac[1],&mac[2],&mac[3],&mac[4],&mac[5]);
	if(ngot!=6)
	{
        //
	    // try with colons
	    //
	    ngot=SformatInput(buffer," %x:%x:%x:%x:%x:%x ",
		    &mac[0],&mac[1],&mac[2],&mac[3],&mac[4],&mac[5]);
	    if(ngot!=6)
		{
            //
	        // try with nothing
	        //
	        ngot=SformatInput(buffer," %2x%2x%2x%2x%2x%2x ",
		        &mac[0],&mac[1],&mac[2],&mac[3],&mac[4],&mac[5]);
		}
	}
	if(ngot==6)
	{
		for(it=0; it<6; it++)
		{
			cmac[it]=mac[it];
		}
	}
	return (ngot!=6);
}
