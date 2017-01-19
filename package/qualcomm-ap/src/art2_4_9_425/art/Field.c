


#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

#include "smatch.h"
#include "UserPrint.h"
#include "Field.h"
#include "ParameterSelect.h"
#include "Card.h"
#include "Device.h"
#include "NartRegister.h"

#define MBUFFER 1024

static struct _Field *_F;
static int _FieldMany=0;

#define MHASH 500
#define MENTRY 10

struct _FieldHash
{
    int max;
    int many;
    struct _Field **fptr;
};

static int FHgood=0;
static struct _FieldHash FH[MHASH];


FIELDDLLSPEC int FieldHashKey(char *name)
{
    int it;
    int key;

    key= 0;
    for(it=0; it<12 && *name!=0; it++, name++)
    {
        key+= (toupper(*name)-'A');
    }
    if(key<0)
    {
        key=0;
    }
    if(key>=MHASH)
    {
        key=MHASH-1;
    }

    return key;
}


static void FieldHashDestroy()
{
    int it;

    for(it=0; it<MHASH; it++)
    {
        if(FH[it].fptr!=0)
        {
            free(FH[it].fptr);
            FH[it].fptr=0;
            FH[it].max=0;
            FH[it].many=0;
        }
    }
    FHgood=0;
}


FIELDDLLSPEC void FieldHashCreate()
{
    struct _Field **fpnew;
    int it, ip;
    int key;

    for(it=0; it<_FieldMany; it++)
    {
        //
        // compute hash key
        //
        key=FieldHashKey(_F[it].fieldName);
        //
        // see if we need to create more room
        //
        if(FH[key].many>=FH[key].max)
        {
            fpnew=(struct _Field **)malloc((FH[key].max+MENTRY)*sizeof(struct _Field *));
            if(fpnew!=0)
            {
                if(FH[key].many>0 && FH[key].fptr!=0)
                {
                    for(ip=0; ip<FH[key].many; ip++)
                    {
                        fpnew[ip]=FH[key].fptr[ip];
                    }
                    free(FH[key].fptr);
                }
                FH[key].fptr=fpnew;
                FH[key].max+=MENTRY;
            }
            else
            {
                FieldHashDestroy();
                return;
            }
        }
        //
        // now we should be able to stick the new one in the structure
        //
        FH[key].fptr[FH[key].many]= &_F[it];
        FH[key].many++;
    }
    FHgood=1;
}


FIELDDLLSPEC void FieldSelect(struct _Field *field, int nfield)
{
    int it;

    if(_F!=field || _FieldMany!=nfield)
    {
        FieldHashDestroy();

	    _F=field;
	    _FieldMany=nfield;

        FieldHashCreate();

        for(it=0; it<MHASH; it++)
        {
            if(FH[it].many>0)
            {
            }
        }
    }
}


//
// Looks up the name of the field specified by address[high,low].
// If none, returns -1.
// Assumes the list is ordered from lowest to highest address.
//
FIELDDLLSPEC int FieldFindByAddress(unsigned int address, int low, int high, char **registerName, char **fieldName)
{	
    int min, max, mid, check;

    min=0;
    max=_FieldMany-1;


    while(min<=max)
    {
        mid=(min+max)/2;

        if(address>_F[mid].address)
        {
            min=mid+1;
        }
        else if(address<_F[mid].address)
        {
            max=mid-1;
        }
        else
        {
            //
            // we found something with the correct address
            // back up until we find the first one with this address
            //
            for(check=mid; check>=0; check--)
            {
                if(address!=_F[check].address)
                {
                    break;
                }
                if((low==_F[check].low && high==_F[check].high) || (low==_F[check].high && high==_F[check].low))
                {
                    *registerName=_F[check].registerName;
                    *fieldName=_F[check].fieldName;
                    return 0;
                }
            }
            //
            // and then go forward until we find the correct [high,low]
            //
            for(check=mid+1; check<_FieldMany; check++)
            {
                if(address!=_F[check].address)
                {
                    break;
                }
                if((low==_F[check].low && high==_F[check].high) || (low==_F[check].high && high==_F[check].low))
                {
                    *registerName=_F[check].registerName;
                    *fieldName=_F[check].fieldName;
                    return 0;
                }
            }
            break;
        }
    }
    return -1;
}

