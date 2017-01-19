/*
 *  Copyright ?2005 Atheros Communications, Inc.,  All Rights Reserved.
 *
 */


#include <errno.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "wlantype.h"
#include "dk_common.h"
#include "dk_cmds.h"
#include "art_utf_common.h"
#include "tlvCmd_if.h"
#include "Device.h"
#ifdef _WINDOWS
#include "AnwiDriverInterface.h"
#include "MyDelay.h"
#endif
#include "DevSetConfig.h"
#include "Qc9KEeprom.h"
#include "Qc9KDevice.h"
#include "DevDeviceFunction.h"
#include "UserPrint.h"
#include "import.h"
#include "smatch.h"
#include "Field.h"
#include "Qc98xxEepromSave.h"

#define MDCU 10			// should we only set the first 8??
#define MQCU 10
#define MAX_FIELD_ENTRIES   100   // Wild guess on unknown # of fields

#ifdef _WINDOWS
extern A_UINT32 loadTarget_ENE(void);
extern A_BOOL IsWinXp();
extern A_BOOL IsWin7();
#endif 

#define SDIO_DEVICE_DESCRIPTION	"SDIO "
#define ETH_DEVICE_DESCRIPTION		"ETH"
#define PCI_DEVICE_DESCRIPTION "PCI"

A_UINT8 *pQc9kEepromArea = NULL;
A_UINT8 *pQc9kEepromBoardArea = NULL;

static char hwID[256] = {0};

MANLIB_API A_BOOL loadTarget()
{
#ifdef _WINDOWS
#else
#ifdef COMMENT_4NOW
    char loadCmd[300];
    char tempStr[100];
    
    strcpy (loadCmd, configSetup.loadFileCmd);
    if (configSetup.devdrvInterface == DEVDRV_INTERFACE_USB)
    {
        strcat (loadCmd, " -i USB");
    }
    else if (configSetup.devdrvInterface != DEVDRV_INTERFACE_SDIO)
    {
        UserPrint("ERROR - DEVDRV_INTERFACE = %d is not supported\n", configSetup.devdrvInterface);
        return FALSE;
    }
    else
    {
        strcat (loadCmd, " -i SDIO");
    }
    
    if (configSetup.eepromFile[0] != '\0')
    {
        strcat (loadCmd, " -e ");
        strcat (loadCmd, configSetup.eepromFile);
    }
    else if (configSetup.eepromBoardFile[0] != '\0')
    {
        strcat (loadCmd, " -e ");
        strcat (loadCmd, configSetup.eepromBoardFile);
    }
    else
    {
        UserPrint("ERROR - no eeprom/board file is set\n");
        return FALSE;
    }
    
    if (configSetup.loadFileArg[0] != '\0')
    {
        strcat (loadCmd, " -l ");
        strcat (loadCmd, configSetup.loadFileArg);
    }
    
    // set ref clock
    if (configSetup.refClockHz)
    {
        itoa (configSetup.refClockHz, tempStr, 10);
        strcat (loadCmd, " -c ");
        strcat (loadCmd, tempStr);
    }
    // if load otp
    if (DeviceCalibrationDataGet() == DeviceCalibrationDataOtp)
    {
        strcat (loadCmd, " -o 1");
    }
    // do memory test
    if (configSetup.doMemoryTest != 0)
    {
        strcat (loadCmd, " -m 1");
    }
    
    UserPrint("system call \"%s\"\n", loadCmd);
    system (loadCmd);
#endif //COMMENT_4NOW
#endif  //_WINDOWS
    return TRUE;
}

MANLIB_API A_BOOL initTarget()
{
#if 0
#ifdef _WINDOWS
    if (IsWinXp() && ((configSetup.devdrvInterface == DEVDRV_INTERFACE_SDIO) || (configSetup.devdrvInterface == DEVDRV_INTERFACE_ETH)))
    {
		if (!loadTarget_ENE())
		{
			return FALSE;
		}
    } else {
        /* set board path for Windows */
        strcpy(configSetup.boardDataPath,"..\\driver\\boardData\\");
    }
#endif
#endif //0
    return TRUE;
}
 
