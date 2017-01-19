#include "opt_ah.h"
#include "ah.h"
#include "ah_internal.h"
#include "ah_eeprom.h"
#include "ar5416/ar5416.h"
#include "ar5416/ar5416reg.h"
#include "ar5416/ar5416phy.h"
#include "ar5416/ar5416eep.h"

#define AR9287_NUM_2G_20_TARGET_POWERS 3
#define AR9287_NUM_5G_20_TARGET_POWERS 8

#define AR9287_NUM_CTLS_2G           12
#define AR9287_NUM_BAND_EDGES_2G     4

#define AR5416_PWR_TABLE_OFFSET  -5

#define SCALE_GAIN_HT20_7_REG (A_UINT32)(0xa398)
#define SCALE_GAIN_MASK       (A_UINT32)(0x1F)
#define PCDAC_PAL_OFF 44
#define PCDAC_PAL_ON    24

extern ar9287_eeprom_t *Ar9287EepromStructGet(void);
extern int Ar9287_LengthSet(int *value, int ix, int iy, int iz, int num, int iBand);
extern int Ar9287_ChecksumSet(int *value, int ix, int iy, int iz, int num, int iBand);
extern int Ar9287_ChecksumCalculate(void);
extern int Ar9287_eepromVersionSet(int *value, int ix, int iy, int iz, int num, int iBand);
extern int Ar9287_OpFlagsSet(int value);
extern int Ar9287_eepMiscSet(int value);
extern int Ar9287_RegDmnSet(int *value, int ix, int iy, int iz, int num, int iBand);
extern A_INT32 Ar9287_MacAdressSet(A_UINT8 *mac);
extern int Ar9287_RxMaskSet(int value);
extern int Ar9287_TxMaskSet(int value);
extern int Ar9287_RFSilentSet(int value);
extern int Ar9287_BlueToothOptionsSet(int value);
extern int Ar9287_DeviceCapSet(int value);
extern A_INT32 Ar9287_CalibrationBinaryVersionSet(int *value, int ix, int iy, int iz, int num, int iBand); 
extern int Ar9287_DeviceTypeSet(int value);
extern int Ar9287_OpenLoopPwrCntlSet(int value); 
extern int Ar9287_PwrTableOffsetSetSet(int value);
extern int Ar9287_TempSensSlopeSet(int value);
extern int Ar9287_TempSensSlopePalOnSet(int value);
extern int Ar9287_FutureBaseSet(int *value, int ix, int iy, int iz, int num, int iBand);
extern A_INT32 Ar9287_CustomerDataSet(A_UCHAR *data, A_INT32 len);
extern int Ar9287_AntCtrlChainSet(int *value, int ix, int iy, int iz, int num, int iBand);
extern int Ar9287_AntCtrlCommonSet(int value);
extern int Ar9287_AntennaGainSet(int *value, int ix, int iy, int iz, int num, int iBand);
extern int Ar9287_SwitchSettlingSet(int value);
extern int Ar9287_TxRxAttenChSet(int *value, int ix, int iy, int iz, int num, int iBand);
extern int Ar9287_RxTxMarginChSet(int *value, int ix, int iy, int iz, int num, int iBand);
extern int Ar9287_AdcDesiredSizeSet(int value);
extern int Ar9287_TxEndToXpaOffSet(int value);
extern int Ar9287_TxEndToRxOnSet(int value);
extern int Ar9287_TxFrameToXpaOnSet(int value);
extern int Ar9287_Thresh62Set(int value);
extern int Ar9287_NoiseFloorThreshChSet(int *value, int ix, int iy, int iz, int num, int iBand);
extern int Ar9287_XpdGainSet(int *value, int ix, int iy, int iz, int num, int iBand);
extern int Ar9287_XpdSet(int *value, int ix, int iy, int iz, int num, int iBand);
extern int Ar9287_IQCalIChSet(int *value, int ix, int iy, int iz, int num, int iBand);
extern int Ar9287_IQCalQChSet(int *value, int ix, int iy, int iz, int num, int iBand);
extern int Ar9287_PdGainOverlapSet(int value);
extern int Ar9287_XpaBiasLvlSet(int value);
extern int Ar9287_TxFrameToDataStartSet(int value);
extern int Ar9287_TxFrameToPaOnSet(int value);
extern int Ar9287_HT40PowerIncForPdadcSet(int value);
extern A_INT32 Ar9287_BswAttenSet(int *value, int ix, int iy, int iz, int num, int iBand);
extern A_INT32 Ar9287_BswMarginSet(int *value, int ix, int iy, int iz, int num, int iBand);
extern A_INT32 Ar9287_SwSettleHT40Set(int *value, int ix, int iy, int iz, int num, int iBand);
extern A_INT32 Ar9287_ModalHeaderVersionSet(int *value, int ix, int iy, int iz, int num, int iBand);
extern A_INT32 Ar9287_db1Set(int *value, int ix, int iy, int iz, int num, int iBand);
extern A_INT32 Ar9287_db2Set(int *value, int ix, int iy, int iz, int num, int iBand);
extern A_INT32 Ar9287_ob_cckSet(int *value, int ix, int iy, int iz, int num, int iBand);
extern A_INT32 Ar9287_ob_pskSet(int *value, int ix, int iy, int iz, int num, int iBand);
extern A_INT32 Ar9287_ob_qamSet(int *value, int ix, int iy, int iz, int num, int iBand);
extern A_INT32 Ar9287_ob_pal_offSet(int *value, int ix, int iy, int iz, int num, int iBand);
extern int Ar9287_FutureModalSet(int *value, int ix, int iy, int iz, int num, int iBand);
extern int Ar9287_SpurChansSet(int *value, int ix, int iy, int iz, int num, int iBand);
extern int Ar9287_SpurRangeLowSet(int *value, int ix, int iy, int iz, int num, int iBand);
extern int Ar9287_SpurRangeHighSet(int *value, int ix, int iy, int iz, int num, int iBand);
extern int Ar9287_CalPierFreqSet(int *value, int ix, int iy, int iz, int num, int iBand);
extern int Ar9287_CalPierDataSet(int *value, int ix, int iy, int iz, int num, int iBand);
extern int Ar9287_CalTgtDatacckSet(int *value, int ix, int iy, int iz, int num, int iBand);
extern int Ar9287_CalTgtDataSet(int *value, int ix, int iy, int iz, int num, int iBand);
extern int Ar9287_CalTgtDataHt20Set(int *value, int ix, int iy, int iz, int num, int iBand);
extern int Ar9287_CalTgtDataHt40Set(int *value, int ix, int iy, int iz, int num, int iBand);
extern A_INT32 Ar9287_CalTGTpwrcckSet(double *value, int ix, int iy, int iz, int num, int iBand);
extern A_INT32 Ar9287_CalTGTPwrLegacyOFDMSet(double *value, int ix, int iy, int iz, int num, int iBand);
extern A_INT32 Ar9287_CalTGTpwrht20Set(double *value, int ix, int iy, int iz, int num, int iBand);
extern A_INT32 Ar9287_CalTGTpwrht40Set(double *value, int ix, int iy, int iz, int num, int iBand);
extern A_INT32 Ar9287_CalTGTpwrcckChannelSet(int *value, int ix, int iy, int iz, int num, int iBand);
extern A_INT32 Ar9287_CalTGTPwrChannelSet(int *value, int ix, int iy, int iz, int num, int iBand);
extern A_INT32 Ar9287_CalTGTpwrht20ChannelSet(int *value, int ix, int iy, int iz, int num, int iBand);
extern A_INT32 Ar9287_CalTGTpwrht40ChannelSet(int *value, int ix, int iy, int iz, int num, int iBand);
extern A_INT32 Ar9287_CtlIndexSet(int *value, int ix, int iy, int iz, int num, int iBand);
extern int Ar9287_CtlDataSet(int *value, int ix, int iy, int iz, int num, int iBand);
extern int Ar9287_PaddingSet(int value);
//extern A_INT32 Ar9287_CalPierOpenPowerSet(int *value, int ix, int iy, int iz, int num, int iBand);
//extern A_INT32 Ar9287_CalPierOpenVoltMeasSet(int *value, int ix, int iy, int iz, int num, int iBand);
//extern A_INT32 Ar9287_CalPierOpenPcdacSet(int *value, int ix, int iy, int iz, int num, int iBand);
extern A_INT32 Ar9287_CtlPowerSet(double *value, int ix, int iy, int iz, int num, int iBand);
extern A_INT32 Ar9287_CtlFlagSet(int *value, int ix, int iy, int iz, int num, int iBand);
extern A_INT32 Ar9287_CtlChannelSet(int *value, int ix, int iy, int iz, int num, int iBand);
extern int Ar9287_RefPwrSet(int freq);
//extern int Ar9287_CalPierUpdate(int pierIdx, int freq, int chain, int pwrCorrection, int voltMeas, int tempMeas,double power);
extern A_INT32 Ar9287_CaldataMemoryTypeSet(A_UCHAR *memType);
extern void ar9287SwapEeprom(ar9287_eeprom_t *eep);
