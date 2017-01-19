

#ifdef _WINDOWS
	#ifdef PARSEDLL
		#define PARSEDLLSPEC __declspec(dllexport)
	#else
		#define PARSEDLLSPEC __declspec(dllimport)
	#endif
#else
	#define PARSEDLLSPEC
#endif


//
// These error codes and their meanings are published. Do not change them. You may add more on the end.
//
enum
{
    LinkTooManyReports=2000,
    LinkRequiresTxPowerControl,
    LinkRequiresTxOrRxDevice,
    LinkRequiresTxGainControl,
    LinkForeverInterleave,

    LinkNoReceiver,
    LinkNoTransmitter,
    LinkNoBlocker,
    LinkNoAttenuator,
    LinkNoPowerMeter,

    LinkNoSpectrumAnalyzer,
    LinkNoVsg,
    LinkNoMultimeter,
    LinkNoEvmMeter,
    LinkStart,

    LinkStop,
    LinkTxAndRxSame,
    LinkNoStart,
    LinkLog,
    LinkIterationStart,

    LinkIterationStop,
    LinkBadAttenuation,
    LinkFrequency,
    LinkRate,
    LinkAttenuation,

    LinkIss,
    LinkPacketCount,
    LinkPacketLength,
    LinkTemperature,
    LinkTxGain,

    LinkTxPower,
    LinkTargetPower,
    LinkTxGainIndex,
    LinkDacGain,
    LinkBlockerDelta,
    LinkBlockerTxPower,
    LinkBlockerIss,

    LinkChain,
    LinkAggregate,
    LinkStartContTx,
    LinkStopContTx,
	TransmitCancel,

	TransmitSetup,
	ReceiveCancel,
	ReceiveSetup,
	CarrierCancel,
	CarrierSetup,

	CalibrateFail,
	LinkRateRemove11B,
	LinkRateRemoveHt40,
	LinkBlockerFrequency,
	LinkRateRemoveHt20,
    LinkBlockerWaveform,
	LinkRateRemoveHt80,
	CalibrateChannSelectFail,

	LinkTxGainTableNoFile,
	LinkTxGainTableIndexOutOfRange,
	LinkTxGainTableOutOfMemory,

    LinkRateRemove2StreamRate,
    LinkRateRemove3StreamRate,
    LinkRateRemoveHt40CenterFrequency,
    LinkRateRemoveHt80CenterFrequency,
};


extern PARSEDLLSPEC void LinkErrorInit(void);
