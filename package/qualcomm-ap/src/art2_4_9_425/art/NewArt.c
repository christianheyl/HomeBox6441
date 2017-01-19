

//  "ACI $Id: //depot/sw/branches/art2_main_per_cs/src/art2/art/NewArt.c#1 $, $Header: //depot/sw/branches/art2_main_per_cs/src/art2/art/NewArt.c#1 $"

#ifdef UNUSED
#ifdef _WINDOWS
 #include <windows.h>
#endif


#ifndef LINUX
#include <conio.h>
#include <io.h>
#endif
#endif


#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>
#include <string.h>

#include "Keyboard.h"
#include "CommandParse.h"
#include "ParameterSelect.h"
#include "Socket.h"
#include "NartRegister.h"
#include "Link.h"
#include "TimeMillisecond.h"
#include "smatch.h"
#include "UserPrint.h"
#include "Card.h"
#include "MyDelay.h"
#include "GainTable.h"
#include "Calibrate.h"
#include "configCmd.h"
#include "ConfigurationCommand.h"
#include "NartVersion.h"
#include "NewArt.h"
#include "NoiseFloor.h"
#include "TargetPowerGet.h"
#include "Template.h"
#include "ParameterParse.h"
#include "Channel.h"

#include "ErrorPrint.h"
#include "ParseError.h"
#include "NartError.h"
#include "CardError.h"
#include "ConnectError.h"
#include "LinkError.h"
#include "DeviceError.h"
#include "EepromError.h"

#include "PacketLog.h"

#include "HelpCommand.h"
#include "ErrorCommand.h"

#include "Device.h"
#include "DeviceLoad.h"
#include "SetConfig.h"
#include "SleepMode.h"
#include "ChipIdentify.h"
#include "CalibrationLoad.h"

extern int setInstance(int devInstance);
extern void CoeffDisplay(int client);

#define MBUFFER 1024

#define MCLIENT 4

static struct _Socket *_ListenSocket;// this is the socket on which we listen for client connections

static struct _Socket *_ClientSocket[MCLIENT];	// these are the client sockets


static int UserPrintClient;
static void UserPrintSendIt(char *buffer);


#define MCOMMAND 50

static int _CommandNext=0;		// index of next command to perform
static int _CommandRead=0;		// index of slot for next command read from socket

static char *_Command[MCOMMAND];
static char _CommandClient[MCOMMAND];

//
// this is the current command
// saved to be used in DONE messages
//
static char *_CommandCurrent[MCOMMAND];
static int _CommandCurrentMany=0;


static char *CommandCurrent()
{
	if(_CommandCurrentMany>0)
	{
		return _CommandCurrent[_CommandCurrentMany-1];
	}
	else
	{
		return "unknown";
	}
}

static void CommandCurrentSave(char *buffer)
{
	//
	// Save the input
	//
	if(_CommandCurrentMany<MCOMMAND)
	{
		if(_CommandCurrent[_CommandCurrentMany]!=0)
		{
			Sdestroy(_CommandCurrent[_CommandCurrentMany]);
		}
		_CommandCurrent[_CommandCurrentMany]=Sduplicate(buffer);
		if(_CommandCurrentMany<MCOMMAND)
		{
			_CommandCurrentMany++;
		}
	}
}


static void CommandCurrentDelete()
{
	if(_CommandCurrentMany>0)
	{
		_CommandCurrentMany--;
		if(_CommandCurrent[_CommandCurrentMany]!=0)
		{
			Sdestroy(_CommandCurrent[_CommandCurrentMany]);
			_CommandCurrent[_CommandCurrentMany]=0;
		}
	}
}


enum
{
    CommandExit=0,
    CommandHello,
    CommandHelp,
    CommandTx,
    CommandRx,
    CommandSleep,
    CommandAttach,
    CommandReset,
    CommandPllScreen,
    CommandResetMCI,
    CommandDetach,
    CommandRegisterRead,
    CommandRegisterWrite,
    CommandFieldRead,
    CommandFieldWrite,
    CommandFieldList,
    CommandConfigRead,
    CommandConfigWrite,
    CommandMemoryRead,
    CommandMemoryWrite,
    CommandEepromRead,
    CommandEepromWrite,
    CommandFlashRead,
    CommandFlashWrite,
	CommandOtpRead,
    CommandOtpWrite,
    CommandOtpLoad,
    CommandRegisterDebug,
    CommandStickyWrite,
    CommandStickyList,
    CommandStickyClear,
    CommandGainTableRead,
    CommandGainTableWrite,
    CommandCommit,
    CommandPcieCommit,
    CommandRestore,
    CommandSet,
    CommandGet,
    CommandSetTP,
    CommandGetTP,
    CommandErase,
	CommandCalibrate,
	CommandCheck,
	CommandPacketLog,
	CommandChannel,
	CommandNoiseFloor,
	CommandTargetPower,
	CommandNoiseFloorGet,
	CommandStart,
	CommandStop,
	CommandTemplate,
	CommandError,
	CommandVersion,
    CommandSetConfig,
    CommandSetCal,
    CommandGetCal,
    CommandRfBbTestPoint,
	CommandHAL,
	CommandCoeffDisplay,
	CommandSleepMode,
};



