
#ifdef _WINDOWS
#ifdef AR9300DLL
		#define AR9300DLLSPEC __declspec(dllexport)
	#else
		#define AR9300DLLSPEC __declspec(dllimport)
	#endif
#else
	#define AR9300DLLSPEC
#endif



#define POW_SM(_r, _s)     (((_r) & 0x3f) << (_s))
#define NUM_TRGT_PWR_REGISTERS 36

#ifdef unused
typedef enum all_target_power_index {
    ALL_TARGET_LEGACY_6_24,
    ALL_TARGET_LEGACY_36,
    ALL_TARGET_LEGACY_48,
    ALL_TARGET_LEGACY_54,
    ALL_TARGET_LEGACY_1L_5L,
    ALL_TARGET_LEGACY_5S,
    ALL_TARGET_LEGACY_11L,
    ALL_TARGET_LEGACY_11S,
    ALL_TARGET_HT20_0_8_16,
    ALL_TARGET_HT20_1_3_9_11_17_19,
    ALL_TARGET_HT20_4,
    ALL_TARGET_HT20_5,
    ALL_TARGET_HT20_6,
    ALL_TARGET_HT20_7,
    ALL_TARGET_HT20_12,
    ALL_TARGET_HT20_13,
    ALL_TARGET_HT20_14,
    ALL_TARGET_HT20_15,
    ALL_TARGET_HT20_20,
    ALL_TARGET_HT20_21,
    ALL_TARGET_HT20_22,
    ALL_TARGET_HT20_23,
    ALL_TARGET_HT40_0_8_16,
    ALL_TARGET_HT40_1_3_9_11_17_19,
    ALL_TARGET_HT40_4,
    ALL_TARGET_HT40_5,
    ALL_TARGET_HT40_6,
    ALL_TARGET_HT40_7,
    ALL_TARGET_HT40_12,
    ALL_TARGET_HT40_13,
    ALL_TARGET_HT40_14,
    ALL_TARGET_HT40_15,
    ALL_TARGET_HT40_20,
    ALL_TARGET_HT40_21,
    ALL_TARGET_HT40_22,
    ALL_TARGET_HT40_23,
};
#endif

//
// set transmit power gain (pcdac) to a specified value
//	
extern AR9300DLLSPEC int Ar9300TransmitGainSet(int mode, int pcdac);

extern AR9300DLLSPEC int Ar9300TransmitGainRead(int entry, unsigned int *rvalue, int *value, int max);

extern AR9300DLLSPEC int Ar9300TransmitINIGainGet(int *total_gain);

extern AR9300DLLSPEC int Ar9300TransmitGainWrite(int entry, int *value, int nvalue);

extern AR9300DLLSPEC int Ar9300TransmitPowerSet(int mode, double txp);

extern int nartAr9300TransmitPowerRegWrite(A_UINT8 *pPwrArray);

extern AR9300DLLSPEC int Ar9300TxGainTableRead_AddressGainTable(unsigned int **address, unsigned int *row, unsigned int *col);

extern AR9300DLLSPEC int Ar9300TxGainTableRead_AddressGainTable(unsigned int **address, unsigned int *row, unsigned int *col);

extern AR9300DLLSPEC int Ar9300TxGainTableRead_AddressHeader(unsigned int address, char *header, char *subheader, int max);

extern AR9300DLLSPEC int Ar9300TxGainTableRead_AddressValue(unsigned int address, int idx, char *rName, char *fName, int *value, int *low, int *high);

