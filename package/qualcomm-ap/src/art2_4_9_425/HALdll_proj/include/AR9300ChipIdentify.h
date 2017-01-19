
extern int Ar9300ChipIdentify(void);

extern char *ChipToLibrary(int devid);

extern int ChipSelect(int devid);

extern int getPcieAddressOffset(void *ah);
extern int getPcieOtpTopAddress(void *ah);

#define ChipUnknown (-1)
#define ChipTest 1
#define ChipLinkTest 2

#define AR6300_DEVID	(0x40)				// LATER LOOK UP CORRECT VALUE
#define CHIP_ID_LOCATION 0xb8060090                     // Chip Revision ID location
#define CHIP_REV_ID_WASP 0x012 // last nibble is for Chip revision which is ignored
#define CHIP_REV_ID_SCORPION 0x13 // last nibble is for Chip revision which is ignored
#define CHIP_REV_ID_HONEYBEE 0x14 // last nibble is for Chip revision which is ignored
#define CHIP_REV_ID_DRAGONFLY 0x15 // last nibble is for Chip revision which is ignored

extern void ChipDevidParameterSplice(struct _ParameterList *list);

