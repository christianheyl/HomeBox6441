


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
#include "Sticky.h"
#include "Device.h"

#define MAX_LINKLIST_IDX   32

static struct _Sticky *_Shead[MAX_LINKLIST_IDX], *_Stail[MAX_LINKLIST_IDX], *_Scurrent[MAX_LINKLIST_IDX];

#ifdef _WINDOWS
#include "windows.h"
static     char szBuf[1536];
void DbgPrint(char * fmt,...)
{
    va_list marker;

    va_start(marker, fmt);
    vsprintf(szBuf, fmt, marker);
    va_end(marker);
    OutputDebugString(szBuf);
}
#else
void DbgPrint(char * fmt,...)
{
	return;
}
#endif //_WINDOWS
//
// execute all of the sticky register and field writes
//
// idx = 0 : Execute after reset, same as single link-list for compatibility
//       1 : Execute after reset and delete link-list
//       2 : Execute at midle reset
//       3 : Execute at midle reset and delete link-list
FIELDDLLSPEC int StickyExecute(int idx)
{
	struct _Sticky *s, *snext;
	int count;

    if (DeviceIsEmbeddedArt()) return 0;

	count=0;
	for(s=_Shead[idx]; s!=0; s=s->next)
	{
		UserPrint("sticky %08x[%d,%d] <- %08x\n",s->address,s->high,s->low,s->value[0]);
		{
			unsigned int rVal;
		    DeviceFieldRead(s->address,s->low,s->high,&rVal);
			if (idx == ARD_LINKLIST_IDX) {
				DbgPrint("sticky %08x[%2d,%2d] Read before Write %08x\n",s->address,s->high,s->low,rVal);
				DbgPrint("sticky %08x[%2d,%2d] Writing --------- %08x\n",s->address,s->high,s->low,s->value[0]);
			}
		}
		DeviceFieldWrite(s->address,s->low,s->high,s->value[0]);

		{
		  unsigned int rVal;
		  DeviceFieldRead(s->address,s->low,s->high,&rVal);
	 	  UserPrint("sticky %08x[%d,%d] -> %08x\n",s->address,s->high,s->low,s->value[0]);
		  if (idx == ARD_LINKLIST_IDX)
			DbgPrint("sticky %08x[%2d,%2d] Read after  Write %08x\n",s->address,s->high,s->low,rVal);
		}
		count++;
	}

	// Delete link-list
    if ((idx == ARD_LINKLIST_IDX) || (idx == MRD_LINKLIST_IDX))
	{
		for(s=_Shead[idx]; s!=0; s=snext)
		{
			snext=s->next;
			free(s);
		}
		_Shead[idx]=0;
		_Stail[idx]=0;
		_Scurrent[idx]=0;
	}

	return count;
}

//
// execute all of the sticky register and field writes
// the sticky list will be sent to DUT
FIELDDLLSPEC int StickyExecuteDut(int idx)
{
	struct _Sticky *s, *snext;
	int count;

    if (!DeviceIsEmbeddedArt()) return 0;

    count=0;
    for(s=_Shead[idx]; s!=0; s=s->next)
	{
        if (s->flag & STICKY_FLAG_SENT_MASK)
        {
            // this entry has been sent to DUT, go to next
            continue;
        }
		DeviceStickyWrite(idx,s->address,s->low,s->high,s->value,s->numVal,
							((s->flag & STICKY_FLAG_PREPOST_MASK) >> STICKY_FLAG_PREPOST_SHIFT));
        s->flag = s->flag | STICKY_FLAG_SENT_MASK;    // mark as sent
		count++;
	}
    DeviceStickyWrite(idx,0,0,0,0,0,0);    //this is the last
    DeviceStickyClear(idx,0,0,0); // send the sticky clear list if any

	// Delete link-list
    if ((idx == ARD_LINKLIST_IDX) || (idx == MRD_LINKLIST_IDX))
	{
		for(s=_Shead[idx]; s!=0; s=snext)
		{
			snext=s->next;
			free(s);
		}
		_Shead[idx]=0;
		_Stail[idx]=0;
		_Scurrent[idx]=0;
	}

	return count;
}
//
// return the values of the first sticky thing on the list
// return value is 0 if successful, non zero if not
//
FIELDDLLSPEC int StickyHead(int idx, unsigned int *address, int *low, int *high, unsigned int *value, int *numVal)
{	
    int i;

	_Scurrent[idx]=_Shead[idx];
	if(_Scurrent[idx]!=0)
	{
		*address=_Scurrent[idx]->address;
		*low=_Scurrent[idx]->low;
		*high=_Scurrent[idx]->high;
        *numVal = _Scurrent[idx]->numVal;
        for (i = 0; i < _Scurrent[idx]->numVal; ++i)
        {
            value[i]=_Scurrent[idx]->value[i];
        }
		return 0;
	}
	return -1;
}

