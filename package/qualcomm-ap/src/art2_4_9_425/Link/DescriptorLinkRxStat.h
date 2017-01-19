



extern void LinkRxStatClear();


extern void LinkRxStatSpectralScanExtract(unsigned int *descriptor, int np);


extern void LinkRxStatExtract(unsigned int *descriptor, int np);


extern struct rxStats *DescriptorLinkRxStatFetch(int rate);


extern struct rxStats *DescriptorLinkRxStatTotalFetch();


extern void LinkRxStatFinish();
