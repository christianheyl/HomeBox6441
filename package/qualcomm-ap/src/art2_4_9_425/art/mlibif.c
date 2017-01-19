/* MLIBif.c - contians wrapper functions for MLIB */

/* Copyright (c) 2000 Atheros Communications, Inc., All Rights Reserved */
//  "ACI $Id: //depot/sw/branches/art2_main_per_cs/src/art2/art/mlibif.c#1 $, $Header: //depot/sw/branches/art2_main_per_cs/src/art2/art/mlibif.c#1 $"

#ifdef _WINDOWS 
#include <windows.h>
#endif

#include <time.h>
#include <errno.h>
#include <stdio.h>
#include "wlantype.h"
#include "athreg.h"
#include "manlib.h"
#include "common_hw.h"
#ifdef JUNGO
#include "mld.h"
#endif
#include "mlibif.h"
#include "art_if.h"
#include <stdlib.h>

#ifdef LINUX
#include <string.h>
#endif

// Dev to driverDev mapping table.  devNum must be in range 0 to LIB_MAX_DEV
A_UINT32   devNum2driverTable[LIB_MAX_DEV];
#ifdef UNUSED
extern A_BOOL thin_client;
#endif
static A_BOOL last_op_was_read = 0;
static A_BOOL last_op_offset_was_fc = 0;
#ifdef UNUSEDHAL
extern A_UINT32 macRev;
extern SUB_DEV_INFO devInfo; /* defined in test.c */
#endif

#include "UserPrint.h"


#ifdef THUNUSED

A_UCHAR *t_bytesRead;
A_UCHAR *t_bytesWrite;

