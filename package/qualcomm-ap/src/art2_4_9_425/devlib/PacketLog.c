
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

#include "smatch.h"
#include "PacketLog.h"
#include "TimeMillisecond.h"

#define MFILENAME 200


struct _PacketLog
{
	unsigned int time;			// time recorded by chip
	unsigned int timems;		// time recorded by pc
	int rate;
	int status;
	int count;
};

static struct _PacketLog *PL;
static int PLmany;
static int PLtotal;


void PacketLogEnable(int many)
{
	if(PLmany!=many)
	{
		if(PL!=0)
		{
			free(PL);
			PL=0;
		}
		PL=(struct _PacketLog *)malloc(many*sizeof(struct _PacketLog));
		if(PL!=0)
		{
			PLmany=many;
			PLtotal=0;
		}
	}
}

void PacketLog(unsigned int time, int rate, int status, int count)
{
	if(PL!=0)
	{
		PL[PLtotal%PLmany].timems=TimeMillisecond();
		PL[PLtotal%PLmany].time=time;
		PL[PLtotal%PLmany].rate=rate;
		PL[PLtotal%PLmany].status=status;
		PL[PLtotal%PLmany].count=count;
		PLtotal++;
	}
}

static void PacketLogDumpHeader(FILE *fp)
{
	if(fp!=0)
	{
		fprintf(fp,"|time|ttag|rate|status|count|\n");
	}
}

static void PacketLogDumpRecord(FILE *fp, struct _PacketLog *pl)
{
	if(fp!=0)
	{
		fprintf(fp,"|%u|%u|%d|%d|%d|\n",pl->timems,pl->time,pl->rate,pl->status,pl->count);
	}
}

void PacketLogDump()
{
	FILE *fp;
	int it;
	time_t now=0;
	struct tm *lnow;
	char buffer[MFILENAME];

	if(PL!=0)
	{
		now=time(0);
		lnow=localtime(&now);
		SformatOutput(buffer,MFILENAME-1,"%02d%02d%02d%02d%02d%02d.txt",
			lnow->tm_year-100,lnow->tm_mon+1,lnow->tm_mday,lnow->tm_hour,lnow->tm_min,lnow->tm_sec);
		fp=fopen(buffer,"w+");
		if(fp!=0)
		{
			fprintf(fp,"%d packets\n",PLtotal);
			PacketLogDumpHeader(fp);
			//
			// if wrapped, start at PLnext and go to the end
			//
			if(PLtotal>PLmany)
			{
				fprintf(fp,"wrapped\n");
				for(it=(PLtotal%PLmany); it<PLmany; it++)
				{
					PacketLogDumpRecord(fp,&PL[it]);
				}
			}
			//
			// then do the records from the beginning to PLnext
			//
			for(it=0; it<(PLtotal%PLmany); it++)
			{
				PacketLogDumpRecord(fp,&PL[it]);
			}
			fclose(fp);
		}
	}
	PLtotal=0;
}

