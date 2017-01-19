/*
 * Copyright (c) 2013 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 */

#if defined(AR9888_HEADERS_DEF)
#define AR9888 1

#define WLAN_HEADERS 1
#include "common_drv.h"
#include "soc_addrs.h"
#include "hw/apb_athr_wlan_map.h"
#include "hw/gpio_athr_wlan_reg.h"
#include "hw/rtc_soc_reg.h"
#include "hw/rtc_wlan_reg.h"
#include "hw/si_reg.h"
#include "extra/hw/pcie_local_reg.h"
#if 0
#include "hw/soc_core_reg.h"
#include "hw/soc_pcie_reg.h"
#include "hw/ce_reg_csr.h"
#endif

#include "soc_core_reg.h"
#include "hw/soc_pcie_reg.h"
#include "ce_reg_csr.h"

/* TBDXXX: Eventually, this Base Address will be defined in HW header files */
#define PCIE_LOCAL_BASE_ADDRESS 0x80000

#define FW_EVENT_PENDING_ADDRESS (SOC_CORE_BASE_ADDRESS+SCRATCH_3_ADDRESS)
#define DRAM_BASE_ADDRESS TARG_DRAM_START

/* Backwards compatibility -- TBDXXX */

#define MISSING 0

#define SYSTEM_SLEEP_OFFSET                     SOC_SYSTEM_SLEEP_OFFSET
#define WLAN_SYSTEM_SLEEP_OFFSET                SOC_SYSTEM_SLEEP_OFFSET
#define WLAN_RESET_CONTROL_OFFSET               SOC_RESET_CONTROL_OFFSET
#define CLOCK_CONTROL_OFFSET                    SOC_CLOCK_CONTROL_OFFSET
#define CLOCK_CONTROL_SI0_CLK_MASK              SOC_CLOCK_CONTROL_SI0_CLK_MASK
#define RESET_CONTROL_MBOX_RST_MASK             MISSING
#define RESET_CONTROL_SI0_RST_MASK              SOC_RESET_CONTROL_SI0_RST_MASK
#define GPIO_BASE_ADDRESS                       WLAN_GPIO_BASE_ADDRESS
#define GPIO_PIN0_OFFSET                        WLAN_GPIO_PIN0_ADDRESS
#define GPIO_PIN1_OFFSET                        WLAN_GPIO_PIN1_ADDRESS
#define GPIO_PIN0_CONFIG_MASK                   WLAN_GPIO_PIN0_CONFIG_MASK
#define GPIO_PIN1_CONFIG_MASK                   WLAN_GPIO_PIN1_CONFIG_MASK
#define SI_BASE_ADDRESS                         WLAN_SI_BASE_ADDRESS
#define SCRATCH_BASE_ADDRESS                    SOC_CORE_BASE_ADDRESS
#define LOCAL_SCRATCH_OFFSET                    0x18
#define CPU_CLOCK_OFFSET                        SOC_CPU_CLOCK_OFFSET
#define LPO_CAL_OFFSET                          SOC_LPO_CAL_OFFSET
#define GPIO_PIN10_OFFSET                       WLAN_GPIO_PIN10_ADDRESS
#define GPIO_PIN11_OFFSET                       WLAN_GPIO_PIN11_ADDRESS
#define GPIO_PIN12_OFFSET                       WLAN_GPIO_PIN12_ADDRESS
#define GPIO_PIN13_OFFSET                       WLAN_GPIO_PIN13_ADDRESS
#define CPU_CLOCK_STANDARD_LSB                  SOC_CPU_CLOCK_STANDARD_LSB
#define CPU_CLOCK_STANDARD_MASK                 SOC_CPU_CLOCK_STANDARD_MASK
#define LPO_CAL_ENABLE_LSB                      SOC_LPO_CAL_ENABLE_LSB
#define LPO_CAL_ENABLE_MASK                     SOC_LPO_CAL_ENABLE_MASK
#define ANALOG_INTF_BASE_ADDRESS                WLAN_ANALOG_INTF_BASE_ADDRESS
#define MBOX_BASE_ADDRESS                       MISSING
#define INT_STATUS_ENABLE_ERROR_LSB             MISSING
#define INT_STATUS_ENABLE_ERROR_MASK            MISSING
#define INT_STATUS_ENABLE_CPU_LSB               MISSING
#define INT_STATUS_ENABLE_CPU_MASK              MISSING
#define INT_STATUS_ENABLE_COUNTER_LSB           MISSING
#define INT_STATUS_ENABLE_COUNTER_MASK          MISSING
#define INT_STATUS_ENABLE_MBOX_DATA_LSB         MISSING
#define INT_STATUS_ENABLE_MBOX_DATA_MASK        MISSING
#define ERROR_STATUS_ENABLE_RX_UNDERFLOW_LSB    MISSING
#define ERROR_STATUS_ENABLE_RX_UNDERFLOW_MASK   MISSING
#define ERROR_STATUS_ENABLE_TX_OVERFLOW_LSB     MISSING
#define ERROR_STATUS_ENABLE_TX_OVERFLOW_MASK    MISSING
#define COUNTER_INT_STATUS_ENABLE_BIT_LSB       MISSING
#define COUNTER_INT_STATUS_ENABLE_BIT_MASK      MISSING
#define INT_STATUS_ENABLE_ADDRESS               MISSING
#define CPU_INT_STATUS_ENABLE_BIT_LSB           MISSING
#define CPU_INT_STATUS_ENABLE_BIT_MASK          MISSING
#define HOST_INT_STATUS_ADDRESS                 MISSING
#define CPU_INT_STATUS_ADDRESS                  MISSING
#define ERROR_INT_STATUS_ADDRESS                MISSING
#define ERROR_INT_STATUS_WAKEUP_MASK            MISSING
#define ERROR_INT_STATUS_WAKEUP_LSB             MISSING
#define ERROR_INT_STATUS_RX_UNDERFLOW_MASK      MISSING
#define ERROR_INT_STATUS_RX_UNDERFLOW_LSB       MISSING
#define ERROR_INT_STATUS_TX_OVERFLOW_MASK       MISSING
#define ERROR_INT_STATUS_TX_OVERFLOW_LSB        MISSING
#define COUNT_DEC_ADDRESS                       MISSING
#define HOST_INT_STATUS_CPU_MASK                MISSING
#define HOST_INT_STATUS_CPU_LSB                 MISSING
#define HOST_INT_STATUS_ERROR_MASK              MISSING
#define HOST_INT_STATUS_ERROR_LSB               MISSING
#define HOST_INT_STATUS_COUNTER_MASK            MISSING
#define HOST_INT_STATUS_COUNTER_LSB             MISSING
#define RX_LOOKAHEAD_VALID_ADDRESS              MISSING
#define WINDOW_DATA_ADDRESS                     MISSING
#define WINDOW_READ_ADDR_ADDRESS                MISSING
#define WINDOW_WRITE_ADDR_ADDRESS               MISSING

#define MY_TARGET_DEF AR9888_TARGETdef
#define MY_HOST_DEF AR9888_HOSTdef
#define MY_TARGET_BOARD_DATA_SZ AR9888_BOARD_DATA_SZ
#define MY_TARGET_BOARD_EXT_DATA_SZ AR9888_BOARD_EXT_DATA_SZ
#include "targetdef.h"
#include "hostdef.h"
#else
#include "common_drv.h"
#include "targetdef.h"
#include "hostdef.h"
struct targetdef_s *AR9888_TARGETdef=NULL;
struct hostdef_s *AR9888_HOSTdef=NULL;
#endif /*AR9888_HEADERS_DEF */
