


extern void RegisterDebugParameterSplice(struct _ParameterList *list);

extern void RegisterReadParameterSplice(struct _ParameterList *list);

extern void FieldReadParameterSplice(struct _ParameterList *list);

extern void MemoryReadParameterSplice(struct _ParameterList *list);

extern void OtpReadParameterSplice(struct _ParameterList *list);

extern void RegisterWriteParameterSplice(struct _ParameterList *list);

extern void FieldWriteParameterSplice(struct _ParameterList *list);

extern void StickyWriteParameterSplice(struct _ParameterList *list);

extern void MemoryWriteParameterSplice(struct _ParameterList *list);

extern void OtpWriteParameterSplice(struct _ParameterList *list);

extern void RegisterDebugCommand(int client);

extern void RegisterReadCommand(int client);

extern void RegisterWriteCommand(int client);

extern void FieldReadCommand(int client);

extern void FieldWriteCommand(int client);

extern void FieldListCommand(int client);

extern void ConfigReadCommand(int client);

extern void ConfigWriteCommand(int client);

extern void MemoryReadCommand(int client);

extern void MemoryWriteCommand(int client);

extern void EepromReadCommand(int client);

extern void EepromWriteCommand(int client);

extern void FlashReadCommand(int client);

extern void FlashWriteCommand(int client);

extern void OtpReadCommand(int client);

extern void OtpWriteCommand(int client);

extern void OtpLoadCommand(int client);

extern void FieldStickyCommand(int client);

extern void FieldStickyClear(int client);

extern void FieldStickyList(int client);

extern unsigned int MaskCreate(int low, int high);

extern int MyFieldRead(unsigned int address, int low, int high, unsigned int *value);

extern int MyFieldWrite(unsigned int address, int low, int high, unsigned int value);

enum {
	COEFF_TX_TYPE = 0,
	COEFF_RX_TYPE
};
