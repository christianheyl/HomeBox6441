
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <assert.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdarg.h>
#include <signal.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>

#include "wlantype.h"

#include "athreg.h"
#include "manlib.h"

#include "UserPrint.h"
#include "common_hw.h"
//#include "ar5210reg.h"
#include "dk_common.h"
#include "event.h"
#include "dk_ioctl.h"
#ifndef ART_BUILD
#include "dk_master.h"
#endif


#define DRV_MAJOR_VERSION   1
#define DRV_MINOR_VERSION   2


#define CHIP_REV_ID_LOCATION 0xb8060090
extern void        envCleanup(A_BOOL closeDriver);
extern DRV_VERSION_INFO driverVer;

#define uiPrintf UserPrint
#define q_uiPrintf UserPrint

void sig_hDevicer
(
 	int arg
) 
{
		uiPrintf("Received SIGINT !! cleanup and close down ! \n");
#ifndef ART_BUILD
#ifndef SOC_LINUX
       dkPerlCleanup();
#endif
#endif
		envCleanup(TRUE);
		exit(0);
}

A_STATUS connectSigHandler(void) {
	signal(SIGINT,sig_hDevicer);
    return A_OK;
}

/**************************************************************************
* driverOpen - device opened in device_init
* 
* RETURNS: 1 
*/
A_UINT16 driverOpen()
{
    return 1;
}

A_UINT32 hwIORead(A_UINT16 devIndex, A_UINT32 address) {
  return 0;
}

void hwIOWrite(A_UINT16 devIndex, A_UINT32 address, A_UINT32 writeValue) {

}

unsigned long FullAddrRead(unsigned long address)
//unsigned long FullAddrRead()
{
        struct cfg_op co;
        A_UINT32 data;
        MDK_WLAN_DEV_INFO    *pdevInfo;
//unsigned int address;
        pdevInfo = globDrvInfo.pDevInfoArray[0];

        co.offset=address;
        co.size = 1;
        co.value = 0;

           if (ioctl(pdevInfo->hDevice,DK_IOCTL_FULL_ADDR_READ,&co) < 0) {
                  uiPrintf("Error: FullAddrRead read failed \n");
                  data = 0xdeadbeef;
           } else {
                  data = co.value;
           }


          return data;


}
void FullAddrWrite
(
        A_UINT32  offset,                    /* the address to write */
        A_UINT32  value                        /* value to write */
)
{
        struct cfg_op co;
        A_UINT32 data;
        MDK_WLAN_DEV_INFO    *pdevInfo;

        pdevInfo =globDrvInfo.pDevInfoArray[0] ;

        co.offset=offset;
        co.size = 4;
        co.value = value;

        if (ioctl(pdevInfo->hDevice,DK_IOCTL_FULL_ADDR_WRITE,&co) < 0) {
                uiPrintf("Error: FullAddrWrite failed \n");
        }
        return;
}

/**************************************************************************
* hwCfgRead8 - read an 8 bit configuration value
*
* This routine will call into the driver to activate a 8 bit PCI configuration 
* read cycle.
*
* RETURNS: the value read
*/
A_UINT8 hwCfgRead8
(
	A_UINT16 devIndex,
	A_UINT32 address                    /* the address to read from */
)
{
	struct cfg_op co;
	A_UINT32 data;
	A_UINT8 out;
	MDK_WLAN_DEV_INFO    *pdevInfo;

	pdevInfo = globDrvInfo.pDevInfoArray[devIndex];

	co.offset=address;
	co.size = 1;
	co.value = 0;

#ifdef OWL_PB42
       if(!isHowlAP(devIndex)){
#endif
	   if (ioctl(pdevInfo->hDevice,DK_IOCTL_CFG_READ,&co) < 0) {
		  uiPrintf("Error: PCI Config read failed \n");
	          data = 0xdeadbeef;
	   } else { 
	          data = co.value;
	   } 

	   out = (A_UINT8)(data & 0x000000ff);

       	  return out;
#ifdef OWL_PB42
      }
#endif

}

