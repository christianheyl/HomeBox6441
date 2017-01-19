
#ifndef _DEV_SET_CONFIG_H_
#define _DEV_SET_CONFIG_H_

#include "ParameterSelect.h"

#define DEVDRV_INTERFACE_SDIO       1
#define DEVDRV_INTERFACE_ETH        2
#define DEVDRV_INTERFACE_UART       3
#define DEVDRV_INTERFACE_USB		4
#define DEVDRV_INTERFACE_PCI		5

enum  
{
    SetConfigEepromFile=0,
	SetConfigEepromBoardFile,
	SetConfigLoadFileCmd,
    SetConfigLoadFileArg,
    SetConfigDutCardSsid,
    SetConfigDevDrvInterface,
    SetConfigRefClockHz,
    SetConfigDoMemoryTest,
    SetConfigCustomName,
	SetConfigCalMem,
};

static unsigned int UnsignedShortMinimum=0;
static unsigned int UnsignedShortMaximum=0xffff;
static unsigned int UnsignedShortDefault=0;
static unsigned int CalMemDafault = 5;

static unsigned int UnsignedIntMinimum=0;
static unsigned int UnsignedIntMaximum=0xffffffff;
static unsigned int UnsignedIntDefault=0;

static unsigned int BooleanMinimum=0;
static unsigned int BooleanMaximum=1;
static unsigned int BooleanDefault=0;

static unsigned int DevdrvInterfaceMinimum = DEVDRV_INTERFACE_SDIO;
static unsigned int DevdrvInterfaceMaximum = DEVDRV_INTERFACE_PCI;
static unsigned int DevdrvInterfaceDefault = DEVDRV_INTERFACE_SDIO;

#define SETCONFIG_EEPROM_FILE {SetConfigEepromFile,{"EEPROM_FILE",0,0},"eeprom file name loaded to DUT",'t',0,1,1,1,0,0,0,0,0}

#define SETCONFIG_EEPROM_BOARD_FILE {SetConfigEepromBoardFile,{"EEPROM_BOARD_FILE",0,0},"eeprom board file name loaded to DUT if different from EEPROM_FILE",'t',0,1,1,1,0,0,0,0,0}

#define SETCONFIG_LOAD_FILE_CMD {SetConfigLoadFileCmd,{"LOAD_FILE_CMD",0,0},"the load command, e.g. ./loadUTFTgt_AR6004.sh (for DUT connected to Linux platform)",'t',0,1,1,1,0,0,0,0,0}

#define SETCONFIG_LOAD_FILE_ARG {SetConfigLoadFileArg,{"LOAD_FILE_ARG",0,0},"the load argument in the load command, e.g. ./LOAD_FILE_COMMAND -l LOAD_FILE_ARG",'t',0,1,1,1,0,0,0,0,0}

#define SETCONFIG_DUT_CARD_SSID {SetConfigDutCardSsid,{"DUT_CARD_SSID",0,0},"the subsystem id",'x',0,1,1,1,\
    &UnsignedShortMinimum,&UnsignedShortMaximum,&UnsignedShortDefault,0,0}

#define SETCONFIG_DEVDRV_INTERFACE {SetConfigDevDrvInterface,{"DEVDRV_INTERFACE",0,0},"DUT interface: 1=SDIO, 2=ETH, 3=UART, 4=USB",'u',0,1,1,1,\
    &DevdrvInterfaceMinimum, &DevdrvInterfaceMaximum, &DevdrvInterfaceDefault,0,0}

#define SETCONFIG_REF_CLOCK_HZ {SetConfigRefClockHz,{"REF_CLOCK_HZ",0,0},"ref clock in Hz",'u',0,1,1,1,\
    &UnsignedIntMinimum, &UnsignedIntMaximum, &UnsignedIntDefault,0,0}

#define SETCONFIG_DO_MEMORY_TEST {SetConfigDoMemoryTest,{"DO_MEMORY_TEST",0,0},"run memory test at load time",'z',0,1,1,1,\
    &BooleanMinimum, &BooleanMaximum, &BooleanDefault,0,0}

#define SETCONFIG_CUSTOM_NAME {SetConfigCustomName,{"CUSTOM_NAME",0,0},"Device name (for windows, in device manager)",'t',0,1,1,1,0,0,0,0,0}
    
#define SETCONFIG_CAL_MEMORY {SetConfigCalMem,{"CAL_MEMORY",0,0},"calibration memory",'u',0,1,1,1,\
    &UnsignedShortMinimum, &UnsignedShortMaximum, &CalMemDafault,0,0}

extern int DevSetConfigParameterSplice(struct _ParameterList *list);
extern int DevSetConfigCommand(void *cmd);

//-----------------------------------------------------------------------------

#define MAX_FILE_LENGTH		265

typedef struct mldConfig {
	A_CHAR	    machName[256];
	A_UINT32	dutSSID;
    A_CHAR      eepromFile[MAX_FILE_LENGTH];
    A_UINT8     devdrvInterface;
    A_UINT32    refClockHz;
    A_BOOL	    doMemoryTest;
    A_CHAR      eepromBoardFile[MAX_FILE_LENGTH];
    A_CHAR      loadFileCmd[MAX_FILE_LENGTH];
    A_CHAR      loadFileArg[MAX_FILE_LENGTH];
    A_CHAR      customName[MAX_FILE_LENGTH];
	A_CHAR		driverPath[MAX_FILE_LENGTH];
	A_CHAR		boardDataPath[MAX_FILE_LENGTH];
	A_BOOL		checkSwVer;
    A_UINT32    SwVersion;
	A_UINT8     calmem;
} MLD_CONFIG;


extern MLD_CONFIG configSetup;

#endif //_DEV_SET_CONFIG_H_