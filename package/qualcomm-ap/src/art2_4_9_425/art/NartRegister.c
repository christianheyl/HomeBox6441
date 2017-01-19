


#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

#include "mlibif.h"

#include "ParameterSelect.h"
#include "NartRegister.h"

#include "smatch.h"
#include "UserPrint.h"
#include "ParameterParse.h"
#include "CommandParse.h"
#include "Card.h"
#include "NewArt.h"
#include "Field.h"
#include "Sticky.h"
#include "Device.h"

#include "ErrorPrint.h"
#include "ParseError.h"
#include "NartError.h"
#include "CardError.h"

#define MBUFFER 1024

enum
{
	ResponseStyleVerbose=0,		// normal nart-style response with header, data, ERROR, and DONE messages
	ResponseStyleSplit,			// writes act as for Simple, reads act as for Verbose
	ResponseStyleSimple,		// writes return nothing, reads return value only or ERROR
};

static int _ResponseStyle=ResponseStyleVerbose;

enum
{
	MemoryAddress=0,
	MemoryValue,
	MemorySize,
	MemoryResponse,
	MemoryDebug,
	MemoryBank,
	MemoryWiFi,
	MemoryBluetooth,
    MemoryPrepost,
};

//
// for otp need to answer the question, WiFi or BT?
//
static int BankDefault=1;

static struct _ParameterList BankParameter[2]=
{
	{1,{"WiFi",0,0},0,0,0,0,0,0,0,0,0},
	{0,{"Bluetooth","bt",0},0,0,0,0,0,0,0,0,0},
};

//
// parameter definitions
// these are used by multiple commands
//
static struct _ParameterList LogicalParameter[2]=
{
	{0,{"no",0,0},0,0,0,0,0,0,0,0,0},
	{1,{"yes",0,0},0,0,0,0,0,0,0,0,0},
};

static struct _ParameterList ResponseParameter[3]=
{
	{ResponseStyleVerbose,{"verbose",0,0},0,0,0,0,0,0,0,0,0},
	{ResponseStyleSplit,{"split",0,0},0,0,0,0,0,0,0,0,0},
	{ResponseStyleSimple,{"simple",0,0},0,0,0,0,0,0,0,0,0},
};

static int DebugDefault=0;
static struct _ParameterList RegisterDebugParameter[]=
{
    {MemoryDebug,{"debug",0,0},"turn register debug log on or off",'z',0,1,1,1,0,0,&DebugDefault,
		sizeof(LogicalParameter)/sizeof(LogicalParameter[0]),LogicalParameter},
    {MemoryResponse,{"response",0,0},"the response to rx commands",'z',0,1,1,1,0,0,&_ResponseStyle,
		sizeof(ResponseParameter)/sizeof(ResponseParameter[0]),ResponseParameter},
};


static struct _ParameterList RegisterReadParameter[]=
{
    {MemoryAddress,{"address",0,0},"the address",'x',0,1,1,1,0,0,0,0,0},
};

static struct _ParameterList FieldReadParameter[]=
{
    {MemoryAddress,{"address","name",0},"the field name or address",'t',0,1,1,1,0,0,0,0,0},
};


#define MRANGE 100

//
// note that most memory read commands start with parameter 1.
// the first parameter is just used for otp.
//
static struct _ParameterList MemoryReadParameter[]=
{
    {MemoryBank,{"bank",0,0},"wifi or bluetooth?",'z',0,1,1,1,0,0,&BankDefault,
		sizeof(BankParameter)/sizeof(BankParameter[0]),BankParameter},
    {MemorySize,{"size","bytes",0},"the number of bytes in each element",'d',0,1,1,1,0,0,0,0,0},
	{MemoryAddress,{"address",0,0},"the addresses in the form minimum:maximum:increment",'x',0,MRANGE,1,1,0,0,0,0,0},
};

static struct _ParameterList MemoryWriteParameter[]=
{
    {MemoryBank,{"bank","wifi",0},"wifi or bluetooth?",'z',0,1,1,1,0,0,&BankDefault,
		sizeof(BankParameter)/sizeof(BankParameter[0]),BankParameter},
    {MemorySize,{"size","bytes",0},"the number of bytes in each element",'d',0,1,1,1,0,0,0,0,0},
    {MemoryAddress,{"address",0,0},"the addresses in the form minimum:maximum:increment",'x',0,MRANGE,1,1,0,0,0,0,0},
    {MemoryValue,{"value",0,0},"the values in the form minimum:maximum:increment",'x',0,MRANGE,1,1,0,0,0,0,0},
};

void MemoryReadParameterSplice(struct _ParameterList *list)
{
    list->nspecial=(sizeof(MemoryReadParameter)/sizeof(MemoryReadParameter[0]))-1;
    list->special=&MemoryReadParameter[1];
}

void OtpReadParameterSplice(struct _ParameterList *list)
{
    list->nspecial=sizeof(MemoryReadParameter)/sizeof(MemoryReadParameter[0]);
    list->special=MemoryReadParameter;
}

void MemoryWriteParameterSplice(struct _ParameterList *list)
{
    list->nspecial=(sizeof(MemoryWriteParameter)/sizeof(MemoryWriteParameter[0]))-1;
    list->special=&MemoryWriteParameter[1];
}

void OtpWriteParameterSplice(struct _ParameterList *list)
{
    list->nspecial=sizeof(MemoryWriteParameter)/sizeof(MemoryWriteParameter[0]);
    list->special=MemoryWriteParameter;
}

static struct _ParameterList RegisterWriteParameter[]=
{
    {MemoryAddress,{"address",0,0},"the address",'x',0,1,1,1,0,0,0,0,0},
    {MemoryValue,{"value",0,0},"the value",'x',0,1,1,1,0,0,0,0,0},
};

static struct _ParameterList FieldWriteParameter[]=
{
    {MemoryAddress,{"address","name",0},"the field name or address",'t',0,1,1,1,0,0,0,0,0},
    {MemoryValue,{"value",0,0},"the value",'x',0,1,0,0,0,0,0},
};

static struct _ParameterList StickyWriteParameter[]=
{
    {MemoryAddress,{"address","name",0},"the field name or address",'t',0,1,1,1,0,0,0,0,0},
    {MemoryValue,{"value",0,0},"the value",'x',0,1,0,0,0,0,0},
    {MemoryPrepost,{"prepost","p",0},"write sticky pre(0) or post(1) hw reset",'z',0,1,1,1,0,0,0},
};

void RegisterDebugParameterSplice(struct _ParameterList *list)
{
    list->nspecial=sizeof(RegisterDebugParameter)/sizeof(RegisterDebugParameter[0]);
    list->special=RegisterDebugParameter;
}

void RegisterReadParameterSplice(struct _ParameterList *list)
{
    list->nspecial=sizeof(RegisterReadParameter)/sizeof(RegisterReadParameter[0]);
    list->special=RegisterReadParameter;
}

void FieldReadParameterSplice(struct _ParameterList *list)
{
    list->nspecial=sizeof(FieldReadParameter)/sizeof(FieldReadParameter[0]);
    list->special=FieldReadParameter;
}

void RegisterWriteParameterSplice(struct _ParameterList *list)
{
    list->nspecial=sizeof(RegisterWriteParameter)/sizeof(RegisterWriteParameter[0]);
    list->special=RegisterWriteParameter;
}

void FieldWriteParameterSplice(struct _ParameterList *list)
{
    list->nspecial=sizeof(FieldWriteParameter)/sizeof(FieldWriteParameter[0]);
    list->special=FieldWriteParameter;
}

void StickyWriteParameterSplice(struct _ParameterList *list)
{
    list->nspecial=sizeof(StickyWriteParameter)/sizeof(StickyWriteParameter[0]);
    list->special=StickyWriteParameter;
}

void RegisterDebugCommand(int client)
{
	int ngot;
	int np;
	int ip;
	int value;
	char *name;
	int done;

	value=0;
	done=0;
	np=CommandParameterMany();
	for(ip=0; ip<np; ip++)
	{
		name=CommandParameterName(ip);
		if(Smatch(name,"d")||Smatch(name,"debug")||Smatch(name,"v")||Smatch(name,"value"))
		{
			ngot=SformatInput(CommandParameterValue(ip,0)," %d ",&value);
			if(ngot==1)
			{
//LATER				DeviceRegisterDebug(value);
				done++;
			}
		}
		else if(Smatch(name,"r")||Smatch(name,"response"))
		{
			ngot=SformatInput(CommandParameterValue(ip,0)," %d ",&value);
			if(ngot==1)
			{
				_ResponseStyle=value;
				done++;
			}
		}
	}
	if(done==0)
	{
		SendError(client,"no value");
	}
		
	SendDone(client);
}



