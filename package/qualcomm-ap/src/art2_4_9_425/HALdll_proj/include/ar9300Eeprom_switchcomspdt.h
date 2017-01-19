
extern int Ar9300Eeprom_switchcomspdtGet(ar9300_eeprom_t *ahp_Eeprom, int iBand);

extern int Ar9300Eeprom_switchcomspdtSet(ar9300_eeprom_t *ahp_Eeprom, int value, int iBand);

extern int get_num_switchcomspdt();

extern void get_switchcomspdt(char *itemName, char *itemValue, int itemIndex, const ar9300_eeprom_t *ahp_Eeprom, int iBand);

extern void Ar9300EepromDifferenceAnalyze_switchcomspdt(void (*print)(char *format, ...), ar9300_eeprom_t *mptr, int mcount, int all);
