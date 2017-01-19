

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

#include "wlantype.h"
#include "rate_constants.h"
#include "vrate_constants.h"
#include "smatch.h"
#include "UserPrint.h"
#include "TimeMillisecond.h"
#include "CommandParse.h"
#include "ParameterSelect.h"	
#include "ParameterParse.h"		
#include "NewArt.h"
#include "MyDelay.h"
#include "Card.h"
#include "ResetForce.h"
#include "Link.h"
#include "Device.h"
#include "Channel.h"
#include "LinkList.h"

#include "ErrorPrint.h"
#include "ParseError.h"
#include "CardError.h"
#include "NartError.h"
#include "LinkError.h"

#define MBUFFER 1000

#define MCHANNEL 2000


static int ChannelFrequency[MCHANNEL];
static int ChannelOption[MCHANNEL];
static int ChannelMany;

//
// look though the channel list and return the first channel that matches.
// frequency<=0 matches anything
//
int ChannelFind(int fmin, int fmax, int bw, int *rfrequency, int *rbw)
{
   	int it;
   	*rfrequency=-100;
   	*rbw=BW_AUTOMATIC;
   	//
   	// if we didn't find anything that works for ht40 and the setting is automatic,
   	// try again just looking for ht20
   	//
   	if(bw==BW_AUTOMATIC)
   	{
   		if(ChannelFind(fmin,fmax,BW_VHT80_0,rfrequency,rbw))
        {
   		    if(ChannelFind(fmin,fmax,BW_VHT80_1,rfrequency,rbw))
            {
   		        if(ChannelFind(fmin,fmax,BW_VHT80_2,rfrequency,rbw))
                {
   		            if(ChannelFind(fmin,fmax,BW_VHT80_3,rfrequency,rbw))
                    {
   		                if(ChannelFind(fmin,fmax,BW_HT40_PLUS,rfrequency,rbw))
   		                {
   			                if(ChannelFind(fmin,fmax,BW_HT40_MINUS,rfrequency,rbw))
   			                {
   				                if(ChannelFind(fmin,fmax,BW_HT20,rfrequency,rbw))
   				                {
   					                if(ChannelFind(fmin,fmax,BW_HALF,rfrequency,rbw))
   					                {
   						                if(ChannelFind(fmin,fmax,BW_QUARTER,rfrequency,rbw))
   						                {
   							                if(ChannelFind(fmin,fmax,BW_OFDM,rfrequency,rbw))
   							                {
   								                return -1;
   							                }
                                        }
                                    }
                                }
                            }
   						}
   					}
   				}
   			}
   		}
   		return 0;
   	}
   	else
   	{
   		for(it=0; it<ChannelMany; it++)
   		{
   			if(fmin<=ChannelFrequency[it] && ChannelFrequency[it]<=fmax)
   			{
   				switch(bw)
   				{
   					case BW_VHT80_0:
   						if(ChannelOption[it]&CHANNEL_VHT80_0)
   						{
   							*rfrequency=ChannelFrequency[it];
   							*rbw=BW_VHT80_0;
   							return 0;
   						}
   						break;
   					case BW_VHT80_1:
   						if(ChannelOption[it]&CHANNEL_VHT80_1)
   						{
   							*rfrequency=ChannelFrequency[it];
   							*rbw=BW_VHT80_1;
   							return 0;
   						}
   						break;
   					case BW_VHT80_2:
   						if(ChannelOption[it]&CHANNEL_VHT80_2)
   						{
   							*rfrequency=ChannelFrequency[it];
   							*rbw=BW_VHT80_2;
   							return 0;
   						}
   						break;
   					case BW_VHT80_3:
   						if(ChannelOption[it]&CHANNEL_VHT80_3)
   						{
   							*rfrequency=ChannelFrequency[it];
   							*rbw=BW_VHT80_3;
   							return 0;
   						}
   						break;
   					case BW_HT40_PLUS:
   						if(ChannelOption[it]&CHANNEL_HT40PLUS)
   						{
   							*rfrequency=ChannelFrequency[it];
   							*rbw=BW_HT40_PLUS;
   							return 0;
   						}
   						break;
   					case BW_HT40_MINUS:
   						if(ChannelOption[it]&CHANNEL_HT40MINUS)
   						{
   							*rfrequency=ChannelFrequency[it];
   							*rbw=BW_HT40_MINUS;
   							return 0;
   						}
   						break;
   					case BW_HT20:
   						if(ChannelOption[it]&CHANNEL_HT20)
   						{
   							*rfrequency=ChannelFrequency[it];
   							*rbw=BW_HT20;
   							return 0;
   						}
   						break;
   					case BW_OFDM:
   						if(ChannelOption[it]&CHANNEL_OFDM)
   						{
   							*rfrequency=ChannelFrequency[it];
   							*rbw=BW_OFDM;
   							return 0;
   						}
   						break;
   					case BW_HALF:
   						if(ChannelOption[it]&CHANNEL_HALF)
   						{
   							*rfrequency=ChannelFrequency[it];
   							*rbw=BW_HALF;
   							return 0;
   						}
   						break;
   					case BW_QUARTER:
   						if(ChannelOption[it]&CHANNEL_QUARTER)
   						{
   							*rfrequency=ChannelFrequency[it];
   							*rbw=BW_QUARTER;
   							return 0;
   						}
   						break;
   				}
   			}
   		}
   	}
   	return -1;
}
   