A_BOOL initializeEnvironment(A_BOOL remote) {
	A_BOOL	openDriver;
  
    // print version string
    UserPrint("\n    --- Atheros Radio Test (ART) ---\n");
    UserPrint(MAUIDK_VER2);
    UserPrint(MAUIDK_VER3);
 
	openDriver = remote ?  0 : 1;

    // perform environment initialization
#ifdef _DEBUG
//    if ( A_ERROR == envInit(TRUE, openDriver) ) {
    if ( A_ERROR == envInit(FALSE, openDriver) ) {
#else
    if ( A_ERROR == envInit(FALSE, openDriver) ) {
#endif
        UserPrint("Error: Unable to initialize the local environment... Closing down!\n");
        return FALSE;
    }
	t_bytesRead = (A_UCHAR *) malloc(MAX_MEMREAD_BYTES * sizeof(A_UCHAR));
	t_bytesWrite = (A_UCHAR *) malloc(MAX_MEMREAD_BYTES * sizeof(A_UCHAR));
	if ((t_bytesRead == NULL) || (t_bytesWrite == NULL)) 	
		return FALSE;
    return TRUE;
}


void closeEnvironment(void) {
    // clean up anything that was allocated and close the local driver
    envCleanup(TRUE);
	if (t_bytesRead != NULL) 
			free(t_bytesRead);
	if (t_bytesWrite != NULL) 
			free(t_bytesWrite);
}

#endif // THUNUSED

A_BOOL devNumValid
(
 A_UINT32 devNum
)
{
    if(globDrvInfo.pDevInfoArray[dev2drv(devNum)] != NULL) {
        return TRUE;
    }
    else {
        return FALSE;
    }
}


#ifdef THUNUSED

/**************************************************************************
 * Workaround for HW bug that causes read or write bursts that
 * cross a 256-boundary to access the incorrect register.
 */  
static void
reg_burst_war
(
	A_UINT16 devIndex,
	A_UINT32 offset, 
	A_BOOL op_is_read
)
{
    A_UINT32 offset_lsb;
	MDK_WLAN_DEV_INFO *pdevInfo;
	
    offset_lsb = offset & 0xff;
	pdevInfo = globDrvInfo.pDevInfoArray[devIndex];

    // Check if offset is aligned to a 256-byte boundary
    if (offset_lsb == 0) {
      if (last_op_offset_was_fc && (last_op_was_read == op_is_read)) {
         // Last access was to an offset with lsbs of 0xfc and was
         // of the same type (read or write) as the current access, so
         // need to break a possible burst across the 256-byte boundary.
         // Do this by reading the SREV reg, as it's always safe to do so.
         hwMemRead32(devIndex, 0x4020 + pdevInfo->pdkInfo->f2MapAddress);
      }
    }
  
    last_op_was_read = op_is_read;
    last_op_offset_was_fc = (offset_lsb == 0xfc);
}

/**************************************************************************
* OSregRead - MLIB command for reading a register
*
* RETURNS: value read
*/
A_UINT32 OSregRead
(
    A_UINT32 devNum,
    A_UINT32 regOffset
)
{
    A_UINT32         regReturn;
    MDK_WLAN_DEV_INFO    *pdevInfo;
	A_UINT16 devIndex;

	if(devNumValid(devNum) == FALSE) {
		UserPrint("Error: OSregRead did not receive a valid devNum\n");
	    return(0xdeadbeef);
	}

	devIndex = (A_UINT16)dev2drv(devNum);
   	pdevInfo = globDrvInfo.pDevInfoArray[devIndex];

#ifdef UNUSED
  if (thin_client) {
        regOffset -= pdevInfo->pdkInfo->aregPhyAddr[0];
        regReturn = art_regRead(devNum, regOffset);
  }
  else 
#endif
  {
   	/* check that the register is within the range of registers */
	if( (regOffset < pdevInfo->pdkInfo->f2MapAddress) || 
		(regOffset > (MAX_REG_OFFSET + pdevInfo->pdkInfo->f2MapAddress)) ) {
#ifdef UNUSEDHAL
		if (!large_pci_addresses(devNum)) 
#endif
		{
		      UserPrint("Error:  OSregRead Not a valid register offset:%x <%x or >%x\n", regOffset,
				  pdevInfo->pdkInfo->f2MapAddress, MAX_REG_OFFSET + pdevInfo->pdkInfo->f2MapAddress);
		      return(0xdeadbeef);
		}
    }

	/* read the register */
	reg_burst_war(devIndex, regOffset, 1);
	regReturn = hwMemRead32(devIndex, regOffset); 
UserPrint("Register at offset %08lx: %08lx\n", regOffset, regReturn);
    //
  }

    return(regReturn);
}

void merlinAnalogShiftWrite
(
	A_UINT16 devIndex,
    A_UINT32 regOffset,
    A_UINT32 regValue
)
{
    MDK_WLAN_DEV_INFO    *pdevInfo;
    A_UINT32 baseAddress;
    A_UINT32 SW_OVERRIDE_addr, SW_CNTL_addr, SW_SCLK_addr, SIN_VAL_addr;
    A_UINT32 addr, j, bitToWrite, counter;


//UserPrint("\n\nSNOOP: analog reg write value %x to %x\n\n", regValue, regOffset); 
   	pdevInfo = globDrvInfo.pDevInfoArray[devIndex];
    baseAddress = pdevInfo->pdkInfo->f2MapAddress;
    SW_OVERRIDE_addr = 0x7900 + baseAddress;
    SIN_VAL_addr     = 0x7904 + baseAddress;
    SW_SCLK_addr     = 0x7908 + baseAddress;
    SW_CNTL_addr     = 0x790c + baseAddress;
    

    //Set override bit
    hwMemWrite32(devIndex, SW_OVERRIDE_addr, 0x1);

    //Send Capture pulse
    hwMemWrite32(devIndex, SW_CNTL_addr, 0x4);    //Capture
    hwMemWrite32(devIndex, SW_SCLK_addr, 0x1);    //sclk
    hwMemWrite32(devIndex, SW_SCLK_addr, 0x0);    //clk
    hwMemWrite32(devIndex, SW_CNTL_addr, 0x0);    
    
    counter = 0;
    for(addr = 0x7800; addr <= 0x7898; addr+=4) {
        for(j = 0; j < 32; j++) {
            bitToWrite = hwMemRead32(devIndex, SIN_VAL_addr) & 0x1;
//UserPrint("SNOOP: read %x from analog, ", bitToWrite);
            if((regOffset & 0xffff) == addr) {
                bitToWrite = regValue & 0x1;
                regValue >>= 1;
            }
//UserPrint("SNOOP: writing %x to analog, counter = %d\n", bitToWrite, counter);
            hwMemWrite32(devIndex, SW_CNTL_addr, bitToWrite & 0x1);//sout
            hwMemWrite32(devIndex, SW_SCLK_addr, 0x1);             //sclk
            hwMemWrite32(devIndex, SW_SCLK_addr, 0x0);             //sclk
            hwMemWrite32(devIndex, SW_CNTL_addr, 0x0);             //sout
            counter++;
        }
//UserPrint("\n");        
    }
//UserPrint("###################################################################\n");    
    //Send Update pulse
    hwMemWrite32(devIndex, SW_CNTL_addr, 0x2);    //update
    hwMemWrite32(devIndex, SW_SCLK_addr, 0x1);    //sclk
    hwMemWrite32(devIndex, SW_SCLK_addr, 0x0);    //sclk
    hwMemWrite32(devIndex, SW_CNTL_addr, 0x0);    

    return;
}

/**************************************************************************
* OSregWrite - MLIB command for writing a register
*
*/
void OSregWrite
(
	A_UINT32 devNum,
    A_UINT32 regOffset,
    A_UINT32 regValue
)
{
    MDK_WLAN_DEV_INFO* pdevInfo;
	A_UINT16           devIndex;

    if(devNumValid(devNum) == FALSE) 
	{
		UserPrint("Error: OSregWrite did not receive a valid devNum\n");
		return;
	}

	devIndex = (A_UINT16)dev2drv(devNum);
   	pdevInfo = globDrvInfo.pDevInfoArray[devIndex];

	if( (regOffset < pdevInfo->pdkInfo->f2MapAddress) || 
		(regOffset > (MAX_REG_OFFSET + pdevInfo->pdkInfo->f2MapAddress)) ) 
	{
#ifdef UNUSEDHAL
		if (!large_pci_addresses(devNum)) 
#endif
		{
		     UserPrint("Error:  OSregWrite Not a valid register offset\n");
		     return;      
		}
    }

	  /* write the register */
	  reg_burst_war(devIndex, regOffset, 0);

      if(((regOffset & 0xffff) >= 0x7800) && ((regOffset & 0xffff) <= 0x7898)) 
	  {
#ifdef UNUSEDHAL
          if(isMerlin_Eng(macRev)) 
		  {
              /* BAA: a hit on EVM performance was noticed when
               *      merlinAnalogShiftWrite() was used to write to
               *      regs 0x7854 and 0x7834. Hence, use hwMemWrite32()
               *      instead. However; this only works for PCI-E devices
               *      as of 2/11/2008 an investigation is still under way
               *      as to why it doesn't work with mini-PCI devices.
               */
              
                if( MERLIN_DEV_AR5416_PCI == devInfo.hwDevID ) 
				{ /* mini-PCI devices */
                    hwMemWrite32(devIndex, regOffset, regValue); 
                }
                else if( 0x002a == devInfo.hwDevID ) 
				{ /* PCI-E devices */
                    if( ((regOffset & 0xffff) == 0x7854) ||
                        ((regOffset & 0xffff) == 0x7834) ) 
					{
                        hwMemWrite32(devIndex, regOffset, regValue); 
                    }
                    else 
					{
                        merlinAnalogShiftWrite(devIndex, regOffset, regValue);
                    }
                }
          } 
		  else 
		  {
              hwMemWrite32(devIndex, regOffset, regValue); 
          }
      } 
	  else 
#endif
	  { 
            hwMemWrite32(devIndex, regOffset, regValue); 
      }

#ifdef UNUSEDHAL
//	  if(pdevInfo->pdkInfo->printPciWrites) 
        if( pdevInfo->pdkInfo->printPciWrites && (((regOffset & 0xffff) < 0x7000) || ((regOffset & 0xffff) > 0x7100)) ) 
		{
            if( large_pci_addresses(devNum) ) 

			{
                UserPrint("0x%08x 0x%08x\n", regOffset & 0xffffffff, regValue);
            }

            else 			
			{
#if (0) /* this code is for debugging, it stores the reg values into a file */                
                FILE* fStream;
                fStream = fopen("regdump.txt", "a");
                fUserPrint(fStream, "0x%04x 0x%08x\n", regOffset & 0xffff, regValue);
                fclose(fStream);
#endif
               

                /* printing from utility menu occurs here */
                UserPrint("0x%04x 0x%08x\n", regOffset & 0xffff, regValue);
            }
        }
#endif
    }
}

