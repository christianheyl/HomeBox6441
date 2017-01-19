#ifndef _QC9K_LINK_RX_H_
#define _QC9K_LINK_RX_H_

extern int Qc9KLinkRxDataStartCmd(int freq, int cenFreqUsed, int antenna, int rxChain, int promiscuous, int broadcast,
                                  unsigned char *bssid, unsigned char *destination,
                                  int numDescPerRate, int *rate, int nrate, int bandwidth, int datacheck);
        
extern int Qc9KLinkRxComplete(int timeout, int enableCompare, unsigned char *dataPattern, 
                               int dataPatternLength, int (*done)());

extern int Qc9KLinkRxStart(int promiscuous);

extern void Qc9KLinkRxSpectralScan(int spectralscan, void ( *f ) ( int rssi, int *rssic, int *rssie, int nchain, int *spectrum, int nspectrum ) );

#endif //_QC9K_LINK_RX_H_