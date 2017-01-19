

extern void NoiseFloorCommand(int client);

extern int NoiseFloorDo(int frequency, int *tnf, int tnfmany, int margin, int attempt, int timeout, int (*done)(),
						int *nfc, int *nfe, int nfmax);

extern int NoiseFloorLoadWait(int *nfc, int *nfe, int nfn, int timeout);

extern int NoiseFloorFetchWait(int *nfc, int *nfe, int nfn, int timeout);

extern int NoiseFloorLoad(int *nfc, int *nfe, int nfn);

extern int NoiseFloorFetch(int *nfc, int *nfe, int nfn);

extern void NoiseFloorParameterSplice(struct _ParameterList *list);