#endif // THUNUSED


/**************************************************************************
* OScfgRead - MLIB command for reading a pci configuration register
*
* RETURNS: value read
*/
ANWIDLLSPEC A_UINT32 OScfgRead
(
	A_UINT32 devNum,
    A_UINT32 regOffset
)
{
    A_UINT32         regReturn;
	A_UINT16 devIndex;

	if(devNumValid(devNum) == FALSE) {
		UserPrint("Error: OScfgRead did not receive a valid devNum\n");
	    return(0xdeadbeef);
	}

#ifdef UNUSED
	if (thin_client) {
       regReturn = art_cfgRead(devNum, regOffset);
	}
	else 
#endif
	{
	   devIndex = (A_UINT16)dev2drv(devNum);

#ifndef LINUX  //rcc: for AP124, pcie config starting address =0x5000
	   if(regOffset > MAX_CFG_OFFSET) {
		   UserPrint("Error:  OScfgRead: not a valid config offset\n");
		   return(0xdeadbeef);
       }
#endif
	   regReturn = hwCfgRead32(devIndex, regOffset); 

       /* display the value */
       //UserPrint("%08lx: ", regOffset);
       //UserPrint("%08lx\n", regReturn);
	}

    return(regReturn);
}

/**************************************************************************
* OScfgWrite - MLIB command for writing a pci config register
*
*/
ANWIDLLSPEC void OScfgWrite
(
	A_UINT32 devNum,
    A_UINT32 regOffset,
    A_UINT32 regValue
)
{
	A_UINT16 devIndex;
   
	if(devNumValid(devNum) == FALSE) {
		UserPrint("Error: OScfgWrite did not receive a valid devNum\n");
		return;
	}

#ifdef UNUSED
	if (thin_client) {
       art_cfgWrite(devNum, regOffset, regValue);
	}
	else 
#endif
	{
	   devIndex = (A_UINT16)dev2drv(devNum);
	
	   if(regOffset > MAX_CFG_OFFSET) {
		   UserPrint("Error:  OScfgWrite: not a valid config offset\n");
		   return;
       }
	   hwCfgWrite32(devIndex, regOffset, regValue); 
	}
}


