/* Copyright (c) 2002-2004 Sam Leffler, Errno Consulting
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer,
    without modification.
 * 2. Redistributions in binary form must reproduce at minimum a disclaimer
 *    similar to the "NO WARRANTY" disclaimer below ("Disclaimer") and any
 *    redistribution must be conditioned upon including a substantially
 *    similar Disclaimer requirement for further binary redistribution.
 * 3. Neither the names of the above-listed copyright holders nor the names
 *    of any contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 *
 * NO WARRANTY
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF NONINFRINGEMENT, MERCHANTIBILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
 * THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGES.
 *
 * $Id: //depot/sw/qca_main/components/system/lsdk-build/1.1/build/scripts/lq-platform/carrier/lantiq/ath_carr_pltfrm.h#1 $
 */

/*
 * Defintions for the Atheros Wireless LAN controller driver.
 */
#ifndef _ATH_CARR_PLTFRM_H_ 
#define _ATH_CARR_PLTFRM_H_

#include <linux/autoconf.h> /* __KERNEL__ is defined when compile hal.o for linux platform, so need to include it */

/* Note: modify the address to reflect real world usage */
#define CARRIER_EEPROM_START_ADDR 0x12345678
#define CARRIER_EEPROM_MAX  0xae0

#define CARRIER_PLTFRM_PRIVATE_SET __stringify(ppa)
#define CARRIER_PLTFRM_PRIVATE_GET __stringify(get_ppa)    

#define ath_carr_get_cal_mem_legacy(_cal_mem) \
    do {        \
        _cal_mem = OS_REMAP(CARRIER_EEPROM_START_ADDR, CARRIER_EEPROM_MAX); \
    } while (0)

#define ath_carr_mb7x_delay(_nsec) do {} while (0)
#define ath_carr_merlin_fill_reg_shadow(_ah, _reg, _val)  do {} while (0) 

#define ath_carr_merlin_update_ob_rf5g_ch0(_ah, _reg_val) \
    do { \
        (_reg_val) = OS_REG_READ(ah, AR_AN_RF5G1_CH0);                \
        (_reg_val) &= (~AR_AN_RF5G1_CH0_OB5) & (~AR_AN_RF5G1_CH0_DB5); \
    } while (0)

#define ath_carr_merlin_update_ob_rf5g_ch1(_ah, _reg_val) \
    do { \
        (_reg_val) = OS_REG_READ(ah, AR_AN_RF5G1_CH1);  \
    } while (0)

#define ath_carr_merlin_update_ob_rf2g_ch0(_ah, _reg_val) \
    do { \
            (_reg_val) = OS_REG_READ(ah, AR_AN_RF2G1_CH0); \
            (_reg_val) &= (~AR_AN_RF2G1_CH0_OB) & (~AR_AN_RF2G1_CH0_DB); \
    } while (0)

#define ath_carr_merlin_update_ob_rf2g_ch1(_ah, _reg_val) \
    do { \
            (_reg_val) = OS_REG_READ(ah, AR_AN_RF2G1_CH1); \
    } while (0)

#define ath_carr_merline_reg_read_an_top2(_ah, _reg_val) \
    do { \
            (_reg_val) = OS_REG_READ(ah, AR_AN_TOP2); \
    } while (0)

static const u_int32_t ar5212Modes_2417_Carr[][6] = {
};

static const u_int32_t ar5212Common_2417_Carr[][2] = {
};

#define HTT_RX_HOST_LATENCY_WORST_LIKELY_MS 2 /* ms */ /* conservative */

#ifdef PLATFORM_BYTE_SWAP

#define A_PCI_READ32(addr)         __bswap32(ioread32((void __iomem *)addr))
#define A_PCI_WRITE32(addr, value) iowrite32(__bswap32((u32)(value)), (void __iomem *)(addr))

/**
 * Move macro definition here from ah_osdep.h!
 * Assume that infineon platform and pb42 have the same way to access to h/w register.
 */
#define _OS_REG_WRITE(_ah, _reg, _val) do {                     \
        writel(__bswap32((_val)),((volatile u_int32_t *)(AH_PRIVATE(_ah)->ah_sh + (_reg))));   \
} while(0)
#define _OS_REG_READ(_ah, _reg) \
        __bswap32(readl((volatile u_int32_t *)(AH_PRIVATE(_ah)->ah_sh + (_reg))))
#else /* PLATFORM_BYTE_SWAP */

#define A_PCI_READ32(addr)         ioread32((void __iomem *)addr)
#define A_PCI_WRITE32(addr, value) iowrite32((u32)(value), (void __iomem *)(addr))

/**
 * Move macro definition here from ah_osdep.h!
 * Assume that infineon platform and pb42 have the same way to access to h/w register.
 */
#define _OS_REG_WRITE(_ah, _reg, _val) do {                     \
        writel((_val),((volatile u_int32_t *)(AH_PRIVATE(_ah)->ah_sh + (_reg))));   \
} while(0)
#define _OS_REG_READ(_ah, _reg) \
        readl((volatile u_int32_t *)(AH_PRIVATE(_ah)->ah_sh + (_reg)))

#endif /* PLATFORM_BYTE_SWAP */

#endif /* _ATH_CARR_1_H_ */