//
// return the values of the next sticky thing on the list
// return value is 0 if successful, non zero if not
//
FIELDDLLSPEC int StickyNext(int idx, unsigned int *address, int *low, int *high, unsigned int *value, int *numVal)
{
    int i;

	if(_Scurrent[idx]!=0)
	{
		_Scurrent[idx]=_Scurrent[idx]->next;
	}
	if(_Scurrent[idx]!=0)
	{
		*address=_Scurrent[idx]->address;
		*low=_Scurrent[idx]->low;
		*high=_Scurrent[idx]->high;
        *numVal = _Scurrent[idx]->numVal;
        for (i = 0; i < _Scurrent[idx]->numVal; ++i)
        {
            value[i]=_Scurrent[idx]->value[i];
        }
		return 0;
	}
	return -1;
}


//
// clear the list of sticky registers
//
FIELDDLLSPEC int StickyClear(int idx, int toDut)
{
	struct _Sticky *s, *snext;
	int count=0;

	for(s=_Shead[idx]; s!=0; s=snext)
	{
		snext=s->next;
		free(s);
		count++;
	}
	_Shead[idx]=0;
	_Stail[idx]=0;
	_Scurrent[idx]=0;
    if (toDut)
    {
        DeviceStickyClear(idx, 0xffffffff, 0, 0);
    }
	return count;
}

//
// find a sticky register on the list
//
FIELDDLLSPEC struct _Sticky *StickyInternalFind(int idx, unsigned int address, int low, int high)
{
	struct _Sticky *s;

	for(s=_Stail[idx]; s!=0; s=s->prev)
	{
		if(s->address==address && s->low==low && s->high==high)
		{
			return s;
		}
	}
	return 0;
}

//
// clear one register from the list
//
FIELDDLLSPEC int StickyInternalClear(int idx, unsigned int address, int low, int high)
{
	struct _Sticky *s;

	s=StickyInternalFind(idx, address,low,high);
	if(s!=0)
	{
        if (DeviceIsEmbeddedArt())
        {
            DeviceStickyClear(idx,s->address, s->low, s->high);
        }
		//
		// adjust pointers to take this entry off the list
		//
		if(s==_Shead[idx])
		{
			_Shead[idx]=s->next;
		}
		if(s==_Stail[idx])
		{
			_Stail[idx]=s->prev;
		}
		if(s==_Scurrent[idx])
		{
			_Scurrent[idx]=s->next;
		}
		if(s->prev!=0)
		{
			s->prev->next=s->next;
		}
		if(s->next!=0)
		{
			s->next->prev=s->prev;
		}
		free(s);
		return 0;
	}
    else if (DeviceIsEmbeddedArt())
    {
        // in case want to clear the sticky write that isn't in the list but already sent to DUT
        DeviceStickyClear(idx, address,low, high);
		return 0;
    }
	return -1;
}

