#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

#include "wlantype.h"
#include "DevSetConfig.h"
#include "SetConfig.h"
#include "UserPrint.h"

#if ((defined(LINUX) || defined(__APPLE__)) && !defined(HAVE_STRNICMP))
    #define _strnicmp   strncasecmp
#endif 

A_BOOL ParseSetConfig (SETCONFIG_HASH *lineBuf);

static char delimiters[]   = " \t";

MLD_CONFIG configSetup =
{
    "",								//machname
    0x0000,                         // dut SSID. an illegal value as default.
    "",                             // EEPROM_FILE filename
    1,                              // devdrvInterface: 1= SDIO, 2 = ETH, 3 = UART, 4 = USB, 5 = PCI
    0,                              // refClockHz
    0,	                            // doMemoryTest
    "",                             //eepromBoardFile
    "",								//loadFileCmd
    "",								//loadFileArg
    "",                             //customName
	"",								//driverPath
	"",								//boardDataPath
	1,								//checkSwVer
	0x00000000,						//SwVersion
	5,                              //calmem: 3=otp; 5=file
};

struct _ParameterList SetConfigParameter[]=
{
    SETCONFIG_EEPROM_FILE,
    SETCONFIG_EEPROM_BOARD_FILE,
    SETCONFIG_LOAD_FILE_CMD,
    SETCONFIG_DUT_CARD_SSID,
    SETCONFIG_DEVDRV_INTERFACE,
    SETCONFIG_REF_CLOCK_HZ,
    SETCONFIG_DO_MEMORY_TEST,
    SETCONFIG_CUSTOM_NAME,
    SETCONFIG_CAL_MEMORY,
};

int DevSetConfigParameterSplice(struct _ParameterList *list)
{
    list->nspecial=sizeof(SetConfigParameter)/sizeof(SetConfigParameter[0]);
    list->special=SetConfigParameter;
    return 0;
}

//
// parse and then set a configuration parameter in the internal structure
//
int DevSetConfigCommand(void *cmd)
{
	SETCONFIG_HASH *pSetConfig = (SETCONFIG_HASH *)cmd;

    if (!ParseSetConfig(pSetConfig))
    {
        UserPrint("Error in command %s\n", cmd);
        return -1;
	}
    return 0;
}

A_BOOL ParseSetConfig(SETCONFIG_HASH *pEntry)
{
    char *pParam, *pStr;
	char *pValue;
    A_UINT32 testVal;

    pParam = pEntry->pKey;
	pValue = pEntry->pVal;
    while(isspace(*pParam)) pParam++;
    
    if(_strnicmp("EEPROM_FILE", pParam, strlen("EEPROM_FILE")) == 0) 
    {
        if(!sscanf(pValue, "%s", (char *)&configSetup.eepromFile)) 
        {
            return 0;
        }
    }
    else if(_strnicmp("EEPROM_BOARD_FILE", pParam, strlen("EEPROM_BOARD_FILE")) == 0) 
    {
        if(!sscanf(pValue, "%s", (char *)&configSetup.eepromBoardFile))
        {
            return 0;
        }
    }
    else if(_strnicmp("LOAD_FILE_CMD", pParam, strlen("LOAD_FILE_CMD")) == 0) 
    {
        pStr = strtok( pValue, delimiters ); //get past any white space etc
        if(pStr == NULL)
        {
            return 0;
        }
        strcpy(configSetup.loadFileCmd, pStr);
        pStr = strtok(NULL, delimiters);
        while (pStr)
        {
            strcat(configSetup.loadFileCmd, " ");
            strcat(configSetup.loadFileCmd ,pStr);
            pStr = strtok(NULL, delimiters);
        }
    }
    else if(_strnicmp("LOAD_FILE_ARG", pParam, strlen("LOAD_FILE_ARG")) == 0) 
    {
        if(pValue == NULL)
        {
            return 0;
        }
        strcpy(configSetup.loadFileArg, pValue);
    }
    else if(_strnicmp("DUT_CARD_SSID", pParam, strlen("DUT_CARD_SSID")) == 0) 
    {
        if(!sscanf(pValue, "%x", (unsigned int *)&configSetup.dutSSID)) 
        {
            return 0;
        }
    }
    else if(_strnicmp("DEVDRV_INTERFACE", pParam, strlen("DEVDRV_INTERFACE")) == 0) 
    {
        if(!sscanf(pValue, "%d", (unsigned int *)&testVal)) 
        {
            return 0;
        }
        configSetup.devdrvInterface = (A_UINT8)testVal;
    }
    else if(_strnicmp("REF_CLOCK_HZ", pParam, strlen("REF_CLOCK_HZ")) == 0) 
    {
        if(!sscanf(pValue, "%d", (unsigned int *)&configSetup.refClockHz)) 
        {
            return 0;
        }
    }
    else if(_strnicmp("DO_MEMORY_TEST", pParam, strlen("DO_MEMORY_TEST")) == 0) 
    {
        if(!sscanf(pValue, "%d", &configSetup.doMemoryTest)) 
        {
            return 0;
        }
    }
    else if(_strnicmp("CUSTOM_NAME", pParam, strlen("CUSTOM_NAME")) == 0)
    {
        if(pValue == NULL)
        {
            return 0;
        }
        strcpy(configSetup.customName, pValue);
    }
	else if(_strnicmp("DRIVER_PATH", pParam, strlen("DRIVER_PATH")) == 0)
    {
        if(pValue == NULL)
        {
            return 0;
        }
        strcpy(configSetup.driverPath, pValue);
    }
	else if(_strnicmp("BOARDDATA_PATH", pParam, strlen("BOARDDATA_PATH")) == 0)
    {
        if(pValue == NULL)
        {
            return 0;
        }
        strcpy(configSetup.boardDataPath, pValue);
    }
    else if(_strnicmp("CHECK_SW_VERSION", pParam, strlen("CHECK_SW_VERSION")) == 0) 
    {
        if(!sscanf(pValue, "%d", &configSetup.checkSwVer)) 
        {
            return 0;
        }
    }
	else if(_strnicmp("CAL_MEMORY", pParam, strlen("CAL_MEMORY")) == 0) 
    {
        if(!sscanf(pValue, "%d", (unsigned int *)(&configSetup.calmem))) 
        {
            return 0;
        }
    }

    return 1;
}




