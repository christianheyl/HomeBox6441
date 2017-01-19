#ifndef _MCAL98XX_H_
#define _MCAL98XX_H_

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
extern  int Qc98xxTransmitGainSet(int mode, int pcdac);

extern  int Qc98xxTransmitGainRead(int entry, unsigned int *rvalue, int *value, int max);

extern int Qc98xxTransmitINIGainGet(int *total_gain);

extern  int Qc98xxTransmitGainWrite(int entry, int *value, int nvalue);

extern  int Qc98xxTransmitPowerSet(int mode, double txp);

extern int nartQc98xxTransmitPowerRegWrite(A_UINT8 *pPwrArray);

extern  int Qc98xxTxGainTableRead_AddressGainTable(unsigned int **address, unsigned int *row, unsigned int *col);

extern  int Qc98xxTxGainTableRead_AddressGainTable(unsigned int **address, unsigned int *row, unsigned int *col);

extern  int Qc98xxTxGainTableRead_AddressHeader(unsigned int address, char *header, char *subheader, int max);

extern  int Qc98xxTxGainTableRead_AddressValue(unsigned int address, int idx, char *rName, char *fName, int *value, int *low, int *high);


#endif // _MCAL98XX_H_