void RegisterReadCommand(int client)
{
	int ngot;
	int np;
	int ip, it;
	int address, low, high;
	unsigned int value;
	char *name;
	char ebuffer[MBUFFER],buffer[MBUFFER];
	int error;
	int done;
	int lc, nc;
	int bad;
	int selection;
	//
	// if there's no card loaded, return error
	//
    if(CardCheckAndLoad(client)!=0)
    {
		if(_ResponseStyle!=ResponseStyleSimple)
		{
			ErrorPrint(CardNoneLoaded);
		}
    }
	else
	{
		//
		// prepare beginning of error message in case we need to use it
		//
		lc=0;
		error=0;
		done=0;
		//
		//parse arguments and do it
		//
		low=0;
		high=0;
		np=CommandParameterMany();
		for(ip=0; ip<np; ip++)
		{
			name=CommandParameterName(ip);
			selection=ParameterSelect(name,RegisterReadParameter,sizeof(RegisterReadParameter)/sizeof(RegisterReadParameter[0]));
			switch(selection)
			{
				case MemoryAddress:
					for(it=0; it<CommandParameterValueMany(ip); it++)
					{
						ngot=SformatInput(CommandParameterValue(ip,it)," %x : %x ",&low,&high);
						if(ngot>=1)
						{
							//
							// check alignment, registers must be addressed as 0, 4, 8, ...
							//
							low=4*(low/4);

							if(ngot<2)
							{
								high=low;
							}
							for(address=low; address<=high; address+=4)
							{
								bad=DeviceRegisterRead(address,&value);
								if(bad)
								{
									if(_ResponseStyle!=ResponseStyleSimple)
									{
										nc=SformatOutput(&ebuffer[lc],MBUFFER-lc-1,"bad read from address %x, ",address);
										if(nc>0)
										{
											lc+=nc;
										}
									}
									else
									{
										nc=SformatOutput(&ebuffer[lc],MBUFFER-lc-1,"ERROR,");
										if(nc>0)
										{
											lc+=nc;
										}
									}
									error++;
								}
								else
								{
									if(_ResponseStyle!=ResponseStyleSimple)
									{
										SformatOutput(buffer,MBUFFER-1,"|rr|%04x|%08x|",address,value);
										if(done==0)
										{
											ErrorPrint(NartDataHeader,"|rr|address|value|");
										}
										ErrorPrint(NartData,buffer);
									}
									else
									{
										nc=SformatOutput(&ebuffer[lc],MBUFFER-lc-1,"%x,",value);
										if(nc>0)
										{
											lc+=nc;
										}
									}
									done++;
								}
							}
						}
						else
						{
							if(_ResponseStyle!=ResponseStyleSimple)
							{
								nc=SformatOutput(&ebuffer[lc],MBUFFER-lc-1,"bad address %s, ",CommandParameterValue(ip,it));
								if(nc>0)
								{
									lc+=nc;
								}
							}
							else
							{
								nc=SformatOutput(&ebuffer[lc],MBUFFER-lc-1,"ERROR,");
								if(nc>0)
								{
									lc+=nc;
								}
							}
							error++;
						}
					}
					break;
				default:
					ErrorPrint(ParseBadParameter,name);
					break;
			}
		}
		//
		// send DONE or ERROR
		//
		if(_ResponseStyle!=ResponseStyleSimple)
		{
			if(done==0 && error==0)
			{
				nc=SformatOutput(&ebuffer[lc],MBUFFER-lc-1,"no valid address");
				if(nc>0)
				{
					lc+=nc;
				}
				error++;
			}
			if(error>0)
			{
				SendError(client,ebuffer);
			}
		}
		else
		{
			ErrorPrint(NartOther,ebuffer);
		}
	}
	if(_ResponseStyle!=ResponseStyleSimple)
	{
		SendDone(client);
	}
}


void RegisterWriteCommand(int client)
{
	int ngot;
	int np;
	int ip, it;
	int address;
	unsigned int value;
	char *name;
	char ebuffer[MBUFFER],buffer[MBUFFER];
	int error;
	int done;
	int lc, nc;
	int bad;
	//
	// check if card is loaded
	//
    if(CardCheckAndLoad(client)!=0)
    {
		if(_ResponseStyle==ResponseStyleVerbose)
		{
			ErrorPrint(CardNoneLoaded);
		}
    }
	else
	{
		//
		// prepare beginning of error message in case we need to use it
		//
		lc=0;
		error=0;
		done=0;
		//
		//parse arguments and do it
		//
		address= -1;
		np=CommandParameterMany();
		for(ip=0; ip<np; ip++)
		{
			name=CommandParameterName(ip);
			if(Smatch(name,"a")||Smatch(name,"address"))
			{
				for(it=0; it<CommandParameterValueMany(ip); it++)
				{
					ngot=SformatInput(CommandParameterValue(ip,it)," %x ",&address);
					if(ngot!=1)
					{
						address=0;
					}
					//
					// check alignment, registers must be addressed as 0, 4, 8, ...
					//
					address=4*(address/4);
				}
			}
			if(Smatch(name,"v")||Smatch(name,"value"))
			{
				for(it=0; it<CommandParameterValueMany(ip); it++)
				{
					ngot=SformatInput(CommandParameterValue(ip,it)," %x ",&value);
					if(ngot==1 && address>=0)
					{
						bad=DeviceRegisterWrite(address+it*sizeof(value),value);
						if(bad)
						{
							if(_ResponseStyle==ResponseStyleVerbose)
							{
								nc=SformatOutput(&ebuffer[lc],MBUFFER-lc-1,"bad write to address %lx, ",address+it*sizeof(value));
								if(nc>0)
								{
									lc+=nc;
								}
							}
							error++;
						}
						else
						{
							if(_ResponseStyle==ResponseStyleVerbose)
							{
								SformatOutput(buffer,MBUFFER-1,"|rw|%04x|%08x|",(unsigned int)(address+it*sizeof(value)),value);
								if(done==0)
								{
									ErrorPrint(NartDataHeader,"|rw|address|value|");
								}
								ErrorPrint(NartData,buffer);
							}
							done++;
						}
					}
					else
					{
						if(_ResponseStyle==ResponseStyleVerbose)
						{
							nc=SformatOutput(&ebuffer[lc],MBUFFER-lc-1,"bad address %s, ",CommandParameterValue(ip,it));
							if(nc>0)
							{
								lc+=nc;
							}
						}
						error++;
					}
				}
			}
		}
		//
		// send DONE or ERROR
		//
		if(_ResponseStyle==ResponseStyleVerbose)
		{
			if(done==0 && error==0)
			{
				nc=SformatOutput(&ebuffer[lc],MBUFFER-lc-1,"no valid address or no valid value");
				if(nc>0)
				{
					lc+=nc;
				}
				error++;
			}
			if(error>0)
			{
				SendError(client,ebuffer);
			}
		}
	}
	if(_ResponseStyle==ResponseStyleVerbose)
	{
		SendDone(client);
	}
}


static int BitFirst(unsigned int mask)
{
	int it;

	for(it=0; it<32; it++)
	{
		if((1<<it) & mask)
		{
			return it;
		}
	}
	return 0;
}

enum
{
    RegisterAddress=0,
    RegisterValue,
};


static void FieldReadHeaderSend(int client)
{
	ErrorPrint(NartDataHeader,"|fr|name|address|register|high|low|mask|hex|decimal|signed|");
}

static void FieldWriteHeaderSend(int client)
{
	ErrorPrint(NartDataHeader,"|fw|name|address|register|high|low|mask|hex|decimal|signed|");
}

static int FieldNameLookup(char *name, int *address, int *high, int *low)
{
    int ngot;
    int flip;
	char extra[2];
 	//
	// look up in our big list of field names
	//
	ngot=FieldFind(name,(unsigned int *)address,low,high);
	if(ngot!=1)
	{
		//
		// let's try to parse it as register[high,low]
		//
		ngot=SformatInput(name," %x [ %d , %d ] %1c ",address,high,low,extra);
		if(ngot!=3)
		{
			//
			// let's try to parse it as register[bit]
			//
			ngot=SformatInput(name," %x [ %d ] %1c ",address,high,extra);
			if(ngot==2)
			{
				*low= *high;
				ngot=3;
			}
			else
			{
				//
				// let's try to parse it as register
				//
				ngot=SformatInput(name," %x %1c ",address,extra);
				if(ngot==1)
				{
					*low=0;
					*high=31;
					ngot=3;
				}
				else
				{
					ngot= -1;
				}
			}
		}
	}
	if(ngot>0 && (*low) > (*high))
	{
		flip= *high;
		*high= *low;
		*low=flip;
	}
		
    return (ngot<=0);
}