static void NewClientInfo(int client)
{
    char buffer[MBUFFER];

    SformatOutput(buffer,MBUFFER-1,"|set|NartVersion|%d.%d|",NartVersionMajor(),NartVersionMinor());
    ErrorPrint(NartData,buffer);
    SformatOutput(buffer,MBUFFER-1,"|set|NartBuildDate|%s%s|",NartVersionDate(),NartVersionTime());
    ErrorPrint(NartData,buffer);
	DeviceDataSend();
    CardDataSend(client);
}


static int ClientAccept()
{
	int it;
	int noblock;
    struct _Socket *TryClientSocket;
    static int OldestClient = 0;

	if(_ListenSocket!=0)
	{
		//
		// If we have no clients, we will block waiting for a client.
		// Otherwise, just check and go on.
		//
		noblock=0;
		for(it=0; it<MCLIENT; it++)
		{
			if(_ClientSocket[it]!=0)
			{
				noblock=1;
				break;
			}
		}
		if(noblock==0)
		{
			ErrorPrint(ConnectGuiWait);
		}
		//
		// Look for new client
		//
		for(it=0; it<MCLIENT; it++)
		{
			if(_ClientSocket[it]==0)
			{
				_ClientSocket[it]=SocketAccept(_ListenSocket,noblock);// don't block
				if(_ClientSocket[it]!=0)
				{
					UserPrintClient=it;
					ErrorPrintFunction(UserPrintSendIt);
					ErrorPrint(ConnectGuiAccept,it);
					NewClientInfo(it);
					return it;
				}
			}
		}
        // In case all clients have been used up, but there is another client want to connect, give away the oldest one
        if (it == MCLIENT)
        {
            TryClientSocket = SocketAccept(_ListenSocket,noblock);
            if (TryClientSocket)
            {
                SocketClose(_ClientSocket[OldestClient]);
                _ClientSocket[OldestClient] = TryClientSocket;
                
                UserPrintClient = OldestClient;
                ErrorPrintFunction(UserPrintSendIt);
                ErrorPrint(ConnectGuiAccept,OldestClient);
                NewClientInfo(OldestClient);
                OldestClient = (OldestClient == MCLIENT-1) ? 0 : (OldestClient + 1);
                return it;
            }
        }
	}
	return -1;
}


static void ClientClose(int client)
{
	if(client>=0 && client<MCLIENT && _ClientSocket[client]!=0)
	{
		SocketClose(_ClientSocket[client]);
		_ClientSocket[client]=0;
	}
}


//
// clears the command queue
//
static int CommandEraseInput()
{
	int it;

	for(it=0; it<MCOMMAND; it++)
	{
		if(_Command[it]!=0)
		{
			Sdestroy(_Command[it]);
			_Command[it]=0;
		}
	}
	_CommandNext=0;
	_CommandRead=0;

	return 0;
}


//
// returns 1 if the command is consumed immediately.
// returns 0 if the command should be queued for processing later.
//
static int CommandImmediate(int client, char *buffer)
{
	char *word;
    int code;
    static struct _ParameterList cl[]=
    {
        {CommandRegisterRead,"rr",0,0},
        {CommandRegisterWrite,"rw",0,0},
        {CommandFieldRead,"fr",0,0},
        {CommandFieldWrite,"fw",0,0},
        {CommandFieldList,"fl",0,0},
        {CommandConfigRead,"cr",0,0},
        {CommandConfigWrite,"cw",0,0},
        {CommandMemoryRead,"mr",0,0},
        {CommandMemoryWrite,"mw",0,0},
        {CommandEepromRead,"er",0,0},
        {CommandEepromWrite,"ew",0,0},
        {CommandFlashRead,"xr",0,0},
        {CommandFlashWrite,"xw",0,0},
		{CommandOtpRead,"or",0,0},
        {CommandOtpRead,"or2",0,0},
        {CommandOtpWrite,"ow",0,0},
        {CommandOtpWrite,"ow2",0,0},
        {CommandOtpLoad,"ol",0,0},
        {CommandRegisterDebug,"rd",0,0},
        {CommandStickyWrite,"sw",0,0},
        {CommandStickyList,"sl",0,0},
        {CommandStickyClear,"sc",0,0},
        {CommandGainTableRead,"tgr",0,0},
        {CommandGainTableWrite,"tgw",0,0},
        {CommandErase,"erase",0,0},
        {CommandCalibrate,"calibrate",0,0},
		{CommandCoeffDisplay,"CoeffDisplay",0,0}
    };

	if(buffer[0]=='!')
	{
        buffer++;
		//
		// Dispatch
		//
		CommandCurrentSave(buffer);
		CommandParse(buffer);
		word=CommandWord();
        //
        // select the correct command and do it
        //
        code=ParameterSelect(word,cl,sizeof(cl)/sizeof(struct _ParameterList));
		switch (code) 
        {
            case CommandRegisterRead:
                RegisterReadCommand(client);
                break;
            case CommandRegisterWrite:
                RegisterWriteCommand(client);
                break;
            case CommandFieldRead:
                FieldReadCommand(client);
                break;
            case CommandFieldWrite:
                FieldWriteCommand(client);
                break;
            case CommandFieldList:
                FieldListCommand(client);
                break;
            case CommandConfigRead:
                ConfigReadCommand(client);
                break;
            case CommandConfigWrite:
                ConfigWriteCommand(client);
                break;
            case CommandMemoryRead:
                MemoryReadCommand(client);
                break;
            case CommandMemoryWrite:
                MemoryWriteCommand(client);
                break;
            case CommandEepromRead:
                EepromReadCommand(client);
                break;
            case CommandEepromWrite:
                EepromWriteCommand(client);
                break;
            case CommandFlashRead:
                FlashReadCommand(client);
                break;
            case CommandFlashWrite:
                FlashWriteCommand(client);
                break;
			case CommandOtpRead:
                OtpReadCommand(client);
                break;
            case CommandOtpWrite:
                OtpWriteCommand(client);
                break;
            case CommandOtpLoad:
                OtpLoadCommand(client);
                break;
            case CommandRegisterDebug:
                RegisterDebugCommand(client);
                break;
            case CommandStickyWrite:
                FieldStickyCommand(client);
                break;
            case CommandStickyList:
                FieldStickyList(client);
                break;
            case CommandStickyClear:
                FieldStickyClear(client);
                break;
            case CommandGainTableRead:
                GainTableReadCommand(client);
                break;
            case CommandGainTableWrite:
 				GainTableWriteCommand(client);
                break;
            case CommandErase:
                CommandEraseInput();
                break;
#ifdef UNUSED
            case CommandCalibrate:
                LinkCalibrate();
                break;
#endif
            default:
				break;
		}
		CommandCurrentDelete();
	}
	return 0;
}