MANLIB_API A_BOOL endTarget()
{
#if 0
#ifdef _WINDOWS
    if (IsWinXp() && ((configSetup.devdrvInterface == DEVDRV_INTERFACE_SDIO) || (configSetup.devdrvInterface == DEVDRV_INTERFACE_ETH)))
    {
		if (!EndAR6000_ene(0))
		{
			return FALSE;
		}
    }
#endif
#endif //0
    return TRUE;
}

MANLIB_API A_BOOL closeTarget()
{
    A_UINT32 subSystemID;

    // If REMOTE_DEVICE is defined in artsetup.txt then use dutSSID
    // This is to eliminate \remote=sdio \id=xxxx in the command line
    subSystemID = configSetup.dutSSID;

#ifdef _WINDOWS
    //if (IsWinXp() && ((configSetup.devdrvInterface == DEVDRV_INTERFACE_SDIO) || (configSetup.devdrvInterface == DEVDRV_INTERFACE_ETH)))
    //{
	//	if (!loadDriver_ENE(FALSE, subSystemID))
	//	{
	//		return FALSE;
	//	}
    //}
#else
    // TRANG system("./unloadTgt.sh"); 
#endif  //_WINDOWS
    return TRUE;
}

int Qc9KCardLoad ()
{
#ifndef CTR_HOSTIO
    if (!loadTarget())
    {
        return -1;
    }
    if (!initTarget())
    {
        closeTarget();
        return -1;
    }
#endif
    //if (art_initF2(1, &pdkInfo) < 0)
    if (art_initF2(1) < 0)
    {
        return -1;
    }
    //return (art_createEvent(0, ISR_INTERRUPT, 1, 0, 0, 0));
	return 0;
}

int Qc9KCardRemove()
{
    art_teardownDevice(0);
    //teardownDevice(0);
    endTarget();
    closeTarget();
    //closeEnvironment();
    return 0;
}

int Qc9KDeviceIdGet(void)
{
	unsigned int address, value;
	int devid;

	address=0;
    value=art_cfgRead(address);
	devid=((value>>16)&0xffff);

	return devid;
}

int Qc9KMemoryRead(unsigned int address, unsigned int *buffer, int many)
{
    if (many <= sizeof(A_UINT32))
    {
        *buffer = art_mem32Read (address);
    }
    else
    {
		art_memRead (address, (A_UCHAR *)buffer, (A_UINT32)many);
    }
    return 0;
}

int Qc9KMemoryWrite(unsigned int address, unsigned int *buffer, int many)
{
	if (many <= sizeof(A_UINT32))
	{
		art_mem32Write (address, *buffer);
	}
	else
	{
		art_memWrite(address, (A_UCHAR *)buffer, many);
	}
	return 0;
}

int Qc9KRegisterRead(unsigned int address, unsigned int *value)
{
    *value = art_regRead (address);
    return 0;
}

int Qc9KRegisterWrite(unsigned int address, unsigned int value)
{
    return (art_regWrite (address, value));
}

void Qc9KUserPrintConsoleSet(int onoff)
{
    UserPrintConsole(onoff);
}