//
// add a sticky register and value to the list
//
FIELDDLLSPEC int StickyInternalAdd(int idx, unsigned int address, int low, int high, unsigned int value, int prepost)
{
	struct _Sticky *s;

    // If there is a duplicate entry, return
	s=StickyInternalFind(idx, address,low,high);
    if (s != 0)
    {
        if ((s->value[0] == value) && (s->numVal == 1) &&
		    (((s->flag & STICKY_FLAG_PREPOST_MASK) >> STICKY_FLAG_PREPOST_SHIFT) == prepost))
        {
            return 0;
        }
    }

#ifdef OVERWRITESTICKY
	s=StickyInternalFind(idx, address,low,high);
	if(s==0)
#endif
	{
		//
		// make a new structure
		//
		s=(struct _Sticky *)malloc(sizeof(struct _Sticky));
		//
		// tack it on the end of the list
		//
		if(s!=0)
		{
			if(_Shead[idx]==0)
			{
				s->next=0;
				s->prev=0;
				_Shead[idx]=s;
				_Stail[idx]=s;
			}
			else
			{
				s->next=0;
				s->prev=_Stail[idx];
				if(s->prev!=0)
				{
					s->prev->next=s;
				}
				_Stail[idx]=s;
			}
		}
		else
		{
			UserPrint("cant save sticky register %x:%d:%d -< %x\n",address,high,low,value);
			return -1;
		}
	}
	if(s!=0)
	{
		s->address=address;
		s->low=low;
		s->high=high;
		s->value[0]=value;
		s->flag = prepost == 1 ? STICKY_FLAG_DEFAULT : ( STICKY_FLAG_DEFAULT & ~STICKY_FLAG_PREPOST_MASK);
        s->numVal=1;
	}
	return 0;
}

FIELDDLLSPEC int StickyInternalAddArray(int idx, unsigned int address, int low, int high, unsigned int *value, int numVal, int prepost)
{
	struct _Sticky *s;
    int i;

    // If there is a duplicate entry, return
	s=StickyInternalFind(idx, address,low,high);
    if (s != 0)
    {
        if ((s->numVal == numVal) &&
		    (((s->flag & STICKY_FLAG_PREPOST_MASK) >> STICKY_FLAG_PREPOST_SHIFT) == prepost))
        {
            if (memcmp(s->value, value, numVal) == 0)
            {
                return 0;
            }
        }
    }

#ifdef OVERWRITESTICKY
	s=StickyInternalFind(idx,address,low,high);
	if(s==0)
#endif
	{
		//
		// make a new structure
		//
		s=(struct _Sticky *)malloc(sizeof(struct _Sticky));
		//
		// tack it on the end of the list
		//
		if(s!=0)
		{
			if(_Shead[idx]==0)
			{
				s->next=0;
				s->prev=0;
				_Shead[idx]=s;
				_Stail[idx]=s;
			}
			else
			{
				s->next=0;
			    s->prev=_Stail[idx];
				if(s->prev!=0)
				{
					s->prev->next=s;
				}
				_Stail[idx]=s;
			}
		}
		else
		{
			UserPrint("cant save sticky register %x:%d:%d -< %x\n",address,high,low,value);
			return -1;
		}
	}
	if(s!=0)
	{
		s->address=address;
		s->low=low;
		s->high=high;
		s->flag = prepost == 1 ? STICKY_FLAG_DEFAULT : ( STICKY_FLAG_DEFAULT & ~STICKY_FLAG_PREPOST_MASK);
        s->numVal=numVal;
        for (i = 0; i < numVal; ++i)
        {
         	s->value[i]=value[i];
        }
	}
	return 0;
}

//
// add a sticky register and value to the list
//
FIELDDLLSPEC int StickyInternalChange(int idx, unsigned int address, int low, int high, unsigned int value, int prepost)
{
	struct _Sticky *s;

	s=StickyInternalFind(idx, address,low,high);
	if(s==0)
	{
		//
		// make a new structure
		//
		s=(struct _Sticky *)malloc(sizeof(struct _Sticky));
		//
		// tack it on the end of the list
		//
		if(s!=0)
		{
			if(_Shead[idx]==0)
			{
				s->next=0;
				s->prev=0;
				_Shead[idx]=s;
				_Stail[idx]=s;
			}
			else
			{
				s->next=0;
				s->prev=_Stail[idx];
				if(s->prev!=0)
				{
					s->prev->next=s;
				}
				_Stail[idx]=s;
			}
		}
		else
		{
			UserPrint("cant save sticky register %x:%d:%d -< %x\n",address,high,low,value);
			return -1;
		}
	}
	if(s!=0)
	{
		s->address=address;
		s->low=low;
		s->high=high;
		s->value[0]=value;
		s->flag = prepost == 1 ? STICKY_FLAG_DEFAULT : ( STICKY_FLAG_DEFAULT & ~STICKY_FLAG_PREPOST_MASK);
        s->numVal=1;
	}
	return 0;
}

