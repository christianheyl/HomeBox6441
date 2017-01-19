#ifndef _QC9K_LINK_TX_H_
#define _QC9K_LINK_TX_H_

#ifdef _WINDOWS
#ifdef LINKDLL
		#define LINKDLLSPEC __declspec(dllexport)
	#else
		#define LINKDLLSPEC __declspec(dllimport)
	#endif
#else
	#define LINKDLLSPEC
#endif

extern LINKDLLSPEC int Qc9KLinkTxDataStartCmd(int freq, int cenFreqUsed, int tpcm, int pcdac, double *txPower, int gainIndex, int dacGain,
                       int ht40, int *rate, int nrate, int ir,
                       unsigned char *bssid, unsigned char *source, unsigned char *destination,
                       int numDescPerRate, int *dataBodyLength,
                       int retries, int antenna, int broadcast, int ifs,
	                   int shortGi, int txchain, int naggregate,
                    unsigned char *pattern, int npattern, int carrier, int bandwidth, int psatcal, int dutycycle);

extern LINKDLLSPEC int Qc9KLinkTxComplete(int timeout, int (*ison)(), int (*done)(), int chipTemperature, int calibrate);

#endif //_QC9K_LINK_TX_H_