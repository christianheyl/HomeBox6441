
extern int Ar9300Eeprom_txGainCapGet(ar9300_eeprom_t *ahp_Eeprom, int iBand);

extern int Ar9300Eeprom_txGainCapEnableGet(ar9300_eeprom_t *ahp_Eeprom);

extern int Ar9300Eeprom_txGainCapSet(ar9300_eeprom_t *ahp_Eeprom, int value, int iBand);

extern int Ar9300Eeprom_txGainCapEnableSet(ar9300_eeprom_t *ahp_Eeprom, int value);

extern int Ar9300Eeprom_CalibrationTxgainCAPSet(struct ath_hal *AH,  int *txgainmax);

extern int get_num_txGainCap();

extern void get_txGainCap(char *itemName, char *itemValue, int itemIndex, const ar9300_eeprom_t *ahp_Eeprom, int iBand);

extern void Ar9300EepromDifferenceAnalyze_txGainCap(void (*print)(char *format, ...), ar9300_eeprom_t *mptr, int mcount, int all);
