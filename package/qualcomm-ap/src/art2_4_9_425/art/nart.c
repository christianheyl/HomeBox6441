
#include <stdio.h>
#include <stdlib.h>

#include "smatch.h"
#include "UserPrint.h"
#include "ErrorPrint.h"
#include "ParseError.h"
#include "NewArt.h"
#include "ParameterSelect.h"
#include "ChipIdentify.h"

#define MBUFFER 1024

int isQDART=0;

int main(int narg, char *arg[]) 
{
	int iarg;
	int console;
    int instance=0;
#ifdef __LINUX_POWERPC_ARCH__
    extern int setPcie(int Pcie);
    int pcie=0;
#endif
    int port;
	char startfile[MBUFFER];
    //
	// temporarily turn on console output while we parse the command line arguments
	//
	UserPrintConsole(1);
	//
	// Initialize system default to non-Qdart mode
	SetQdartMode(0);
	//
	// process command line options
	//
	SformatOutput(startfile,MBUFFER-1,"%s","nart.art");
	startfile[MBUFFER-1]=0;
    console=0;
    port = -1;
	for(iarg=1; iarg<narg; iarg++)
	{
        if(Smatch(arg[iarg],"-port"))
        {
            if(iarg+1<narg)
            {
                SformatInput(arg[iarg+1]," %d ",&port);
                iarg++;
            }
        }
        else if(Smatch(arg[iarg],"-console"))
		{
			console=1;
		}
		else if(Smatch(arg[iarg],"-instance"))
		{
			if(iarg<narg-1)
			{
				iarg++;
				SformatInput(arg[iarg]," %d ",&instance); 
			}
			else
			{
				ErrorPrint(ParseBadValue,"NULL","-instance");
			}
		}
#ifdef __LINUX_POWERPC_ARCH__
		else if(Smatch(arg[iarg],"-pcie"))
		{
			if(iarg<narg-1)
			{
				iarg++;
				SformatInput(arg[iarg]," %d ",&pcie); 
			}
			else
			{
				ErrorPrint(ParseBadValue,"NULL","-pcie");
			}

                        setPcie(pcie);
		}
#endif
		else if(Smatch(arg[iarg],"-start"))
		{
			if(iarg<narg-1)
			{
				iarg++;
				SformatOutput(startfile,MBUFFER-1,"%s",arg[iarg]);
				startfile[MBUFFER-1]=0;
			}
			else
			{
				ErrorPrint(ParseBadValue,"NULL","-start");
			}
		}
		else if(Smatch(arg[iarg],"-log"))
		{            
			if(iarg<narg-1)
			{
				iarg++;
                                UserPrintFile(arg[iarg]);                
			}
			else
			{
				ErrorPrint(ParseBadValue,"NULL","-log");
			}
		}
		else if(Smatch(arg[iarg],"-help"))
		{
			ErrorPrint(ParseHelp,"-console");
			ErrorPrint(ParseHelp,"-log [log file name]");
			ErrorPrint(ParseHelp,"-port [port number]");
			ErrorPrint(ParseHelp,"-instance [device index]");
			ErrorPrint(ParseHelp,"-start [startup command file]");
			exit(0);
		}
		else if(Smatch(arg[iarg],"-qdart"))
		{
			// User select to run system in Qdart mode
			SetQdartMode(1);
		}
		else
		{
			ErrorPrint(ParseBadParameter,arg[iarg]);
		}
	}

	UserPrintConsole(console);
    if(instance == 0)
	{
		if(port<0)
		{
			port=2390;
		} 
	}
    else if(instance == 1)
	{
		if(port<0)
		{
#ifndef __APPLE__	
			port=2391;
#else
			port=2390;
#endif			
		}
    }
    else
    {
#ifndef __APPLE__	
        printf("Only instance 0 and 1 are supported\n");
		exit(1);
#else
		if(port<0)
		{
			port=2390;
		} 	
#endif	
    }
    NewArt(instance,port,startfile);
	exit(0);
}


void SetQdartMode(int isQdart)
{	
	isQDART=isQdart;	
}

int GetQdartMode(void) 
{ 
	return isQDART;
}
