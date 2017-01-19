

//  "ACI $Id: //depot/sw/branches/art2_main_per_cs/src/art2/shared/CardError.c#1 $, $Header: //depot/sw/branches/art2_main_per_cs/src/art2/shared/CardError.c#1 $"




#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

#include "ErrorPrint.h"
#include "CardError.h"


static struct _Error _CardError[]=
{
    {CardLoadGood,ErrorInformation,CardLoadGoodFormat},
    {CardLoadBad,ErrorFatal,CardLoadBadFormat},
    {CardUnloadGood,ErrorInformation,CardUnloadGoodFormat},
	{CardNoneLoaded,ErrorFatal,CardNoneLoadedFormat},
	{CardResetSuccess,ErrorControl,CardResetSuccessFormat},
	{CardResetFail,ErrorFatal,CardResetFailFormat},
	{CardLoadAnwi,ErrorFatal,CardLoadAnwiFormat},
	{CardLoadHal,ErrorFatal,CardLoadHalFormat},
	{CardLoadAttach,ErrorFatal,CardLoadAttachFormat},
	{CardLoadNoChannel,ErrorFatal,CardLoadNoChannelFormat},
	{CardLoadCalibrationNone,ErrorInformation,CardLoadCalibrationNoneFormat},
	{CardLoadCalibrationFlash,ErrorInformation,CardLoadCalibrationFlashFormat},
	{CardLoadCalibrationEeprom,ErrorInformation,CardLoadCalibrationEepromFormat},
	{CardLoadCalibrationOtp,ErrorInformation,CardLoadCalibrationOtpFormat},
	{CardLoadPcie,ErrorFatal,CardLoadPcieFormat},
	{CardLoadDevid,ErrorFatal,CardLoadDevidFormat},
	{CardNoLoadOrReset,ErrorFatal,CardNoLoadOrResetFormat},
	{CardNoiseFloorBad,ErrorWarning,CardNoiseFloorBadFormat},
	{CardResetBad,ErrorWarning,CardResetBadFormat},
	{CardChipUnknown,ErrorFatal,CardChipUnknownFormat},
	{CardPcieSave,ErrorInformation,CardPcieSaveFormat},
	{CardPcieSaveError,ErrorFatal,CardPcieSaveFormat},
	{CardCalibrationSave,ErrorInformation,CardCalibrationSaveFormat},
	{CardCalibrationSaveError,ErrorFatal,CardCalibrationSaveErrorFormat},
	{CardFreeMemory,ErrorInformation,CardFreeMemoryFormat},
	{CardLoadCalibrationFile,ErrorInformation,CardLoadCalibrationFileFormat},
};

static int _ErrorFirst=1;

PARSEDLLSPEC void CardErrorInit(void)
{
    if(_ErrorFirst)
    {
        ErrorHashCreate(_CardError,sizeof(_CardError)/sizeof(_CardError[0]));
    }
    _ErrorFirst=0;
}

