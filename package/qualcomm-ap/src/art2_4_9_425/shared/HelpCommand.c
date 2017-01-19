
// "ACI $Id: //depot/sw/branches/art2_main_per_cs/src/art2/shared/HelpCommand.c#1 $, $Header: //depot/sw/branches/art2_main_per_cs/src/art2/shared/HelpCommand.c#1 $"

#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>



#include "smatch.h"
#include "ErrorPrint.h"
#include "UserPrint.h"
#include "ParseError.h"
#include "ParameterSelect.h"
#include "CommandParse.h"
#include "ParameterParse.h"
#include "Help.h"

#include "HelpCommand.h"

#define MBUFFER 1024

#define MSHOW 5
#define MTOPIC 10


static void HelpPrint(char *buffer)
{
	ErrorPrint(ParseHelp,buffer);
}

enum 
{
	HelpTopic=0,
	HelpShow,
	HelpLevel,
	HelpList,
};

enum
{
	HelpShowAll=0,
	HelpShowSynopsis,
    HelpShowParameters,
	HelpShowDescription,
};

static struct _ParameterList HelpShowParameter[]=
{
	{HelpShowAll,{"all",0,0},0,0,0,0,0,0,0,0,0,0,0},
	{HelpShowSynopsis,{"synopsis",0,0},0,0,0,0,0,0,0,0,0,0,0},
	{HelpShowParameters,{"parameters",0,0},0,0,0,0,0,0,0,0,0,0,0},
	{HelpShowDescription,{"description",0,0},0,0,0,0,0,0,0,0,0,0,0},
};

static struct _ParameterList LogicalParameter[]=
{
	{0,{"no",0,0},0,0,0,0,0,0,0,0,0,0,0},
	{1,{"yes",0,0},0,0,0,0,0,0,0,0,0,0,0},
};

static int HelpLevelMinimum=0;
static int HelpLevelMaximum=2;
static int HelpLevelDefault=2;
static struct _ParameterList HelpParameter[]=
{
	{HelpTopic,{"topic","name",0},"the command name, parameter name, or topic",'t',0,MTOPIC,1,1,0,0,0,0,0},
	{HelpShow,{"show",0,0},"what do you want to see?",'z',0,MSHOW,1,1,0,0,0,
	    sizeof(HelpShowParameter)/sizeof(HelpShowParameter[0]),HelpShowParameter},
	{HelpLevel,{"depth","level",0},"the numbers of levels of documentation shown",'d',0,1,1,1,&HelpLevelMinimum,&HelpLevelMaximum,&HelpLevelDefault,0,0},
	{HelpList,{"index",0,0},"show an index of topics?",'z',0,MSHOW,1,1,0,0,0,
	    sizeof(LogicalParameter)/sizeof(LogicalParameter[0]),LogicalParameter},
};


PARSEDLLSPEC void HelpParameterSplice(struct _ParameterList *list)
{
    list->nspecial=sizeof(HelpParameter)/sizeof(HelpParameter[0]);
    list->special=HelpParameter;
}


