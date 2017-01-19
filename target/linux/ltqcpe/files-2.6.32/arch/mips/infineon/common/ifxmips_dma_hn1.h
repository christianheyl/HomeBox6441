/**
** FILE NAME    : ifxmips_dma_hn1.h
** PROJECT      : HN1
** MODULES      : Central DMA
** DATE         : 11 January 2011
** AUTHOR       : Kishore Kankipati
** DESCRIPTION  : LANTIQ HN1 Central DMA driver header file
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
** $Date            $Author         $Comment
** 11 January 2011 Kishore Kankipati  Initial HN1 release (file derived from VR9)
** 21 January 2011 Yinglei 	      
** 13 May 2011     Kishore Kankipati  Mapping of DMA channels is wrong in earlier
			 	      version. These are corrected.
*******************************************************************************/

#ifndef _IFXMIPS_DMA_HN1_H_
#define _IFXMIPS_DMA_HN1_H_

/*!
  \file ifxmips_dma_hn1.h
  \ingroup IFX_DMA_CORE
  \brief Header file for IFX Central DMA  driver internal definition for HN1 platform.
*/

/* ============================= */
/* Local Macros & Definitions    */
/* ============================= */

/*! \enum ifx_dma_chan_dir_t
 \brief DMA Rx/Tx channel
*/
typedef enum {
    IFX_DMA_RX_CH = 0,  /*!< Rx channel */
    IFX_DMA_TX_CH = 1,  /*!< Tx channel */
} ifx_dma_chan_dir_t;

/*! \typedef _dma_chan_map
 \brief The parameter structure is used to dma channel map
*/
typedef struct ifx_dma_chan_map{
    int 				port_num;       /*!< Port number */
    char    			dev_name[16];   /*!< Device Name */
    ifx_dma_chan_dir_t 	dir;            /*!< Direction of the DMA channel */
    int     			pri;            /*!< Class value */
    int     			irq;            /*!< DMA channel irq number(refer to the platform irq.h file) */
    int     			rel_chan_no;    /*!< Relative channel number */
} _dma_chan_map;

/* HN1 Supported Devices */
static char dma_device_name[MAX_DMA_DEVICE_NUM][20] = {{"SWITCH"},{"SPI" },{"SDIO" },{"MCTRL0"},{"USIF"  },{"HSNAND"}};
static _dma_chan_map ifx_default_dma_map[MAX_DMA_CHANNEL_NUM] = {
    /* portnum, device name, channel direction, class value, IRQ number, relative channel number */
    {0, "SWITCH",      IFX_DMA_RX_CH,  0,  IFX_DMA_CH0_INT,    0},
    {0, "SWITCH",      IFX_DMA_TX_CH,  0,  IFX_DMA_CH1_INT,    0},
    {0, "SWITCH",      IFX_DMA_RX_CH,  1,  IFX_DMA_CH2_INT,    1},
    {0, "SWITCH",      IFX_DMA_TX_CH,  1,  IFX_DMA_CH3_INT,    1},
    {0, "SWITCH",      IFX_DMA_RX_CH,  2,  IFX_DMA_CH4_INT,    2},
    {0, "SWITCH",      IFX_DMA_TX_CH,  2,  IFX_DMA_CH5_INT,    2},
    {0, "SWITCH",      IFX_DMA_RX_CH,  3,  IFX_DMA_CH6_INT,    3},
    {0, "SWITCH",      IFX_DMA_TX_CH,  3,  IFX_DMA_CH7_INT,    3},
    {1, "",            IFX_DMA_RX_CH,  0,  IFX_DMA_CH8_INT,    0},
    {1, "",            IFX_DMA_TX_CH,  0,  IFX_DMA_CH9_INT,    0},
    {1, "",            IFX_DMA_RX_CH,  1,  IFX_DMA_CH10_INT,   1},
    {1, "",            IFX_DMA_TX_CH,  1,  IFX_DMA_CH11_INT,   1},
    {2, "SPI",         IFX_DMA_RX_CH,  0,  IFX_DMA_CH12_INT,   0},
    {2, "SPI",         IFX_DMA_TX_CH,  0,  IFX_DMA_CH13_INT,   0},
    {3, "SDIO",        IFX_DMA_RX_CH,  0,  IFX_DMA_CH14_INT,   0},
    {3, "SDIO",        IFX_DMA_TX_CH,  0,  IFX_DMA_CH15_INT,   0},
    {4, "MCTRL0",      IFX_DMA_RX_CH,  0,  IFX_DMA_CH16_INT,   0},
    {4, "MCTRL0",      IFX_DMA_TX_CH,  0,  IFX_DMA_CH17_INT,   0},
    {4, "MCTRL1",      IFX_DMA_RX_CH,  1,  IFX_DMA_CH18_INT,   1},
    {4, "MCTRL1",      IFX_DMA_TX_CH,  1,  IFX_DMA_CH19_INT,   1},
    {0, "",            IFX_DMA_RX_CH,  4,  IFX_DMA_CH20_INT,   4},
    {0, "",            IFX_DMA_RX_CH,  5,  IFX_DMA_CH21_INT,   5},
    {0, "",            IFX_DMA_RX_CH,  6,  IFX_DMA_CH22_INT,   6},
    {0, "",            IFX_DMA_RX_CH,  7,  IFX_DMA_CH23_INT,   7},
    {5, "USIF",        IFX_DMA_RX_CH,  0,  IFX_DMA_CH24_INT,   0},
    {5, "USIF",        IFX_DMA_TX_CH,  0,  IFX_DMA_CH25_INT,   0},
    {6, "HSNAND",      IFX_DMA_RX_CH,  0,  IFX_DMA_CH26_INT,   0},
    {6, "HSNAND",      IFX_DMA_TX_CH,  0,  IFX_DMA_CH27_INT,   0},
};

#endif /* _IFXMIPS_DMA_HN1_H_ */