#ifdef THUNUSED

/**************************************************************************
* OSmemRead - MLIB command to read a block of memory
*
* read a block of memory
*
* RETURNS: An array containing the bytes read
*/
void OSmemRead
(
	A_UINT32 devNum,
    A_UINT32 physAddr, 
	A_UCHAR  *bytesRead,
    A_UINT32 length
)
{
#ifdef UNUSED
	A_UINT32 t_length, i;
	A_UCHAR *t_bytesRead;
    A_UINT32 t_physAddr, t_sOffset;
#endif
	A_UINT16 devIndex;

	if(devNumValid(devNum) == FALSE) {
		UserPrint("Error: OSmemRead did not receive a valid devNum\n");
		return;
	}

    /* check to see if the size will make us bigger than the send buffer */
    if (length > MAX_MEMREAD_BYTES) {
        UserPrint("Error: OSmemRead length too large, can only read %x bytes\n", MAX_MEMREAD_BYTES);
		return;
    }
	if (bytesRead == NULL) {
        UserPrint("Error: OSmemRead received a NULL ptr to the bytes to read - please preallocate\n");
		return;
    }

#ifdef UNUSED
	if (thin_client) {
	   t_length = length;
	   t_physAddr = physAddr;
	   t_sOffset = 0;
	   if (t_physAddr % 4) {
	      t_length += (t_physAddr % 4);
	      t_sOffset = (t_physAddr % 4);
	      t_physAddr &= 0xfffffffc;
	   }
 	   if ( (t_length % 4)) { 
		   t_length += (4 -(t_length % 4));
	   }
	   t_bytesRead = (A_UCHAR *) malloc(t_length * sizeof(A_UCHAR));
//       UserPrint("SNOOP::OSmemRead:physAddr=%x:t_physAddr=%x:t_sOffset=%d:length=%d:t_length=%d:\n", physAddr, t_physAddr, t_sOffset, length, t_length);
       art_memRead(devNum, t_physAddr, t_bytesRead, t_length);

	   for(i=0;i<length;i++) {
	     bytesRead[i] = t_bytesRead[i+t_sOffset];
	   }
	   free(t_bytesRead);
	}
    else 
#endif
	{
	   devIndex = (A_UINT16)dev2drv(devNum);

	   if(hwMemReadBlock(devIndex, bytesRead, physAddr, length) == -1) {
		   UserPrint("Error: OSmemRead failed call to hwMemReadBlock()\n");
		return;
	   }
	}

}