int Qc9KStickyWrite(int idx, unsigned int address, int low, int  high, unsigned int *value, int numVal, int prepost)
{
	int i;
    A_UINT32 mask, entryLen, mode;
    static A_UINT32 buffer[61];     //(255-8)/4
    static A_UINT32 *pBuf = buffer;
    static A_UINT32 count = 0;
    static A_UINT32 bufLen = 0;
    A_UINT32 *ptr;
    A_UINT32 num;

    if (address == 0 && low == 0 && high == 0 && value == 0)
    {
        // no more entry, send to UTF
        if (count)
        {
            art_stickyWrite(count, (unsigned char *)buffer, bufLen);
            memset(buffer, 0, sizeof(buffer));
            pBuf = buffer;
            bufLen = 0;
            count = 0;
        }
        return 0;
    }
    // compute length of this entry (numVal + address + mask) * 4-byte
    entryLen = (numVal + 2) * sizeof(A_UINT32);
    if ((bufLen + entryLen) > sizeof(buffer))
    {
        art_stickyWrite(count, (unsigned char *)buffer, bufLen);
        memset(buffer, 0, sizeof(buffer));
        pBuf = buffer;
        bufLen = 0;
        count = 0;
    }
    if (numVal != 1 && numVal != 2 && numVal != 5)
    {
        UserPrint("Qc9KStickyWrite - There must be 1 2 or 5 values\n");
        return -1;
    }
    mode = (numVal == 5) ? OTP_CONFIG_MODE_5MODAL : ((numVal == 2) ? OTP_CONFIG_MODE_2MODAL : OTP_CONFIG_MODE_COMMON);
    address |= (mode << CONFIG_ADDR_MODE_SHIFT) | (prepost << CONFIG_ADDR_CTRL_SHIFT);
#ifdef AP_BUILD
    {
        A_UINT32 swappedByte4;
        swappedByte4 = SWAP32(address);
        address = swappedByte4;
    }
#endif

    mask = 0;
    for (i = low; i <= high; ++i)
    {
        mask |= (1 << i);
    }
#ifdef AP_BUILD
    {
        A_UINT32 swappedByte4;
        swappedByte4 = SWAP32(mask);
        mask = swappedByte4;
    }
#endif

    // check for duplicate entry
    ptr = buffer;
    while (*ptr)
    {
        num = (address & CONFIG_ADDR_MODE_MASK) >> CONFIG_ADDR_MODE_SHIFT;
        num = GET_LENGTH_CONFIG_ENTRY_32B(num) + 1;
        if (*ptr == address && *(ptr+1) == mask)
        {
            return 0;
        }
        ptr += num;
    }

    *pBuf++ = address;       // 4-byte address
    *pBuf++ = mask;             // 4-byte mask
    for (i = 0; i < numVal; ++i)
    {
        A_UINT32 byte4;
        
        byte4 = value[i] << low;         // 4-byte value
#ifdef AP_BUILD
        {
            A_UINT32 swappedByte4;
            swappedByte4 = SWAP32(byte4);
            byte4 = swappedByte4;
        }
#endif
        *pBuf++ = byte4;         // 4-byte value

    }
    UserPrint("sticky write 0x%08x[%d:%d,0x%08x](%s)<- ",
				((address & CONFIG_ADDR_ADDRESS_MASK) << CONFIG_ADDR_ADDRESS_SHIFT),
				high, low, mask, (prepost==0 ? "pre" : "post"));
    for (i = 0; i < numVal; ++i)
    {
        UserPrint("0x%08x ", (value[i] << low));
    }
    UserPrint("\n");
    bufLen += entryLen;
	count++;
    
	return 0;
}