int CommandRead()
{
	char buffer[MBUFFER];
	int nread;
	int nloop;
	int ntotal;
	int it;
    int diagPacketReceived = 0;
    int cmdLen;

	char termChar = GetStrterminationChar();

	//
	// look for new clients
	//
	ClientAccept();
	//
	// try to read everything on the client socket
	//
	ntotal=0;
	while(_CommandNext!=_CommandRead || _Command[_CommandRead]==0)
	{
		if(_ListenSocket==0)
		{
#if !defined(LINUX) && !defined(__APPLE__)
			nread=KeyboardRead(buffer,MBUFFER-1);
#endif

			if(nread>0)
			{
				buffer[nread]=0;
				if(nread>1 && (buffer[nread-1]=='\r' || buffer[nread-1]=='\n'))
				{
					buffer[nread-1]=0;
				}
				if(nread>2 && (buffer[nread-2]=='\r' || buffer[nread-2]=='\n'))
				{
					buffer[nread-2]=0;
				}
				if(!CommandImmediate(it,buffer))
				{
					//
					// put it on our queue of commands
					//
					if(_Command[_CommandRead]!=0)
					{
						Sdestroy(_Command[_CommandRead]);
						_Command[_CommandRead]=0;
					}
					_Command[_CommandRead]=Sduplicate(buffer);
					_CommandClient[_CommandRead]= 0;
					_CommandRead=(_CommandRead+1)%MCOMMAND;
					nloop++;
					ntotal++;
				}
			}
		}
		else
		{
			//
			// read commands from each client in turn
			//
			nloop=0;
			for(it=0; it<MCLIENT; it++)
			{
				if(_ClientSocket[it]!=0)
				{
					nread = (int)SocketRead(_ClientSocket[it],buffer,MBUFFER-1);
					if(nread>0)
					{
						buffer[nread]=0;
						if (GetQdartMode()==0) {
							if(nread>1 && (buffer[nread-1]=='\r' || buffer[nread-1]=='\n'))
							{
								buffer[nread-1]=0;
							}
							if(nread>2 && (buffer[nread-2]=='\r' || buffer[nread-2]=='\n'))
							{
								buffer[nread-2]=0;
							}
						} else {
							if(nread>1 && buffer[nread-1]==DIAG_TERM_CHAR)
							{
								diagPacketReceived = 1;
								buffer[nread-1]=0;
								cmdLen = nread-0;
 							}
							//Needed for linux path
							if(nread>2 && buffer[nread-2]==DIAG_TERM_CHAR)
							{
								diagPacketReceived = 1;
								buffer[nread-2]=0;
								cmdLen = nread - 1;
							}
#ifdef _WINDOWS
							//
							// check to see if we received a diag packet
							//
							if(diagPacketReceived) {
								if(processDiagPacket(it,(unsigned char *)buffer,cmdLen)) {
									//command was completed,
									//otherwise buffer should now contain an actual command
									//and will get completed below
                        			CommandCurrentDelete(); //not sure if this is needed
									printf( "\n--processDiagPacket-succeed------ Wait For Next Diag Packet ----------------\n\n" );
									continue;
								}
								else
								{
									printf( "\n--processDiagPacket-failed------- Wait For Next Diag Packet ----------------\n\n" );
								}
							}
#endif
						}
						if(!CommandImmediate(it,buffer))
						{
							//
							// put it on our queue of commands
							//
							if(_Command[_CommandRead]!=0)
							{
								Sdestroy(_Command[_CommandRead]);
								_Command[_CommandRead]=0;
							}
							_Command[_CommandRead]=Sduplicate(buffer);
							_CommandClient[_CommandRead]=it;
							_CommandRead=(_CommandRead+1)%MCOMMAND;
							nloop++;
							ntotal++;
						}
					}
					else if(nread<0)
					{
						ClientClose(it);
					}
#ifdef UNUSED
					else
					{
						break;
					}
#endif
				}
			}
		}
		if(nloop<=0)
		{
			break;
		}
	}
	return ntotal;
}


