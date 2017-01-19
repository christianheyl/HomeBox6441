/******************************************************************************
**
** FILE NAME    : hn1_eval_board.h
** MODULES      : BSP Basic
**
** DATE         : 11 Jan 2011
** AUTHOR       : Kishore Kankipati
** DESCRIPTION  : header file for HN1
** COPYRIGHT    :       Copyright (c) 2009
**                      Lantiq Deutschland GmbH
**                      Am Campeon 3, 85579 Neubiberg, Germany
**
**    This program is free software; you can redistribute it and/or modify
**    it under the terms of the GNU General Public License as published by
**    the Free Software Foundation; either version 2 of the License, or
**    (at your option) any later version.
**
** HISTORY
** $Date        $Author         $Comment
** 11 Jan 2011   Kishore 	First version for HN1 derived from VR9
*******************************************************************************/

#ifndef HN1_EVAL_BOARD_H
#define HN1_EVAL_BOARD_H
#ifndef AUTOCONF_INCLUDED
#include <linux/config.h>
#endif /* AUTOCONF_INCLUDED */

#if defined(CONFIG_IFX_SPI_FLASH) || defined (CONFIG_IFX_SPI_FLASH_MODULE) \
    || defined(CONFIG_IFX_USIF_SPI_FLASH) || defined (CONFIG_IFX_USIF_SPI_FLASH_MODULE)
#define IFX_MTD_SPI_PART_NB               3
#define IFX_SPI_FLASH_MAX                 8
#endif /* defined(CONFIG_IFX_SPI_FLASH) || defined (CONFIG_IFX_SPI_FLASH_MODULE) */

#endif  /* HN1_EVAL_BOARD_H */