/**************************************************************************
* hwCfgRead16 - read a 16 bit value
*
* This routine will call into the driver to activate a 16 bit PCI configuration 
* read cycle.
*
* RETURNS: the value read
*/
A_UINT16 hwCfgRead16
(
	A_UINT16 devIndex,
	A_UINT32 address                    /* the address to read from */
)
{
	struct cfg_op co;
	A_UINT32 data;
	A_UINT16 out;
	MDK_WLAN_DEV_INFO    *pdevInfo;

	pdevInfo = globDrvInfo.pDevInfoArray[devIndex];

	address = address & 0xfffffffe;
	co.offset=address;
	co.size = 2;
	co.value = 0;

#ifdef OWL_PB42
       if(!isHowlAP(devIndex)){
#endif
	    if (ioctl(pdevInfo->hDevice,DK_IOCTL_CFG_READ,&co) < 0) {
	        	uiPrintf("Error: PCI Config read failed \n");
		        data = 0xdeadbeef;
	    } else { 
	        	data = co.value;
	    } 
        	        out = (A_UINT16)(data & 0x0000ffff);
 
            return out;
#ifdef OWL_PB42
      }
#endif
}
A_UINT32 get_chip_id
(
        A_UINT32 devIndex
)
{
        struct cfg_op co;
        A_UINT32 data;
        MDK_WLAN_DEV_INFO    *pdevInfo;

        pdevInfo =globDrvInfo.pDevInfoArray[devIndex] ;

        co.offset=CHIP_REV_ID_LOCATION;
        co.size = 2;
        co.value = 0;
        if (ioctl(pdevInfo->hDevice,DK_IOCTL_GET_CHIP_ID,&co) < 0) {
                uiPrintf("Error::Get_chip_id  failed::co.value=%x\n", co.value);
                data = 0xdeadbeef;
        } else {
                data = co.value;
        }
    return data;

        //return 0xb1;
}

/**************************************************************************
* hwCfgRead32 - read an 32 bit value
*
* This routine will call into the driver
* 32 bit PCI configuration read.
*
* RETURNS: the value read
*/
A_UINT32 hwCfgRead32
(
	A_UINT16 devIndex,
	A_UINT32 offset                    /* the address to read from */
)
{
	struct cfg_op co;
	A_UINT32 data;
	MDK_WLAN_DEV_INFO    *pdevInfo;

	pdevInfo =globDrvInfo.pDevInfoArray[devIndex] ;

	co.offset=offset;
	co.size = 4;
	co.value = 0;
#ifdef OWL_PB42
        if(!isHowlAP(devIndex)){
	  if (ioctl(pdevInfo->hDevice,DK_IOCTL_CFG_READ,&co) < 0) {
		uiPrintf("Error::PCI Config read32 failed::co.value=%x\n", co.value);
		data = 0xdeadbeef;
	  } else { 
		data = co.value;
	 } 
         return data;
        }
        else{//howlAP
           if(offset==0){
            return 0xa027168c;// hwDevID; it will be right shifted by 16
           }
           if(offset==0x2c){
             return 0xa0810000;// subsystemID
           }
           if(offset==0x2e){
             return 0xa0810000;// subsystemID
           }      
        }
#else 
          // For OB42 and AP71 platforms
	  if (ioctl(pdevInfo->hDevice,DK_IOCTL_CFG_READ,&co) < 0) {
		uiPrintf("Error::PCI Config read32 failed::co.value=%x\n", co.value);
		data = 0xdeadbeef;
	  } else { 
		data = co.value;
	 } 
         return data;

#endif
       return 1; // will not take this return anyway

}


#ifdef OWL_PB42	
/**************************************************************************
* hwRtcRegRead - read an 32 bit value
*
* This routine will call into the driver
* 32 bit PCI configuration read.
*
* RETURNS: the value read
*/
A_UINT32 hwRtcRegRead
(
        A_UINT16 devIndex,
        A_UINT32 offset                    /* the address to read from */
)
{
        struct cfg_op co;
        A_UINT32 data;
        MDK_WLAN_DEV_INFO    *pdevInfo;
        pdevInfo =globDrvInfo.pDevInfoArray[devIndex] ;

        co.offset=offset;
        co.size = 4;
        co.value = 0;
        if(isHowlAP(devIndex)){
          if (ioctl(pdevInfo->hDevice,DK_IOCTL_RTC_REG_READ,&co) < 0) {
                uiPrintf("Error::RTC Reg read failed::co.value=%x\n", co.value);
                data = 0xdeadbeef;
          } else {
                data = co.value;
         }
         return data;
        }
       return 1;
}
#endif