/**************************************************************************
* OSmemWrite - MLIB command to write a block of memory
*
*/
void OSmemWrite
(
	A_UINT32 devNum,
    A_UINT32 physAddr,
	A_UCHAR  *bytesWrite,
	A_UINT32 length
)
{
#ifdef UNUSED
	A_UINT32 t_length;
    A_UINT32 t_physAddr, t_sOffset;
#endif
	A_UINT16 devIndex;//, iIndex; 

	if(devNumValid(devNum) == FALSE) {
		UserPrint("Error: OSmemWrite did not receive a valid devNum\n");
		return;
	}
    /* check to see if the size will make us bigger than the send buffer */
    if (length > MAX_MEMREAD_BYTES) {
        UserPrint("Error: OSmemWrite length too large, can only read %x bytes\n", MAX_MEMREAD_BYTES);
		return;
    }
	if (bytesWrite == NULL) {
        UserPrint("Error: OSmemWrite received a NULL ptr to the bytes to write\n");
		return;	
	}

#ifdef UNUSED
	if (thin_client) {
	   t_length = length;
	   t_physAddr = physAddr;
	   t_sOffset = 0;
	   if (t_physAddr % 4) {
	      t_length += (t_physAddr % 4);
	      t_sOffset = (t_physAddr % 4);
	      t_physAddr &= 0xfffffffc;
	   }
 	   if ( (t_length % 4)) { 
		   t_length += (4 -(t_length % 4));
	   }
	   memcpy(t_bytesWrite, bytesWrite, length * sizeof(A_UCHAR));
       art_memWrite(devNum, t_physAddr, t_bytesWrite, t_length);
	}
    else 
#endif
	{
	  devIndex = (A_UINT16)dev2drv(devNum);

	  if(hwMemWriteBlock(devIndex,bytesWrite, length, &(physAddr)) == -1) {
			UserPrint("Error:  OSmemWrite failed call to hwMemWriteBlock()\n");
			return;
      }
	}

}

