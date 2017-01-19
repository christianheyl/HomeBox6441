//
// File: ConfigDiff.c
//

#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

#include "wlantype.h"
#include "dk_cmds.h"
#include "dk_common.h"
#include "dkCmd_if.h"
#include "DevConfigDiff.h"

#include "smatch.h"
#include "UserPrint.h"
#include "ParameterSelect.h"
#include "Card.h"

#define CONFIG_DIFF_MAX_APART       8

static CONFIG_DIFF *_CdHead, *_CdTail, *_CdCurrent;
static A_UINT8 ConfigDiffBuffer[CONFIG_DIFF_MAX_BUFFER];

static CONFIG_DIFF *_CalInfoHead = NULL;
static CONFIG_DIFF *_CalInfoTail = NULL;
static CONFIG_DIFF *_CalInfoCurrent = NULL;
static A_UINT8 CalInfoBuffer[CONFIG_DIFF_MAX_BUFFER];


//*****************************************************************************
// Generic Functions
//*****************************************************************************
//
// clear the list of diff config
//
int GenClearAll(CONFIG_DIFF **Head, CONFIG_DIFF **Tail, CONFIG_DIFF **Current)
{
	CONFIG_DIFF *s, *snext;
	int count=0;

	for(s=*Head; s!=0; s=snext)
	{
		snext=s->next;
		free(s);
		count++;
	}
	*Head=0;
	*Tail=0;
	*Current=0;
	return count;
}

//
// find a diff config entry on the list
//
CONFIG_DIFF *GenFind(CONFIG_DIFF *Tail, A_UINT16 offset)
{
	CONFIG_DIFF *s;

	for(s=Tail; s!=0; s=s->prev)
	{
		if(s->offset==offset)
		{
			return s;
		}
	}
	return 0;
}

//
// find a diff config entry on the list
//
CONFIG_DIFF *GenFindWithin(CONFIG_DIFF *Tail, A_UINT16 offset, A_UINT8 size, A_UINT16 *newOffset, A_UINT8 *newSize)
{
	CONFIG_DIFF *s = NULL;

	for(s=Tail; s!=0; s=s->prev)
	{
		if((offset >= s->offset) && (offset+size <= s->offset+s->size))
		{
            *newOffset = s->offset;
            *newSize = s->size;
			break;
		}
        else if ((offset < s->offset) && (offset+size >= s->offset+s->size))
        {
            *newOffset = offset;
            *newSize = size;
            break;
        }
        else if ((s->offset <  offset) && (s->offset+s->size+CONFIG_DIFF_MIN_APART > offset))
        {
            *newOffset = s->offset;
            *newSize = offset + size - s->offset;
            break;
        }
        else if ((offset < s->offset) && (offset+size+CONFIG_DIFF_MIN_APART > s->offset))
        {
            *newOffset = offset;
            *newSize = s->offset + s->size - offset;
            break;
        }
 	}
    if (s && (*newSize > CONFIG_DIFF_MAX_BUFFER))
    {
        // exceed the max length, cannot add in
        s = NULL;
    }
	return s;
}