static int FieldParse(char *name, 
    unsigned int *address, int *high, int *low, int max) 
{
	char *first,*last;
    unsigned int faddress, laddress;
    int flow, fhigh, llow, lhigh;
    int error;
    int it;
    //
    // see if this is a range specification
    //
    last=strchr(name,':');
    if(last)
    {
        *last=0;
        first=name;
        last++;
    }
    else
    {
        first=name;
        last=0;
    }
    error=FieldNameLookup(first,(int *)&faddress,&fhigh,&flow);
    if(last!=0 && !Smatch(last,""))
    {
        error+=FieldNameLookup(last,(int *)&laddress,&lhigh,&llow);
    }
    else
    {
        laddress=faddress;
        lhigh=fhigh;
        llow=flow;
    }
    if(last==0 )
    {
        last="";
    }

    it=1;
    address[0]=faddress;
    low[0]=flow;
    high[0]=fhigh;
    for(it=1, faddress+=4; it<max && faddress<laddress; it++, faddress+=4)
    {
        address[it]=faddress;
        high[it]=31;
        low[it]=0;
    }
    if(it<max && faddress<=laddress)
    {
        address[it]=laddress;
        high[it]=lhigh;
        low[it]=llow;
        it++;
    }

    return it;
}


#define MFIELD 200

static int FieldParseLookupAndReturn(int client, char *name)
{
	unsigned int mask;
	unsigned int reg;
	unsigned int value;
    int sign;
	char fname[MBUFFER],buffer[MBUFFER];
    unsigned int address[MFIELD];
    int low[MFIELD], high[MFIELD];
    int nfield;
    int ia;
	int nc;
	int bad;
    char *registerName,*fieldName;

    nfield=FieldParse(name,address,high,low,MFIELD);
	//
	// now do the read operation
	//
    if(nfield>0)
    {
        for(ia=0; ia<nfield; ia++)
        {
			//
			// check alignment, registers must be addressed as 0, 4, 8, ...
			//
			address[ia]=4*(address[ia]/4);

		    bad=DeviceRegisterRead(address[ia],&reg);
		    if(bad)
		    {
		        nc=SformatOutput(buffer,MBUFFER-1,"bad read from address %x for %s",address[ia],name);
                SendError(client,buffer);
		    }
		    else
		    {	
			    mask=MaskCreate(low[ia],high[ia]);
			    value=(reg&mask)>>low[ia];
                if(value&(1<<(high[ia]-low[ia])))
                {
                    sign=(0xffffffff^(mask>>low[ia]))|value;
                }
                else
                {
                    sign=value;
                }

                if(!FieldFindByAddress(address[ia], low[ia], high[ia], &registerName, &fieldName))
                {
                    SformatOutput(fname,MBUFFER-1,"%s.%s",registerName,fieldName);
                    fname[MBUFFER-1]=0;
                }
                else
                {
                    SformatOutput(fname,MBUFFER-1,"%04x[%d,%d]",address[ia],high[ia],low[ia]);
                    fname[MBUFFER-1]=0;
                }

		        SformatOutput(buffer,MBUFFER-1,"|fr|%s|%04x|%08x|%d|%d|%08x|%08x|%u|%d|",
                    fname,address[ia],reg,high[ia],low[ia],mask,value,value,sign);
		        ErrorPrint(NartData,buffer);
		    }
	    }
    }
	else
	{
		nc=SformatOutput(buffer,MBUFFER-1,"bad field name %s",name);
        SendError(client,buffer);
	}
    return 0;
}


void FieldReadCommand(int client)
{
	int ngot;
	int np;
	int ip, it;
	char *name;
    int done;
    int code;
    int nvalue;
    static struct _ParameterList rr[]=
    {
	    {RegisterAddress,"address","name",0},
    };
	//
	// if there's no card loaded, return error
	//
    if(CardCheckAndLoad(client)!=0)
    {
		if(_ResponseStyle!=ResponseStyleSimple)
		{
			ErrorPrint(CardNoneLoaded);
		}	
    }
	else
	{
		done=0;
		//
		// parse arguments and do it
		//
		np=CommandParameterMany();
		for(ip=0; ip<np; ip++)
		{
			name=CommandParameterName(ip);
			//
			// see if this is a valid parameter name
			//
			code=ParameterSelect(name,rr,sizeof(rr)/sizeof(struct _ParameterList));
			switch(code)
			{
				case RegisterAddress:
					if(done==0)
					{
						FieldReadHeaderSend(client);
						done=1;
					}
					nvalue=CommandParameterValueMany(ip);
					for(it=0; it<nvalue; it++)
					{	
						name=CommandParameterValue(ip,it);
						ngot=FieldParseLookupAndReturn(client,name);
					}
					break;
				default:
					if(done==0)
					{
						FieldReadHeaderSend(client);
						done=1;
					}
					ngot=FieldParseLookupAndReturn(client,name);
				break;
			}
		}
	}
	//
	// send DONE
	//
	if(_ResponseStyle!=ResponseStyleSimple)
	{
		SendDone(client);
	}
}


static int SetAndReturn(int client, unsigned int address, int high, int low, unsigned int value)
{
    char buffer[MBUFFER];
    int done;
    int error;
    int bad;
    unsigned int mask;
    unsigned int reg;
    int sign;
    char *registerName,*fieldName;
    char fname[MBUFFER];

    error=0;
    done=0;
	//
	// check alignment, registers must be addressed as 0, 4, 8, ...
	//
	address=4*(address/4);

	bad=DeviceRegisterRead(address,&reg);
	if(bad)
	{
		SformatOutput(buffer,MBUFFER-1,"bad read from address %x",address);
        SendError(client,buffer);
		error++;
	}
	else
	{
		mask=MaskCreate(low,high);
		reg &= ~(mask);							// clear bits
		reg |= ((value<<low)&mask);				// set new value
        UserPrint("|=%08x  ",reg);
		DeviceRegisterWrite(address,reg);
        if(value&(1<<(high-low)))
        {
            sign=(0xffffffff^(mask>>low))|value;
        }
        else
        {
            sign=value;
        }

        if(!FieldFindByAddress(address, low, high, &registerName, &fieldName))
        {
            SformatOutput(fname,MBUFFER-1,"%s.%s",registerName,fieldName);
            fname[MBUFFER-1]=0;
        }
        else {
        
            SformatOutput(fname,MBUFFER-1,"%04x[%d,%d]",address,high,low);
            fname[MBUFFER-1]=0;
        }

		SformatOutput(buffer,MBUFFER-1,"|fw|%s|%04x|%08x|%d|%d|%08x|%08x|%u|%d|",
            fname,address,reg,high,low,mask,value,value,sign);
		ErrorPrint(NartData,buffer);
		done++;
	}
    return done+error;
}


