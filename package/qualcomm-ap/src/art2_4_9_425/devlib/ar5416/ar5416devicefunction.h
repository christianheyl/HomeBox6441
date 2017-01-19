
extern void Ar5416SetTargetPowerFromEeprom(struct ath_hal *ah, u_int16_t freq);
extern int Ar5416EepromRead(unsigned long address, unsigned char *value, int count);
extern int Ar5416EepromWrite(unsigned long address, unsigned char *value, int count);
extern int Ar5416GetCommand(int code, int client, int ip, int *out_error, int *out_done, unsigned char* in_name);
extern int Ar5416SetCommand(int code, int client, int *out_ip, int *out_error, int *out_done, unsigned char *in_name);
extern A_INT32 Ar5416ConfigSpaceCommit(void);
extern A_INT32 Ar5416pcieAddressValueDataInit(void);
extern A_INT32 Ar5416_MacAdressGet(A_UINT8 *mac);
extern A_INT32 Ar5416_CustomerDataGet(A_UCHAR *data, A_INT32 max);
extern int Ar5416EepromGetTargetPwr(int freq, int rateIndex, double *powerOut) ;
extern int Ar5416EepromSave(void);
extern int Ar5416TemperatureGet(int forceTempRead);
extern int Ar5416VoltageGet(void);
extern int Ar5416Deaf(int deaf) ;
extern int Ar5416EepromReport(void (*print)(char *format, ...), int all);
extern u_int16_t *ar5416RegulatoryDomainGet(struct ath_hal *ah);