int Qc9KStickyClear(int idx, unsigned int address, int low, int  high)
{
	int i;
    A_UINT32 mask;
    static A_UINT32 buffer[61];     //(255-8)/4
    static A_UINT32 *pBuf = buffer;
    static A_UINT32 count = 0;
    A_UINT32 *ptr;

    // clear all sticky entries
    if (address == 0xffffffff)
    {
        art_stickyClear(0xffffffff, NULL, 0);
        pBuf = buffer;
        count = 0;
        return 0;
    }

    // send all entries in buffer
    if (address == 0 && low == 0 && high == 0)
    {
        if (count)
        {
            art_stickyClear(count, (unsigned char *)buffer, count*2*sizeof(A_UINT32));
            memset(buffer, 0, sizeof(buffer));
            pBuf = buffer;
            count = 0;
        }
        return 0;
    }
    
    if (((count+1)*2*sizeof(A_UINT32)) > sizeof(buffer))
    {
        art_stickyClear(count, (unsigned char *)buffer, count*2*sizeof(A_UINT32));
        memset(buffer, 0, sizeof(buffer));
        pBuf = buffer;
        count = 0;
    }
    mask = 0;
    for (i = low; i <= high; ++i)
    {
        mask |= (1 << i);
    }
    // check for duplicate entry
    ptr = buffer;
    while (*ptr)
    {
        if (*ptr++ == address && *(ptr++) == mask)
        {
            return 0;
        }
    }

#ifdef AP_BUILD
    {
        A_UINT32 swappedByte4;
        swappedByte4 = SWAP32(address);
        address = swappedByte4;
        swappedByte4 = SWAP32(mask);
        mask = swappedByte4;
    }
#endif
    *pBuf++ = address;       // 4-byte address
    *pBuf++ = mask;             // 4-byte mask
	UserPrint("sticky clear %08x[%d,%d] (%08x)\n",address, high,low, mask);
	count++;
	return 0;
}

int Qc9KChipIdentify(void)
{
#if defined(LINUX) || defined(__APPLE__)
#ifdef COMMENT_4NOW
    char loadCmd[200];
    int error, devid;
    char *p;

    
    strcpy (loadCmd, configSetup.loadFileCmd);
    if (configSetup.devdrvInterface == 4)
    {
        strcat (loadCmd, " -i USB");
    }
    else if (configSetup.devdrvInterface == 1)
    {
        strcat (loadCmd, " -i SDIO");
    }
    else
    {
        UserPrint("ERROR - DEVDRV_INTERFACE = %d is not supported\n", configSetup.devdrvInterface);
        return 0;
    }
    
    if (configSetup.loadFileArg[0] != '\0')
    {
        strcat (loadCmd, " -l ");
        strcat (loadCmd, configSetup.loadFileArg);
    }
    else
    {
        strcat (loadCmd, " -l ");
        strcat (loadCmd, "loadHostDriver.sh");
    }
        
    UserPrint("system call \"%s\"\n", loadCmd);
    error = system (loadCmd);
    if (error)
    {
        UserPrint("Qc9KChipIdentify - error = %d\n", error);
        return 0;
    }
    
    devid = 0;
    p = getenv ("TARGET_TYPE");
    if (p)
    {
        UserPrint ("Qc9KChipIdentify - TARGET_TYPE=%s\n", p);
        if (strcmp(p, "AR6004") == 0)
        {
            devid = 0x3c;
        }
    }
    return devid;
#endif //COMMENT_4NOW
    return 0x3c;
#endif //LINUX  

#ifdef _WINDOWS

    return Qc9KCheckDeviceID();
#endif
    
    return 0;
}

#ifdef _WINDOWS
#define DEVCON_FILE_NAME	"devcon.exe"
#define DEVCON_BUFFER		4096 

int Qc9KGetDeviceName (char *strDeviceName)
{
    if (configSetup.devdrvInterface == DEVDRV_INTERFACE_SDIO)
    {
        strcpy(strDeviceName, SDIO_DEVICE_DESCRIPTION);
    }
	else if (configSetup.devdrvInterface == DEVDRV_INTERFACE_ETH)
	{
		strcpy(strDeviceName, ETH_DEVICE_DESCRIPTION);
	}
    else if (configSetup.devdrvInterface == DEVDRV_INTERFACE_USB)
    {
		DevDeviceCustomNameGet (strDeviceName);
    }
	else if (configSetup.devdrvInterface == DEVDRV_INTERFACE_PCI)
	{
		DevDeviceCustomNameGet (strDeviceName);
		if (strDeviceName[0] == '\0')
		{
			strcpy(strDeviceName, PCI_DEVICE_DESCRIPTION);
		}
	}
    else
    {
        UserPrint("Invalid interface. Only SDIO and USB are supported.\n");
        return -1;
    }
	return 0;
}