//
// Looks up the register name, field name, low and high by address.
// If none, returns -1.
// Assumes the list is ordered from lowest to highest address.
// index is 0.. upto return -1
//
FIELDDLLSPEC int FieldFindByAddressOnly(unsigned int address, int index, int *low, int *high, char **registerName, char **fieldName)
{	
    int min, max, mid, check;

    min=0;
    max=_FieldMany-1;


    while(min<=max)
    {
        mid=(min+max)/2;

        if(address>_F[mid].address)
        {
            min=mid+1;
        }
        else if(address<_F[mid].address)
        {
            max=mid-1;
        }
        else
        {
            //
            // we found something with the correct address
            // back up until we find the first one with this address
            //
            for(check=mid; check>=0; check--)
            {
                if(address!=_F[check].address)
                {
                    break;
                }
				// find lowest index for address
                while (check && address==_F[check].address)
					check--;
				check++;
				if (((check+index)<=max) && (address==_F[check+index].address))
                {
                    *registerName=_F[check+index].registerName;
                    *fieldName=_F[check+index].fieldName;
					*low = _F[check+index].low;
					*high = _F[check+index].high;
                    return 0;
                }
				return -1;
            }
            //
            // and then go forward until we find the correct [high,low]
            //
            for(check=mid+1; check<_FieldMany; check++)
            {
                if(address!=_F[check].address)
                {
                    break;
                }
				// find lowest index for address
                while (check && address==_F[check].address)
					check--;
				check++;
				if (((check+index)<=max) && (address==_F[check+index].address))
                {
                    *registerName=_F[check+index].registerName;
                    *fieldName=_F[check+index].fieldName;
					*low = _F[check+index].low;
					*high = _F[check+index].high;
                    return 0;
                }
				return -1;
            }
            break;
        }
    }
    return -1;
}



static int FieldFindHashEither(char *name, unsigned int *address, int *low, int *high)
{
	int it;
    int key;
 
    key=FieldHashKey(name);
    for(it=0; it<FH[key].many; it++)
    {
	    if(Smatch(name,FH[key].fptr[it]->fieldName) || (Smatch(name,FH[key].fptr[it]->registerName) && Smatch("",FH[key].fptr[it]->fieldName)))
		{
			*address=FH[key].fptr[it]->address;
			//
			// this is the correct order
			//
			if(FH[key].fptr[it]->low<=FH[key].fptr[it]->high)
			{
			    *low=FH[key].fptr[it]->low;
			    *high=FH[key].fptr[it]->high;
			}
			//
			// but some files are reversed
			//
			else
			{
			    *low=FH[key].fptr[it]->high;
			    *high=FH[key].fptr[it]->low;
			}
			return 1;
		}
    }
    return 0;
}


static int FieldFindHashBoth(char *rname, char *fname, unsigned int *address, int *low, int *high)
{
	int it;
    int key;
 
    key=FieldHashKey(fname);
    for(it=0; it<FH[key].many; it++)
    {
		if(Smatch(rname,FH[key].fptr[it]->registerName) && Smatch(fname,FH[key].fptr[it]->fieldName))
		{
			*address=FH[key].fptr[it]->address;
			//
			// this is the correct order
			//
			if(FH[key].fptr[it]->low<=FH[key].fptr[it]->high)
			{
			    *low=FH[key].fptr[it]->low;
			    *high=FH[key].fptr[it]->high;
			}
			//
			// but some files are reversed
			//
			else
			{
			    *low=FH[key].fptr[it]->high;
			    *high=FH[key].fptr[it]->low;
			}
			return 1;
		}
    }
    return 0;
}


FIELDDLLSPEC int FieldFind(char *name, unsigned int *address, int *low, int *high)
{
	int it;
	char *dot;
	char registerName[MBUFFER],fieldName[MBUFFER];
    //
	// did the user supply "register.field" or just "field" or just "register"
	//
	dot=strchr(name,'.');
	//
	// register.field
	//
	if(dot!=0)
	{
		//
		// Split the user provided input into register name and field name
		//
		strncpy(registerName,name,dot-name);
		registerName[dot-name]=0;
		strcpy(fieldName,dot+1);
        //remove any trailing blank
        it = strlen(fieldName)-1;
        while(fieldName[it] == ' ' || fieldName[it] == '\t')
        {
            it--;
        }
        fieldName[it+1] = 0;

        if(FHgood)
        {
            return FieldFindHashBoth(registerName,fieldName,address,low,high);
        }

        //
		// Search for match of both names.
		//
	    for(it=0; it<_FieldMany; it++)
		{
		    if(Smatch(registerName,_F[it].registerName) && Smatch(fieldName,_F[it].fieldName))
			{
			    *address=_F[it].address;
				//
				// this is the correct order
				//
				if(_F[it].low<=_F[it].high)
				{
			        *low=_F[it].low;
			        *high=_F[it].high;
				}
				//
				// but some files are reversed
				//
				else
				{
			        *low=_F[it].high;
			        *high=_F[it].low;
				}
			    return 1;
			}
		}
	}
	else
	{
		//
		// Must match either a register name with a blank field name or
		// a field name with any register name.
		//
		// "name" and ""
		//    or
		// "anything" and "name"
		//
        if(FHgood)
        {
            return FieldFindHashEither(name,address,low,high);
        }

	    for(it=0; it<_FieldMany; it++)
		{
			if(Smatch(name,_F[it].fieldName) || (Smatch(name,_F[it].registerName) && Smatch("",_F[it].fieldName)))
			{
			    *address=_F[it].address;
				//
				// this is the correct order
				//
				if(_F[it].low<=_F[it].high)
				{
			        *low=_F[it].low;
			        *high=_F[it].high;
				}
				//
				// but some files are reversed
				//
				else
				{
			        *low=_F[it].high;
			        *high=_F[it].low;
				}
				return 1;
			}
		}
	}

	*address=0;
	*low=0;
	*high=0;

	return 0;
}


