



//
// check saved calibration information
//
extern AR9300DLLSPEC int Ar9300EepromReport(void (*print)(char *format, ...), int all);

//
// specify which templates may be used in compression.
// if many=0, all templates are allowed.
//
extern AR9300DLLSPEC int Ar9300EepromTemplateAllowed(unsigned int *value, unsigned int many);

//
// turn compression on/off.
//
extern AR9300DLLSPEC int Ar9300EepromCompress(unsigned int value);

//
// turn overwrite on/off.
//
extern AR9300DLLSPEC int Ar9300EepromOverwrite(unsigned int value);

//
// override default address for saving 
//
extern AR9300DLLSPEC int Ar9300EepromSaveAddressSet(int address);

//
// override default memory type for saving 
//
extern AR9300DLLSPEC int Ar9300EepromSaveMemorySet(int memory);


extern AR9300DLLSPEC int Ar9300EepromTemplatePreference(int preference);
extern AR9300DLLSPEC int Ar9300EepromSize(void);
extern AR9300DLLSPEC int Ar9300CalibrationDataAddressSet(int size);
extern AR9300DLLSPEC int Ar9300CalibrationDataAddressGet(void);
extern AR9300DLLSPEC int Ar9300CalibrationDataSet(int source);
extern AR9300DLLSPEC int Ar9300CalibrationDataGet(void);
extern AR9300DLLSPEC int Ar9300EepromTemplateInstall(int preference);

extern AR9300DLLSPEC int Ar9300ConfigurationSave(void);
extern AR9300DLLSPEC int Ar9300ConfigurationRestore(void);

//
// returns lowest free memory address
//
int Ar9300EepromUsed(void);
