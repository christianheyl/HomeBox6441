/*
 * Copyright (c) 2002-2004 Atheros Communications, Inc.
 * All rights reserved.
 *
 * $Id: //depot/sw/branches/art2_main_per_cs/src/art2/devlib/qc98xx/Qc98xxmEep.h#1 $
 */
#ifndef _QC98XX_M_EEP_H_
#define _QC98XX_M_EEP_H_

extern A_UINT8 Qc98xxEepromArea[QC98XX_EEPROM_SIZE_LARGEST];
extern A_UINT8 Qc98xxEepromBoardArea[QC98XX_EEPROM_SIZE_LARGEST];

extern void qc98xxInitTemplateTbl();
extern A_BOOL qc98xxEepromAttach();

extern A_BOOL readCalDataFromFile(char *fileName, QC98XX_EEPROM *eepromData, A_UINT32 *bytes);
extern void computeChecksum(QC98XX_EEPROM *pEepStruct);

extern int Qc98xxUserRateIndex2Stream (A_UINT16 userRateGroupIndex);
extern int Qc98xxRateIndex2Stream (A_UINT16 rateIndex);
extern int Qc98xxRateGroupIndex2Stream (A_UINT16 rateGroupIndex, A_UINT16 neighborRateIndex);
extern A_BOOL Qc98xxIsRateInStream (A_UINT32 stream, A_UINT16 rateIndex);

extern int Qc98xxEepromTemplatePreference(int templateId);
extern int Qc98xxGetEepromTemplatePreference();
extern int Qc98xxEepromStructDefaultMany(void);
extern int Qc98xxEepromTemplateVersionValid (int templateVersion);
extern QC98XX_EEPROM *Qc98xxEepromStructDefault(int index) ;
extern QC98XX_EEPROM *Qc98xxEepromStructDefaultFindById(int templateId);
extern QC98XX_EEPROM *Qc98xxEepromStructDefaultFindByTemplateVersion(int templateVer); 
extern char *Qc98xxGetTemplateNameGivenVersion(int templateVer);

#endif  //_QC98XX_M_EEP_H_
