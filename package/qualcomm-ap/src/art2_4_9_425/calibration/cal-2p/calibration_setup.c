#define _CRT_SECURE_NO_DEPRECATE  1
#define _CRT_NONSTDC_NO_DEPRECATE 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "UserPrint.h"
#include "ErrorPrint.h"
#include "ParseError.h"

#include "smatch.h"
#include "calibration.h"
#include "calibration_setup.h"

#define MBUFFER 1024
#define MARG 100
#define MVALUE 100
#define MWORD 100

char _Input[MBUFFER];

struct _Parameter
{
    char name[MWORD];
    int nvalue;
    char value[MVALUE][MWORD];
};

// the total number of parameters
// the total is the sum of all of the saved sets plus the current set
//
static int ArgMany=0;
//
// the list of all of the parameters
//
static struct _Parameter Arg[MARG];


static int IsComment(char *buffer)
{
	char *ptr;
	//
	// we ignore lines that start with #
	//
	for(ptr=buffer; *ptr!=0; ptr++)
	{
		//
		// found comment character
		//
		if(*ptr=='#')
		{
			return 1;
		}
		//
		// skip spaces
		//
		else if(!(*ptr==' ' || *ptr=='\t'))
		{
			return 0;
		}
	}
	return 1;
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
int CommandParse(char *buffer)
{
    char *ptr,*pname,*pvalue,*sptr = NULL;
    int equal;
    int done = 0;
	int noEqualSign = 0;
	int isArray = 0;
	//
    // find beginning of command Word
    //
    for(ptr=buffer; *ptr!=0; ptr++)
    {
        if(!(*ptr==' ' || *ptr=='\t'))
        {
            break;
        }
    }

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

//        if(*ptr=='=')	// get sometime doesn't have =
        if(*ptr=='=' || *ptr==';' || *ptr=='\t' || *ptr==' ')
        {
			if (*ptr=='\t' || *ptr==' ')
				isArray = 1;

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
		return 0;
    //
    // skip leading spaces on parameter values
    //
    for(Arg[ArgMany].nvalue=0; Arg[ArgMany].nvalue<MARG && *ptr!=0; )
    {
        pvalue=Arg[ArgMany].value[Arg[ArgMany].nvalue];
        for( ; *ptr!=0 ; ptr++)
        {
            if(!(*ptr==' ' || *ptr=='\t'))
            {
                Arg[ArgMany].nvalue++;
                break;
            }
        }
        //
        // now go looking for a semicolon or comma
        //
        for( ; *ptr!=0 ; ptr++)
        {
            if(*ptr==',' || *ptr==' ' || *ptr=='\t')
			{
                *pvalue=0;
                ptr++;
                done=0;
                break;
			}
			else if(*ptr==';' || *sptr==' ' || *sptr=='\t' || *ptr=='#' || *ptr=='\n')
			{
                *pvalue=0;
                ptr++;
                done=1;
                break;
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
	if (Arg[ArgMany].nvalue>0)
		ArgMany++;
    return ArgMany;
}

int SettingSelect()
{
	int arg;
	int i;
	for (arg=0; arg<ArgMany; arg++) {
		if (Sequal(Arg[arg].name, CALIBRATION_SCHEME)) {
			cal.scheme = atoi(Arg[arg].value[0]);
		} else if (Sequal(Arg[arg].name, POWER_GOAL_MODE)) {
			cal.PowerGoalMode = atoi(Arg[arg].value[0]);
		} else if (Sequal(Arg[arg].name, GAIN_CHANN_2G)) {
			for (i=0; i<Arg[arg].nvalue; i++) {
				if (Slength(Arg[arg].value[i])>0) {
					cal.gainChann_2g[i] = atoi(Arg[arg].value[i]);
				} else {
					Arg[arg].nvalue--;
				}
			}
		} else if (Sequal(Arg[arg].name, GAIN_INDEX_2G_CH0)) {
			for (i=0; i<Arg[arg].nvalue; i++) {
				if (Slength(Arg[arg].value[i])>0) {
					cal.gainIndex_2g_ch0[i] = atoi(Arg[arg].value[i]);
				} else {
					Arg[arg].nvalue--;
				}
			}
		}else if (Sequal(Arg[arg].name, GAIN_INDEX_2G_CH1)) {
			for (i=0; i<Arg[arg].nvalue; i++) {
				if (Slength(Arg[arg].value[i])>0) {
					cal.gainIndex_2g_ch1[i] = atoi(Arg[arg].value[i]);
				} else {
					Arg[arg].nvalue--;
				}
			}
		}else if (Sequal(Arg[arg].name, GAIN_INDEX_2G_CH2)) {
			for (i=0; i<Arg[arg].nvalue; i++) {
				if (Slength(Arg[arg].value[i])>0) {
					cal.gainIndex_2g_ch2[i] = atoi(Arg[arg].value[i]);
				} else {
					Arg[arg].nvalue--;
				}
			}
		} else if (Sequal(Arg[arg].name, GAIN_INDEX2_2G_CH0)) {
			for (i=0; i<Arg[arg].nvalue; i++) {
				if (Slength(Arg[arg].value[i])>0) {
					cal.gainIndex2_2g_ch0[i] = atoi(Arg[arg].value[i]);
				} else {
					Arg[arg].nvalue--;
				}
			}
		}else if (Sequal(Arg[arg].name, GAIN_INDEX2_2G_CH1)) {
			for (i=0; i<Arg[arg].nvalue; i++) {
				if (Slength(Arg[arg].value[i])>0) {
					cal.gainIndex2_2g_ch1[i] = atoi(Arg[arg].value[i]);
				} else {
					Arg[arg].nvalue--;
				}
			}
		}else if (Sequal(Arg[arg].name, GAIN_INDEX2_2G_CH2)) {
			for (i=0; i<Arg[arg].nvalue; i++) {
				if (Slength(Arg[arg].value[i])>0) {
					cal.gainIndex2_2g_ch2[i] = atoi(Arg[arg].value[i]);
				} else {
					Arg[arg].nvalue--;
				}
			}
		} else if (Sequal(Arg[arg].name, GAIN_INDEX2_DELTA_2G)) {
			for (i=0; i<Arg[arg].nvalue; i++) {
				if (Slength(Arg[arg].value[i])>0) {
					cal.gainIndex2Delta_2g[i] = atoi(Arg[arg].value[i]);
				} else {
					Arg[arg].nvalue--;
				}
			}
		} else if (Sequal(Arg[arg].name, DAC_GAIN_2G)) {
			for (i=0; i<Arg[arg].nvalue; i++) {
				if (Slength(Arg[arg].value[i])>0) {
					cal.dacGain_2g[i] = atoi(Arg[arg].value[i]);
				} else {
					Arg[arg].nvalue--;
				}
			}
		} else if (Sequal(Arg[arg].name, DAC_GAIN2_2G)) {
			for (i=0; i<Arg[arg].nvalue; i++) {
				if (Slength(Arg[arg].value[i])>0) {
					cal.dacGain2_2g[i] = atoi(Arg[arg].value[i]);
				} else {
					Arg[arg].nvalue--;
				}
			}
		} else if (Sequal(Arg[arg].name, POWER_GOAL_2G)) {
			for (i=0; i<Arg[arg].nvalue; i++) {
				if (Slength(Arg[arg].value[i])>0) {
					cal.PowerGoal_2g[i] = atof(Arg[arg].value[i]);
				} else {
					Arg[arg].nvalue--;
				}
			}
		} else if (Sequal(Arg[arg].name, POWER_GOAL2_2G)) {
			for (i=0; i<Arg[arg].nvalue; i++) {
				if (Slength(Arg[arg].value[i])>0) {
					cal.PowerGoal2_2g[i] = atof(Arg[arg].value[i]);
				} else {
					Arg[arg].nvalue--;
				}
			}
		} else if (Sequal(Arg[arg].name, GAIN_CHANN_5G)) {
			for (i=0; i<Arg[arg].nvalue; i++) {
				if (Slength(Arg[arg].value[i])>0) {
					cal.gainChann_5g[i] = atoi(Arg[arg].value[i]);
				} else {
					Arg[arg].nvalue--;
				}
			}
		} else if (Sequal(Arg[arg].name, GAIN_INDEX_5G_CH0)) {
			for (i=0; i<Arg[arg].nvalue; i++) {
				if (Slength(Arg[arg].value[i])>0) {
					cal.gainIndex_5g_ch0[i] = atoi(Arg[arg].value[i]);
				} else {
					Arg[arg].nvalue--;
				}
			}
		} else if (Sequal(Arg[arg].name, GAIN_INDEX_5G_CH1)) {
			for (i=0; i<Arg[arg].nvalue; i++) {
				if (Slength(Arg[arg].value[i])>0) {
					cal.gainIndex_5g_ch1[i] = atoi(Arg[arg].value[i]);
				} else {
					Arg[arg].nvalue--;
				}
			}
		} else if (Sequal(Arg[arg].name, GAIN_INDEX_5G_CH2)) {
			for (i=0; i<Arg[arg].nvalue; i++) {
				if (Slength(Arg[arg].value[i])>0) {
					cal.gainIndex_5g_ch2[i] = atoi(Arg[arg].value[i]);
				} else {
					Arg[arg].nvalue--;
				}
			}
		} else if (Sequal(Arg[arg].name, GAIN_INDEX2_5G_CH0)) {
			for (i=0; i<Arg[arg].nvalue; i++) {
				if (Slength(Arg[arg].value[i])>0) {
					cal.gainIndex2_5g_ch0[i] = atoi(Arg[arg].value[i]);
				} else {
					Arg[arg].nvalue--;
				}
			}
		} else if (Sequal(Arg[arg].name, GAIN_INDEX2_5G_CH1)) {
			for (i=0; i<Arg[arg].nvalue; i++) {
				if (Slength(Arg[arg].value[i])>0) {
					cal.gainIndex2_5g_ch1[i] = atoi(Arg[arg].value[i]);
				} else {
					Arg[arg].nvalue--;
				}
			}
		} else if (Sequal(Arg[arg].name, GAIN_INDEX2_5G_CH2)) {
			for (i=0; i<Arg[arg].nvalue; i++) {
				if (Slength(Arg[arg].value[i])>0) {
					cal.gainIndex2_5g_ch2[i] = atoi(Arg[arg].value[i]);
				} else {
					Arg[arg].nvalue--;
				}
			}
		} else if (Sequal(Arg[arg].name, GAIN_INDEX2_DELTA_5G)) {
			for (i=0; i<Arg[arg].nvalue; i++) {
				if (Slength(Arg[arg].value[i])>0) {
					cal.gainIndex2Delta_5g[i] = atoi(Arg[arg].value[i]);
				} else {
					Arg[arg].nvalue--;
				}
			}
		} else if (Sequal(Arg[arg].name, DAC_GAIN_5G)) {
			for (i=0; i<Arg[arg].nvalue; i++) {
				if (Slength(Arg[arg].value[i])>0) {
					cal.dacGain_5g[i] = atoi(Arg[arg].value[i]);
				} else {
					Arg[arg].nvalue--;
				}
			}
		} else if (Sequal(Arg[arg].name, DAC_GAIN2_5G)) {
			for (i=0; i<Arg[arg].nvalue; i++) {
				if (Slength(Arg[arg].value[i])>0) {
					cal.dacGain2_5g[i] = atoi(Arg[arg].value[i]);
				} else {
					Arg[arg].nvalue--;
				}
			}
		} else if (Sequal(Arg[arg].name, POWER_GOAL_5G)) {
			for (i=0; i<Arg[arg].nvalue; i++) {
				if (Slength(Arg[arg].value[i])>0) {
					cal.PowerGoal_5g[i] = atof(Arg[arg].value[i]);
				} else {
					Arg[arg].nvalue--;
				}
			}
		} else if (Sequal(Arg[arg].name, POWER_GOAL2_5G)) {
			for (i=0; i<Arg[arg].nvalue; i++) {
				if (Slength(Arg[arg].value[i])>0) {
					cal.PowerGoal2_5g[i] = atof(Arg[arg].value[i]);
				} else {
					Arg[arg].nvalue--;
				}
			}
		} else if (Sequal(Arg[arg].name, POWER_DEVIATION)) {
			cal.powerDeviation = atof(Arg[arg].value[0]);
		} else if (Sequal(Arg[arg].name, POWER_DEVIATION2)) {
			cal.powerDeviation2 = atof(Arg[arg].value[0]);
		} else if (Sequal(Arg[arg].name, TXGAIN_SLOPE)) {
			cal.txgainSlope = atof(Arg[arg].value[0]);
		} else if (Sequal(Arg[arg].name, TXGAIN_SLOPE2)) {
			cal.txgainSlope2 = atof(Arg[arg].value[0]);
		} else if (Sequal(Arg[arg].name, CALIBRATION_ATTEMPT)) {
			cal.attempt = atoi(Arg[arg].value[0]);
		} else if (Sequal(Arg[arg].name, CALIBRATION_ATTEMPT2)) {
			cal.attempt2 = atoi(Arg[arg].value[0]);
        }else if (Sequal(Arg[arg].name, RESET_UNUSED_PIERS)) {
            cal.resetUnusedCalPiers = atoi(Arg[arg].value[0]);
        }
	}
	return 0;
}

int setup_file(char *filename)
{
	FILE *File;
	char *eof;
	int length;
	char buffer[MBUFFER];

	File = fopen(filename,"r");
    if(File)
	{
		while (1) {
			eof=fgets(buffer,MBUFFER,File);
			if(eof!=buffer) {
				fclose(File);
				break;
			}
			length=Slength(buffer);
			if (length<=0)
				break;
			if(!IsComment(buffer))
			{
				strcpy(_Input,buffer);
				CommandParse(buffer);
			}
		}
		SettingSelect();

		return 0;
	}
    else
    {
        return -1;
    }
	return -1;
}