FIELDDLLSPEC int StickyInternalChangeArray(int idx, unsigned int address, int low, int high, unsigned int *value, int numVal, int prepost)
{
	struct _Sticky *s;
    int i;

	s=StickyInternalFind(idx, address,low,high);
	if(s==0)
	{
		//
		// make a new structure
		//
		s=(struct _Sticky *)malloc(sizeof(struct _Sticky));
		//
		// tack it on the end of the list
		//
		if(s!=0)
		{
			if(_Shead[idx]==0)
			{
				s->next=0;
				s->prev=0;
				_Shead[idx]=s;
				_Stail[idx]=s;
			}
			else
			{
				s->next=0;
				s->prev=_Stail[idx];
				if(s->prev!=0)
				{
					s->prev->next=s;
				}
				_Stail[idx]=s;
			}
		}
		else
		{
			UserPrint("cant save sticky register %x:%d:%d -< %x\n",address,high,low,value);
			return -1;
		}
	}
	if(s!=0)
	{
		s->address=address;
		s->low=low;
		s->high=high;
		s->flag = prepost == 1 ? STICKY_FLAG_DEFAULT : ( STICKY_FLAG_DEFAULT & ~STICKY_FLAG_PREPOST_MASK);
        s->numVal=numVal;
        for (i = 0; i < numVal; ++i)
        {
         	s->value[i]=value[i];
        }
	}
	return 0;
}

//
// find a sticky register on the list
//
FIELDDLLSPEC struct _Sticky *StickyRegisterFind(int idx, unsigned int address)
{
	return StickyInternalFind(idx, address,0,31);
}

//
// clear one register from the list
//
FIELDDLLSPEC int StickyRegisterClear(int idx, unsigned int address)
{
	return StickyInternalClear(idx, address,0,31);
}

//
// add a sticky register and value to the list
//
FIELDDLLSPEC int StickyRegisterAdd(int idx, unsigned int address, unsigned int value)
{
	return StickyInternalAdd(idx,address,0,31,value, 1);
}

//
// find a sticky register on the list
//
FIELDDLLSPEC struct _Sticky *StickyFieldFind(int idx,char *name)
{
	unsigned int address;
	int low, high;

    if(FieldFind(name, &address, &low, &high))
	{
		return StickyInternalFind(idx, address,low,high);
	}
	return 0;
}

//
// clear one field from the list
//
FIELDDLLSPEC int StickyFieldClear(int idx, char *name)
{
	unsigned int address;
	int low, high;

    if(FieldFind(name, &address, &low, &high))
	{
		return StickyInternalClear(idx, address,low,high);
	}
	return -1;
}

//
// add one sticky field and value to the list
//
FIELDDLLSPEC int StickyFieldAdd(int idx,char *name, unsigned int value)
{
	unsigned int address;
	int low, high;

    if(FieldFind(name, &address, &low, &high))
	{
		return StickyInternalAdd(idx,address,low,high,value, 1);
	}
	return -1;
}

FIELDDLLSPEC int StickyListToEeprom(int idx)
{
	struct _Sticky *s;
	int count;

    if (!DeviceIsEmbeddedArt()) return 0;

    count=0;
    for(s=_Shead[idx]; s!=0; s=s->next)
	{
		DeviceConfigAddrSet(s->address, s->low, s->high, s->value, s->numVal,
                            ((s->flag & STICKY_FLAG_PREPOST_MASK) >> STICKY_FLAG_PREPOST_SHIFT));
		count++;
	}
	return count;
}

