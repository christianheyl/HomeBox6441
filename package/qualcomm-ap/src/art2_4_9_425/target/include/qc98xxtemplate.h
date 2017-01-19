/*
 * Copyright (c) 2012 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 * $ATH_LICENSE_TARGET_C$
 */

/*
 * READ THIS NOTICE!
 *
 * Values defined in this file may only be changed under exceptional circumstances.
 *
 * Please ask Fiona Cain before making any changes.
 */

#ifndef __qc98xxtemplate_h__
#define __qc98xxtemplate_h__

// This qc98xx_eeprom_template_base should be equal to qc98xx_eeprom_template_generic in templatelist.h
#define qc98xx_eeprom_template_base 20

// Template version 1 in equivalent to template ID qc98xx_eeprom_template_base
#define QC98XX_FIRST_TEMPLATEVERSION				1
#define QC98XX_TEMPLATEVERSION_TO_TEMPLATEID(ver)	(ver - QC98XX_FIRST_TEMPLATEVERSION + qc98xx_eeprom_template_base)
#define QC98XX_TEMPLATEID_TO_TEMPLATEVERSION(id)	(id + QC98XX_FIRST_TEMPLATEVERSION - qc98xx_eeprom_template_base)



#endif //__qc98xxtemplate_h__
