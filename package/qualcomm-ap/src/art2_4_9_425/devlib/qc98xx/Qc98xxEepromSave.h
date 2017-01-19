#ifndef _QC98XX_EEPROM_SAVE_H_
#define _QC98XX_EEPROM_SAVE_H_


#ifdef AP_BUILD
#define SWAP16(_x) ( (u_int16_t)( (((const u_int8_t *)(&_x))[0] ) |\
                         ( ( (const u_int8_t *)( &_x ) )[1]<< 8) ) )

#define SWAP32(_x) ((u_int32_t)(                       \
                    (((const u_int8_t *)(&_x))[0]) |        \
                    (((const u_int8_t *)(&_x))[1]<< 8) |    \
                    (((const u_int8_t *)(&_x))[2]<<16) |    \
                    (((const u_int8_t *)(&_x))[3]<<24)))

#define MAX_EEPROM_SIZE 16*1024  // Size of calibration structure for 11n radio
#define FLASH_BASE_CALDATA_OFFSET 0x1000  // First 0x1000 byets are for board config data

#endif

extern int Qc98xxCalibrationDataSet(int source);
extern int Qc98xxCalibrationDataGet(void);

extern A_BOOL Qc98xxWriteFixedBytesIfOtpEmpty(A_BOOL otpEmptyChecked);
extern int Qc98xxEepromSave(void);

extern int Qc98xxCalibrationDataAddressSet(int address);
extern int Qc98xxCalibrationDataAddressGet(void);
extern int Qc98xxCalibrationDataSet(int source);
extern int Qc98xxCalibrationDataGet(void);
extern int Qc98xxEepromReport(void (*print)(char *format, ...), int all);
extern int Qc98xxEepromSaveMemorySet(int memory);

extern void Qc98xxEepromFile(int file);
extern A_BOOL Qc98xxLoadOtp();

extern int Qc98xxEepromTemplateAllowed(unsigned int *value, unsigned int many);
extern int Qc98xxEepromCompress(unsigned int value);

#endif //_QC98XX_EEPROM_SAVE_H_