void FieldWriteCommand(int client)
{
    int ngot;
	int nt;
	int np;
	int ip, it;

    int naddress;
	unsigned int address[MFIELD];
	int low[MFIELD], high[MFIELD];
    int nvalue;
    unsigned int value[MFIELD];

	char *name;
	char buffer[MBUFFER];
	int error;
	int done;
	int lc;
    int code;

    static struct _ParameterList rr[]=
    {
	    {RegisterAddress,"address","name",0},
	    {RegisterValue,"value",0,0},
    };
	//
	// check if card is loaded
	//
	if(CardValid())
	{
		//
		// prepare beginning of error message in case we need to use it
		//
		lc=0;
		error=0;
		done=0;
		naddress=0;
		nvalue=0;
		//
		// parse arguments and do it
		//
		np=CommandParameterMany();
		for(ip=0; ip<np; ip++)
		{
			name=CommandParameterName(ip);
			//
			// see if this is a valid parameter name
			//
			code=ParameterSelect(name,rr,sizeof(rr)/sizeof(struct _ParameterList));
			nt=CommandParameterValueMany(ip);
			switch(code)
			{
				case RegisterAddress:
					for(it=0; it<nt; it++)
					{	
						name=CommandParameterValue(ip,it);
						ngot=FieldParse(name,&address[naddress],&high[naddress],&low[naddress],MFIELD-naddress);
						if(ngot>0)
						{
							naddress+=ngot;
						}
						else
						{
							SformatOutput(buffer,MBUFFER-1,"bad field name %s",name);
							SendError(client,buffer);
							error++;
						}
					}
					break;
				case RegisterValue:
					nvalue=ParseHex(ip, name, MFIELD, (unsigned int *)value);
					if(nvalue>0)
					{
						if(done==0)
						{
							FieldWriteHeaderSend(client);
							done=1;
						}
						for(it=0; it<naddress; it++)
						{
							done+=SetAndReturn(client,address[it],high[it],low[it],value[it%nvalue]);
						}
					}
					break;
				default:
					naddress=0;
					ngot=FieldParse(name,&address[naddress],&high[naddress],&low[naddress],MFIELD-naddress);
					if(ngot>0)
					{
						naddress+=ngot;
					}
					else
					{
						SformatOutput(buffer,MBUFFER-1,"bad field name %s",name);
						SendError(client,buffer);
						error++;
					}

					nvalue=ParseHex(ip, name, MFIELD, (unsigned int *)value);
					if(nvalue>0)
					{
						if(done==0)
						{
							FieldWriteHeaderSend(client);
							done=1;
						}
						for(it=0; it<naddress; it++)
						{
							done+=SetAndReturn(client,address[it],high[it],low[it],value[it%nvalue]);
						}
					}
					break;
			}
		}
		//
		// send DONE or ERROR
		//
		if(done==0 && error==0)
		{
			SformatOutput(buffer,MBUFFER-1,"no valid field name or no valid value");
			SendError(client,buffer);
			error++;
		}
	}
	else
	{
		if(_ResponseStyle==ResponseStyleVerbose)
		{
			ErrorPrint(CardNoneLoaded);
		}
	}
	if(_ResponseStyle==ResponseStyleVerbose)
	{
		SendDone(client);
	}
}


void FieldStickyCommand(int client)
{
	int ngot;
	int np;
	int ip, it;
	unsigned int address;
	int low, high;
	unsigned int value[8] = {0};
	char *name;
	char ebuffer[MBUFFER];
	int error;
	int done;
	int lc, nc;
	int gotaddress;
	int gotvalue;
	int doit;
	int multiple;
	int prepost;
	int stickylevel;
	int resetlocation;
    int modal;

	address=0;
	low=0;
	high=0;
	prepost = 1; // write sticky after hw reset
	//value=0;
	//
	// if there's no card loaded, return error
	//
    if(CardCheckAndLoad(client)!=0)
    {
		ErrorPrint(CardNoneLoaded);
    }
	else
	{
		//
		// prepare beginning of error message in case we need to use it
		//
		lc=0;
		error=0;
		done=0;
		doit=1;
		multiple=1;
		stickylevel=DEF_LINKLIST_IDX;  // if "l" para, maintain for backward compatibility
		resetlocation=0;
		//
		//parse arguments and do it
		//
		gotaddress=0;
		gotvalue=0;
		np=CommandParameterMany();
		for(ip=0; ip<np; ip++)
		{
			name=CommandParameterName(ip);
			if(Smatch(name,"a")||Smatch(name,"address")||Smatch(name,"n")||Smatch(name,"name"))
			{
	//			for(it=0; it<CommandParameterValueMany(ip); it++)
				it=0;
				{
					//
					// look up in our big list of field names
					//
					ngot=FieldFind(CommandParameterValue(ip,it),&address,&low,&high);
					if ((ngot!=1) && (Smatch(name,"a")||Smatch(name,"address")))
					{
						//
						// let's try to parse it as register[low:high]
						//
						ngot=SformatInput(CommandParameterValue(ip,it)," %x [ %d : %d ]",&address,&low,&high);
						if(ngot>0)
						{
							if(ngot==1)
							{
								low=0;
								high=31;
								ngot=3;
							}
							if(ngot==2)
							{
								high=31;
								ngot=3;
							}
						}
					}
					//
					// now do the read operation
					//
					if(ngot<=0 || address == 0)
					{
						nc=SformatOutput(&ebuffer[lc],MBUFFER-lc-1,"bad field name %s, ",CommandParameterValue(ip,it));
						if(nc>0)
						{
							lc+=nc;
						}
						error++;
					}
					else
					{
						gotaddress=1;
					}
				}
			}
			else if(Smatch(name,"v")||Smatch(name,"value"))
			{
				for(it=0; it<CommandParameterValueMany(ip); it++)
				//it=0;
				{
					ngot=SformatInput(CommandParameterValue(ip,it)," %x ",&value[it]);
					if(ngot==1)
					{
						gotvalue=1;	
					}
					else
					{
						nc=SformatOutput(&ebuffer[lc],MBUFFER-lc-1,"bad field name %s, ",CommandParameterValue(ip,it));
						if(nc>0)
						{
							lc+=nc;
						}
						error++;
					}
				}
                // number of values should be 1 (common), 2 (2-value modal) or 4/5 (4-value modal or 5-value modal if 11ac)
                modal = DeviceIs11ACDevice() ? 5 : 4;
                if (it != 1 && it != 2 && it != modal)
                {
				    nc=SformatOutput(&ebuffer[lc],MBUFFER-lc-1,"bad number of values, it should be 1 (common), 2 (2-value modal) or %d (%d-value modal), ",
                                        modal, modal);
					if(nc>0)
					{
						lc+=nc;
					}
					error++;
                    gotvalue = 0;
                }
                if ((it == 2 || it == modal) && (low != 0 || high != 31))
                {
				    nc=SformatOutput(&ebuffer[lc],MBUFFER-lc-1,"no low/high parameters are accepted for modal register, ");
					if(nc>0)
					{
						lc+=nc;
					}
					error++;
                    gotvalue = 0;
                }
                modal = it;
			}
			else if(Smatch(name,"stickylevel")||Smatch(name,"slevel"))
			{
				it=0;
				{
					ngot=SformatInput(CommandParameterValue(ip,it)," %d ",&stickylevel);
				}
			}
			else if(Smatch(name,"resetlocation")||Smatch(name,"rloc"))
			{
				it=0;
				{
					ngot=SformatInput(CommandParameterValue(ip,it)," %d ",&resetlocation);
				}
			}
			else if(Smatch(name,"do")||Smatch(name,"d"))
			{
				it=0;
				{
					ngot=SformatInput(CommandParameterValue(ip,it)," %d ",&doit);
				}
			}
			else if(Smatch(name,"multiple")||Smatch(name,"m"))
			{
				it=0;
				{
					ngot=SformatInput(CommandParameterValue(ip,it)," %d ",&multiple);
				}
			}
			else if(Smatch(name,"prepost")||Smatch(name,"p"))
			{
				it=0;
				{
					ngot=SformatInput(CommandParameterValue(ip,it)," %d ",&prepost);
				}
			}
		}
		//
		// send DONE or ERROR
		//
		if(gotaddress && gotvalue)
		{
			if(multiple)
			{
                if (modal > 1)
                {
                    StickyInternalAddArray(stickylevel,address,low,high,value,modal,prepost);
                }
                else
                {
				    StickyInternalAdd(stickylevel,address,low,high,value[0],prepost);
                }
			}
			else
			{
                if (modal > 1)
                {
                    StickyInternalChangeArray(stickylevel,address,low,high,value,modal,prepost);
                }
                else
                {
				    StickyInternalChange(stickylevel,address,low,high,value[0],prepost);
                }
			}
            if(!DeviceIsEmbeddedArt() && doit)
			{
				DeviceFieldWrite(address,low,high,value[0]);
			}
		}
		else if(error>0)
		{
			SendError(client,ebuffer);
		}
		else
		{
			lc=0;
			nc=SformatOutput(&ebuffer[lc],MBUFFER-lc-1,"no valid field name or no valid value");
			if(nc>0)
			{
				lc+=nc;
				SendError(client,ebuffer);
			}
		}
	}
	SendDone(client);
}