/**************************************************************************
* hwCfgWrite8 - write an 8 bit value
*
* This routine will call into the simulation environment to activate an
* 8 bit PCI configuration write cycle
*
* RETURNS: N/A
*/
void hwCfgWrite8
(
	A_UINT16 devIndex,
	A_UINT32 address,                    /* the address to write */
	A_UINT8  value                        /* value to write */
)
{
	struct cfg_op co;
	MDK_WLAN_DEV_INFO    *pdevInfo;

	pdevInfo = globDrvInfo.pDevInfoArray[devIndex];
	
	co.offset=address;
	co.size = 1;
	co.value = value;

#ifdef OWL_PB42	
       if(!isHowlAP(devIndex)){
#endif
	 if (ioctl(pdevInfo->hDevice,DK_IOCTL_CFG_WRITE,&co) < 0) {
		uiPrintf("Error: PCI Config write failed \n");
	 } 
#ifdef OWL_PB42		 
       }
#endif
       return;
}

/**************************************************************************
* hwCfgWrite16 - write a 16 bit value
*
* This routine will call into the simulation environment to activate a
* 16 bit PCI configuration write cycle
*
* RETURNS: N/A
*/
void hwCfgWrite16
(
 	A_UINT16 devIndex,
	A_UINT32 address,                    /* the address to write */
	A_UINT16  value                        /* value to write */
)
{
	struct cfg_op co;
	MDK_WLAN_DEV_INFO    *pdevInfo;

	pdevInfo = globDrvInfo.pDevInfoArray[devIndex];
	
	co.offset=address;
	co.size = 2;
	co.value = value;

#ifdef OWL_PB42	
       if(!isHowlAP(devIndex)){
#endif
	  if (ioctl(pdevInfo->hDevice,DK_IOCTL_CFG_WRITE,&co) < 0) {
		uiPrintf("Error: PCI Config write failed \n");
	  } 
#ifdef OWL_PB42	
       }
#endif
       return;	
}
/**************************************************************************
* hwCfgWrite32 - write a 32 bit value
*
* This routine will call into the driver to activate a
* 32 bit PCI configuration write.
*
* RETURNS: N/A
*/
void hwCfgWrite32
(
	A_UINT16 devIndex,
	A_UINT32  offset,                    /* the address to write */
	A_UINT32  value                        /* value to write */
)
{
	struct cfg_op co;
	A_UINT32 data;
	MDK_WLAN_DEV_INFO    *pdevInfo;

	pdevInfo =globDrvInfo.pDevInfoArray[devIndex] ;

	co.offset=offset;
	co.size = 4;
	co.value = value;

#ifdef OWL_PB42	
       if(!isHowlAP(devIndex)){
#endif
	if (ioctl(pdevInfo->hDevice,DK_IOCTL_CFG_WRITE,&co) < 0) {
		uiPrintf("Error: PCI Config write failed \n");
	}
#ifdef OWL_PB42	 
       }	
#endif
	return;
}

#ifdef OWL_PB42	
/**************************************************************************
* hwRtcRegWrite - write a 32 bit value
*
* This routine will call into the driver to activate a
* 32 bit PCI configuration write.
*
* RETURNS: N/A
*/
void hwRtcRegWrite
(
        A_UINT16 devIndex,
        A_UINT32  offset,                    /* the address to write */
        A_UINT32  value                        /* value to write */
)
{
        struct cfg_op co;
        A_UINT32 data;
        MDK_WLAN_DEV_INFO    *pdevInfo;

        pdevInfo =globDrvInfo.pDevInfoArray[devIndex] ;

        co.offset=offset;
        co.size = 4;
        co.value = value;

       if(isHowlAP(devIndex)){
        if (ioctl(pdevInfo->hDevice,DK_IOCTL_RTC_REG_WRITE,&co) < 0) {
                uiPrintf("Error: RTC reg write failed \n");
        }
       }
        return;
}
#endif

