#ifndef	_QC9K_EEPROM_H_
#define	_QC9K_EEPROM_H_

#ifdef QC98XXDLL
#include "qc98xx_eeprom.h"
#endif //QC98XXDLL

typedef enum Ar_Rates {
    rate6mb,  rate9mb,  rate12mb, rate18mb,
    rate24mb, rate36mb, rate48mb, rate54mb,
    rate1l,   rate2l,   rate2s,   rate5_5l,
    rate5_5s, rate11l,  rate11s,  rateXr,
    rateHt20_0, rateHt20_1, rateHt20_2, rateHt20_3,
    rateHt20_4, rateHt20_5, rateHt20_6, rateHt20_7,
    rateHt40_0, rateHt40_1, rateHt40_2, rateHt40_3,
    rateHt40_4, rateHt40_5, rateHt40_6, rateHt40_7,
    rateDupCck, rateDupOfdm, rateExtCck, rateExtOfdm,
    ArRateSize, Ar5416RateSize = ArRateSize,
} AR_RATES;


extern A_UINT8 *pQc9kEepromArea;
extern A_UINT8 *pQc9kEepromBoardArea;

#endif //_QC9K_EEPROM_H_