int CommandNext(char *command, int max, int *client)
{
	int length;
    //
	// try to read new commands
	//
	if(CommandRead()<0)
	{
		return -1;
	}
	//
	// if we have a command, return it
	//
	if(_Command[_CommandNext]!=0)
	{
		length=Slength(_Command[_CommandNext]);
		if(length>max)
		{
			_Command[_CommandNext][max]=0;
			length=max;
		}
		Scopy(command,_Command[_CommandNext]);
		*client=_CommandClient[_CommandNext];
		Sdestroy(_Command[_CommandNext]);
		_Command[_CommandNext]=0;
		_CommandNext=(_CommandNext+1)%MCOMMAND;

		UserPrint("> %s\n",command);

		return length;
	}
	return 0;
}

int SendItDiag(int client, char *buffer, int length)
{
	int nwrite;

	if(_ListenSocket==0|| (client>=0 && client<MCLIENT && _ClientSocket[client]!=0))
	{
		if(_ListenSocket==0)
		{
			//printf("%s",response);
		}
		else
		{
#ifdef _WINDOWS
			if ( GetQdartMode() )
			{
				//printf( "SocketWriteEnableMode( 1 );\n" );
				SocketWriteEnableMode( 1 );
				nwrite=SocketWrite(_ClientSocket[client],buffer,length);
				SocketWriteEnableMode( 0 );
				//printf( "SocketWriteEnableMode( 0 );\n" );
			}
			else
#endif
			{
				nwrite=SocketWrite(_ClientSocket[client],buffer,length);
			}
			if(nwrite<0)
			{
//				ErrorPrint(ConnectGuiWrite);
				ClientClose(client);
				return -1;
			}	
		}
	    return 0;
	}
	else
	{
		return -1;
	}

}

int SendIt(int client, char *buffer)
{
	char response[MBUFFER];
	int nwrite;
	int cradd,nladd;
	int lc,nc;

	if(_ListenSocket==0|| (client>=0 && client<MCLIENT && _ClientSocket[client]!=0))
	{
	    lc=SformatOutput(response,MBUFFER-1,"%s",buffer);
        //
		// look for terminating \r\n
		// we add them if they are missing
		//
		cradd=1;
		nladd=1;
		if((lc>=2 && buffer[lc-2]=='\r') || (lc>=1 && buffer[lc-1]=='\r'))
		{
			cradd=0;
		}
		if((lc>=2 && buffer[lc-2]=='\n') || (lc>=1 && buffer[lc-1]=='\n'))
		{
			nladd=0;
		}
		//
		// add <new line> if not there already
		//
		if(cradd && nladd)
		{
			nc=SformatOutput(&response[lc],MBUFFER-lc-1,"\r\n");
			if(nc>0)
			{
				lc+=nc;
			}
		}
		//
		// add \r if not there already
		//
		else if(cradd && !nladd)
		{
			lc--;
			nc=SformatOutput(&response[lc],MBUFFER-lc-1,"\r\n");
			if(nc>0)
			{
				lc+=nc;
			}
		}
		//
		// add <new line> if not there already
		//
		else if(nladd)
		{
			nc=SformatOutput(&response[lc],MBUFFER-lc-1,"\n");
			if(nc>0)
			{
				lc+=nc;
			}
		}

		if(_ListenSocket==0)
		{
			printf("%s",response);
		}
		else
		{
			UserPrint("< %s",response);
			nwrite = (int)SocketWrite(_ClientSocket[client],response,strlen(response));
			if(nwrite<0)
			{
//				ErrorPrint(ConnectGuiWrite);
				ClientClose(client);
				return -1;
			}	
		}
	    return 0;
	}
	else
	{
		return -1;
	}
}


int SendDebug(int client, char *buffer)
{
	ErrorPrint(NartDebug,buffer);
	return 0;
}


int SendDone(int client)
{
	ErrorPrint(NartDone,CommandCurrent());
	return 0;
}


int SendError(int client, char *message)
{
	ErrorPrint(NartError,message);
	return 0;
}


int SendOk(int client)
{
	ErrorPrint(NartOk);
	return 0;
}


int SendOn(int client)
{
	ErrorPrint(NartOn);
	return 0;
}


int SendOff(int client)
{
	ErrorPrint(NartOff);
	return 0;
}

static void UserPrintSendIt(char *buffer)
{
	if(UserPrintClient>=0)
	{
		SendIt(UserPrintClient,buffer);
	}
	else
	{
		UserPrintIt(buffer);
	}
}


static void DoSleep(int client)
{		
	int it;

	for(it=0; it<10; it++)
	{
		CommandRead();
	    MyDelay(1000);
	}
	SendDone(client);						
}


static void Hello(int client)
{							
    NewClientInfo(client);
	/*char buffer[MBUFFER];

	ErrorPrint(NartDataHeader,"|hello|major|minor|date|time|");
    SformatOutput(buffer,MBUFFER-1,"|hello|%d|%d|%s|%s|",NartVersionMajor(),NartVersionMinor(),NartVersionDate(),NartVersionTime());
	ErrorPrint(NartData,buffer);
    */
	SendDone(client);						
}

static void Exit()
{
#ifdef CTR_HOSTIO
    int it;

    for(it=0; it<MCLIENT; it++)
    {
        CardRemove(it);
        ClientClose(it);
    }
    //MANLIB_API int hostio_disconnect();
    //hostio_disconnect();
#endif
	exit(0);
}