#ifdef ART_BUILD
/**************************************************************************
* hwCreateEvent - Handle event creation within windows environment
*
* Create an event within windows environment
*
*
* RETURNS: 0 on success, -1 on error
*/
A_INT16 hwCreateEvent
(
	A_UINT16 devIndex,
    A_UINT32 type, 
    A_UINT32 persistent, 
    A_UINT32 param1,
    A_UINT32 param2,
    A_UINT32 param3,
    EVT_HANDLE eventHandle
)
{
    MDK_WLAN_DEV_INFO *pdevInfo;
	A_BOOL status; 
	struct event_op event;

	    pdevInfo = globDrvInfo.pDevInfoArray[devIndex];

		event.valid = 1;
		event.param[0] = type;
		event.param[1] = persistent;
		event.param[2] = param1;
		event.param[3] = param2;
		event.param[4] = param3;
		event.param[5] = (eventHandle.f2Handle << 16) | eventHandle.eventID;

		if (ioctl(pdevInfo->hDevice,DK_IOCTL_CREATE_EVENT,&event) < 0) {
			uiPrintf("Error:Create Event failed \n");
			return -1;
		}

	   return(0);
}

A_UINT16 getNextEvent
(
	A_UINT16 devIndex,
	EVENT_STRUCT *pEvent
)
{
	MDK_WLAN_DEV_INFO *pdevInfo;
	A_BOOL status; 
	struct event_op event;
	A_UINT32 i;

	    pdevInfo = globDrvInfo.pDevInfoArray[devIndex];

		event.valid = 0;
		pEvent->type = 0;

		if (ioctl(pdevInfo->hDevice,DK_IOCTL_GET_NEXT_EVENT,&event) < 0) {
			uiPrintf("Error:Get next Event failed \n");
			return (A_UINT16)-1;
		}

		if (!event.valid) {
			return 0;
		}

		pEvent->type = event.param[0];
		pEvent->persistent = event.param[1];
		pEvent->param1 = event.param[2];
		pEvent->param2 = event.param[3];
		pEvent->param3 = event.param[4];
		pEvent->eventHandle.eventID = event.param[5] & 0xffff;
		pEvent->eventHandle.f2Handle = (event.param[5] >> 16 ) & 0xffff;
		for (i=0;i<6;i++) {
			pEvent->result[i] = event.param[6+i];
		}
		
   		 return(1);
}

#else // ART_BUILD
/**************************************************************************
* hwCreateEvent - Handle event creation within windows environment
*
* Create an event within windows environment
*
*
* RETURNS: 0 on success, -1 on error
*/

A_INT16 hwCreateEvent
(
	A_UINT16 devIndex,
	PIPE_CMD *pCmd
)
{
	struct event_op event;
	MDK_WLAN_DEV_INFO    *pdevInfo;

		pdevInfo = globDrvInfo.pDevInfoArray[devIndex];

		event.valid = 1;
		event.param[0] = pCmd->CMD_U.CREATE_EVENT_CMD.type;
		event.param[1] = pCmd->CMD_U.CREATE_EVENT_CMD.persistent;
		event.param[2] = pCmd->CMD_U.CREATE_EVENT_CMD.param1;
		event.param[3] = pCmd->CMD_U.CREATE_EVENT_CMD.param2;
		event.param[4] = pCmd->CMD_U.CREATE_EVENT_CMD.param3;
		event.param[5] = (pCmd->CMD_U.CREATE_EVENT_CMD.eventHandle.f2Handle << 16) | pCmd->CMD_U.CREATE_EVENT_CMD.eventHandle.eventID;

		if (ioctl(pdevInfo->hDevice,DK_IOCTL_CREATE_EVENT,&event) < 0) {
			uiPrintf("Error:Create Event failed \n");
			return -1;
		}

	   return(0);
}

