
#include "ErrorPrint.h"
#include "LinkError.h"

static struct _Error _LinkError[]=
{
    {LinkTooManyReports,ErrorFatal,"Too many reports. Maximum is %d."},
    {LinkRequiresTxPowerControl,ErrorFatal,"Input signal strength requires use of tx power control."},
    {LinkRequiresTxOrRxDevice,ErrorFatal,"You must specify a transmitter or a receiver device."},
    {LinkRequiresTxGainControl,ErrorFatal,"Calibration requires use of tx gain setting."},
    {LinkForeverInterleave,ErrorWarning,"Transmit forever requires rate interleaving. pc<0 => ir=1."},
    {LinkNoReceiver,ErrorFatal,"No connection to receiver %d."},
    {LinkNoTransmitter,ErrorFatal,"No connection to transmitter %d."},
    {LinkNoBlocker,ErrorFatal,"No connection to blocker %d."},
    {LinkNoAttenuator,ErrorFatal,"No attenuators."},
    {LinkNoPowerMeter,ErrorFatal,"No power meter."},
    {LinkNoSpectrumAnalyzer,ErrorFatal,"No spectrum analyzer."},
    {LinkNoVsg,ErrorFatal,"No VSG."},
    {LinkNoMultimeter,ErrorFatal,"No multimeter."},
    {LinkNoEvmMeter,ErrorFatal,"No EVM analyzer."},
    {LinkStart,ErrorControl,"Link test started at %d"},
    {LinkStop,ErrorControl,"Link test finished at %d. Elapsed time was %d ms."},
    {LinkTxAndRxSame,ErrorFatal,"The transmitter and receiver must be different."},
    {LinkNoStart,ErrorFatal,"Link test not started."},
    {LinkLog,ErrorInformation,"Data log is in file \"%s\"."},
    {LinkIterationStart,ErrorControl,"Link iteration started at %d"},
    {LinkIterationStop,ErrorControl,"Link iteration finished at %d. Elapsed time was %d ms."},
    {LinkBadAttenuation,ErrorWarning,"Attenuation %d is out of range [%d,%d] for chain %d."},
    {LinkFrequency,ErrorControl,"Frequency is %d MHz."},
    {LinkRate,ErrorControl,"Rate is %s."},
    {LinkAttenuation,ErrorControl,"Attenuation is %d dB."},
    {LinkIss,ErrorControl,"Input signal strength is %d dBm."},
    {LinkPacketCount,ErrorControl,"Packet count is %d."},
    {LinkPacketLength,ErrorControl,"Packet length is %d."},
    {LinkTemperature,ErrorControl,"Temperature is %d C."},
    {LinkTxGain,ErrorControl,"Transmit gain is %d."},
    {LinkTxPower,ErrorControl,"Transmit power is %.1lf dBm."},
    {LinkTargetPower,ErrorControl,"Transmit power is target power."},
    {LinkTxGainIndex,ErrorControl,"Transmit gain index is %d."},
    {LinkDacGain,ErrorControl,"Transmit gain index is %d, DAC gain is %d."},
    {LinkBlockerDelta,ErrorControl,"Blocker frequency delta is %d MHz."},
    {LinkBlockerTxPower,ErrorControl,"Blocker transmit power is %d dBm."},
    {LinkBlockerIss,ErrorControl,"Blocker input signal strength is %d dBm."},
    {LinkChain,ErrorControl,"Transmit chain is 0x%x. Receive chain is 0x%x."},
    {LinkAggregate,ErrorControl,"Aggregation is %d."},
	{LinkStartContTx,ErrorControl,"Transmit test started at %d."}, 
	{LinkStopContTx,ErrorControl,"Transmit test finished at %d. Elapsed time was %d ms."}, 
	{TransmitCancel,ErrorInformation,"Transmit operation canceled."},
	{TransmitSetup,ErrorFatal,"Can't setup transmit operation."},
	{ReceiveCancel,ErrorInformation,"Receive operation canceled."},
	{ReceiveSetup,ErrorFatal,"Can't setup receive operation."},
	{CarrierCancel,ErrorInformation,"Carrier operation canceled."},
	{CarrierSetup,ErrorFatal,"Can't setup carrier operation."},
	{CalibrateFail,ErrorFatal,"Calibration failed for chain %d. txgain=%d, power=%.1lf."},
	{LinkRateRemove11B,ErrorInformation,"Removed rate %s because 11B rates are not allowed at 5GHz."},
	{LinkRateRemoveHt40,ErrorInformation,"Removed rate %s because HT40 rates are not allowed."},
    {LinkBlockerFrequency,ErrorControl,"Blocker frequency is %d MHz."},
	{LinkRateRemoveHt20,ErrorInformation,"Removed rate %s because HT20 rates are not allowed."},
    {LinkBlockerWaveform,ErrorControl,"Blocker waveform file name: %s."},
	{LinkRateRemoveHt80,ErrorInformation,"Removed rate %s because HT80 rates are not allowed."},
	{LinkTxGainTableNoFile,ErrorInformation,"Register file %s does not exist."},
	{LinkTxGainTableIndexOutOfRange,ErrorInformation,"Register Index is out of range."},
	{LinkTxGainTableOutOfMemory,ErrorInformation,"System is out of memory."},
	{CalibrateChannSelectFail,ErrorInformation,"Calibration Channel Selection Failed."},
	{LinkRateRemove2StreamRate,ErrorInformation,"Removed rate %s because 2 stream rates are not allowed."},
	{LinkRateRemove3StreamRate,ErrorInformation,"Removed rate %s because 3 stream rates are not allowed."},
	{LinkRateRemoveHt40CenterFrequency,ErrorInformation,"Removed rate %s because %d is not 40MHz center frequency."},
	{LinkRateRemoveHt80CenterFrequency,ErrorInformation,"Removed rate %s because %d is not 80MHz center frequency."},
};


static int _ErrorFirst=1;

PARSEDLLSPEC void LinkErrorInit(void)
{
    if(_ErrorFirst)
    {
        ErrorHashCreate(_LinkError,sizeof(_LinkError)/sizeof(_LinkError[0]));
    }
    _ErrorFirst=0;
}
