



extern struct txStats *DescriptorLinkTxStatFetch(int rate);


extern struct txStats *DescriptorLinkTxStatTotalFetch(void);


extern int DescriptorLinkTxStart(void);


extern int DescriptorLinkTxComplete(int timeout, int (*ison)(), int (*done)(), int chipTemperature, int calibrate);

//
// get all of the packets descriptors ready to run
//
extern int DescriptorLinkTxSetup(int *rate, int nrate, int interleave,
	unsigned char *bssid, unsigned char *source, unsigned char *destination, 
	int PacketMany, int PacketLength,
	int RetryMax, int Antenna, int Bc, int Ifs,
	int shortGi, unsigned int txchain,
	int naggregate,
	unsigned char *pattern, int npattern);


extern int DescriptorLinkTxOptions(void);

extern int DescriptorLinkTxForPAPD(int ChainNum);