FIELDDLLSPEC int FieldWrite(char *name, unsigned int value)
{
	int low, high;
	unsigned int address;

    if(FieldFind(name, &address, &low, &high))
	{
	    DeviceFieldWrite(address,low,high,value);
		return 0;
	}
	else
	{
		UserPrint("FieldWrite - cant find field %s\n",name);
	    return -1;
	}
}

FIELDDLLSPEC int FieldWriteNoMask(char *name, unsigned int value)
{
	int low, high;
	unsigned int address;

    if(FieldFind(name, &address, &low, &high))
	{
	    DeviceFieldWrite(address,0,31,value);
		return 0;
	}
	else
	{
		UserPrint("FieldWriteNoMask - cant find field %s\n",name);
	    return -1;
	}
}

FIELDDLLSPEC int FieldRead(char *name, unsigned int *value)
{
	int low, high;
	unsigned int address;

    if(FieldFind(name, &address, &low, &high))
	{
	    DeviceFieldRead(address,low,high,value);
		return 0;
	}
	else
	{
		*value=0xbadbad;
		UserPrint("FieldRead - cant find field %s\n",name);
	    return -1;
	}
}

FIELDDLLSPEC int FieldReadNoMask(char *name, unsigned int *value)
{
	int low, high;
	unsigned int address;

    if(FieldFind(name, &address, &low, &high))
	{
	    DeviceFieldRead(address,0,31,value);
		return 0;
	}
	else
	{
		*value=0xbadbad;
		UserPrint("FieldReadNoMask - cant find field %s\n",name);
	    return -1;
	}
}

static int PatternMatch(char *pattern, char *name)
{
   char *p;
   char *n;

   p=pattern;
   n=name;

   while(*p!=0 && *n!=0)
   {
	   //
	   // normal character, must match
	   //
	   if(*p!='*')
	   {
		   if(tolower(*p)!=tolower(*n))
		   {
			   return 0;
		   }
		   p++;
		   n++;
	   }
	   //
	   // * matches anything, so move ahead to next character and then search for it
	   //
	   else
	   {
		    p++;
		    if(*p==0)
		    {
				//
				// * on the end, everything matches
				//
			    return 1;
		    }
			//
			// look for an occurence of the next character in the pattern
			// will look at every occurence of *p in name until one matches
			//
		    for( ; *n!=0; n++)
		    {
			    if(tolower(*p)==tolower(*n))
			    {
					//
					// see if the substrings match
					//
				    if(PatternMatch(p,n))
				    {
					    return 1;
				    }
			    }
		    }
			//
			// got to the end of name and no match
			//
            return 0;
	    }
	}

    if(*n==0)
    {
		//
		// ignore multiple * on the end of the pattern
		//
		while(*p=='*')
		{
			p++;
		}
		if(*p==0)
		{
			return 1;
		}
		else
		{
			return 0;
		}
    }
    else
    {
        return 0;
    }
}


FIELDDLLSPEC int FieldList(char *pattern, void (*print)(char *name, unsigned int address, int low, int high))
{
	char name[MBUFFER];
	int count;
	int it;

	count=0;
	if(print!=0)
	{
		for(it=0; it<_FieldMany; it++)
		{
			SformatOutput(name,MBUFFER-1,"%s.%s",_F[it].registerName,_F[it].fieldName);
			name[MBUFFER-1]=0;
			if(PatternMatch(pattern,name))
			{
				count++;
				(*print)(name,_F[it].address,_F[it].low,_F[it].high);
			}
		}
	}
	return count;
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

FIELDDLLSPEC int FieldGet(char *name, unsigned int *value, unsigned int reg)
{
	int low, high;
	unsigned int address;

    if(FieldFind(name, &address, &low, &high))
	{
	    *value = (reg & MaskCreate(low, high)) >> low;
		return 0;
	}
	else
	{
		*value=0xbadbad;
		UserPrint("FieldGet - cant find field %s\n",name);
	    return -1;
	}
}

FIELDDLLSPEC int FieldSet(char *name, unsigned int value, unsigned int *reg)
{
	int low, high;
	unsigned int address, mask;

    if(FieldFind(name, &address, &low, &high))
	{
		mask=MaskCreate(low,high);
		*reg &= ~(mask);							// clear bits
		*reg |= ((value << low) & mask);			// set new value
		return 0;
	}
	else
	{
		UserPrint("FieldSet - cant find field %s\n",name);
	    return -1;
	}
}