/**************************************************************************
* getISREvent - MLIB command get latest ISR event
*
*/
ISR_EVENT getISREvent
(
 A_UINT32 devNum
)
{
	// Initialize event to invalid.
  ISR_EVENT event = {0, 0};
#ifdef UNUSED
  EVENT_STRUCT ppEvent;
#endif
  MDK_WLAN_DEV_INFO    *pdevInfo;
  A_UINT16 devIndex;

#ifdef UNUSED
  if (thin_client) {

	if(devNumValid(devNum) == FALSE) {
		UserPrint("Error: getISREvent did not receive a valid devNum\n");
	    return event;
	}

    if (art_getISREvent(devNum, &ppEvent) == 0xdead) return event;
    if (ppEvent.type == ISR_INTERRUPT) {
		event.valid = 1;
		event.ISRValue = ppEvent.result[0];
     }
    //UserPrint("SNOOP::event type=%d:valid=%d:isrvalue=%d:\n", ppEvent.type, event.valid,  event.ISRValue);
    
  }
  else 
#endif
  {

     #if defined(ANWI) || defined(LINUX) || defined (__APPLE__)
          EVENT_STRUCT pLocEventSpace;
     #else
          EVENT_STRUCT *pLocEventSpace;
     #endif // defined(ANWI/..
     devIndex = (A_UINT16)dev2drv(devNum);
     pdevInfo = globDrvInfo.pDevInfoArray[devIndex];
        #ifdef JUNGO
         if(checkForEvents(globDrvInfo.triggeredQ)) {
                 //call into the kernel plugin to get the event
                 if((pLocEventSpace = popEvent(globDrvInfo.triggeredQ)) != NULL) {
                     if(pLocEventSpace->type == ISR_INTERRUPT) {
                         event.valid = 1;
                         event.ISRValue = pLocEventSpace->result;
                      }
                  else {
                      UserPrint("Error: getISREvent - found a non-ISR event in a client - is this possible???\n");
                      UserPrint("If this has become a possibility... then remove this error check\n");
                      }
                  }
                  else {
                     UserPrint("Error: getISREvent Unable to get event\n");
                  }
          }
        #endif // JUNGO
     #if defined(ANWI) || defined (LINUX) || defined (__APPLE__)
          if (getNextEvent(devIndex, &pLocEventSpace)) {
              if(pLocEventSpace.type == ISR_INTERRUPT) {
                    event.valid = 1;
                    event.ISRValue = pLocEventSpace.result[0];
              }   
          }
     //    UserPrint("SNOOP::event type=%d:valid=%d:isrvalue=%d:\n", pLocEventSpace.type, event.valid,  event.ISRValue);
     #endif
    }
        
	return(event);
}

unsigned long eepromReadNull(unsigned long a, unsigned long b){return -1;};
void eepromWriteNull(unsigned long a, unsigned long b, unsigned long c){};
void eepromReadBlockNull(unsigned long a, unsigned long b, unsigned long c, unsigned long *d){};