static int Ar6kGetHwID(char* hwID, char* strData, DWORD dwBytes)
{
    char* pStart = 0;
    char* pEnd = 0;
    DWORD cnt = 0;
    char strDeviceName[DEVCON_BUFFER];
    
    /* 
     * Structure:
     *   HW ID     Description
     * XXXXXXXXX : YYYYYYYYYYY
     */
	Qc9KGetDeviceName(strDeviceName);
    pEnd = strstr(strData, strDeviceName);
    
    if(!pEnd)return -1;	//Keyword not found
    
    while(pEnd>=strData){
    	if(*pEnd != ':')pEnd--;
	else break;
    }
    pEnd--;
    if(pEnd<strData)return -1;
    while(pEnd>=strData){
    	if(*pEnd == ' ')pEnd--;
	else break;
    }
    if(pEnd<strData)return -1;
    pStart = pEnd;

    while(pStart>=strData){
    	if((*pStart != '\r') && (*pStart != '\n'))pStart--;
	else break;
    }
    pStart++;
    cnt = (DWORD)(pEnd) - (DWORD)(pStart) + 1;
    strncpy(hwID, pStart, cnt);
    hwID[cnt] = '\0';
    
    UserPrint("Found Device HWID %s\r\n", hwID);
    
    /* 
     * Here is another issue:
     * devcon.exe do not take '&' character;
     * thus replace it with wildcard '*' and delete
     * the suceeding characters.
     */
    
    pEnd = strchr(hwID, '&');
    if(pEnd){
    	*pEnd = '*';
	pEnd++;
	*pEnd = '\0';
    }

    return 0;
}

static int RetrieveResult(char* cmdLine, char* strData, DWORD* pdwBytes, STARTUPINFO* psi, PROCESS_INFORMATION* ppi)
{
    SECURITY_ATTRIBUTES sa;
    HANDLE hSTDINWrite, hSTDINRead, hSTDINWriteup;
    HANDLE hSTDOUTWrite, hSTDOUTRead, hSTDOUTReadup;

    ZeroMemory( &sa, sizeof(sa) );
    sa.nLength= sizeof(SECURITY_ATTRIBUTES);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = TRUE;

    psi->cb = sizeof(STARTUPINFO);
    psi->dwFlags = STARTF_USESTDHANDLES;
    psi->wShowWindow = SW_HIDE;

    if(!CreatePipe(&hSTDOUTRead, &hSTDOUTWrite, &sa, 0)) 
    { 
	UserPrint( "Create STDOUT pipe failed.\r\n"); 
	return -1; 
    }
    
    if(!DuplicateHandle(GetCurrentProcess(), hSTDOUTRead, 
      GetCurrentProcess(), &hSTDOUTReadup, 0, FALSE, 
      DUPLICATE_SAME_ACCESS))
    { 
		UserPrint( "DuplicateHandle failed.\r\n"); 
		CloseHandle(hSTDOUTRead); 
		CloseHandle(hSTDOUTWrite); 
		return -1; 
    }
    
    CloseHandle(hSTDOUTRead); 
    
    if(!CreatePipe(&hSTDINRead, &hSTDINWrite, &sa, 0)) 
    { 
		UserPrint( "Create STDIN pipe failed.\r\n"); 
		CloseHandle(hSTDOUTWrite); 
		CloseHandle(hSTDOUTReadup); 
		return -1; 
    }
    
    if(!DuplicateHandle(GetCurrentProcess(), hSTDINWrite, 
      GetCurrentProcess(), &hSTDINWriteup, 0, FALSE, 
      DUPLICATE_SAME_ACCESS))
    { 
		UserPrint( "DuplicateHandle failed.\r\n"); 
		CloseHandle(hSTDOUTWrite); 
		CloseHandle(hSTDOUTReadup); 
		CloseHandle(hSTDINRead); 
		CloseHandle(hSTDINWrite); 
		return -1; 
    }
    
    CloseHandle(hSTDINWrite); 
    
    psi->hStdInput = hSTDINRead;
    psi->hStdOutput = hSTDOUTWrite;
    psi->hStdError = GetStdHandle(STD_ERROR_HANDLE);
    
    if(!CreateProcess(DEVCON_FILE_NAME, cmdLine ,NULL, NULL, TRUE, 0,	
     NULL, NULL, psi, ppi))
    {
		UserPrint("devcon execution failed\r\n");
		CloseHandle(hSTDOUTWrite); 
		CloseHandle(hSTDOUTReadup); 
		CloseHandle(hSTDINRead); 
		CloseHandle(hSTDINWriteup);
		return -1;
    }
    	
    CloseHandle(hSTDINRead); 
    CloseHandle(hSTDOUTWrite);

	while(TRUE)
    {
    	char buf[1024];
		DWORD cnt=0;	
		if(!ReadFile(hSTDOUTReadup, buf, sizeof(buf), &cnt, NULL)|| !cnt)
		{
			if (GetLastError() == ERROR_BROKEN_PIPE){
               break; // pipe done - normal exit path.
			}
		}
		if(*pdwBytes + cnt > DEVCON_BUFFER)
		{
			UserPrint("devcon buffer overflows\r\n");
			CloseHandle( ppi->hProcess );
			CloseHandle( ppi->hThread );	 
			CloseHandle(hSTDOUTReadup); 
			CloseHandle(hSTDINWriteup); 
			return -1;
		}	
		strncpy(strData+*pdwBytes, buf, cnt);
		*pdwBytes += cnt;
    }
    
    strData[*pdwBytes] = '\0';

    WaitForSingleObject( ppi->hProcess, INFINITE );
    
    CloseHandle( ppi->hProcess );
    CloseHandle( ppi->hThread );	 
    CloseHandle(hSTDOUTReadup); 
    CloseHandle(hSTDINWriteup); 

    return 0;
}