A_UINT16 getNextEvent
(
	MDK_WLAN_DEV_INFO *pdevInfo,
	EVENT_STRUCT *pEvent
)
{
	struct event_op event;
	A_INT32 i;

		event.valid = 0;
		pEvent->type = 0;

		if (ioctl(pdevInfo->hDevice,DK_IOCTL_GET_NEXT_EVENT,&event) < 0) {
			uiPrintf("Error:Get next Event failed \n");
			return (A_UINT16)-1;
		}

		if (!event.valid) {
			return 0;
		}

		pEvent->type = event.param[0];
		pEvent->persistent = event.param[1];
		pEvent->param1 = event.param[2];
		pEvent->param2 = event.param[3];
		pEvent->param3 = event.param[4];
		pEvent->eventHandle.eventID = event.param[5] & 0xffff;
		pEvent->eventHandle.f2Handle = (event.param[5] >> 16 ) & 0xffff;
		pEvent->result = event.param[6];
#ifdef MAUI
		for (i=0;i<5;i++) {
			pEvent->additionalParams[i] = event.param[7+i];
		}
#endif
		
   		 return(1);
}



A_INT16 hwGetNextEvent
(
	A_UINT16 devIndex,
	void *pBuf
)
{
	MDK_WLAN_DEV_INFO    *pdevInfo;

	pdevInfo = globDrvInfo.pDevInfoArray[devIndex];
	return getNextEvent(pdevInfo, (EVENT_STRUCT *)pBuf);
}

#endif  // else ART_BUILD

HANDLE open_device(A_UINT32 device_fn, A_UINT32 instance, char * pipeName) {
   char        dev_name[16];
   HANDLE       hDevice;

    if (device_fn == WMAC_FUNCTION) {
       strcpy(dev_name,"/dev/dk");
       dev_name[7]='0'+instance;
       dev_name[8]='\0';
    }
    else
    if (device_fn == SDIO_FUNCTION) {
       strcpy(dev_name,"/dev/dksdio");
       dev_name[11]='0'+instance;
       dev_name[12]='\0';
	}
    else {
       strcpy(dev_name,"/dev/dk_uart");
       dev_name[12]='0'+ (instance - (device_fn * MDK_MAX_NUM_DEVICES));
       dev_name[13]='\0';
    }
    uiPrintf("Opening device %s\n", dev_name);

	hDevice = open(dev_name,O_RDWR|O_SYNC);

	if (hDevice < 0) {
       uiPrintf("@Error opening device %s:%d\n", dev_name, hDevice);
	}
    return hDevice;

}

A_STATUS get_version_info(HANDLE hDevice, PDRV_VERSION_INFO pDrvVer) {
        A_UINT32 version;

	if (ioctl(hDevice,DK_IOCTL_GET_VERSION,&version) < 0) {
        uiPrintf("Error: get version ioctl failed !\n");
        return(A_ERROR);
	}

    pDrvVer->minorVersion = version & 0xffff;
    pDrvVer->majorVersion = (version>>16) & 0xffff;
    if (pDrvVer->majorVersion != DRV_MAJOR_VERSION) {
		uiPrintf("Error: Driver (%d) and application (%d) version mismatch \n");
        return(A_ERROR);
	}
    return A_OK;
}