static void MenuFixup(struct _ParameterList *cl, int nl)
{
    int it;

    for(it=0; it<nl; it++)
    {
        switch(cl[it].code)
        {
			case CommandExit:
				// no parameters
				break;
            case CommandHello: 
                // no parameters 
                break;
			case CommandHelp: 
				HelpParameterSplice(&cl[it]); 
				break;
            case CommandError:
                ErrorParameterSplice(&cl[it]);
                break;
            case CommandTx:
            case CommandRx:
			    LinkParameterSplice(&cl[it]);
                break;
            case CommandSleep:
//                DoSleep(client);
                break;
            case CommandAttach:
                CardLoadParameterSplice(&cl[it]);
                break;
            case CommandReset:
                CardResetParameterSplice(&cl[it]);
	        case CommandResetMCI:
                CardResetMCIParameterSplice(&cl[it]);
                break;
            case CommandPllScreen:
                // no parameters
                break;
            case CommandChannel:
                // no parameters
                break;
            case CommandDetach:
                // no parameters
                break;
            case CommandRegisterRead:
                RegisterReadParameterSplice(&cl[it]);
                break;
            case CommandRegisterWrite:
                RegisterWriteParameterSplice(&cl[it]);
                break;
            case CommandFieldRead:
                FieldReadParameterSplice(&cl[it]);
                break;
            case CommandFieldWrite:
                FieldWriteParameterSplice(&cl[it]);
                break;
           case CommandFieldList:
                FieldReadParameterSplice(&cl[it]);
                break;
            case CommandConfigRead:
                MemoryReadParameterSplice(&cl[it]);
                break;
            case CommandConfigWrite:
                MemoryWriteParameterSplice(&cl[it]);
                break;
            case CommandMemoryRead:
                MemoryReadParameterSplice(&cl[it]);
                break;
            case CommandMemoryWrite:
                MemoryWriteParameterSplice(&cl[it]);
                break;
            case CommandEepromRead:
                MemoryReadParameterSplice(&cl[it]);
                break;
            case CommandEepromWrite:
                MemoryWriteParameterSplice(&cl[it]);
                break;
            case CommandFlashRead:
                MemoryReadParameterSplice(&cl[it]);
                break;
            case CommandFlashWrite:
                MemoryWriteParameterSplice(&cl[it]);
                break;
			case CommandOtpRead:
                OtpReadParameterSplice(&cl[it]);
                break;
            case CommandOtpWrite:
                OtpWriteParameterSplice(&cl[it]);
                break;
            case CommandOtpLoad:
                break;
            case CommandRegisterDebug:
                RegisterDebugParameterSplice(&cl[it]);
                break;
            case CommandStickyWrite:
                StickyWriteParameterSplice(&cl[it]);
                break;
            case CommandStickyList:
				// no parameters
                break;
            case CommandStickyClear:
                FieldReadParameterSplice(&cl[it]);
                break;
			case CommandGainTableRead:
//                GainTableReadCommand(client);
                break;
            case CommandGainTableWrite:
//			    GainTableWriteCommand(client);
                break;
            case CommandRestore:
                ConfigurationRestoreParameterSplice(&cl[it]);
                break;
            case CommandCommit:
                ConfigurationSaveParameterSplice(&cl[it]);
                break;
            case CommandPcieCommit:
                ConfigurationPcieSaveParameterSplice(&cl[it]);
                break;
            case CommandCheck:
                ConfigurationCheckParameterSplice(&cl[it]);
                break;
            case CommandSet:
                ConfigurationSetParameterSplice(&cl[it]);
                break;
            case CommandGet:
                ConfigurationGetParameterSplice(&cl[it]);
                break;
            case CommandGetTP:
//                ConfigGetTPCommand(client);
                break;
            case CommandSetTP:
//                ConfigSetTPCommand(client);
                break;
            case CommandErase:
//                CommandEraseInput(client);
                break;
           case CommandPacketLog:
//                PacketLogEnable(10000);
                break;
           case CommandNoiseFloor:
                NoiseFloorParameterSplice(&cl[it]);
                break;
           case CommandNoiseFloorGet:
				NoiseFloorGetParameterSplice(&cl[it]);
                break;
           case CommandTargetPower:
                TargetPowerParameterSplice(&cl[it]);
                break;
           case CommandTemplate:
                TemplateParameterSplice(&cl[it]);
                break;
		   case CommandStart:
		   case CommandStop:
				// no parameters
			    break;
           case CommandSetConfig:
               SetConfigParameterSplice(&cl[it]);
               break;
           case CommandSetCal:
               SetCalParameterSplice(&cl[it]);
               break;
		   case CommandRfBbTestPoint:
               RfBbTestPointParameterSplice(&cl[it]);
				break;
		   case CommandHAL:
				HALParameterSplice(&cl[it]);
				break;
		   case CommandSleepMode:
				SleepModeParameterSplice(&cl[it]);
           default:
			    break;
        }
    }
}

