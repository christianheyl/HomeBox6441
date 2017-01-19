#ifndef _LINKTXRX_H_
#define _LINKTXRX_H_


#ifdef _WINDOWS
	#ifdef FIELDDLL
		#define DEVICEDLLSPEC __declspec(dllexport)
	#else
		#define DEVICEDLLSPEC __declspec(dllimport)
	#endif
#else
	#define DEVICEDLLSPEC
#endif


//
// The following functions control the device.
//
// unless otherwise noted, all of these functions return 0 on success
//

extern DEVICEDLLSPEC int LinkTxSetup(int *rate, int nrate, int interleave,
	unsigned char *bssid, unsigned char *source, unsigned char *destination, 
	int PacketMany, int PacketLength,
	int RetryMax, int Antenna, int Bc, int Ifs,
	int shortGi, unsigned int txchain,
	int naggregate,
	unsigned char *pattern, int npattern);
extern DEVICEDLLSPEC int LinkTxStart(void);
extern DEVICEDLLSPEC int LinkTxComplete(int timeout, int (*ison)(), int (*done)(), int chipTemperature, int calibrate);
extern DEVICEDLLSPEC struct txStats *LinkTxStatFetch(int rate);

extern DEVICEDLLSPEC int LinkRxSetup(unsigned char *bssid, unsigned char *macaddr);
extern DEVICEDLLSPEC int LinkRxStart(int promiscuous);
extern DEVICEDLLSPEC int LinkRxComplete(int timeout, 
		int enableCompare, unsigned char *dataPattern, int dataPatternLength, int (*done)());
extern DEVICEDLLSPEC struct rxStats *LinkRxStatFetch(int rate);

extern DEVICEDLLSPEC int LinkRxSpectralScan(int spectralscan, void (*f)(int rssi, int *rssic, int *rssie, int nchain, int *spectrum, int nspectrum));

extern DEVICEDLLSPEC int LinkTxForPAPD(int ChainNum);

extern DEVICEDLLSPEC int LinkTxDataStartCmd(int freq, int cenFreqUsed, int tpcm, int pcdac, double *txPower,  int gainIndex, int dacGain,
                            int ht40, int *rate, int nrate, int ir,
                            unsigned char *bssid, unsigned char *source, unsigned char *destination,
                            int numDescPerRate, int *dataBodyLength,
                            int retries, int antenna, int broadcast, int ifs,
	                        int shortGi, int txchain, int naggregate,
	                        unsigned char *pattern, int npattern, int carrier, int bandwidth, int psatcal, int dutycycle);
extern DEVICEDLLSPEC int LinkRxDataStartCmd(int freq, int cenFreqUsed, int antenna, int rxChain, int promiscuous, int broadcast,
                                    unsigned char *bssid, unsigned char *destination,
                                    int numDescPerRate, int *rate, int nrate, int bandwidth, int datacheck);
extern DEVICEDLLSPEC int LinkTxStatTemperatureGet();

extern DEVICEDLLSPEC int LinkRxFindSpur(int findspur);

//
//
// These are the function pointers to the device dependent control functions
//
struct _LinkFunction
{
	int (*TxSetup)(int *rate, int nrate, int interleave,
		unsigned char *bssid, unsigned char *source, unsigned char *destination, 
		int PacketMany, int PacketLength,
		int RetryMax, int Antenna, int Bc, int Ifs,
		int shortGi, unsigned int txchain,
		int naggregate,
		unsigned char *pattern, int npattern);
	int (*TxStart)(void);
	int (*TxComplete)(int timeout, int (*ison)(), int (*done)(), int chipTemperature, int calibrate);
	struct txStats *(*TxStatFetch)(int rate);

	int (*RxSetup)(unsigned char *bssid, unsigned char *macaddr);
	int (*RxStart)(int promiscuous);
	int (*RxComplete)(int timeout, 
		int enableCompare, unsigned char *dataPattern, int dataPatternLength, int (*done)());
	struct rxStats *(*RxStatFetch)(int rate);

	int (*RxSpectralScan)(int spectralscan, void (*f)(int rssi, int *rssic, int *rssie, int nchain, int *spectrum, int nspectrum));

	int (*TxForPAPD)(int ChainNum);

    int (*TxDataStartCmd)(int freq, int cenFreqUsed, int tpcm, int pcdac, double *txPower, int gainIndex, int dacGain,
                            int ht40, int *rate, int nrate, int ir,
                            unsigned char *bssid, unsigned char *source, unsigned char *destination,
                            int numDescPerRate, int *dataBodyLength,
                            int retries, int antenna, int broadcast, int ifs,
	                        int shortGi, int txchain, int naggregate,
                         unsigned char *pattern, int npattern, int carrier, int bandwidth, int psatcal, int dutycycle);
    int (*RxDataStartCmd)(int freq, int cenFreqUsed, int antenna, int rxChain, int promiscuous, int broadcast,
                                    unsigned char *bssid, unsigned char *destination,
                                    int numDescPerRate, int *rate, int nrate, int bandwidth, int datacheck);
    int (*TxStatTemperatureGet)();
    int (*RxFindSpur)(int findspur);
};


//
// The following functions setup the device control functions for a specific device type.
//


//
// clear all device control function pointers and set to default behavior
//
extern DEVICEDLLSPEC void LinkFunctionReset(void);

//
// clear all device control function pointers and set to default behavior
//
extern DEVICEDLLSPEC int LinkFunctionSelect(struct _LinkFunction *device);





#endif /* _LINKTXRX_H_ */
