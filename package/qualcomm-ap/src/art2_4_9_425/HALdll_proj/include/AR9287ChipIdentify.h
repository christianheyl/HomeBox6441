
extern int Ar9287ChipIdentify(void);

extern char *ChipToLibrary(int devid);

extern int ChipSelect(int devid);

extern int getPcieAddressOffset(void *ah);
extern int getPcieOtpTopAddress(void *ah);

#define ChipUnknown (-1)
#define ChipTest 1
#define ChipLinkTest 2

extern void ChipDevidParameterSplice(struct _ParameterList *list);

