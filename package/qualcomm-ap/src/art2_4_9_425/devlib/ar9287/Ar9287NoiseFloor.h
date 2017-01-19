
extern int Ar9287NoiseFloorFetch(int *nfc, int *nfe, int nfn);
extern int Ar9287NoiseFloorLoad(int *nfc, int *nfe, int nfn);
extern int Ar9287NoiseFloorReady(void);
extern int Ar9287NoiseFloorEnable(void);
extern void Ar9287_REG_enable_noisefloor_Write(int value);
extern void Ar9287_REG_no_update_noisefloor_Write(int value);
extern void Ar9287_REG_do_noisefloor_Write(int value);
extern void Ar9287_REG_do_noisefloor_Read(unsigned long *value);