void FieldStickyClear(int client)
{
	int ngot;
	int np;
	int ip, it;
	unsigned int address;
	int low, high;
	char *name;
	char ebuffer[MBUFFER];
	int error;
	int done;
	int lc, nc;
	int multiple;
	int serror;
	int slevel = -1;  // -1 = no slevel para yet, using default 
	//
	// if there's no card loaded, return error
	//
    if(CardCheckAndLoad(client)!=0)
    {
		ErrorPrint(CardNoneLoaded);
    }
	else
	{
		//
		// prepare beginning of error message in case we need to use it
		//
		lc=0;
		error=0;
		done=0;
		multiple=1;
		//
		//parse arguments and do it
		//
		np=CommandParameterMany();
		for(ip=0; ip<np; ip++)
		{
			name=CommandParameterName(ip);
			if(Smatch(name,"slevel")||Smatch(name,"stickylevel"))
			{
				it=0;
				{
					ngot=SformatInput(CommandParameterValue(ip,it)," %d ",&slevel);
				}
			} else
			if(Smatch(name,"a")||Smatch(name,"address")||Smatch(name,"n")||Smatch(name,"name"))
			{
				for(it=0; it<CommandParameterValueMany(ip); it++)
				{
					if(Smatch(CommandParameterValue(ip,it),"all") || Smatch(CommandParameterValue(ip,it),"*"))
					{
						StickyClear(ARN_LINKLIST_IDX, DeviceIsEmbeddedArt());
					    StickyClear(ARD_LINKLIST_IDX, 0);
					    StickyClear(MRN_LINKLIST_IDX, 0);
					    StickyClear(MRD_LINKLIST_IDX, 0);
						done++;
					}
					else
					{
						//
						// look up in our big list of field names
						//
						ngot=FieldFind(CommandParameterValue(ip,it),&address,&low,&high);
						if(ngot!=1)
						{
							//
							// let's try to parse it as register:low:high
							//
							ngot=SformatInput(CommandParameterValue(ip,it)," %x [ %d : %d ]",&address,&low,&high);
							if(ngot>0)
							{
								if(ngot==1)
								{
									low=0;
									high=31;
									ngot=3;
								}
								if(ngot==2)
								{
									high=31;
									ngot=3;
								}
							}
						}
						//
						// now do the read operation
						//
						if(ngot<=0 || address==0)
						{
							nc=SformatOutput(&ebuffer[lc],MBUFFER-lc-1,"bad field name %s, ",CommandParameterValue(ip,it));
							if(nc>0)
							{
								lc+=nc;
							}
							error++;
						}
						else
						{ 
							done++;
							if (slevel == -1)  // not set yet, use default
								slevel = DEF_LINKLIST_IDX;
							serror=StickyInternalClear(slevel,address,low,high);
							if(multiple==0)
							{
								while(serror==0)
								{
									serror=StickyInternalClear(slevel,address,low,high);
								}
							}
							done++;
						}
					}
				}
			}
			else if(Smatch(name,"multiple")||Smatch(name,"m"))
			{
				it=0;
				{
					ngot=SformatInput(CommandParameterValue(ip,it)," %d ",&multiple);
				}
			}
		}
		//
		// send ERROR
		//
		if(done<=0 && error<=0)
		{
			lc=0;
			nc=SformatOutput(&ebuffer[lc],MBUFFER-lc-1,"no valid field name or no valid value");
			if(nc>0)
			{
				lc+=nc;
				error++;
			}
		}
		if(error>0)
		{
			SendError(client,ebuffer);
		}
	}
	SendDone(client);
}

static void StickyListHeaderSend(int client)
{
	ErrorPrint(NartDataHeader,"|sl|name|address|high|low|mask|hex|decimal|signed|");
}


void FieldStickyList(int client)
{
	unsigned int address;
	int low, high;
    unsigned int value[4] = {0};
	char buffer[MBUFFER];
	int error;
    int sign[4];
	char fname[MBUFFER];
    char *registerName,*fieldName;
    unsigned int mask;
    int count, i;
	char *name;
	int np, ngot;
	int ip, it;
	int slevel = -1;  // -1 = no slevel para yet, using default 
	//
	// check if card is loaded
	//
	if(!CardValid())
	{
		ErrorPrint(CardNoneLoaded);
		return;
	}

    StickyListHeaderSend(client);

	np=CommandParameterMany();
	for(ip=0; ip<np; ip++)
	{
		name=CommandParameterName(ip);
		if(Smatch(name,"slevel")||Smatch(name,"stickylevel"))
		{
			it=0;
			{
				ngot=SformatInput(CommandParameterValue(ip,it)," %d ",&slevel);
			}
		}
	}
	if (slevel == -1)  // not set yet, use default
		slevel = DEF_LINKLIST_IDX;

	for(error=StickyHead(slevel,&address,&low,&high,value,&count); error==0; error=StickyNext(slevel,&address,&low,&high,value,&count))
	{
		mask=MaskCreate(low,high);
        for (i = 0; i < count; ++i)
        {
            if(value[i]&(1<<(high-low)))
            {
                sign[i]=(0xffffffff^(mask>>low))|value[i];
        }
        else
        {
                sign[i]=value[i];
            }
        }

        if(!FieldFindByAddress(address, low, high, &registerName, &fieldName))
        {
            SformatOutput(fname,MBUFFER-1,"%s.%s",registerName,fieldName);
            fname[MBUFFER-1]=0;
        }
        else
        {
            SformatOutput(fname,MBUFFER-1,"%04x[%d,%d]",address,high,low);
            fname[MBUFFER-1]=0;
        }

		SformatOutput(buffer,MBUFFER-1,"|sl|%s|%08x|%d|%d|%08x|",
					  fname,address,high,low,mask);

        for (i = 0; i < count; ++i)
        {
            SformatOutput(buffer,MBUFFER-1,"%s%08lx|%lu|%ld|", buffer, value[i],value[i],sign[i]);
        }
		ErrorPrint(NartData,buffer);
	}
	
	SendDone(client);
}


void ConfigReadCommand(int client)
{
	int ngot;
	int np;
	int ip, it;
	int address, low, high;
	unsigned int value;
	char *name;
	char ebuffer[MBUFFER],buffer[MBUFFER];
	int error;
	int done;
	int lc, nc;
	//
	// if there's no card loaded, return error
	//
    if(CardCheckAndLoad(client)!=0)
    {
		ErrorPrint(CardNoneLoaded);
    }
	else
	{
		//
		// prepare beginning of error message in case we need to use it
		//
		lc=0;
		error=0;
		done=0;
		//
		//parse arguments and do it
		//
		low=0;
		high=0;
		np=CommandParameterMany();
		for(ip=0; ip<np; ip++)
		{
			name=CommandParameterName(ip);
			if(Smatch(name,"a")||Smatch(name,"address"))
			{
				for(it=0; it<CommandParameterValueMany(ip); it++)
				{
					ngot=SformatInput(CommandParameterValue(ip,it)," %x : %x ",&low,&high);
					if(ngot>=1)
					{
						if(ngot<2)
						{
							high=low;
						}
						for(address=low; address<=high; address+=4)
						{
                            if (DeviceIsEmbeddedArt() == 1)
                            {
                                DeviceReadPciConfigSpace(address, &value);
	                        }
	                        else 
                            {
							    value=OScfgRead(0,address);
                            }
							{
								if(_ResponseStyle!=ResponseStyleSimple)
								{
									SformatOutput(buffer,MBUFFER-1,"|cr|%04x|%08x|",address,value);
									if(done==0)
									{
										ErrorPrint(NartDataHeader,"|cr|address|value|");
									}
									ErrorPrint(NartData,buffer);
								}
								else
								{
									nc=SformatOutput(&ebuffer[lc],MBUFFER-lc-1,"%x,",value);
									if(nc>0)
									{
										lc+=nc;
									}
								}
								done++;
							}
						}
					}
					else
					{
						if(_ResponseStyle!=ResponseStyleSimple)
						{
							nc=SformatOutput(&ebuffer[lc],MBUFFER-lc-1,"bad address %s, ",CommandParameterValue(ip,it));
							if(nc>0)
							{
								lc+=nc;
							}
						}
						else
						{
							nc=SformatOutput(&ebuffer[lc],MBUFFER-lc-1,"ERROR,");
							if(nc>0)
							{
								lc+=nc;
							}
						}
						error++;
					}
				}
			}
		}
		//
		// send DONE or ERROR
		//
		if(_ResponseStyle!=ResponseStyleSimple)
		{
			if(done==0 && error==0)
			{
				nc=SformatOutput(&ebuffer[lc],MBUFFER-lc-1,"no valid address");
				if(nc>0)
				{
					lc+=nc;
				}
				error++;
			}
			if(error>0)
			{
				SendError(client,ebuffer);
			}
		}
		else
		{
			ErrorPrint(NartOther,ebuffer);
		}
	}
	if(_ResponseStyle!=ResponseStyleSimple)
	{
		SendDone(client);
	}
}

static void FieldListHeaderSend(void)
{
	ErrorPrint(NartDataHeader,"|fl|name|address|high|low|mask|");
}


static void FieldListPrint(char *name, unsigned int address, int low, int high)
{
	char buffer[MBUFFER];
	unsigned int mask;

	mask=MaskCreate(low,high);
	SformatOutput(buffer,MBUFFER-1,"|fl|%s|%x|%d|%d|%x|",name,address,high,low,mask);
	ErrorPrint(NartData,buffer);
}


