/*
 * Copyright (c) 2002-2006 Atheros Communications, Inc.
 * All rights reserved.
 *
 */

// "ACI $Id: //depot/sw/branches/art2_main_per_cs/src/art2/devlib/ar9287/mEepStruct9287.h#1 $, $Header: //depot/sw/branches/art2_main_per_cs/src/art2/devlib/ar9287/mEepStruct9287.h#1 $"

#ifndef _KIWI_EEPROM_STRUCT_H_
#define _KIWI_EEPROM_STRUCT_H_

#define OSPREY_NUM_2G_CAL_PIERS      3

#define OSPREY_OPFLAGS_11G           0x02
#define OSPREY_OPFLAGS_2G_HT20       0x20
#define OSPREY_OPFLAGS_2G_HT40       0x08
#define OSPREY_EEPMISC_BIG_ENDIAN    0x01
#define OSPREY_EEPMISC_WOW           0x02

#define FREQ2FBIN(x,y) ((y) ? ((x) - 2300) : (((x) - 4800) / 5))
#define FBIN2FREQ(x,y) ((y) ? (2300 + x) : (4800 + 5 * x))

const static char *sDeviceType[] = {
  "   UNKNOWN  ",
  "   Cardbus  ",
  "     PCI    ",
  "   MiniPCI  ",
  "     AP     ",
  "  PCIE Mini ",
  "    PCIE    ",
  "PCIE Desktop",
  "NOT DEFINED ",
};

enum
{
	calibration_data_none = 0,
	calibration_data_dram,
	calibration_data_flash,
	calibration_data_eeprom,
	calibration_data_otp,
#ifdef ATH_CAL_NAND_FLASH
	calibration_data_nand,
#endif
	CalibrationDataDontLoad,
};

#endif //_KIWI_EEPROM_STRUCT_H_

