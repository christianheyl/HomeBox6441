
extern int Ar9300Eeprom_rfGainCapGet(ar9300_eeprom_t *ahp_Eeprom, int iBand);

extern int Ar9300Eeprom_rfGainCapSet(ar9300_eeprom_t *ahp_Eeprom, int value, int iBand);

extern int get_num_rfGainCap();

extern void get_rfGainCap(char *itemName, char *itemValue, int itemIndex, const ar9300_eeprom_t *ahp_Eeprom, int iBand);

extern void Ar9300EepromDifferenceAnalyze_rfGainCap(void (*print)(char *format, ...), ar9300_eeprom_t *mptr, int mcount, int all);
