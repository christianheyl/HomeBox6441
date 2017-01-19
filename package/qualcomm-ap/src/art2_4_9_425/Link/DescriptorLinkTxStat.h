

extern void LinkTxStatFinish();


extern struct txStats *DescriptorLinkTxStatTotalFetch();


extern struct txStats *DescriptorLinkTxStatFetch(int rate);


extern void LinkTxStatClear();
			  

extern void LinkTxStatExtract(unsigned int *descriptor, unsigned int *control, int nagg, int np);