static int FileExists(const TCHAR *fileName)
{
    DWORD       fileAttr;

    fileAttr = GetFileAttributes(fileName);
    if (0xFFFFFFFF == fileAttr)
        return FALSE;
    return TRUE;
}

int Qc9KDisableDevice(void)
{
    char cmdLine[266];
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
               
    if (configSetup.devdrvInterface == DEVDRV_INTERFACE_ETH)
    {
		return 0;
	}

	ZeroMemory( &si, sizeof(si) );
    ZeroMemory( &pi, sizeof(pi) );
    
    sprintf(cmdLine, " disable %s", hwID);
    if(!CreateProcess(DEVCON_FILE_NAME, cmdLine ,NULL, NULL, FALSE, 0,	
     NULL, NULL, &si, &pi))
    {
	UserPrint("Disable device failed");
	return -1;
    }

    WaitForSingleObject( pi.hProcess, INFINITE );			
    CloseHandle( pi.hProcess );
    CloseHandle( pi.hThread );	
    
    return 0;
}

int Qc9KReEnableDevice(void)
{
    /*
     *  This function execute the external program
     *  "devcon.exe" to disable and then re-enable
     *  the device.
     *
     *  Note: Currently only search through USB devices.
     */
   
    char strData[DEVCON_BUFFER];
    char cmdLine[266];
    DWORD dwBytes = 0;
    DWORD dwRetryTimes = 100;
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
               
    if (configSetup.devdrvInterface == DEVDRV_INTERFACE_ETH)
    {
		return 0;
	}

    ZeroMemory( &si, sizeof(si) );
    ZeroMemory( &pi, sizeof(pi) );
    
    /* Check if devcon.exe exists */
    if(!FileExists(DEVCON_FILE_NAME)){
    	UserPrint("%s does not exist!\r\n", DEVCON_FILE_NAME);
	return -1;
    }
    
    if (configSetup.devdrvInterface == DEVDRV_INTERFACE_SDIO)
    {
        strcpy (cmdLine, " find SD*");
    }
    else if (configSetup.devdrvInterface == DEVDRV_INTERFACE_USB)
    {
        strcpy (cmdLine, " find USB*");
    }
	else if (configSetup.devdrvInterface == DEVDRV_INTERFACE_PCI)
	{
        strcpy (cmdLine, " find PCI*");
	}
    else
    {
        UserPrint("Invalid interface. Only SDIO and USB are supported.\n");
        return -1;
    }

    /* 1. Check the device hardware ID */
    if(RetrieveResult(cmdLine, strData, &dwBytes, &si, &pi)){
    	return -1;
    }
    
    //UserPrint(strData);
    
    if(Ar6kGetHwID(hwID, strData, dwBytes)){
    	UserPrint("Cannot get HW ID\r\n");
	return -1;
    }
	
    /* 2. Disable the device */
    sprintf(cmdLine, " disable %s", hwID);
    if(!CreateProcess(DEVCON_FILE_NAME, cmdLine ,NULL, NULL, FALSE, 0,	
     NULL, NULL, &si, &pi))
    {
	UserPrint("Disable device failed");
	return -1;
    }

    WaitForSingleObject( pi.hProcess, INFINITE );			
    CloseHandle( pi.hProcess );
    CloseHandle( pi.hThread );		
    
    /* 3. Check if the device is disabled */
    sprintf(cmdLine, " status %s", hwID);

    do{
    	dwBytes = 0;
    	if(RetrieveResult(cmdLine, strData, &dwBytes, &si, &pi)){
    		return -1;
    	}
    	dwRetryTimes--;
    	if(dwRetryTimes<=0)break;

    }while(!strstr(strData, "disabled"));

    if(dwRetryTimes<=0){
    	UserPrint("Disble device failed.\r\n");
    	return -1;
    }
    
    /* 4. Enable the device */
ENABLE_AGAIN:
    sprintf(cmdLine, " enable %s", hwID); 
    if(!CreateProcess(DEVCON_FILE_NAME, cmdLine ,NULL, NULL, FALSE, 0,	
     NULL, NULL, &si, &pi))
    {
	UserPrint("Enable device failed");
	return -1;
    }

    WaitForSingleObject( pi.hProcess, INFINITE );			
    CloseHandle( pi.hProcess );
    CloseHandle( pi.hThread );	

    /* 5. Check if the device is enabled */
    sprintf(cmdLine, " status %s", hwID);
    dwRetryTimes = 100;

    do{
    	dwBytes = 0;
    	if(RetrieveResult(cmdLine, strData, &dwBytes, &si, &pi)){
	    return -1;
	}
	dwRetryTimes--;
	if(dwRetryTimes<=0)break;

    }while(!strstr(strData, "running"));
    if(dwRetryTimes<=0){
    	UserPrint("Enable device failed. Try again\r\n");
    	goto ENABLE_AGAIN;
    }
	/*
	* WAR, some win7 PCs (link R400) need a delay, otherwise there is socket handler error.
	*/
	if (!IsWinXp())
	{
        MyDelay(1500);
	}

    if(dwRetryTimes<=0){
    	UserPrint("Re-enable device failed.\r\n");
    }else{
    	UserPrint("Device re-enabled.\r\n");
    }
    
    return 0;
}