A_STATUS get_device_client_info(MDK_WLAN_DEV_INFO *pdevInfo, PDRV_VERSION_INFO pDrvVer, PCLI_INFO cliInfo) {

   A_INT32       hDevice;
   A_UINT32 iIndex;
   struct client_info linux_cliInfo;
   A_UINT32      *vir_addr;
   A_UINT32  *mem_vir_addr;
   A_STATUS status;


   /* open the device based on the device index */
   if (pdevInfo->pdkInfo->instance > (WLAN_MAX_DEV-1)) {
	    A_FREE(pdevInfo->pdkInfo);
        A_FREE(pdevInfo);
        uiPrintf("Error: Only 4 devices/functions supported !\n");
        return(A_ERROR);
	}

    hDevice = open_device(pdevInfo->pdkInfo->device_fn, pdevInfo->pdkInfo->instance, NULL);
    if (hDevice == A_ERROR) {
        uiPrintf("Error: Unable to open the device !\n");
	    A_FREE(pdevInfo->pdkInfo);
        A_FREE(pdevInfo);
        return(A_ERROR);
    }
	
    if ((status=get_version_info(hDevice, pDrvVer)) != A_OK) {
		close(hDevice);
        A_FREE(pdevInfo->pdkInfo);
        A_FREE(pdevInfo);
        return status;
    }
	memcpy(&driverVer, pDrvVer, sizeof(DRV_VERSION_INFO));
	q_uiPrintf("pDrv Maj Ver=%d:Min Ver=%d\n", pDrvVer->majorVersion, pDrvVer->minorVersion);
	q_uiPrintf("driverVer Maj Ver=%d:Min Ver=%d\n", driverVer.majorVersion, driverVer.minorVersion);

	if (ioctl(hDevice,DK_IOCTL_GET_CLIENT_INFO,&linux_cliInfo) < 0) {
		close(hDevice);
        uiPrintf("Error: get version ioctl failed !\n");
        return(A_ERROR);
    }
		q_uiPrintf("rpa=%lx:rr=%x:nB=%d:mpa=%lx:ms=%x:irq=%x:dma_addr=%x\n", linux_cliInfo.reg_phy_addr, linux_cliInfo.reg_range, linux_cliInfo.numBars, linux_cliInfo.mem_phy_addr, linux_cliInfo.mem_size, linux_cliInfo.irq, linux_cliInfo.dma_mem_addr);
    if (pDrvVer->minorVersion >= 2) {
      for(iIndex=0;iIndex<linux_cliInfo.numBars;iIndex++) {
	     vir_addr = (A_UINT32 *)mmap((char *)0,linux_cliInfo.areg_range[iIndex],PROT_READ|PROT_WRITE,MAP_SHARED,hDevice,linux_cliInfo.areg_phy_addr[iIndex]);
         cliInfo->aregVirAddr[iIndex] = (A_UINT_PTR) vir_addr;
	     if (cliInfo->aregVirAddr[iIndex] == 0) {
		   uiPrintf("Error: Cannot map the device registers in user address space \n");
		   close(hDevice);
	       A_FREE(pdevInfo->pdkInfo);
           A_FREE(pdevInfo);
		   return A_ERROR;
	   }
         q_uiPrintf("reg vir addr[%d]=%x\n", iIndex, cliInfo->aregVirAddr[iIndex]);
         q_uiPrintf("reg range[%d]=%x\n", iIndex, linux_cliInfo.areg_range[iIndex]);
      }
    }
    else {
         vir_addr = (A_UINT32 *)mmap((char *)0,linux_cliInfo.reg_range,PROT_READ|PROT_WRITE,MAP_SHARED,hDevice,linux_cliInfo.reg_phy_addr);
         cliInfo->aregVirAddr[0] = (A_UINT_PTR) vir_addr;
         if (cliInfo->aregVirAddr[0] == 0) {
            close(hDevice);
            uiPrintf("Error: Cannot map the device registers in user address space \n");
            return A_ERROR;
         }
    }


	mem_vir_addr = (A_UINT32 *)mmap((char *)0,linux_cliInfo.mem_size,PROT_READ|PROT_WRITE,MAP_SHARED,hDevice,linux_cliInfo.mem_phy_addr);
	if (mem_vir_addr == NULL) {
		uiPrintf("Error: Cannot map memory in user address space \n");
		if (munmap((void *)linux_cliInfo.reg_phy_addr, linux_cliInfo.reg_range) == -1) {
                uiPrintf("Error: munmap to address %x:range=%x: failed with error %s\n", linux_cliInfo.reg_phy_addr,  linux_cliInfo.reg_range, strerror(errno));
        }
		close(hDevice);
	    A_FREE(pdevInfo->pdkInfo);
        A_FREE(pdevInfo);
		return A_ERROR;
	}
    q_uiPrintf("memsize=%x:mem phy addr = %x: mem vir addr=%x\n", linux_cliInfo.mem_size, linux_cliInfo.mem_phy_addr, mem_vir_addr);

	pdevInfo->hDevice = hDevice;
    //printf("open handle = %d \n",pdevInfo->hDevice);

    // Copy from driver client info structure 
    cliInfo->regPhyAddr = linux_cliInfo.areg_phy_addr[0];
    cliInfo->regVirAddr = cliInfo->aregVirAddr[0];
    cliInfo->memPhyAddr = linux_cliInfo.mem_phy_addr;
    cliInfo->memVirAddr = (A_UINT_PTR) mem_vir_addr;
    cliInfo->irqLevel = linux_cliInfo.irq;
    cliInfo->regRange = linux_cliInfo.reg_range;
    cliInfo->memSize = linux_cliInfo.mem_size;
    memcpy(cliInfo->aregPhyAddr, linux_cliInfo.areg_phy_addr, sizeof(cliInfo->aregPhyAddr));
    memcpy(cliInfo->aregRange, linux_cliInfo.areg_range, sizeof(cliInfo->aregRange));
    cliInfo->numBars = linux_cliInfo.numBars;
    cliInfo->dma_mem_addr = linux_cliInfo.dma_mem_addr;   
    return A_OK;

} // end of get_device_client_info