static void VersionCommand(int client)
{
	char buffer[MBUFFER];

	SformatOutput(buffer,MBUFFER-1,"Version %d.%d. Build date %s %s.",NartVersionMajor(),NartVersionMinor(),NartVersionDate(),NartVersionTime());
	buffer[MBUFFER-1]=0;
	ErrorPrint(NartData,buffer);
}

    static struct _ParameterList cl[]=
    {
		{CommandExit,{"exit",0,0},"exits the program",0,0,0,0,0,0,0,0},
		{CommandHelp,{"help","?",0},"supplies information about the commands and parameters",0,0,0,0,0,0,0,0},
		{CommandHello,{"hello",0,0},"checks that the network link to nart is working, synchronizes commands and responses",0,0,0,0,0,0,0,0},
		{CommandTx,{"transmit","tx","t"},"causes the specified device to transmit",0,0,0,0,0,0,0,0},
		{CommandRx,{"receive","rx","r"},"causes the specified device to receive",0,0,0,0,0,0,0,0},
        {CommandSleep,{"sleep",0,0},"pauses program execution for the specified number of milliseconds",0,0,0,0,0,0,0,0},
        {CommandAttach,{"load","card","attach"},"loads the card",0,0,0,0,0,0,0,0},
        {CommandDetach,{"unload","remove","detach"},"unloads the card",0,0,0,0,0,0,0,0},
        {CommandReset,{"reset",0,0},"resets the device",0,0,0,0,0,0,0,0},
        {CommandPllScreen,{"pllscreen","pllscreen",0},"do pll creen on the device",0,0,0,0,0,0,0,0},
        {CommandResetMCI,{"resetMCI","MCIreset",0},"resets the device MCI",0,0,0,0,0,0,0,0},
		{CommandRegisterRead,{"rr",0,0},"reads a device register",0,0,0,0,0,0,0,0},
		{CommandRegisterWrite,{"rw",0,0},"writes a device register with the specified value",0,0,0,0,0,0, 0,0},       
		{CommandStickyList,{"sr","sl",0},"prints the current list of sticky writes",0,0,0,0,0,0,0,0},
		{CommandStickyWrite,{"sw",0,0},"sticky write of a device field/register, sticky writes are performed after every device reset",0,0,0,0,0,0,0,0},
		{CommandStickyClear,{"sc",0,0},"clear sticky field/register from list, last-on, first-off order",0,0,0,0,0,0,0,0},
		{CommandFieldRead,{"fr",0,0},"field read",0,0,0,0,0,0,0,0},
		{CommandFieldWrite,{"fw",0,0},"field write",0,0,0,0,0,0,0,0},
		{CommandFieldList,{"fl",0,0},"list all the matching fields",0,0,0,0,0,0,0,0},
		{CommandMemoryRead,{"mr",0,0},"memory read",0,0,0,0,0,0,0,0},
		{CommandMemoryWrite,{"mw",0,0},"memory write",0,0,0,0,0,0,0,0},
		{CommandEepromRead,{"er",0,0},"eeprom read",0,0,0,0,0,0,0,0},
		{CommandEepromWrite,{"ew",0,0},"eeprom write",0,0,0,0,0,0,0,0},
		{CommandFlashRead,{"xr",0,0},"flash read",0,0,0,0,0,0,0,0},
		{CommandFlashWrite,{"xw",0,0},"flash write",0,0,0,0,0,0,0,0},
		{CommandOtpRead,{"or",0,0},"otp read",0,0,0,0,0,0,0,0},
        {CommandOtpRead,{"or2",0,0},"otp read",0,0,0,0,0,0,0,0},
		{CommandOtpWrite,{"ow",0,0},"otp write",0,0,0,0,0,0,0,0},
        {CommandOtpWrite,{"ow2",0,0},"otp write",0,0,0,0,0,0,0,0},
		{CommandOtpLoad,{"ol",0,0},"otp load",0,0,0,0,0,0,0,0},
		{CommandConfigRead,{"cr",0,0},"pci config read",0,0,0,0,0,0,0,0},
		{CommandConfigWrite,{"cw",0,0},"pci config write",0,0,0,0,0,0,0,0},
		{CommandRegisterDebug,{"rd",0,0},"enable register read/write debug mode",0,0,0,0,0,0,0,0},
        {CommandGainTableRead,{"tgr",0,0},"transmit gain table read",0,0,0,0,0,0,0,0},
        {CommandGainTableWrite,{"tgw",0,0},"transmit gain table write",0,0,0,0,0,0,0,0},
		{CommandRestore,{"restore",0,0},"restores calibration information from card",0,0,0,0,0,0,0,0},
        {CommandCommit,{"commit","save",0},"finalize calibration and write data to device",0,0,0,0,0,0,0,0},
        {CommandPcieCommit,{"pcie","boot",0},"write pcie configuration data to chip",0,0,0,0,0,0,0,0},
        {CommandCheck,{"check",0,0},"check calibration data on device",0,0,0,0,0,0,0,0},
        {CommandSet,{"set",0,0},"set a configuration parameter on the card",0,0,0,0,0,0,0,0},
        {CommandGet,{"get",0,0},"get a configuration parameter from the card",0,0,0,0,0,0,0,0},
        {CommandSetTP,{"setTP",0,0},"set target power configuration parameter on the card",0,0,0,0,0,0,0,0},
        {CommandGetTP,{"getTP",0,0},"get target power configuration parameter from the card",0,0,0,0,0,0,0,0},
        {CommandErase,"erase",0,0},
		{CommandPacketLog,{"pl",0,0},"turn on packet logging",0,0,0,0,0,0,0,0},
		{CommandChannel,{"channel",0,0},"retrieve and display a list of the valid channels",0,0,0,0,0,0,0,0},
		{CommandNoiseFloor,{"noisefloor","nf",0},"compute the noise floor",0,0,0,0,0,0,0,0},
		{CommandNoiseFloorGet,{"nfg",0,0},"retrieve and display the calibrated noise floor measurements",0,0,0,0,0,0,0,0},
		{CommandTargetPower,{"targetPower","tp",0},"retrieve and display the target power values",0,0,0,0,0,0,0,0},
		{CommandStart,{"start",0,0},"start the current command",0,0,0,0,0,0,0,0},
		{CommandStop,{"stop",0,0},"stop the current command",0,0,0,0,0,0,0,0},
		{CommandTemplate,{"template",0,0},"Manipulates the configuration and calibration template",0,0,0,0,0,0,0,0},
		{CommandError,{"error",0,0},"allows you to control how error messages are displayed",0,0,0,0,0,0,0,0},
		{CommandVersion,{"version",0,0},"retrieve version information",0,0,0,0,0,0,0,0},
        {CommandSetConfig,{"setconfig",0,0},"set a configuration parameter before loading card",0,0,0,0,0,0,0,0},
        {CommandSetCal,{"setcal",0,0},"set a calibration parameter",0,0,0,0,0,0,0,0},
        {CommandGetCal,{"getcal",0,0},"get a calibration parameter",0,0,0,0,0,0,0,0},
		{CommandRfBbTestPoint,{"rfbbtp",0,0},"causes the specified device in RF_BB test points mode",0,0,0,0,0,0,0,0},
		{CommandHAL,{"HAL","Chip",0},"Add a new HAL dll in the chipIdentify list",0,0,0,0,0,0,0,0,0,0},
		{CommandCoeffDisplay,{"CoeffDisplay","cd",0},"Display IQCal Coeff",0,0,0,0,0,0,0,0},
        {CommandSleepMode,{"sleepmode","sm",0},"set device sleep mode",0,0,0,0,0,0,0,0},
	};


