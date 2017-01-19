


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
    DeviceNoFunction=6500,
	DeviceNoLoad,
	DeviceFound,
	DeviceMissing,
	DeviceSummary,
	DeviceLoadBad,
	DeviceLoadGood,
	DeviceFunction,

    LinkNoFunction=6600,
	LinkNoLoad,
	LinkFound,
	LinkMissing,
	LinkSummary,
	LinkLoadBad,
	LinkLoadGood,

    BusNoFunction=6700,
	BusNoLoad,
	BusFound,
	BusMissing,
	BusSummary,
	BusLoadBad,
	BusLoadGood,

    CalibrationNoFunction=6800,
	CalibrationNoLoad,
	CalibrationMissing,
	CalibrationSummary,
	CalibrationLoadBad,
	CalibrationLoadGood,

};


#define DeviceNoFunctionFormat "No function defined for \"%s\"."
#define DeviceNoLoadFormat "Unable to find device dll \"%s\"."
#define DeviceMissingFormat "%8d %s"
#define DeviceFoundFormat "%08x %s"
#define DeviceSummaryFormat "Loaded %d functions. Failed to load %d functions. Missing %d required functions."
#define DeviceLoadBadFormat "Device control function load failed for \"%s\" from \"%s\".\n"
#define DeviceLoadGoodFormat "Device control functions loaded for \"%s\" from \"%s\". Version \"%s\" built on \"%s\".\n"
#define DeviceFunctionFormat "%s"

#define DeviceNoFunctionFormatInput "No function defined for \"%[^\"]\"."
#define DeviceNoLoadFormatInput "Unable to find device dll \"%[^\"]\"."
#define DeviceFoundFormatInput DeviceFoundFormat
#define DeviceMissingFormatInput DeviceMissingFormat
#define DeviceSummaryFormatInput DeviceSummaryFormat
#define DeviceLoadBadFormatInput "Device control function load failed for \"%[^\"]\" from \"%[^\"]\".\n"
#define DeviceLoadGoodFormatInput "Device control functions loaded for \"%[^\"]\" from \"%[^\"]\". Version \"%[^\"]\" built on \"%[^\"]\".\n"
#define DeviceFunctionFormatInput DeviceFunctionFormat

#define LinkNoFunctionFormat "No function defined for \"%s\"."
#define LinkNoLoadFormat "Unable to find link dll \"%s\"."
#define LinkMissingFormat "%8d %s"
#define LinkFoundFormat "%08x %s"
#define LinkSummaryFormat "Loaded %d functions. Failed to load %d functions. Missing %d required functions."
#define LinkLoadBadFormat "Link control function load failed for \"%s\" from \"%s\".\n"
#define LinkLoadGoodFormat "Link control functions loaded for \"%s\" from \"%s\".\n"

#define LinkNoFunctionFormatInput "No function defined for \"%[^\"]\"."
#define LinkNoLoadFormatInput "Unable to find link dll \"%[^\"]\"."
#define LinkFoundFormatInput LinkFoundFormat
#define LinkMissingFormatInput LinkMissingFormat
#define LinkSummaryFormatInput LinkSummaryFormat
#define LinkLoadBadFormatInput "Link control function load failed for \"%[^\"]\" from \"%[^\"]\".\n"
#define LinkLoadGoodFormatInput "Link control functions loaded for \"%[^\"]\" from \"%[^\"]\".\n"

#define BusNoFunctionFormat "No function defined for \"%s\"."
#define BusNoLoadFormat "Unable to find bus dll \"%s\"."
#define BusMissingFormat "%8d %s"
#define BusFoundFormat "%08x %s"
#define BusSummaryFormat "Loaded %d functions. Failed to load %d functions. Missing %d required functions."
#define BusLoadBadFormat "Bus control function load failed for \"%s\" from \"%s\".\n"
#define BusLoadGoodFormat "Bus control functions loaded for \"%s\" from \"%s\".\n"

#define BusNoFunctionFormatInput "No function defined for \"%[^\"]\"."
#define BusNoLoadFormatInput "Unable to find bus dll \"%[^\"]\"."
#define BusFoundFormatInput BusFoundFormat
#define BusMissingFormatInput BusMissingFormat
#define BusSummaryFormatInput BusSummaryFormat
#define BusLoadBadFormatInput "Bus control function load failed for \"%[^\"]\" from \"%[^\"]\".\n"
#define BusLoadGoodFormatInput "Bus control functions loaded for \"%[^\"]\" from \"%[^\"]\".\n"

#define CalibrationNoLoadFormat "Unable to find calibration dll \"%s\"."
#define CalibrationNoFunctionFormat "Unable to find calibration dll function \"%s\"."
#define CalibrationMissingFormat "Calibration DLL is not loaded. Setcal command failed."

extern PARSEDLLSPEC void DeviceErrorInit(void);

