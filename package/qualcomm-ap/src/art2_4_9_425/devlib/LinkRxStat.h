



extern void LinkRxStatClear();


extern void LinkRxStatSpectralScanExtract(unsigned int *descriptor, int np);


extern void LinkRxStatExtract(unsigned int *descriptor, int np);


extern struct rxStats *LinkRxStatFetch(int rate);


extern struct rxStats *LinkRxStatTotalFetch();


extern void LinkRxStatFinish();