static void CommandProcess(int client, char *buffer)
{
	char *bufptr,*word;
    int code;
    int error;
	int returnCode;

	ErrorPrint(NartProcess,buffer,client);
    //
    // ignore leading !
    //
    bufptr=buffer;
    if(*bufptr=='!')
    {
        bufptr++;
    }
	//
	// Dispatch
	//
	CommandCurrentSave(bufptr);
	CommandParse(bufptr);
	word=CommandWord();
    //
    // select the correct command and do it
    //
    code=ParameterSelect(word,cl,sizeof(cl)/sizeof(struct _ParameterList));
    switch (code) 
    {
		case CommandExit:
			Exit();
			break;
        case CommandHello: 
            Hello(client); 
            break;
		case CommandHelp: 
			HelpCommand(cl,sizeof(cl)/sizeof(cl[0])); 
			SendDone(client);
			break;
		case CommandError: 
			// ############# needs client added
			ErrorCommand(); 
			break;
        case CommandTx:
		    LinkTransmit(client);
            break;
        case CommandRx:
            LinkReceive(client);
            break;
        case CommandSleep:
            DoSleep(client);
            break;
        case CommandAttach:
            error = CardLoad(client);
			if (error == 0)
            {
			MenuFixup(cl,sizeof(cl)/sizeof(cl[0]));
            }
            break;
        case CommandReset:
            CardReset(client);	
            break;
        case CommandPllScreen:
            error = CardPllScreen(client);	
            if(error == 0)			
			MenuFixup(cl,sizeof(cl)/sizeof(cl[0]));
            else
                SendError(client, "pllscreen");
				
            break;
        case CommandResetMCI:
            MCIReset(client);	
			MenuFixup(cl,sizeof(cl)/sizeof(cl[0]));
            break;
        case CommandChannel:
            ChannelCommand(client);
            break;
        case CommandDetach:
            CardRemove(client);
			MenuFixup(cl,sizeof(cl)/sizeof(cl[0]));
            break;
        case CommandRegisterRead:
            RegisterReadCommand(client);
            break;
        case CommandRegisterWrite:
            RegisterWriteCommand(client);
            break;
        case CommandFieldRead:
            FieldReadCommand(client);
            break;
        case CommandFieldWrite:
            FieldWriteCommand(client);
            break;
        case CommandFieldList:
            FieldListCommand(client);
            break;
        case CommandConfigRead:
            ConfigReadCommand(client);
            break;
        case CommandConfigWrite:
            ConfigWriteCommand(client);
            break;
        case CommandMemoryRead:
            MemoryReadCommand(client);
            break;
        case CommandMemoryWrite:
            MemoryWriteCommand(client);
            break;
        case CommandEepromRead:
            EepromReadCommand(client);
            break;
        case CommandEepromWrite:
            EepromWriteCommand(client);
            break;
        case CommandFlashRead:
            FlashReadCommand(client);
            break;
        case CommandFlashWrite:
            FlashWriteCommand(client);
            break;
		case CommandOtpRead:
            OtpReadCommand(client);
            break;
        case CommandOtpWrite:
            OtpWriteCommand(client);
            break;
        case CommandOtpLoad:
            OtpLoadCommand(client);
            break;
        case CommandRegisterDebug:
            RegisterDebugCommand(client);
            break;
        case CommandStickyWrite:
            FieldStickyCommand(client);
            break;
        case CommandStickyList:
            FieldStickyList(client);
            break;
        case CommandStickyClear:
            FieldStickyClear(client);
            break;
        case CommandGainTableRead:
            GainTableReadCommand(client);
            break;
        case CommandGainTableWrite:
		    GainTableWriteCommand(client);
            break;
        case CommandRestore:
            ConfigurationRestoreCommand(client);
            break;
        case CommandCommit:
            ConfigurationSaveCommand(client);
            break;
        case CommandPcieCommit:
            ConfigurationPcieSaveCommand(client);
            break;
        case CommandCheck:
            ConfigurationCheckCommand(client);
            break;
        case CommandSet:
            ConfigurationSetCommand(client);
            break;
        case CommandGet:
            ConfigurationGetCommand(client);
            break;
        case CommandGetTP:
            ConfigGetTPCommand(client);
            break;
        case CommandSetTP:
            ConfigSetTPCommand(client);
            break;
        case CommandErase:
            CommandEraseInput();
            break;
       case CommandPacketLog:
            PacketLogEnable(10000);
            break;
       case CommandNoiseFloor:
            NoiseFloorCommand(client);
            break;
       case CommandNoiseFloorGet:
            NoiseFloorGetCommand(client);
            break;
       case CommandTargetPower:
            TargetPowerGetCommand(client);
            break;
       case CommandTemplate:
            TemplateCommand(client);
            break;
       case CommandVersion:
            VersionCommand(client);
            break;
	   case CommandStart:
	   case CommandStop:
			//
			// these two commands are processed internal to commands such as "tx" or "rx".
			// they appear in this list so that no error messages are generated if they come at the wrong time.
			//
		   SendDone(client);
		   break;
        case CommandSetConfig:
            SetConfigCommand(client);
            break;
        case CommandSetCal:
            returnCode = CalibrationSetCommand(client);
			if(returnCode == 1)
			{
				MenuFixup(cl,sizeof(cl)/sizeof(cl[0]));
			}
			SendDone(client);
			break;
        case CommandGetCal:
			CalibrationGetCommand(client);
			break;
		case CommandRfBbTestPoint:
            RfBbTestPoint(client);
            break;
		case CommandHAL:
			HALCommand();
			MenuFixup(cl,sizeof(cl)/sizeof(cl[0]));
			break;
		case CommandCoeffDisplay:  // Display Tx/Rx Corr Coeff
            CoeffDisplay(client);
            break;
		case CommandSleepMode:
			SleepModeCommand(client);
			break;
        default:
		    ErrorPrint(ParseBadCommand,word);
			SendDone(client);
		    break;
	}
}