/**************************************************************************
* milliSleep - sleep for the specified number of milliseconds
*
* This routine calls a OS specific routine for sleeping
* 
* RETURNS: N/A
*/

void milliSleep
	(
	A_UINT32 millitime
	)
{
	usleep(millitime*1000);
}

A_UINT32 milliTime
(
 	void
)
{
	struct timeval tv;
	A_UINT32 millisec;

	if (gettimeofday(&tv,NULL) < 0) {
			millisec = 0;
	} else {
		millisec = tv.tv_sec * 1000 + tv.tv_usec / 1000;
	}

	return millisec;
}

char *strupr(char* str) {
        A_UINT32 iIndex, iLen;
        iLen = strlen(str);
        for(iIndex=0; iIndex<iLen; iIndex++) {
                str[iIndex] = toupper(str[iIndex]);
        }
        return str;
}

void close_device(MDK_WLAN_DEV_INFO *pdevInfo) {
    A_UINT32 iIndex;

        //printf("Closing handle = %x \n",pdevInfo->hDevice);
 //   uiPrintf("close_device::regrange=%x:reg vir addr=%x\n", pdevInfo->pdkInfo->regMapRange, pdevInfo->pdkInfo->regVirAddr);
  //  uiPrintf("close_device::memsize=%x:mem vir addr=%x\n", pdevInfo->pdkInfo->memSize, pdevInfo->pdkInfo->memVirAddr);
        
		 //printf("hD=%d:minVer=%d\n", pdevInfo->hDevice, driverVer.minorVersion);
      if (pdevInfo->hDevice>0)  {
         if (driverVer.minorVersion >= 2) {
            for(iIndex=0; iIndex<pdevInfo->pdkInfo->numBars; iIndex++) {
	          if (munmap((void *)pdevInfo->pdkInfo->aregVirAddr[iIndex], pdevInfo->pdkInfo->aregRange[iIndex]) == -1)
                 uiPrintf("Error: munmap to address %x:range=%x: failed with error %s\n", pdevInfo->pdkInfo->aregVirAddr[iIndex],  pdevInfo->pdkInfo->aregRange[iIndex], strerror(errno));
            }
         }
         else {
	       if (munmap((void *)pdevInfo->pdkInfo->regVirAddr, pdevInfo->pdkInfo->regMapRange) == -1)
              uiPrintf("Error: munmap to address %x:range=%x: failed with error %s\n", pdevInfo->pdkInfo->regVirAddr,  pdevInfo->pdkInfo->regMapRange, strerror(errno));
         }
	     if (munmap((void *)pdevInfo->pdkInfo->memVirAddr, pdevInfo->pdkInfo->memSize) == -1)
              uiPrintf("Error: munmap to address %x:range=%x: failed with error %s\n", pdevInfo->pdkInfo->memVirAddr,  pdevInfo->pdkInfo->memSize, strerror(errno));
         close(pdevInfo->hDevice);
      }
}
void close_driver() {
}


