/******************************************************************************
**
** FILE NAME    : ifxmips_rcu_hn1.h
** PROJECT      : UEIP
** MODULES      : RCU (Reset Control Unit)
**
** DATE         : 25 March 2011
** AUTHOR       : Yinglei Huang
** DESCRIPTION  : RCU driver header file for HN1
** COPYRIGHT    :       Copyright (c) 2006
**                      Lantiq Technologies AG
**                      Am Campeon 1-12, 85579 Neubiberg, Germany
**
**    This program is free software; you can redistribute it and/or modify
**    it under the terms of the GNU General Public License as published by
**    the Free Software Foundation; either version 2 of the License, or
**    (at your option) any later version.
**
** HISTORY
** $Date           $Author               $Comment
** 25 March 2011    Yinglei Huang        Initially ported from vr9
*******************************************************************************/



#ifndef IFXMIPS_RCU_HN1_H
#define IFXMIPS_RCU_HN1_H

static ifx_rcu_domain_t g_rcu_domains[IFX_RCU_DOMAIN_MAX] = {
    {
        .affected_domains   = 1 << IFX_RCU_DOMAIN_HRST,
        .rst_req_value      = 1 << 0,
        .rst_req_mask       = 1 << 0,
        .rst_stat_mask      = 1 << 0,
        .latch              = 0,
        .udelay             = 0,
        .handlers           = NULL,
    },
    {
        .affected_domains   = 1 << IFX_RCU_DOMAIN_CPU0,
        .rst_req_value      = 1 << 1,
        .rst_req_mask       = 1 << 1,
        .rst_stat_mask      = 1 << 1,
        .latch              = 0,
        .udelay             = 0,
        .handlers           = NULL,
    },
    {
        .affected_domains   = 1 << IFX_RCU_DOMAIN_FPI,
        .rst_req_value      = 1 << 2,
        .rst_req_mask       = 1 << 2,
        .rst_stat_mask      = 1 << 2,
        .latch              = 0,
        .udelay             = 0,
        .handlers           = NULL,
    },
    {
        .affected_domains   = (1 << IFX_RCU_DOMAIN_DLLCORE),
        .rst_req_value      = 1 << 3,
        .rst_req_mask       = 1 << 3,
        .rst_stat_mask      = 1 << 3,
        .latch              = 0,
        .udelay             = 0,
        .handlers           = NULL,
    },
    {
        .affected_domains   = (1 << IFX_RCU_DOMAIN_I2C),
        .rst_req_value      = 1 << 4,
        .rst_req_mask       = 1 << 4,
        .rst_stat_mask      = 1 << 4,
        .latch              = 0,
        .udelay             = 0,
        .handlers           = NULL,
    },
    {
        .affected_domains   = 1 << IFX_RCU_DOMAIN_AHB,
        .rst_req_value      = 1 << 6,
        .rst_req_mask       = 1 << 6,
        .rst_stat_mask      = 1 << 6,
        .latch              = 0,
        .udelay             = 0,
        .handlers           = NULL,
    },
    {
        .affected_domains   = 1 << IFX_RCU_DOMAIN_BULKSRAM,
        .rst_req_value      = 1 << 7,
        .rst_req_mask       = 1 << 7,
        .rst_stat_mask      = 1 << 7,
        .latch              = 0,
        .udelay             = 0,
        .handlers           = NULL,
    },
    {
        .affected_domains   = 1 << IFX_RCU_DOMAIN_I2S,
        .rst_req_value      = 1 << 8,
        .rst_req_mask       = 1 << 8,
        .rst_stat_mask      = 1 << 8,
        .latch              = 0,
        .udelay             = 0,
        .handlers           = NULL,
    },
    {
        .affected_domains   = 1 << IFX_RCU_DOMAIN_DMA,
        .rst_req_value      = 1 << 9,
        .rst_req_mask       = 1 << 9,
        .rst_stat_mask      = 1 << 9,
        .latch              = 0,
        .udelay             = 0,
        .handlers           = NULL,
    },
    {
        .affected_domains   = 1 << IFX_RCU_DOMAIN_SDIO,
        .rst_req_value      = 1 << 10,
        .rst_req_mask       = 1 << 10,
        .rst_stat_mask      = 1 << 10,
        .latch              = 0,
        .udelay             = 0,
        .handlers           = NULL,
    },
    {
        .affected_domains   = 1 << IFX_RCU_DOMAIN_PHYCORE,
        .rst_req_value      = 1 << 11,
        .rst_req_mask       = 1 << 11,
        .rst_stat_mask      = 1 << 11,
        .latch              = 0,
        .udelay             = 0,
        .handlers           = NULL,
    },
    {
        .affected_domains   = 1 << IFX_RCU_DOMAIN_PCIEPHY,
        .rst_req_value      = 1 << 12,
        .rst_req_mask       = 1 << 12,
        .rst_stat_mask      = 1 << 24,   
        .latch              = 0,
        .udelay             = 0,
        .handlers           = NULL,
    },
    {
        .affected_domains   = 1 << IFX_RCU_DOMAIN_MC,
        .rst_req_value      = 1 << 14,
        .rst_req_mask       = 1 << 14,
        .rst_stat_mask      = 1 << 14,
        .latch              = 0,
        .udelay             = 0,
        .handlers           = NULL,
    },
    {
        .affected_domains   = 1 << IFX_RCU_DOMAIN_HSNAND,
        .rst_req_value      = 1 << 16,
        .rst_req_mask       = 1 << 16,
        .rst_stat_mask      = 1 << 5,
        .latch              = 0,
        .udelay             = 0,
        .handlers           = NULL,
    },
    {
        .affected_domains   = 1 << IFX_RCU_DOMAIN_TDM,
        .rst_req_value      = 1 << 19,
        .rst_req_mask       = 1 << 19,
        .rst_stat_mask      = 1 << 25,
        .latch              = 0,
        .udelay             = 0,
        .handlers           = NULL,
    },
    {
        .affected_domains   = 1 << IFX_RCU_DOMAIN_SW,
        .rst_req_value      = 1 << 21,
        .rst_req_mask       = 1 << 21,
        .rst_stat_mask      = 1 << 16,
        .latch              = 0,
        .udelay             = 0,
        .handlers           = NULL,
    },
    {
        .affected_domains   = 1 << IFX_RCU_DOMAIN_PCIE,
        .rst_req_value      = 1 << 22,
        .rst_req_mask       = 1 << 22,
        .rst_stat_mask      = 1 << 22,
        .latch              = 0,
        .udelay             = 0,
        .handlers           = NULL,
    },
    {
        .affected_domains   = 1 << IFX_RCU_DOMAIN_AHBDLL,
        .rst_req_value      = 1 << 23,
        .rst_req_mask       = 1 << 23,
        .rst_stat_mask      = 1 << 23,
        .latch              = 0,
        .udelay             = 0,
        .handlers           = NULL,
    },
    {
        .affected_domains   = 1 << IFX_RCU_DOMAIN_GPHY0,
        .rst_req_value      = 1 << 31,
        .rst_req_mask       = 1 << 31,
        .rst_stat_mask      = 1 << 30,
        .latch              = 0,
        .udelay             = 0,
        .handlers           = NULL,
    },
};

#endif  //  IFXMIPS_RCU_HN1_H