static int ChannelMessageHeader(int client)
{
	return ErrorPrint(NartDataHeader,"|channel|frequency|CCK|OFDM|ht20|ht40p|ht40m|TURBO|STURBO|HALF|QUARTER|vht20|vht40p|vht40m|vht80_0|vht80_1|vht80_2|vht80_3|");
}
   
static int ChannelMessageSend(int client, int frequency, int option)
{
   	char buffer[MBUFFER];
   	int lc;
   
   	lc=SformatOutput(buffer,MBUFFER-1,"|channel|%d|%d|%d|%d|%d|%d|%d|%d|%d|%d|%d|%d|%d|%d|%d|%d|%d|",
           frequency,
   		   (((option&CHANNEL_2GHZ) && (option&CHANNEL_CCK))==0? 0:1),
           ((option&CHANNEL_OFDM)==0? 0:1),
           ((option&CHANNEL_HT20)==0? 0:1),
           ((option&CHANNEL_HT40PLUS)==0? 0:1),
           ((option&CHANNEL_HT40MINUS)==0? 0:1),
           ((option&CHANNEL_TURBO)==0? 0:1),
           ((option&CHANNEL_STURBO)==0? 0:1),
           ((option&CHANNEL_HALF)==0? 0:1),
           ((option&CHANNEL_QUARTER)==0? 0:1),
		   ((option&CHANNEL_HT20)==0? 0:1),       //VHT20
           ((option&CHANNEL_HT40PLUS)==0? 0:1),   //VHT40P
           ((option&CHANNEL_HT40MINUS)==0? 0:1),  //VHT40M
		   ((option&CHANNEL_VHT80_0)==0? 0:1),
           ((option&CHANNEL_VHT80_1)==0? 0:1),
           ((option&CHANNEL_VHT80_2)==0? 0:1),
           ((option&CHANNEL_VHT80_3)==0? 0:1)
		   );
   
       return ErrorPrint(NartData,buffer);
}
   
int ChannelCalculate()
{
	ChannelMany=DeviceChannelCalculate(ChannelFrequency,ChannelOption,MCHANNEL);
	return ChannelMany;
}

int ChannelCommand(int client)
{
	int it;

	ChannelMessageHeader(client);
	//
	// return channel information
	//
	for(it=0; it<ChannelMany; it++)
	{
		ChannelMessageSend(client,ChannelFrequency[it],ChannelOption[it]);
	}
	return 0;
}

