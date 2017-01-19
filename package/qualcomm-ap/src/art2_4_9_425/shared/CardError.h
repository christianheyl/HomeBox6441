


#ifdef _WINDOWS
	#ifdef PARSEDLL
		#define PARSEDLLSPEC __declspec(dllexport)
	#else
		#define PARSEDLLSPEC __declspec(dllimport)
	#endif
#else
	#define PARSEDLLSPEC
#endif


enum
{
    CardLoadGood=6000,
    CardLoadBad,
    CardUnloadGood,
	CardNoneLoaded,
	CardResetSuccess,

	CardResetFail,
	CardLoadAnwi,
	CardLoadHal,
	CardLoadAttach,
	CardLoadNoChannel,

	CardLoadCalibrationNone,
	CardLoadCalibrationFlash,
	CardLoadCalibrationEeprom,
	CardLoadCalibrationOtp,
	CardLoadPcie,

	CardLoadDevid,
	CardNoLoadOrReset,
	CardNoiseFloorBad,
	CardResetBad,
	CardChipUnknown,

	CardPcieSave,
	CardPcieSaveError,
	CardCalibrationSave,
	CardCalibrationSaveError,
	CardFreeMemory,
	CardLoadCalibrationFile,
};


#define CardLoadGoodFormat "Loaded card."
#define CardLoadBadFormat "Can't load card."
#define CardUnloadGoodFormat "Unloaded card."
#define CardNoneLoadedFormat "No card loaded."
#define CardResetSuccessFormat "Device reset successfully. frequency=%d, ht40=%d, tx chain=%d, rx chain=%d."
#define CardResetFailFormat "Device reset error %d. frequency=%d, ht40=%d, tx chain=%d, rx chain=%d."
#define CardLoadAnwiFormat "Anwi driver load error."
#define CardLoadHalFormat "HAL load error."
#define CardLoadAttachFormat "Device attach error %d."
#define CardLoadNoChannelFormat "No legal channels."
#define CardLoadCalibrationNoneFormat "No calibration information found."
#define CardLoadCalibrationFlashFormat "Calibration information read from flash."
#define CardLoadCalibrationEepromFormat "Calibration information read from eeprom at 0x%x."
#define CardLoadCalibrationOtpFormat "Calibration information read from otp at 0x%x."
#define CardLoadPcieFormat "Can't load pcie initilization space."
#define CardLoadDevidFormat "No support for device type 0x%x."
#define CardNoLoadOrResetFormat "Device not loaded or not reset."
#define CardNoiseFloorBadFormat "Bad noise floor value: (%d,%d) (%d,%d) (%d,%d)."
#define CardResetBadFormat "Device reset error %d. frequency=%d, ht40=%d, tx chain=%d, rx chain=%d. Retrying."
#define CardChipUnknownFormat "Unknown chip. Please specify devid."
#define CardPcieSaveFormat "Chip initializtion space saved in %d bytes."
#define CardPcieSaveErrorFormat "Chip initializtion space save error."
#define CardCalibrationSaveFormat "Calibration structure saved in %d bytes."
#define CardCalibrationSaveErrorFormat "Calibration structure save error."
#define CardFreeMemoryFormat "Free memory for initialization and calibration is %d (%d - %d) bytes."
#define CardLoadCalibrationFileFormat "Calibration information read from a file."

#define CardLoadGoodFormatInput CardLoadGoodFormat
#define CardLoadBadFormatInput CardLoadBadFormat
#define CardUnloadGoodFormatInput CardUnloadGoodFormat
#define CardNoneLoadedFormatInput CardNoneLoadedFormat
#define CardResetSuccessFormatInput CardResetSuccessFormat
#define CardResetFailFormatInput CardResetFailFormat
#define CardLoadHalFormatInput CardLoadHalFormat
#define CardLoadAttachFormatInput CardLoadAttachFormat
#define CardLoadNoChannelFormatInput CardLoadNoChannelFormat
#define CardLoadCalibrationNoneFormatInput CardLoadCalibrationNoneFormat
#define CardLoadCalibrationFlashFormatInput CardLoadCalibrationFlashFormat
#define CardLoadCalibrationEepromFormatInput CardLoadCalibrationEepromFormat 
#define CardLoadCalibrationOtpFormatInput CardLoadCalibrationOtpFormat
#define CardLoadPcieFormatInput CardLoadPcieFormat
#define CardLoadDevidFormatInput CardLoadDevidFormat
#define CardNoLoadOrResetFormatInput CardNoLoadOrResetFormat
#define CardNoiseFloorBadFormatInput CardNoiseFloorBadFormat
#define CardResetBadFormatInput CardResetBadFormat
#define CardChipUnknownFormatInput CardChipUnknownFormat
#define CardPcieSaveFormatInput CardPcieSaveFormat
#define CardPcieSaveErrorFormatInput CardPcieSaveErrorFormat
#define CardCalibrationSaveFormatInput CardCalibrationSaveFormat
#define CardCalibrationSaveErrorFormatInput CardCalibrationSaveErrorFormat
#define CardFreeMemoryFormatInput CardFreeMemoryFormat 
#define CardLoadCalibrationFileFormatInput CardLoadCalibrationFileFormat


extern PARSEDLLSPEC void CardErrorInit(void);