int Qc9KCheckDeviceID(void)
{
    /* This function checks if device exists via "devcon.exe" */
    char strData[DEVCON_BUFFER];
    char strDeviceName[DEVCON_BUFFER];
    char cmdLine[266];
    DWORD dwBytes = 0;
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    int devid = 0;
               
    if (configSetup.devdrvInterface == DEVDRV_INTERFACE_ETH)
    {
		return 0;
	}

    ZeroMemory( &si, sizeof(si) );
    ZeroMemory( &pi, sizeof(pi) );
    
    /* Check if devcon.exe exists */
    if(!FileExists(DEVCON_FILE_NAME)){
    	UserPrint("%s does not exist!\r\n", DEVCON_FILE_NAME);
	   return 0;
    }
    
    /* 1. Check the device hardware ID */
    if (configSetup.devdrvInterface == DEVDRV_INTERFACE_SDIO)
    {
        strcpy (cmdLine, " find SD*");
    }
    else if (configSetup.devdrvInterface == DEVDRV_INTERFACE_USB)
    {
        strcpy (cmdLine, " find USB*");
    }
    else if (configSetup.devdrvInterface == DEVDRV_INTERFACE_PCI)
    {
        strcpy (cmdLine, " find PCI*");
    }
    else
    {
        UserPrint("Invalid interface. Only SDIO and USB are supported.\n");
        return 0;
    }
    if(RetrieveResult(cmdLine, strData, &dwBytes, &si, &pi)){
    	return 0;
    }
    
    //UserPrint(strData);
    Qc9KGetDeviceName(strDeviceName);
    
    if(strstr(strData, strDeviceName)){
        /* Found device */
        devid = 0x3c;
    }
              
    return devid;
}
#else /* _WINDOWS */
int Qc9KDisableDevice(void)
{
    //TODO in Linux...??
    return 0;
}
#endif /* _WINDOWS */