static void HelpTreePrint(struct _ParameterList *list, int nlist, int indent, int synopsis, int parameters, int level)
{
	char buffer[MBUFFER];
	int lc,nc;
	int it;
	int id;

	if(list!=0)
	{
		for(it=0; it<nlist; it++)
		{
			if(synopsis)
			{
				lc=0;
				for(id=0; id<indent; id++)
				{
					nc=SformatOutput(&buffer[lc],MBUFFER-lc-1,"    ");
					if(nc>0)
					{
						lc+=nc;
					}
				}
				//
				// command word 
				//
				if(list[it].word[0]!=0)
				{
					nc=SformatOutput(&buffer[lc],MBUFFER-lc-1,"%s",list[it].word[0]);
					if(nc>=0)
					{
						lc+=nc;
					}
				}
				//
				// and synonyms
				//
				for(id=1; id<MPWORD; id++)
				{
					if(list[it].word[id]!=0)
					{
						nc=SformatOutput(&buffer[lc],MBUFFER-lc-1,", %s",list[it].word[id]);
						if(nc>=0)
						{
							lc+=nc;
						}
					}
				}
				//
				// for special values add the actual value
				//
				if(indent>0 && list[it].nspecial==0 && list[it].type==0)
				{
					nc=SformatOutput(&buffer[lc],MBUFFER-lc-1,"[%d]",list[it].code);
					if(nc>0)
					{
						lc+=nc;
					}
				}
				//
				// add the synopsis
				//
				if(list[it].help!=0)
				{
					nc=SformatOutput(&buffer[lc],MBUFFER-lc-1,": %s",list[it].help);
					if(nc>0)
					{
						lc+=nc;
					}
				}
				ErrorPrint(ParseHelp,buffer);
			}
			if(parameters && level>0)
			{
				//
				// #### need to print numeric entry here, type[min,max]
				//
				if(list[it].type!=0 && list[it].type!='z')
				{
					lc=0;
					for(id=0; id<indent+1; id++)
					{
						nc=SformatOutput(&buffer[lc],MBUFFER-lc-1,"    ");
						if(nc>0)
						{
							lc+=nc;
						}
					}			
					ParameterHelpLine(&list[it],&buffer[lc],MBUFFER-lc-1);
					ErrorPrint(ParseHelp,buffer);
				}
				//
				// recursively do the next level
				//
				if(list[it].nspecial>0)
				{
					if(indent==0)
					{
						ErrorPrint(ParseHelpParametersStart);
					}

					HelpTreePrint(list[it].special,list[it].nspecial,indent+1,synopsis|parameters,parameters,level-1);

					if(indent==0)
					{
						ErrorPrint(ParseHelpParametersEnd);
					}
				}
			}
		}
	}
}


static void HelpTopicPrint(char *topic, int synopsis, int parameters, int description, struct _ParameterList *list, int nlist, int level)
{
	char *word, *next, *space, *colon;
    struct _ParameterList *previous;
    char lookup[MBUFFER];
    int nlookup;
    int more;
	int index;
	int code;
	char *name;

    previous=0;
    lookup[0]=0;
    nlookup=0;
	word=topic;
	while(word!=0)
	{
		//
		// find end of word and remember pointer to next word
		//
		colon=strchr(word,':');
		space=strchr(word,' ');
		if(colon!=0 && space!=0)
		{
			if(space<colon)
			{
				next=space;
			}
			else
			{
				next=colon;
			}
		}
		else if(colon!=0)
		{
			next=colon;
		}
		else if(space!=0)
		{
			next=space;
		}
		else
		{
			next=0;
		}
		if(next!=0)
		{
			*next=0;
			next++;
			//
			// and take off leading spaces
			//
			while(*next==' ')
			{
				*next=0;
				next++;
			}
		}
		//
		// we start with the commands that are always available
		//
		index=ParameterSelectIndex(word,list,nlist);
		if(index>=0)
		{
			code=list[index].code;
			name=list[index].word[0];
			previous= &list[index];
			nlist=list[index].nspecial;
			list=list[index].special;
		}
		else
		{
			code= -1;
			name=word;
			list=0;
			nlist=0;
		}
		if(nlookup>0)
		{
			more=SformatOutput(&lookup[nlookup],MBUFFER-nlookup,":%s",name);
		}
		else
		{
			more=SformatOutput(&lookup[nlookup],MBUFFER-nlookup,"%s",name);
		}
		if(more>0)
		{
			nlookup+=more;
		}
		//
		// increment to next word
		//
		word=next;
	}

	ErrorPrint(ParseHelpStart);

	if(previous!=0)
	{
		HelpTreePrint(previous,1,0,synopsis,parameters,level);
	}	

	if(description && Help(lookup,HelpPrint)<=0)
	{
		if(previous==0 && nlist<=0)
		{
			ErrorPrint(ParseHelpUnknown);
		}
	}
	ErrorPrint(ParseHelpEnd);

}


