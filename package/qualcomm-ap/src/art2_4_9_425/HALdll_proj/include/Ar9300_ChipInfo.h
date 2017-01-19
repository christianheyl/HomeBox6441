extern int Ar9300_FieldSelect(int devid);

extern int Ar9300pcieDefault(int devid);

extern int Ar9380pcieDefault(int devid);	// osprey
extern int Ar9580pcieDefault(int devid);	// peacock
extern int Ar9330pcieDefault(int devid);	// hornet  ???
extern int Ar9485pcieDefault(int devid);	// poseidon		// which HAL?
extern int Ar946XpcieDefault(int devid);	// Jupiter
extern int Ar956XpcieDefault(int devid);	// aphrodite
extern int Ar934XpcieDefault(int devid);	// wasp


extern int Ar9300_TxChainMany(int txMask);	
extern int Ar9300_RxChainMany(int rxMask);
extern int Ar9300_is2GHz(int opflag);
extern int Ar9300_is5GHz(int opflag);
extern int Ar9300_is4p9GHz(void);
extern int Ar9300_HalfRate(void);
extern int Ar9300_QuarterRate(void);