void FieldListCommand(int client)
{
	int ngot;
	int np;
	int ip, it;
	char *name;
	//
	// if there's no card loaded, return error
	//
    if(CardCheckAndLoad(client)!=0)
    {
		ErrorPrint(CardNoneLoaded);
    }
	else
	{
		//
		// parse arguments and do it
		//
		FieldListHeaderSend();
		np=CommandParameterMany();
		for(ip=0; ip<np; ip++)
		{
			name=CommandParameterName(ip);
			if(Smatch(name,"a")||Smatch(name,"address")||Smatch(name,"n")||Smatch(name,"name"))
			{
				for(it=0; it<CommandParameterValueMany(ip); it++)
				{
					ngot=FieldList(CommandParameterValue(ip,it),FieldListPrint);
				}
			}
			else
			{
				ErrorPrint(ParseBadParameter,name);
			}
		}
	}
	SendDone(client);
}


void ConfigWriteCommand(int client)
{
	int ngot;
	int np;
	int ip, it;
	int address;
	unsigned int value;
	char *name;
	char ebuffer[MBUFFER],buffer[MBUFFER];
	int error;
	int done;
	int lc, nc;
	//
	// check if card is loaded
	//
	if(!CardValid())
	{
		if(_ResponseStyle==ResponseStyleVerbose)
		{
		    ErrorPrint(CardNoneLoaded);
		}
	}
	else
	{
		//
		// prepare beginning of error message in case we need to use it
		//
		lc=0;
		error=0;
		done=0;
		//
		//parse arguments and do it
		//
		address= -1;
		np=CommandParameterMany();
		for(ip=0; ip<np; ip++)
		{
			name=CommandParameterName(ip);
			if(Smatch(name,"a")||Smatch(name,"address"))
			{
				for(it=0; it<CommandParameterValueMany(ip); it++)
				{
					ngot=SformatInput(CommandParameterValue(ip,it)," %x ",&address);
					if(ngot!=1)
					{
						address=0;
					}
				}
			}
			if(Smatch(name,"v")||Smatch(name,"value"))
			{
				for(it=0; it<CommandParameterValueMany(ip); it++)
				{
					ngot=SformatInput(CommandParameterValue(ip,it)," %x ",&value);
					if(ngot==1 && address>=0)
					{
                        if (DeviceIsEmbeddedArt() == 1)
                        {
                            DeviceWritePciConfigSpace(address, value);
	                    }
	                    else 
                        {
						    OScfgWrite(0,address,value);
                        }
						{
							if(_ResponseStyle==ResponseStyleVerbose)
							{
								SformatOutput(buffer,MBUFFER-1,"|cw|%04x|%08x|",(unsigned int)(address+it*sizeof(value)),value);
								if(done==0)
								{
									ErrorPrint(NartDataHeader,"|cw|address|value|");
								}
								ErrorPrint(NartData,buffer);
							}
							done++;
						}
					}
					else
					{
						if(_ResponseStyle==ResponseStyleVerbose)
						{
							nc=SformatOutput(&ebuffer[lc],MBUFFER-lc-1,"bad address %s, ",CommandParameterValue(ip,it));
							if(nc>0)
							{
								lc+=nc;
							}
						}
						error++;
					}
				}
			}
		}
		//
		// send DONE or ERROR
		//
		if(_ResponseStyle==ResponseStyleVerbose)
		{
			if(done==0 && error==0)
			{
				nc=SformatOutput(&ebuffer[lc],MBUFFER-lc-1,"no valid address or no valid value");
				if(nc>0)
				{
					lc+=nc;
				}
				error++;
			}
			if(error>0)
			{
				SendError(client,ebuffer);
			}
		}
	}
	if(_ResponseStyle==ResponseStyleVerbose)
	{
		SendDone(client);
	}
}


#define ABS(x) ((x)>=0?(x):(-(x)))

static void InternalReadCommand(int client, 
	int (*read1)(unsigned int address, unsigned int *value, int bank), 
	int (*read2)(unsigned int address, unsigned int *value, int bank), 
	int (*read4)(unsigned int address, unsigned int *value, int bank), 
	int dsize, char *response)
{
	int ngot;
	int np;
	int ip, it;
	unsigned int address, low[MRANGE], high[MRANGE], size[MRANGE];
	int increment[MRANGE];
	int nrange, nnew;
	unsigned int value; 
	char *name;
	char ebuffer[MBUFFER],buffer[MBUFFER],header[MBUFFER];
	int error;
	int done;
	int lc, nc;
	int bad;
	int index;
	int code;
	int wifi;
	//
	// if there's no card loaded, return error
	//
    if(CardCheckAndLoad(client)!=0)
    {
		if(_ResponseStyle!=ResponseStyleSimple)
		{
			ErrorPrint(CardNoneLoaded);
		}
    }
	else
	{
		//
		// prepare beginning of error message in case we need to use it
		//
		wifi=1;
		done=0;
		error=0;
		nrange=0;
		lc=0;
		nc=0;
		//
		// parse parameters and values
		//
		np=CommandParameterMany();
		for(ip=0; ip<np; ip++)
		{
			name=CommandParameterName(ip);
			index=ParameterSelectIndex(name,MemoryReadParameter,sizeof(MemoryReadParameter)/sizeof(MemoryReadParameter[0]));
			if(index>=0)
			{
				code=MemoryReadParameter[index].code;
				switch(code) 
				{
					case MemoryBank:
						ngot=ParseIntegerList(ip,name,&wifi,&MemoryReadParameter[index]);
						if(ngot<=0)
						{
							error++;
						}
						break;
					case MemorySize:
						ngot=ParseIntegerList(ip,name,&dsize,&MemoryReadParameter[index]);
						if(ngot<=0)
						{
							error++;
						}
						break;
					case MemoryAddress:
						nnew=ParseAddressList(ip,name,&low[nrange],&high[nrange],&increment[nrange],nrange,&MemoryReadParameter[index]);
						if(nnew<=0)
						{
							error++;
						}
						for(it=0; it<nnew; it++)
						{
							if(increment[nrange+it]==0)
							{
								increment[nrange+it]=dsize;
							}
							if(ABS(increment[nrange+it])==1 || ABS(increment[nrange+it])==2 || ABS(increment[nrange+it])==4)
							{
								size[nrange+it]=ABS(increment[nrange+it]);
							}
							else
							{
								size[nrange+it]=dsize;
							}
						}
						nrange+=nnew;
						break;
				}
			}
			else
			{
				ErrorPrint(ParseBadParameter,name);
				error++;
			}
		}

		if(error<=0 && nrange>0)
		{
			done=0;
			for(it=0; it<nrange; it++)
			{
				for(address=low[it]; address<=high[it]; address+=increment[it])
				{
					switch(size[it])
					{
						case 1:
							bad=(*read1)(address,&value,wifi);
							break;
						case 2:
							bad=(*read2)(address,&value,wifi);
							break;
						case 4:
							bad=(*read4)(address,&value,wifi);
							break;
					}
					if(bad)
					{
						if(_ResponseStyle!=ResponseStyleSimple)
						{
	// ###########need error message here						ErrorPrint(BadRead,address,increment[it]);
							UserPrint("Bad read from address %x of %d bytes.\n",address,size[it]);
						}
						else
						{
							nc=SformatOutput(&ebuffer[lc],MBUFFER-lc-1,"BAD,");
							if(nc>0)
							{
								lc+=nc;
							}
						}
					}
					else
					{
						if(_ResponseStyle!=ResponseStyleSimple)
						{
							switch(size[it])
							{
								case 1:
									SformatOutput(buffer,MBUFFER-1,"|%s|%04x|%02x|",response,address,value);
									break;
								case 2:
									SformatOutput(buffer,MBUFFER-1,"|%s|%04x|%04x|",response,address,value);
									break;
								case 4:
									SformatOutput(buffer,MBUFFER-1,"|%s|%04x|%08x|",response,address,value);
									break;
							}

							if(done==0)
							{
								SformatOutput(header,MBUFFER-1,"|%s|address|value|",response);
								ErrorPrint(NartDataHeader,header);
							}

							ErrorPrint(NartData,buffer);
						}
						else
						{
							nc=SformatOutput(&ebuffer[lc],MBUFFER-lc-1,"%x,",value);
							if(nc>0)
							{
								lc+=nc;
							}
						}
						done++;
					}
				}
			}
		}
		if(_ResponseStyle==ResponseStyleSimple)
		{
			ebuffer[MBUFFER-1]=0;
			ErrorPrint(NartOther,ebuffer);
		}
	}
	if(_ResponseStyle!=ResponseStyleSimple)
	{
		SendDone(client);
	}
}