PARSEDLLSPEC void HelpCommand(struct _ParameterList *inlist, int nlist)
{
	int ip,nparam;
	int iv, nvalue;
	int code;
    int index;
	char *name;
	int is,nshow,show[MSHOW];
	char *topic[MTOPIC];
	int it,ntopic;
	int ntree,tree;
	int error;
	int synopsis,parameters,description;
	int level,nlevel;
	int list,ngot;

	nparam=CommandParameterMany();
	list=0;
	nlevel=0;
	level=0;
	nshow=1;
	show[0]=HelpShowAll;
	ntree= -1;
	tree=0;
	ntopic= 0;
	error=0;
	for(it=0; it<MTOPIC; it++)
	{
		topic[it]=0;
	}
	//
	// parse arguments and do it
	//
	for(ip=0; ip<nparam; ip++)
	{
		level=10;
		name=CommandParameterName(ip);
		index=ParameterSelectIndex(name,HelpParameter,sizeof(HelpParameter)/sizeof(HelpParameter[0]));
		if(index>=0)
		{
			code=HelpParameter[index].code;
			switch(code) 
			{
				case HelpShow:
					nshow=ParseIntegerList(ip,name,show,&HelpParameter[index]);
					if(nshow<=0)
					{
						error++;
					}
					break;
				case HelpList:
					ngot=ParseIntegerList(ip,name,&list,&HelpParameter[index]);
					if(ngot<=0)
					{
						error++;
					}
					break;
				case HelpLevel:
					nlevel=ParseIntegerList(ip,name,&level,&HelpParameter[index]);
					if(nlevel<=0)
					{
						error++;
					}
					break;
				case HelpTopic:
					nvalue=CommandParameterValueMany(ip);
					for(iv=0; iv<nvalue; iv++)
					{
						if(ntopic>=MTOPIC)
						{
							ErrorPrint(ParseTooMany,name,MTOPIC);
						}
						else
						{
							topic[ntopic]=Sduplicate(CommandParameterValue(ip,iv));
							ntopic++;
						}
					}
					break;
				default:
					if(ntopic>=MTOPIC)
					{
						ErrorPrint(ParseTooMany,name,MTOPIC);
					}
					else
					{
						topic[ntopic]=Sduplicate(CommandParameterName(ip));
						ntopic++;
					}
					break;
			}
		}
		else
		{
			if(ntopic>=MTOPIC)
			{
				ErrorPrint(ParseTooMany,name,MTOPIC);
			}
			else
			{
				topic[ntopic]=Sduplicate(CommandParameterName(ip));
				ntopic++;
			}
		}
	}

	if(error==0)
	{
		if(list!=0)
		{
			ErrorPrint(ParseHelpStart);
			HelpIndex(HelpPrint);
			ErrorPrint(ParseHelpEnd);
		}
		else
		{
			synopsis=0;
			parameters=0;
			description=0;
			for(is=0; is<nshow; is++)
			{
				if(show[is]==HelpShowAll || show[is]==HelpShowSynopsis)
				{
					synopsis=1;
				}
				if(show[is]==HelpShowAll || show[is]==HelpShowParameters)
				{
					parameters=1;
				}
				if(show[is]==HelpShowAll || show[is]==HelpShowDescription)
				{
					description=1;
				}
			}

			if(ntopic==0)
			{
				if(nlevel==0)
				{
					level=HelpLevelMinimum;
				}
				HelpTreePrint(inlist,nlist,0,synopsis,parameters,level);
			}
			else
			{
				if(nlevel==0)
				{
					level=HelpLevelMaximum;
				}
				for(it=0; it<ntopic; it++)
				{
					HelpTopicPrint(topic[it],synopsis,parameters,description,inlist,nlist,level);
				}
			}
		}
	}	
	for(it=0; it<ntopic; it++)
	{
		Sdestroy(topic[it]);
	}

}

