
extern int Ar9300Eeprom_tempslopeextensionGet(ar9300_eeprom_t *ahp_Eeprom, int *value, int ix, int *num);

extern int Ar9300Eeprom_tempslopeextensionSet(ar9300_eeprom_t *ahp_Eeprom, int *value, int ix, int iy, int iz, int num, int iBand);

extern int get_num_tempslopextension();

extern void get_tempslopextension(char *itemName, char *itemValue, int itemIndex, const ar9300_eeprom_t *ahp_Eeprom);

extern void Ar9300EepromDifferenceAnalyze_tempslopextension(void (*print)(char *format, ...), ar9300_eeprom_t *mptr, int mcount, int all);