/**************************************************************************
* setupDevice - initialization call to ManLIB
*
* RETURNS: devNum or -1 if fail
*/
A_INT32 setupDevice
(
 A_UINT32 whichDevice,
 DK_DEV_INFO *pdkInfo,
 A_UINT16 remoteLib
)
{
	static DEVICE_MAP devMap;
    MDK_WLAN_DEV_INFO    *pdevInfo;
    A_UINT32 devNum;
    EVT_HANDLE  eventHdl;
	A_UINT16 devIndex;
    A_UINT16 device_fn = WMAC_FUNCTION;



        UserPrint("setupDevice whichDevice=%d pdkInfo=%x remoteLib=%d\n",whichDevice,pdkInfo,remoteLib);

 	if ( whichDevice > WLAN_MAX_DEV ) {
		/* don't have a large enough array to accommodate this device */
		UserPrint("Error: devInfo array not large enough, only support %d devices\n", WLAN_MAX_DEV);
	    return -1;
	}


    devIndex = (A_UINT16)(whichDevice - 1);

	if ( A_FAILED( deviceInit(devIndex, device_fn, pdkInfo) ) ) {
		UserPrint("setupDevice Error: Failed call to local deviceInit()!\n");
#ifndef _IQV
        exit(EXIT_FAILURE);
#endif
	    return -1;
	}
	pdevInfo = globDrvInfo.pDevInfoArray[devIndex];

    // Finally, setup the library mapping
	devMap.DEV_CFG_ADDRESS = 0;
	devMap.DEV_CFG_RANGE = MAX_CFG_OFFSET;
#if defined(ANWI) || defined (LINUX) || defined (__APPLE__)
	devMap.DEV_MEMORY_ADDRESS = (unsigned long) pdevInfo->pdkInfo->memPhyAddr;
#endif
#ifdef JUNGO
	devMap.DEV_MEMORY_ADDRESS = (unsigned long) pdevInfo->pdkInfo->dma.Page[0].pPhysicalAddr;
#endif
	devMap.DEV_MEMORY_RANGE = pdevInfo->pdkInfo->memSize; 
	devMap.DEV_REG_ADDRESS = pdevInfo->pdkInfo->f2MapAddress;
	devMap.DEV_REG_RANGE = 65536;
	devMap.OScfgRead = OScfgRead;
	devMap.OScfgWrite = OScfgWrite;
	devMap.OSmemRead = OSmemRead;
	devMap.OSmemWrite = OSmemWrite;
	devMap.OSregRead = OSregRead;
	devMap.OSregWrite = OSregWrite;
	devMap.getISREvent = getISREvent;
	devMap.devIndex = devIndex;

    devMap.remoteLib = remoteLib;
#ifdef UNUSED
    devMap.r_eepromRead = art_eepromRead;
    devMap.r_eepromWrite = art_eepromWrite;
    devMap.r_eepromReadBlock = art_eepromReadBlock;
#else
    devMap.r_eepromRead = eepromReadNull;
    devMap.r_eepromWrite = eepromWriteNull;
    devMap.r_eepromReadBlock = eepromReadBlockNull;
#endif
#ifdef UNUSED
	if (thin_client) {
       devMap.r_hwReset = art_hwReset;
       devMap.r_pllProgram = art_pllProgram;
	   devMap.r_calCheck = art_calCheck;
	   devMap.r_pciWrite = art_pciWrite;
	   devMap.r_fillTxStats = art_fillTxStats;
	   devMap.r_createDescriptors = art_createDescriptors;
   }
   else 
#endif
   {
       devMap.r_hwReset = NULL;
       devMap.r_pllProgram = NULL;
	   devMap.r_calCheck = NULL;
	   devMap.r_pciWrite = NULL;
	   devMap.r_fillTxStats = NULL;
	   devMap.r_createDescriptors = NULL;
	}
#ifdef UNUSEDHAL
	devNum = initializeDevice(devMap);
#else
	devNum=0;
#endif

    if(devNum > WLAN_MAX_DEV) {
        UserPrint("setupDevice Error: Manlib Failed to initialize for this Device:devNum returned = %x\n", devNum);
        return -1;
    }
   	devNum2driverTable[devNum] = devIndex;

	UserPrint("devNum=%d devIndex=%d\n",devNum,devIndex);

    // assign the handle (id, actually) that will be sent back to Perl to uniquely
    eventHdl.eventID = 0;
    eventHdl.f2Handle = (A_UINT16)devNum;

    if (!pdkInfo)
	if(!pdevInfo->pdkInfo->haveEvent) {
        if(hwCreateEvent(devIndex, ISR_INTERRUPT, 1, 0, 0, 0, eventHdl) == -1) {
            UserPrint("setupDevice Error: Could not initalize driver ISR events\n");
            teardownDevice(devNum);
            return -1;
        }
		pdevInfo->pdkInfo->haveEvent = TRUE;
	}

    return devNum;
}

void teardownDevice
(
 A_UINT32 devNum
) 
{
        //UserPrint("SNOOP::teardownDevice called\n");
    if(devNumValid(devNum) == FALSE) {
		UserPrint("Error: teardownDevice did not receive a valid devNum\n");
	    return;
	}

#ifdef UNUSEDHAL
    // Close the Manufacturing Lib
    closeDevice(devNum);
#endif
    // Close the driver Jungo driver entries
    deviceCleanup((A_UINT16)dev2drv(devNum));
	
        //UserPrint("SNOOP::teardownDevice exit\n");
}

/**************************************************************************
* setPciWritesFlag - Change the value to flag for printing pci writes data
*
* RETURNS: N/A
*/
void changePciWritesFlag
(
	A_UINT32 devNum,
	A_UINT32 flag
)
{
    MDK_WLAN_DEV_INFO    *pdevInfo;
	A_UINT16 devIndex;

	devIndex = (A_UINT16)dev2drv(devNum);
	pdevInfo = globDrvInfo.pDevInfoArray[devIndex];

	pdevInfo->pdkInfo->printPciWrites = (A_BOOL)flag;
	return;
}



#endif // THUNUSED
