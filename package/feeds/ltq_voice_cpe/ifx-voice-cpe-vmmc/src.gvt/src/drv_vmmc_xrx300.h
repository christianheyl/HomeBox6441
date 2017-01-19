#ifndef _DRV_VMMC_GPIO_H
#define _DRV_VMMC_GPIO_H
/******************************************************************************

                              Copyright (c) 2013
                            Lantiq Deutschland GmbH
                             http://www.lantiq.com

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

*******************************************************************************/

/**
   \file drv_vmmc_xrx300.h
   This file contains macros to set the GPIOs for TDM bus operation.
*/
#ifdef LINUX
#if defined(SYSTEM_XRX300)
   #if (BSP_API_VERSION < 3)
      #include <asm/ifx/ifx_gpio.h>
   #else
      #include <ltq_gpio.h>
   #endif
#else
   #error no system selected
#endif
#endif /* LINUX */

#define VMMC_TAPI_GPIO_MODULE_ID                      IFX_GPIO_MODULE_TAPI_VMMC

/**
*/
#define VMMC_PCM_IF_CFG_HOOK(mode, GPIOreserved, ret) \
do { \
   ret = VMMC_statusOk; \
   /* Reserve P1.9 as TDM/DO */ \
   if (!GPIOreserved) \
   ret |= ifx_gpio_pin_reserve(IFX_GPIO_PIN_ID(1, 9), VMMC_TAPI_GPIO_MODULE_ID); \
   ret |= ifx_gpio_altsel0_set(IFX_GPIO_PIN_ID(1, 9), VMMC_TAPI_GPIO_MODULE_ID); \
   ret |= ifx_gpio_altsel1_clear(IFX_GPIO_PIN_ID(1, 9), VMMC_TAPI_GPIO_MODULE_ID); \
   ret |= ifx_gpio_dir_out_set(IFX_GPIO_PIN_ID(1, 9), VMMC_TAPI_GPIO_MODULE_ID); \
   ret |= ifx_gpio_open_drain_set(IFX_GPIO_PIN_ID(1, 9), VMMC_TAPI_GPIO_MODULE_ID); \
   \
   /* Reserve P1.10 as TDM/DI */ \
   if (!GPIOreserved) \
   ret |= ifx_gpio_pin_reserve(IFX_GPIO_PIN_ID(1, 10), VMMC_TAPI_GPIO_MODULE_ID); \
   ret |= ifx_gpio_altsel0_clear(IFX_GPIO_PIN_ID(1, 10), VMMC_TAPI_GPIO_MODULE_ID); \
   ret |= ifx_gpio_altsel1_set(IFX_GPIO_PIN_ID(1, 10), VMMC_TAPI_GPIO_MODULE_ID);\
   ret |= ifx_gpio_dir_in_set(IFX_GPIO_PIN_ID(1, 10), VMMC_TAPI_GPIO_MODULE_ID); \
   \
   /* Reserve P1.11 as TDM/DCL */ \
   if (!GPIOreserved) \
   ret |= ifx_gpio_pin_reserve(IFX_GPIO_PIN_ID(1, 11), VMMC_TAPI_GPIO_MODULE_ID); \
   ret |= ifx_gpio_altsel0_set(IFX_GPIO_PIN_ID(1, 11), VMMC_TAPI_GPIO_MODULE_ID); \
   ret |= ifx_gpio_altsel1_clear(IFX_GPIO_PIN_ID(1, 11), VMMC_TAPI_GPIO_MODULE_ID); \
   ret |= ifx_gpio_open_drain_set(IFX_GPIO_PIN_ID(1, 11), VMMC_TAPI_GPIO_MODULE_ID); \
   \
   /* Reserve P3.10 as TDM/FSC */ \
   if (!GPIOreserved) \
   ret |= ifx_gpio_pin_reserve(IFX_GPIO_PIN_ID(3, 10), VMMC_TAPI_GPIO_MODULE_ID); \
   ret |= ifx_gpio_altsel0_clear(IFX_GPIO_PIN_ID(3, 10), VMMC_TAPI_GPIO_MODULE_ID); \
   ret |= ifx_gpio_altsel1_set(IFX_GPIO_PIN_ID(3, 10), VMMC_TAPI_GPIO_MODULE_ID); \
   ret |= ifx_gpio_open_drain_set(IFX_GPIO_PIN_ID(3, 10), VMMC_TAPI_GPIO_MODULE_ID);\
   \
   if (mode == 2) { \
      /* TDM/FSC+DCL Master */ \
      ret |= ifx_gpio_dir_out_set(IFX_GPIO_PIN_ID(1, 11), VMMC_TAPI_GPIO_MODULE_ID); \
      ret |= ifx_gpio_dir_out_set(IFX_GPIO_PIN_ID(3, 10), VMMC_TAPI_GPIO_MODULE_ID); \
   } else { \
      /* TDM/FSC+DCL Slave */ \
      ret |= ifx_gpio_dir_in_set(IFX_GPIO_PIN_ID(1, 11), VMMC_TAPI_GPIO_MODULE_ID); \
      ret |= ifx_gpio_dir_in_set(IFX_GPIO_PIN_ID(3, 10), VMMC_TAPI_GPIO_MODULE_ID); \
   } \
} while(0);

/**
*/
#define VMMC_DRIVER_UNLOAD_HOOK(ret) \
do { \
   ret = VMMC_statusOk; \
   ret |= ifx_gpio_pin_free(IFX_GPIO_PIN_ID(1,  9), VMMC_TAPI_GPIO_MODULE_ID); \
   ret |= ifx_gpio_pin_free(IFX_GPIO_PIN_ID(1, 10), VMMC_TAPI_GPIO_MODULE_ID); \
   ret |= ifx_gpio_pin_free(IFX_GPIO_PIN_ID(1, 11), VMMC_TAPI_GPIO_MODULE_ID); \
   ret |= ifx_gpio_pin_free(IFX_GPIO_PIN_ID(3, 10), VMMC_TAPI_GPIO_MODULE_ID); \
} while (0)

extern unsigned int ifx_get_cpu_hz(void); /* exported by the CGU driver */

/** Returns the MIPS core frequency in Hz. */
#define VMMC_GET_MIPS_HZ() \
   ifx_get_cpu_hz()

#endif /* _DRV_VMMC_GPIO_H */
