
extern int Ar9300Eeprom_xLNABiasStrengthGet(ar9300_eeprom_t *ahp_Eeprom, int iBand);

extern int Ar9300Eeprom_xLNABiasStrengthSet(ar9300_eeprom_t *ahp_Eeprom, int value, int iBand, int ix, int num);

extern int get_num_xLNABiasStrength();

extern void get_xLNABiasStrength(char *itemName, char *itemValue, int itemIndex, const ar9300_eeprom_t *ahp_Eeprom, int iBand);

extern void Ar9300EepromDifferenceAnalyze_xLNABiasStrength(void (*print)(char *format, ...), ar9300_eeprom_t *mptr, int mcount, int all);