//
// clear one entry from the list
//
int GenClearEntry(CONFIG_DIFF **Head, CONFIG_DIFF **Tail, CONFIG_DIFF **Current, CONFIG_DIFF *s)
{
	if(s!=0)
	{
		//
		// adjust pointers to take this entry off the list
		//
		if(s==*Head)
		{
			*Head=s->next;
		}
		if(s==*Tail)
		{
			*Tail=s->prev;
		}
		if(s==*Current)
		{
			*Current=s->next;
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
	return -1;
}

//
// clear one entry from the list
//
int GenClear(CONFIG_DIFF **Head, CONFIG_DIFF **Tail, CONFIG_DIFF **Current, A_UINT16 offset)
{
	CONFIG_DIFF *s;

	s=GenFind(*Tail, offset);
	if(s!=0)
	{
        return (GenClearEntry(Head, Tail, Current, s));
	}
	return -1;
}

//
// add a diff config entry to the list
//
int GenAdd (CONFIG_DIFF **Head, CONFIG_DIFF **Tail, A_UINT16 offset, A_UINT8 size, A_UINT8 *pData)
{
	CONFIG_DIFF *s;

    if (size == 0 || pData == NULL)
    {
        UserPrint("GenAdd - called without data\n");
        return 0;
    }

    //
	// make a new structure
	//
	s=(CONFIG_DIFF *)malloc(sizeof(CONFIG_DIFF));
	//
	// tack it on the end of the list
	//
	if(s!=0)
	{
		if(*Head==0)
		{
			s->next=0;
			s->prev=0;
			*Head=s;
			*Tail=s;
		}
		else
		{
			s->next=0;
			s->prev=*Tail;
			if(s->prev!=0)
			{
				s->prev->next=s;
			}
			*Tail=s;
		}
	}
	else
	{
		UserPrint("cant save diff config entry 0x%x:%d -< %x\n",offset,size,pData[0]);
		return -1;
	}
	if(s!=0)
	{
		s->offset=offset;
		s->size=size;
		s->pData=pData;
	}
	return 0;
}

//
// execute all of the configuration differences
//
int GenExecute(CONFIG_DIFF *Head, A_UINT8 *pBuffer)
{
	CONFIG_DIFF *s;
	int i;
    unsigned int count, bufLen;
    A_UINT8 *pBuf, *pBufStart;

#if AP_BUILD
    DevDeviceSwapCalStruct();
#endif

	count=0;
    pBuf = pBuffer;
    bufLen = 0;
	for(s=Head; s!=0; s=s->next)
	{
        if ((pBuf + s->size + 4) > (pBuffer + CONFIG_DIFF_MAX_BUFFER))
        {
            art_eepromWriteItems(count, pBuffer, bufLen);
            count = 0;
            pBuf = pBuffer;
            bufLen = 0;
        }
        pBufStart = pBuf;
        // 2-byte offset
        *pBuf++ = (A_UINT8)(s->offset & 0xff);
        *pBuf++ = (A_UINT8)((s->offset >> 8) & 0xff);
        // 1-byte size
        *pBuf++ = s->size;
        
		//UserPrint("[%04x,%2x] = ", s->offset, s->size);
        // size-byte data
        for(i = 0; i < s->size ; ++i)
        {
            //if ((i != 0) && ((i % 16) == 0)) UserPrint("\n            ");
            *pBuf++ = s->pData[i];
            //if (i < 0x20)
            //{
            //    UserPrint("0x%02x ", s->pData[i]);
            //}
        }
        //UserPrint("\n");*/
        bufLen += pBuf - pBufStart;
		count++;
	}
    if (count)
    {
        UserPrint("Number of items = %d; buffer size = %d\n", count, bufLen);
        art_eepromWriteItems(count, pBuffer, bufLen);
    }
#if AP_BUILD
    DevDeviceSwapCalStruct();
#endif
	return count;
}

//*****************************************************************************
// Config Diff Functions
//*****************************************************************************
//
// initialize the list
//
void ConfigDiffInit()
{
    _CdHead = 0;
    _CdTail = 0;
   	_CdCurrent=0;
}


//
// clear the list of diff config
//
int ConfigDiffClearAll()
{
    return GenClearAll(&_CdHead, &_CdTail, &_CdCurrent);
}

//
// find a diff config entry on the list
//
CONFIG_DIFF *ConfigDiffFind(A_UINT16 offset)
{
    return (GenFind(_CdTail, offset));
}

//
// find a diff config entry on the list
//
CONFIG_DIFF *ConfigDiffFindWithin(A_UINT16 offset, A_UINT8 size, A_UINT16 *newOffset, A_UINT8 *newSize)
{
    return (GenFindWithin(_CdTail, offset, size, newOffset, newSize));
}

//
// clear one entry from the list
//
int ConfigDiffClearEntry(CONFIG_DIFF *s)
{
    return (GenClearEntry(&_CdHead, &_CdTail, &_CdCurrent, s));
}

//
// clear one entry from the list
//
int ConfigDiffClear(A_UINT16 offset)
{
	CONFIG_DIFF *s;

	s=GenFind(_CdTail, offset);
	if(s!=0)
	{
        return (ConfigDiffClearEntry(s));
	}
	return -1;
}

//
// add a diff config entry to the list
//
int ConfigDiffAdd(A_UINT16 offset, A_UINT8 size, A_UINT8 *pData)
{
    return (GenAdd(&_CdHead, &_CdTail, offset, size, pData));
}

//
// add/replace a diff config entry to the list
//
int ConfigDiffChange(A_UINT16 offset, A_UINT8 size, A_UINT8 *pData)
{
	CONFIG_DIFF *s;
    A_UINT16 newOffset;
    A_UINT8 newSize;

    s=GenFindWithin(_CdTail, offset, size, &newOffset, &newSize);
	if(s==0)
	{
        //not found add a new entry
        return (ConfigDiffAdd(offset, size, pData));
	}
    // Data has been already updated in set functions, just need to adjust the size of the entry 
    // if the new entry is outside of the old entry bound.
    s->size = newSize;
    s->pData = s->pData - s->offset + newOffset; //base + newoffset
    s->offset = newOffset;
	return 0;
}

//
// execute all of the configuration differences
//
int ConfigDiffExecute()
{
    int count;

    count = GenExecute(_CdHead, ConfigDiffBuffer);
    if (count) { UserPrint("Configuration differences have been sent to UTF....\n"); }
    // clear all entries after push them to DUT
    ConfigDiffClearAll();
	return count;
}

//
// Create diff config list between 2 memory locations
// The list will contains the values of the first location
//
int ConfigDiffCreateDiffList (A_UINT8 *pArea1, A_UINT8 *pArea2, A_UINT32 numBytes) 
{
    
    A_UINT8 *p1, *p2, *p2End, *pData;
    A_UINT16 offset, size, sameCount, tempSize;
    A_UINT8 done, firstDiffFound;
    int numOfEntries;

    ConfigDiffInit();
    p1 = pArea1;
    p2 = pArea2;
    p2End = pArea2 + numBytes;
    firstDiffFound = 0;
    numOfEntries = 0;
    sameCount = 0;
    done = 0;
            
    offset = 0;
    size = 0;
    pData = 0;

    // move (offset,size,pData) to the location where a diff is found
    while(!done)
    {
        // skip if they are the same
        while (*p1 == *p2)
        {
            sameCount++;
            size++;
            if (p2++ == p2End)
            {
                done = 1;
                break;
            }
            p1++;
        }
        if (done)
        {
            break;
        }
        if (firstDiffFound == 0)
        {
            firstDiffFound = 1;
            offset = p1 - pArea1;
            size = 0;
            pData = p1;
        }
        // From one diff to next diff, there should be at least CONFIG_DIFF_MIN_APART same bytes; 
        // otherwise concatenate them as one diff.
        // However, if from one diff to the next has less than CONFIG_DIFF_MIN_APART same bytes, 
        // but the previous diff's size exceeds CONFIG_DIFF_MAX_SIZE bytes, don't need to concatenate the 2 diff's together.
        else if ((sameCount >= CONFIG_DIFF_MIN_APART) || (size-sameCount >= CONFIG_DIFF_MAX_SIZE))
        {
            size = size - sameCount;
            while(size)
            {
                tempSize = (size > CONFIG_DIFF_MAX_SIZE) ? CONFIG_DIFF_MAX_SIZE : size;                       
                // add the diff to the ConfDiff list
                ConfigDiffAdd(offset, (A_UINT8)tempSize, pData);
                numOfEntries++;
                offset += tempSize;
                size -= tempSize;
                pData += tempSize;
            }
            // next difference
            offset = p1 - pArea1;
            //in case the
            if (offset == numBytes)
            {
                pData = 0;
                break;
            }
            size = 0;
            pData = p1;
        }
        // count the differences
        while (*p1 != *p2)
        {
            size++;
            if (p2++ == p2End)
            {
                done = 1;
                break;
            }
            p1++;
        }
        sameCount = 0;
    } // while (!done)
    if (pData)
    {
        size = size - sameCount;
        while(size)
        {
            tempSize = (size > CONFIG_DIFF_MAX_SIZE) ? CONFIG_DIFF_MAX_SIZE : size;                       
            // add the diff to the ConfDiff list
            ConfigDiffAdd(offset, (A_UINT8)tempSize, pData);
            numOfEntries++;
            offset += tempSize;
            size -= tempSize;
            pData += tempSize;
        }
    }
    return numOfEntries;
}

//*****************************************************************************
// Calibration Info Functions
//*****************************************************************************
//
// initialize the list
//
void CalInfoInit()
{
    _CalInfoHead = 0;
    _CalInfoTail = 0;
   	_CalInfoCurrent=0;
}


//
// clear the list of cal info
//
int CalInfoClearAll()
{
    return GenClearAll(&_CalInfoHead, &_CalInfoTail, &_CalInfoCurrent);
}

//
// clear one entry from the list
//
int CalInfoClearEntry(CONFIG_DIFF *s)
{
    return (GenClearEntry(&_CalInfoHead, &_CalInfoTail, &_CalInfoCurrent, s));
}

//
// clear one entry from the list
//
int CalInfoClear(A_UINT16 offset)
{
	CONFIG_DIFF *s;

	s=GenFind(_CalInfoTail, offset);
	if(s!=0)
	{
        return (CalInfoClearEntry(s));
	}
	return -1;
}

//
// add a cal info entry to the list
//
int CalInfoAdd(A_UINT16 offset, A_UINT8 size, A_UINT8 *pData)
{
    return (GenAdd(&_CalInfoHead, &_CalInfoTail, offset, size, pData));
}

//
// add/replace a cal info entry to the list
//
int CalInfoChange(A_UINT16 offset, A_UINT8 size, A_UINT8 *pData)
{
    CONFIG_DIFF *s;
    A_UINT16 newOffset;
    A_UINT8 newSize;

    s=GenFindWithin(_CalInfoTail, offset, size, &newOffset, &newSize);
    if(s==0)
    {
        //not found add a new entry
        //UserPrint("CalInfoChange - new entry (0x%x, %d)\n", offset, size);
        return (CalInfoAdd(offset, size, pData));
    }
    // Data has been already updated in set functions, just need to adjust the size of the entry 
    // if the new entry is outside of the old entry bound.
    s->size = newSize;
    s->pData = s->pData - s->offset + newOffset; //base + newoffset
    s->offset = newOffset;
    //UserPrint("CalInfoChange - (0x%x, %d)\n", newOffset, newSize);
    return 0;
}

//
// execute all of the cal info
//
int CalInfoExecute()
{
    int count;

    count = GenExecute(_CalInfoHead, CalInfoBuffer);
    if (count) { UserPrint("Cal data has been sent to UTF....\n"); }
    CalInfoClearAll();
	return count;
}