//
// When the underlying function supports reading a single byte,
// override ReadByte() and use ReadByte1(), ReadByte2(), or ReadByte4() to read the appropriate size field.
//
static int (*ReadByte)(unsigned int address, unsigned char *value, int count, int bank); 

static int ReadByte1(unsigned int address, unsigned int *value, int bank)
{
	int bad;
	unsigned char val[1];

	bad=(*ReadByte)(address,val,1,bank); 
	*value = val[0];
	return bad;
}

static int ReadByte2(unsigned int address, unsigned int *value, int bank)
{
	int bad;
	unsigned char val[2];

	bad=(*ReadByte)(address,val,2,bank); 
	*value = val[0] | (val[1]<<8);
	return bad;
}

static int ReadByte4(unsigned int address, unsigned int *value, int bank)
{
	int bad;
	unsigned char val[4];

	bad=(*ReadByte)(address,&val[0],4,bank); 
	*value = val[0] | (val[1]<<8) | (val[2]<<16) | (val[3]<<24);
	return bad;
}

static int InternalEepromRead(unsigned int address, unsigned char *value, int many, int bank)
{
	return DeviceEepromRead(address,value,many);
}

void EepromReadCommand(int client)
{
	ReadByte=InternalEepromRead;
	InternalReadCommand(client, ReadByte1, ReadByte2, ReadByte4, 2, "er");
}

static int InternalOtpRead(unsigned int address, unsigned char *value, int many, int bank)
{
	return DeviceOtpRead(address,value,many,bank);
}

void OtpReadCommand(int client)
{
	ReadByte=InternalOtpRead;
	InternalReadCommand(client, ReadByte1, ReadByte2, ReadByte4, 2, "or");
}

static int InternalFlashRead(unsigned int address, unsigned char *value, int many, int bank)
{
	return DeviceFlashRead(address,value,many);
}

void FlashReadCommand(int client)
{
	ReadByte=InternalFlashRead;
	InternalReadCommand(client, ReadByte1, ReadByte2, ReadByte4, 1, "xr");
}


//
// When the underlying function supports reading a 4-byte word,
// override ReadWord() and use ReadWord1(), ReadWord2(), or ReadWord4() to read the appropriate size field.
//

static int (*ReadWord)(unsigned int address, unsigned int *value, int count, int bank); 

static int ReadWord4(unsigned int address, unsigned int *value, int bank)
{
	return ReadWord(address,value,1,bank);
}

static int ReadWord2(unsigned int address, unsigned int *value, int bank)
{
	int error;
	int shift;

	shift=address%2;
	error=ReadWord(address/2,value,1,bank);
	if(error==0)
	{
		*value>>=(shift*16);
		*value&=0xffff;
	}
	return error;
}

static int ReadWord1(unsigned int address, unsigned int *value, int bank)
{
	int error;
	int shift;

	shift=(address)%4;
	error=ReadWord((address)/4,value,1,bank);
	if(error==0)
	{
		*value>>=(shift*8);
		*value&=0xff;
	}
	return error;
}

static int InternalMemoryRead(unsigned int address, unsigned int *value, int many, int bank)
{
	return DeviceMemoryRead(address,value,many);
}

void MemoryReadCommand(int client)
{
	ReadWord=InternalMemoryRead;
	InternalReadCommand(client, ReadWord1, ReadWord2, ReadWord4, 4, "mr");
}


static int ValueFind(int done, int *vlow, int *vhigh, int *vincrement, int nvalue)
{
	int count;
	int more;
	int it;
	int value;

	count=0;
	while(count<=done)
	{
		for(it=0; it<nvalue; it++)
		{
			more=1+((vhigh[it]-vlow[it])/vincrement[it]);
			//
			// is it in this range?
			//
			if(count+more>done)
			{
				value=vlow[it]+(done-count)*vincrement[it];
				return value;
			}
			count+=more;
		}
	}
	return 0;
}

static void InternalWriteCommand(int client, 
	int (*Write1)(unsigned int address, unsigned int *value, int bank), 
	int (*Write2)(unsigned int address, unsigned int *value, int bank), 
	int (*Write4)(unsigned int address, unsigned int *value, int bank), 
	int dsize, char *response)
{
	int ngot;
	int np;
	int ip, it;
	unsigned int address, low[MRANGE], high[MRANGE], size[MRANGE];
	int increment[MRANGE];
	int nrange, nnew;
	unsigned int value, vlow[MRANGE], vhigh[MRANGE];
	int vincrement[MRANGE];
	int nvalue;
	char *name;
	char ebuffer[MBUFFER],buffer[MBUFFER],header[MBUFFER];
	int error;
	int done;
	int lc, nc;
	int bad;
	int index;
	int code;
	int wifi;
	//
	// if there's no card loaded, return error
	//
    if(CardCheckAndLoad(client)!=0)
    {
		if(_ResponseStyle!=ResponseStyleSimple)
		{
			ErrorPrint(CardNoneLoaded);
		}
    }
	else
	{
		//
		// prepare beginning of error message in case we need to use it
		//
		wifi=1;
		done=0;
		error=0;
		nrange=0;
		nvalue=0;
		lc=0;
		nc=0;
		//
		// parse parameters and values
		//
		np=CommandParameterMany();
		for(ip=0; ip<np; ip++)
		{
			name=CommandParameterName(ip);
			index=ParameterSelectIndex(name,MemoryWriteParameter,sizeof(MemoryWriteParameter)/sizeof(MemoryWriteParameter[0]));
			if(index>=0)
			{
				code=MemoryWriteParameter[index].code;
				switch(code) 
				{
					case MemoryBank:
						ngot=ParseIntegerList(ip,name,&wifi,&MemoryReadParameter[index]);
						if(ngot<=0)
						{
							error++;
						}
						break;
					case MemorySize:
						ngot=ParseIntegerList(ip,name,&dsize,&MemoryWriteParameter[index]);
						if(ngot<=0)
						{
							error++;
						}
						break;
					case MemoryAddress:
						nnew=ParseAddressList(ip,name,&low[nrange],&high[nrange],&increment[nrange],nrange,&MemoryWriteParameter[index]);
						if(nnew<=0)
						{
							error++;
						}
						for(it=0; it<nnew; it++)
						{
							if(increment[nrange+it]==0)
							{
								increment[nrange+it]=dsize;
							}
							if(ABS(increment[nrange+it])==1 || ABS(increment[nrange+it])==2 || ABS(increment[nrange+it])==4)
							{
								size[nrange+it]=ABS(increment[nrange+it]);
							}
							else
							{
								size[nrange+it]=dsize;
							}
						}
						nrange+=nnew;
						break;
					case MemoryValue:
						nnew=ParseAddressList(ip,name,&vlow[nvalue],&vhigh[nvalue],&vincrement[nvalue],nvalue,&MemoryWriteParameter[index]);
						if(nnew<=0)
						{
							error++;
						}
						for(it=0; it<nnew; it++)
						{
							if(vincrement[nvalue+it]==0)
							{
								vincrement[nvalue+it]=1;
							}
						}
						nvalue+=nnew;
						break;
				}
			}
			else
			{
				ErrorPrint(ParseBadParameter,name);
				error++;
			}
		}

		if(error<=0 && nrange>0)
		{
			done=0;
			for(it=0; it<nrange; it++)
			{
				for(address=low[it]; address<=high[it]; address+=increment[it])
				{
					value=ValueFind(done,vlow,vhigh,vincrement,nvalue);
					switch(size[it])
					{
						case 1:
							bad=(*Write1)(address,&value,wifi);
							break;
						case 2:
							bad=(*Write2)(address,&value,wifi);
							break;
						case 4:
							bad=(*Write4)(address,&value,wifi);
							break;
					}
					if(bad)
					{
						if(_ResponseStyle!=ResponseStyleSimple)
						{
							UserPrint("Bad write of %x to address %x of %d bytes.\n",value,address,size[it]);
	// ###########need error message here						ErrorPrint(BadWrite,address,increment[it]);
						}
						else
						{
							nc=SformatOutput(&ebuffer[lc],MBUFFER-lc-1,"BAD,");
							if(nc>0)
							{
								lc+=nc;
							}
						}
					}
					else
					{
						if(_ResponseStyle!=ResponseStyleSimple)
						{
							switch(size[it])
							{
								case 1:
									SformatOutput(buffer,MBUFFER-1,"|%s|%04x|%02x|",response,address,value);
									break;
								case 2:
									SformatOutput(buffer,MBUFFER-1,"|%s|%04x|%04x|",response,address,value);
									break;
								case 4:
									SformatOutput(buffer,MBUFFER-1,"|%s|%04x|%08x|",response,address,value);
									break;
							}

							if(done==0)
							{
								SformatOutput(header,MBUFFER-1,"|%s|address|value|",response);
								ErrorPrint(NartDataHeader,header);
							}

							ErrorPrint(NartData,buffer);
						}
						else
						{
							nc=SformatOutput(&ebuffer[lc],MBUFFER-lc-1,"%x,",value);
							if(nc>0)
							{
								lc+=nc;
							}
						}
					}
					done++;
				}
			}
		}
		if(_ResponseStyle==ResponseStyleSimple)
		{
			ebuffer[MBUFFER-1]=0;
			ErrorPrint(NartOther,ebuffer);
		}
	}
	if(_ResponseStyle!=ResponseStyleSimple)
	{
		SendDone(client);
	}
}

