


//
// Commit the internal structure to the card.
// If OTP is in use, this command consumes additional card memory.
// If Eeprom or FLASH is in use, the memory is overwritten.
//
extern void ConfigurationSaveCommand(int client);

extern void ConfigurationSaveParameterSplice(struct _ParameterList *list);

//
// Commit the PCIE startup data to the card.
// If OTP is in use, this command consumes additional card memory.
// If Eeprom or FLASH is in use, the memory is overwritten.
//
extern void ConfigurationPcieSaveCommand(int client);

extern void ConfigurationPcieSaveParameterSplice(struct _ParameterList *list);

//
// Check the eeprom/otp structure on the card.
//
extern void ConfigurationCheckCommand(int client);

extern void ConfigurationCheckParameterSplice(struct _ParameterList *list);

//
// Restore the internal structure to the state on the card.
// That is, forget all changes that we have made since the lst ConfigureSave() command.
//
extern void ConfigurationRestoreCommand(int client);

extern void ConfigurationRestoreParameterSplice(struct _ParameterList *list);

//
// parse and then set a configuration parameter in the internal structure
//
extern void ConfigurationSetCommand(int client);

extern void ConfigurationSetParameterSplice(struct _ParameterList *list);


//
// parse and then get a configuration parameter in the internal structure
//
extern void ConfigurationGetCommand(int client);

extern void ConfigurationGetParameterSplice(struct _ParameterList *list);


extern void ConfigurationSetCalTGTCommand(int client);

extern void ConfigurationGetCalTGTCommand(int client);