//
// read and process commands from startup file
//
static void StartFile(char *filename)
{
	char buffer[MBUFFER];
	char *eof;
	int length;
	FILE *file;

	file=fopen(filename,"r");
	if(file!=0)
	{
		while(1)
		{
			eof=fgets(buffer,MBUFFER-1,file);
			if(eof!=buffer)
			{
				fclose(file);
				return;
			}
			else
			{
				length = (int)strlen(buffer);
				if(length>0 && !Smatch(buffer," "))
				{
					buffer[length-1]=0;
					length--;
					CommandProcess(-1,buffer);
				}
			}
		}
	}
}


void NewArt(unsigned int instance, int port, char *filename)
{
	char buffer[MBUFFER];
	int nread;
	int client;

#ifdef _WINDOWS
	if ( GetQdartMode() ) 
	{
		SetStrTerminationChar( DIAG_TERM_CHAR );
		SocketWriteEnableMode( 0 ); // Only response to host command if running as QCMBR - initialize to "disable" state
		//printf( "NewArt()::SocketWriteEnableMode( 0 );\n" );
	}
	else
	{
		SetStrTerminationChar( '\n' );
		SocketWriteEnableMode( 1 ); // Enable all the writes to socket if not in QCMBR/QDART mode
		//printf( "NewArt()::SocketWriteEnableMode( 1 );\n" );
	}
#endif

    //
    // load standard parsing error messages
    //
    ErrorInit();
    ParseErrorInit();
    LinkErrorInit();
    ConnectErrorInit();
    CardErrorInit();
	DeviceErrorInit();
//    PauseErrorInit();
    NartErrorInit();
	EepromErrorInit();
	//
	// link in parameter menus
	//
	MenuFixup(cl,sizeof(cl)/sizeof(cl[0]));
    //
    // open listen socket
    //
    setInstance(instance);
	if(port>0)
	{
		_ListenSocket=SocketListen(port);
		if(_ListenSocket==0)
		{
			ErrorPrint(ConnectGuiListenBad,port);
			ErrorPrint(NartActive);
			exit(-1);
		}
		ErrorTypeResponseSet(ErrorControl,(1<<ErrorResponseShowCode)|(1<<ErrorResponseShowType)|(1<<ErrorResponseShowMessage));
	}
	else
	{
		//
		// this means accept commands from the keyboard
		// and display result by typing in the console window
		//
		_ListenSocket= 0;
//    ErrorPauseFunction(ErrorPause);
//    ErrorBellFunction(ErrorBell);
	}
	//
	// run startup file
	//
	if(filename!=0 && filename[0]!=0 && !Smatch(filename,""))
	{
		StartFile(filename);
	}

    //
    // wait for commands or new clients
    //
	while(1)
	{
		ClientAccept();

	    while(1)
		{
			nread=CommandNext(buffer,MBUFFER-1,&client);
			//
			// Got data. Process it.
			//
		    if(nread>0)
			{
				//
				// divert error and information message to the correct client
				//
				UserPrintClient=client;
				ErrorPrintFunction(UserPrintSendIt);
				CommandProcess(client,buffer);
				CommandCurrentDelete();
			}
			//
			// Got error. Probably lost command module. Redo socket accept.
			//
			else if(nread<0)
			{
			}
			//
			// slow down
			//
			else
			{
				MyDelay(50);
			}
		}
	}
}


