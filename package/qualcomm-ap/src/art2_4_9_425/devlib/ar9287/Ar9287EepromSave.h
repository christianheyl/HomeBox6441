typedef enum ar9287_Rates {
    ALL_TARGET_LEGACY_6_24,
    ALL_TARGET_LEGACY_36,
    ALL_TARGET_LEGACY_48,
    ALL_TARGET_LEGACY_54,
    ALL_TARGET_LEGACY_1,
    ALL_TARGET_LEGACY_2,
    ALL_TARGET_LEGACY_5_5,
    ALL_TARGET_LEGACY_11,
    ALL_TARGET_HT20_0,
    ALL_TARGET_HT20_1,
    ALL_TARGET_HT20_2,
    ALL_TARGET_HT20_3,
    ALL_TARGET_HT20_4,
    ALL_TARGET_HT20_5,
    ALL_TARGET_HT20_6,
    ALL_TARGET_HT20_7,
    ALL_TARGET_HT40_0,
    ALL_TARGET_HT40_1,
    ALL_TARGET_HT40_2,
    ALL_TARGET_HT40_3,
    ALL_TARGET_HT40_4,
    ALL_TARGET_HT40_5,
    ALL_TARGET_HT40_6,
    ALL_TARGET_HT40_7,
    ar9287RateSize
} AR9287_RATES;


extern int Ar9287EepromRead(unsigned int address, unsigned char *value, int count);
extern int Ar9287EepromWrite(unsigned int address, unsigned char *value, int count);
//extern void ar9287_computeCheckSum( ar9287_eeprom_t* pEepStruct);
extern AR9287DLLSPEC int Ar9287EepromSaveAddressSet(int address);
extern AR9287DLLSPEC int Ar9287EepromSaveMemorySet(int memory);
extern int Ar9287EepromSave(void);
extern HAL_BOOL Ar9287FlashSave(struct ath_hal *ah);
extern AR9287DLLSPEC int Ar9287CalibrationDataAddressSet(int size);
extern AR9287DLLSPEC int Ar9287CalibrationDataAddressGet(void);
extern AR9287DLLSPEC int Ar9287CalibrationDataSet(int source);
extern AR9287DLLSPEC int Ar9287CalibrationDataGet(void);
extern A_INT32 Ar9287ConfigurationSave() ;
extern int Ar9287EepromReport(void (*print)(char *format, ...), int all);
//extern void Ar9287SetTargetPowerFromEeprom(struct ath_hal *ah, A_INT16 freq);