//Function - Qc9KTxGainTableRead_AddressHeader
//Purpose  - Retrive all field names associated with register name from input address
//Parameter- address : adress of entry
//           header  : buffer returned with regsiter name and associated field names for NART to display
//           max     : maximum size of input buffer for header
//Return   - number of fields
int Qc9KTxGainTableRead_AddressHeader(unsigned int address, char *header, char *subheader, int max)
{
	char  *rName, *fName;
	int low, high, i, lc=0, lc2=0, nc, nc2;

	for (i=0; i<MAX_FIELD_ENTRIES; i++)
	{
		if (FieldFindByAddressOnly(address, i, &high, &low, &rName, &fName) == -1)
		{
			break;
		}
		if (lc == 0) 
		{
			nc=SformatOutput(header,max-1, "|%s|32regValue|%s|",rName, fName);
			nc2=SformatOutput(subheader,max-1, "|Bit|31..0|%d..%d|",high,low);
		}
		else 
		{
			nc=SformatOutput((header+lc),max-lc-1, "%s|",fName);
			nc2=SformatOutput((subheader+lc2),max-lc2-1, "%d..%d|",high,low);
		}
		lc+=nc;
		lc2+=nc2;
	}
	return (i);
}

//Function - Qc9KTxGainTableRead_AddressValue
//Purpose  - Retrive field value from address along with nth field in case of multiple fields with same register name
//Parameter- address : adress of entry
//           idx     : index of field wish to retrive
//                   : -1 means getting whole 32 bit value regardless low and high bit address
//           rName   : returned register name for subsequent fucntion FiledRead call
//           fName   : returned field name for subsequent fucntion FiledRead call
//           value   : returned value of field value with low and high bit mask
//           low     : low address bit
//           high    : hgih address bit
//           max     : maximum size of input buffer for header
//Return   -  0 suceed
//           -1 failed
int Qc9KTxGainTableRead_AddressValue(unsigned int address, int idx, char *rName, char *fName, int *value, int *low, int *high)
{
	char rfName[256], *rPtr, *fPtr;

	if (FieldFindByAddressOnly(address, idx == -1 ? 0: idx, high, low, &rPtr, &fPtr) == -1)
	{
		return (-1);
	}
	strcpy(rName,rPtr);
	strcpy(fName,fPtr);
	SformatOutput(rfName,256-1, "%s.%s",rName, fName);
	if (idx == -1)
	{
		return(FieldReadNoMask(rfName,value));
	}
	else
	{
		return(FieldRead(rfName,value));
	}
}