//
// When the underlying function supports writing a single byte,
// override WriteByte() and use WriteByte1(), WriteByte2(), or WriteByte4() to read the appropriate size field.
//
static int (*WriteByte)(unsigned int address, unsigned char *value, int count, int bank); 

static int WriteByte1(unsigned int address, unsigned int *value, int bank)
{
	int bad;
	unsigned char val[1];

	val[0]= *value;
	bad=(*WriteByte)(address,val,1,bank); 
	return bad;
}

static int WriteByte2(unsigned int address, unsigned int *value, int bank)
{
	int bad;
	unsigned char val[2];

	val[0]= (*value)&0xff;
	val[1]= ((*value)>>8)&0xff;
	bad=(*WriteByte)(address,val,2,bank); 
	return bad;
}

static int WriteByte4(unsigned int address, unsigned int *value, int bank)
{
	int bad;
	unsigned char val[4];

	val[0]= (*value)&0xff;
	val[1]= ((*value)>>8)&0xff;
	val[2]= ((*value)>>16)&0xff;
	val[3]= ((*value)>>24)&0xff;
	bad=(*WriteByte)(address,val,4,bank); 
	return bad;
}

static int InternalEepromWrite(unsigned int address, unsigned char *value, int many, int bank)
{
	return DeviceEepromWrite(address,value,many);
}

void EepromWriteCommand(int client)
{
	WriteByte=InternalEepromWrite;
	InternalWriteCommand(client, WriteByte1, WriteByte2, WriteByte4, 2, "ew");
}

static int InternalOtpWrite(unsigned int address, unsigned char *value, int many, int bank)
{
	return DeviceOtpWrite(address,value,many,bank);
}

void OtpWriteCommand(int client)
{
	WriteByte=InternalOtpWrite;
	InternalWriteCommand(client, WriteByte1, WriteByte2, WriteByte4, 2, "ow");
}

void OtpLoadCommand(int client)
{
    if (DeviceOtpLoad() == 0)
    {
   		SendDone(client);
    }
    else
    {
        SendError(client, "could not laod OTP");
    }
}

static int InternalFlashWrite(unsigned int address, unsigned char *value, int many, int bank)
{
	return DeviceFlashWrite(address,value,many);
}

void FlashWriteCommand(int client)
{
	WriteByte=InternalFlashWrite;
	InternalWriteCommand(client, WriteByte1, WriteByte2, WriteByte4, 1, "xw");
}


//
// When the underlying function supports writing a 4-byte word,
// override WriteWord() and ReadWord() and use WriteWord1(), WriteWord2(), or WriteWord4() to read the appropriate size field.
//

static int (*WriteWord)(unsigned int address, unsigned int *value, int count, int bank); 

static int WriteWord4(unsigned int address, unsigned int *value, int bank)
{
	return WriteWord(address,value,1,bank);
}

static int WriteWord2(unsigned int address, unsigned int *value, int bank)
{
	int error;
	int shift;
	unsigned int current;
	unsigned int mask[]={0x0000ffff,0xffff0000};
	unsigned int vwrite;

	shift=address%2;
	address/=2;
	error=ReadWord4(address,&current,bank);
	if(error==0)
	{
		vwrite=((*value<<(shift*16))&mask[shift])|(current&(~mask[shift]));
		error=WriteWord4(address,&vwrite,bank);
	}
	return error;
}

static int WriteWord1(unsigned int address, unsigned int *value, int bank)
{
	int error;
	int shift;
	unsigned int current;
	unsigned int mask[]={0x000000ff,0x0000ff00,0x00ff0000,0xff000000};
	unsigned int vwrite;

	shift=address%4;
	address/=4;
	error=ReadWord4(address,&current,bank);
	if(error==0)
	{
		vwrite=((*value<<(shift*8))&mask[shift])|(current&(~mask[shift]));
		error=WriteWord4(address,&vwrite,bank);
	}
	return error;
}

static int InternalMemoryWrite(unsigned int address, unsigned int *value, int many, int bank)
{
	return DeviceMemoryWrite(address,value,many);
}

void MemoryWriteCommand(int client)
{
	WriteWord=InternalMemoryWrite;
	ReadWord=InternalMemoryRead;
	InternalWriteCommand(client, WriteWord1, WriteWord2, WriteWord4, 4, "mw");
}

unsigned int MaskCreate(int low, int high)
{
	unsigned int mask;
	int ib;

	mask=0;
	if(low<=0)
	{
		low=0;
	}
	if(high>=31)
	{
		high=31;
	}
	for(ib=low; ib<=high; ib++)
	{
		mask|=(1<<ib);
	}
	return mask;
}

int MyFieldRead(unsigned int address, int low, int high, unsigned int *value)
{
	unsigned int mask;
	unsigned int reg;
	int error;

    error=DeviceRegisterRead(address,&reg);
	if(error==0)
	{
		mask=MaskCreate(low,high);
		*value=(reg&mask)>>low;
	}
	
	return error;
}

int MyFieldWrite(unsigned int address, int low, int high, unsigned int value)
{
	unsigned int mask;
	unsigned int reg;
	int error;

	error=DeviceRegisterRead(address,&reg);
	if(error==0)
	{
		mask=MaskCreate(low,high);
		reg &= ~(mask);							// clear bits
		reg |= ((value<<low)&mask);				// set new value
		error=DeviceRegisterWrite(address,reg);
	}
	return error;
}

static void DisplayTxCorrCoeff(unsigned int *coeff_array, unsigned int row, unsigned col)
{
	unsigned int i, j, lc=0, nc;
	char buffer[MBUFFER];
	int corr_idx[8]={1,1,23,23,45,45,67,67};  // hardcode based on ar9300_reset.c

	row=(row>8)?(8):(row);
	nc=SformatOutput(&buffer[lc],MBUFFER-lc-1,"TX_IQCAL_CORR_COEFF_");
	lc+=nc;
	for (i=0; i<row; i++)
		for (j=0; j<col; j++)
		{
			nc=SformatOutput(&buffer[lc],MBUFFER-lc-1,"|%2.2d_B%d",corr_idx[i],j);
			lc+=nc;
		}
	ErrorPrint(NartData,buffer);

	lc=0;
	nc=SformatOutput(&buffer[lc],MBUFFER-lc-1,"TX_IQCAL_CORR_COEFF_");
	lc+=nc;
	for (i=0; i<row; i++)
		for (j=0; j<col; j++)
		{
			nc=SformatOutput(&buffer[lc],MBUFFER-lc-1,"|%d",*(coeff_array+(i*j)));
			lc+=nc;
		}
	ErrorPrint(NartData,buffer);
}



void CoeffDisplay(int client)
{
	int np, ip;
	char *cName, *name;
	unsigned int *coeff_array, row, col;

	if(CardCheckAndLoad(client)!=0)
	{
		ErrorPrint(CardNoneLoaded);
    }
	else
	{
        np=CommandParameterMany();
		for(ip=0; ip<np; ip++)
		{
			name=CommandParameterName(ip);
			if(Smatch(name,"coeff"))
			{	
				cName=CommandParameterValue(ip, 0);  //
				if (Smatch(cName,"TXIQcal"))
				{
					if (Device_get_corr_coeff(COEFF_TX_TYPE, &coeff_array, &row, &col)>=0)
					  DisplayTxCorrCoeff(coeff_array,row,col);
				} 
				else if (Smatch(cName,"RXIQcal"))
				{
					Device_get_corr_coeff(COEFF_RX_TYPE, &coeff_array, &row, &col);
				}

			}
		}

	}

	// tell the client we are done
    SendDone(client);
}

