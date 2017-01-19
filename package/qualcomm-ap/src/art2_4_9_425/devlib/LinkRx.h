
extern unsigned int MemoryLayout(int size);

extern unsigned int MemoryInit(int size);

// this frees everything.
extern void MemoryFree(unsigned int buffer);

//
// Return address of first rx descriptor.
//
extern unsigned int LinkRxLoopFirst();


//
// Destroy loop of rx descriptors and free all of the memory.
//
extern int LinkRxLoopDestroy();


//
// Create a loop of rx descriptors.
// Return the address of the first one.
//
extern int LinkRxLoopCreate(int many);


extern void LinkRxSetup(unsigned char *bssid, unsigned char *macaddr);

//
// Setup to do spectral scan. Must be called after LinkRxSetup() and before LinkRxStart().
// Spectral scan fft data is returned through the specified callback function.
// In addition regular processing of rssi and other errors occurs.
//
extern void LinkRxSpectralScan(int spectralscan, void (*f)(int rssi, int *rssic, int *rssie, int nchain, int *spectrum, int nspectrum));


extern void LinkRxStart(int promiscuous);


extern void LinkRxComplete(int timeout, 
	int enableCompare, unsigned char *dataPattern, int dataPatternLength, int (*done)());


extern void LinkRxOptions();
