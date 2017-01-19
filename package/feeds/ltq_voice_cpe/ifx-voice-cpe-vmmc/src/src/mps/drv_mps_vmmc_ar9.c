/******************************************************************************

                              Copyright (c) 2013
                            Lantiq Deutschland GmbH
                             http://www.lantiq.com

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

****************************************************************************
   Module      : drv_mps_vmmc_ar9.c
   Description : This file contains the implementation of the AR9/VR9 specific
                 driver functions.
*******************************************************************************/

/* ============================= */
/* Includes                      */
/* ============================= */
#include "drv_config.h"
#ifdef LINUX
#include "drv_mps_vmmc_bsp.h"
#endif /* LINUX */

#if (defined(GENERIC_OS) && defined(GREENHILLS_CHECK))
   #include "drv_vmmc_ghs.h"
#endif /* (defined(GENERIC_OS) && defined(GREENHILLS_CHECK)) */

/* defined in drv_config.h */
#if defined(SYSTEM_AR9) || defined(SYSTEM_VR9) || defined(SYSTEM_XRX300)

/* lib_ifxos headers */
#include "ifx_types.h"
#include "ifxos_linux_drv.h"
#include "ifxos_copy_user_space.h"
#include "ifxos_event.h"
#include "ifxos_lock.h"
#include "ifxos_select.h"
#include "ifxos_interrupt.h"

#ifdef LINUX
/* board specific headers */
#if (BSP_API_VERSION < 3)
   #include <asm/ifx_vpe.h>
   #include <asm/ifx/ifx_regs.h>
   #include <asm/ifx/ifx_gpio.h>
#else
   #include <asm/ltq_vpe.h>
   #include <ltq_regs.h>
   #include <ltq_gpio.h>
#endif
#endif /* LINUX */

/* device specific headers */
#include "drv_mps_vmmc.h"
#include "drv_mps_vmmc_dbg.h"
#include "drv_mps_vmmc_device.h"

#if defined(ARCADYAN_TARGET_kpn_VGV951ABWAC23)
#include <asm/ifx/ifx_ledc.h>
#include "ifxos_time.h"
#elif defined(ARCADYAN_TARGET_o2_VRV9518SWAC33) || defined(ARCADYAN_TARGET_zz_VRV9518SWAC33) || defined(ARCADYAN_TARGET_o2_VRV9518SWAC24) || defined(ARCADYAN_TARGET_zz_VRV7518CW22) || defined (ARCADYAN_TARGET_tr_VRV7518CW22) || defined (ARCADYAN_TARGET_gr_VRV7518CW22)
#include "ifxos_time.h"
#endif

/* ============================= */
/* Local Macros & Definitions    */
/* ============================= */

/* xRX platform runtime flags */
#define VPE1_STARTED        (1 << 0)
#define SSLIC_GPIO_RESERVED (1 << 1)

/* Firmware watchdog timer counter address */
#define VPE1_WDOG_CTR_ADDR ((IFX_uint32_t)((IFX_uint8_t* )IFX_MPS_SRAM + 432))

/* Firmware watchdog timeout range, values in ms */
#define VPE1_WDOG_TMOUT_MIN 20
#define VPE1_WDOG_TMOUT_MAX 5000

#define IFX_MPS_UNUSED(var) ((IFX_void_t)(var))

/* ============================= */
/* Global variable definition    */
/* ============================= */
extern mps_comm_dev *pMPSDev;

/* ============================= */
/* Global function declaration   */
/* ============================= */
IFX_void_t ifx_mps_release (void);
extern IFX_uint32_t ifx_mps_reset_structures (mps_comm_dev * pMPSDev);
extern IFX_int32_t ifx_mps_bufman_close (void);
IFX_int32_t ifx_mps_wdog_callback (IFX_uint32_t wdog_cleared_ok_count);
extern IFXOS_event_t fw_ready_evt;
/* ============================= */
/* Local function declaration    */
/* ============================= */
static IFX_int32_t ifx_mps_fw_wdog_start_ar9(void);

/* ============================= */
/* Local variable definition     */
/* ============================= */
static IFX_uint32_t lq_mps_xRX_flags = 0;

/* VMMC watchdog timer callback */
IFX_int32_t (*ifx_wdog_callback) (IFX_uint32_t flags) = IFX_NULL;

/* ============================= */
/* Local function definition     */
/* ============================= */

/******************************************************************************
 * AR9 Specific Routines
 ******************************************************************************/

/**
 * Reserve GPIO lines used by SmartSLIC.
 * Called every time before VPE1 startup.
 *
 * \param    none
 * \return   0         IFX_SUCCESS
 * \return   -1        IFX_ERROR
 * \ingroup  Internal
 * \remarks  Reservation is necessary to protect GPIO lines used
 *           by SmartSLIC from being seized by other SW modules.
 */
static IFX_int32_t lq_mps_sslic_gpio_reserve(void)
{
   IFX_int32_t ret = IFX_SUCCESS;

   if (!(lq_mps_xRX_flags & SSLIC_GPIO_RESERVED))
   {
#if defined(SYSTEM_AR9) || defined(SYSTEM_VR9)
      /* SmartSLIC reset (GPIO31, P1.15) */
      const IFX_int32_t nPort = 1,
                        nPin  = 15;
#elif defined(SYSTEM_XRX300)
//#if defined(ARCADYAN_TARGET_kpn_VGV951ABWAC23) // for KPN-R0A
//#elif defined(ARCADYAN_TARGET_o2_VRV9518SWAC33) || defined(ARCADYAN_TARGET_zz_VRV9518SWAC33)
#if defined(ARCADYAN_TARGET_o2_VRV9518SWAC33) || defined(ARCADYAN_TARGET_zz_VRV9518SWAC33) || defined(ARCADYAN_TARGET_o2_VRV9518SWAC24)
      /* SmartSLIC reset (GPIO 25, P1.9) */
      const IFX_int32_t nPort = 1,
                        nPin  = 9;
#else /* else and KPN-R0A1 */
      /* SmartSLIC reset (GPIO 0, P0.0) */
      const IFX_int32_t nPort = 0,
                        nPin  = 0;
#endif
#else
#error no system selected
#endif /* SYSTEM_... */

//#if defined(SYSTEM_XRX300) && defined(ARCADYAN_TARGET_kpn_VGV951ABWAC23)
#if 0 // for KPN-R0A
       /* SmartSLIC reset (SOUT0) */
       ret = ifx_ledc_set_data(0, 0);
       if (ret)
       {
           printk("lq_mps_sslic_gpio_reserve(): ifx_ledc_set_data(0) return error!\n");
           return ret;
       }

       ret = ifx_ledc_set_data(0, 1);
       if (ret)
       {
           printk("lq_mps_sslic_gpio_reserve(): ifx_ledc_set_data(1) return error!\n");
           return ret;
       }
       IFXOS_MSecSleep(100);
#else
      /* SmartSLIC reset */
      ret = ifx_gpio_pin_reserve(IFX_GPIO_PIN_ID(nPort, nPin),
                                 IFX_GPIO_MODULE_TAPI_VMMC);
      if (ret)
         return ret;

      /* P1.15_ALTSEL0 = 0 */
      ret = ifx_gpio_altsel0_clear(IFX_GPIO_PIN_ID(nPort, nPin),
                                   IFX_GPIO_MODULE_TAPI_VMMC);
      if (ret)
         return ret;

      /* P1.15_ALTSEL1 = 0 */
      ret = ifx_gpio_altsel1_clear(IFX_GPIO_PIN_ID(nPort, nPin),
                                   IFX_GPIO_MODULE_TAPI_VMMC);
      if (ret)
         return ret;

      /* P1.15_DIR = 1 */
      ret = ifx_gpio_dir_out_set(IFX_GPIO_PIN_ID(nPort, nPin),
                                 IFX_GPIO_MODULE_TAPI_VMMC);
      if (ret)
         return ret;

      /* P1.15_OD = 1 */
      ret = ifx_gpio_open_drain_set(IFX_GPIO_PIN_ID(nPort, nPin),
                                    IFX_GPIO_MODULE_TAPI_VMMC);
#endif
#if (defined(SYSTEM_XRX300) && (defined(ARCADYAN_TARGET_o2_VRV9518SWAC33) || defined(ARCADYAN_TARGET_zz_VRV9518SWAC33) || defined(ARCADYAN_TARGET_o2_VRV9518SWAC24))) || ((defined(SYSTEM_AR9) || defined(SYSTEM_VR9)) && (defined(ARCADYAN_TARGET_zz_VRV7518CW22) || defined(ARCADYAN_TARGET_tr_VRV7518CW22) || defined (ARCADYAN_TARGET_gr_VRV7518CW22) ))
      ret = ifx_gpio_output_clear(IFX_GPIO_PIN_ID(nPort, nPin),
                                    IFX_GPIO_MODULE_TAPI_VMMC);
      if (ret)
      {
         printk("lq_mps_sslic_gpio_reserve(): ifx_gpio_output_clear() return error!\n");
         return ret;
      }

      ret = ifx_gpio_output_set(IFX_GPIO_PIN_ID(nPort, nPin),
                                    IFX_GPIO_MODULE_TAPI_VMMC);
      if (ret)
      {
         printk("lq_mps_sslic_gpio_reserve(): ifx_gpio_output_set() return error!\n");
         return ret;
      }

      IFXOS_MSecSleep(100);
#endif

      /* SmartSLIC clock (GPIO8, P0.8) */
      ret = ifx_gpio_pin_reserve(IFX_GPIO_PIN_ID (0, 8),
                                 IFX_GPIO_MODULE_TAPI_VMMC);
      if (ret)
         return ret;

      /* SmartSLIC interface SSI0 (GPIO34, P2.2) */
      ret = ifx_gpio_pin_reserve(IFX_GPIO_PIN_ID (2, 2),
                                 IFX_GPIO_MODULE_TAPI_VMMC);
      if (ret)
         return ret;

      /* SmartSLIC interface SSI0 (GPIO35, P2.3) */
      ret = ifx_gpio_pin_reserve(IFX_GPIO_PIN_ID (2, 3),
                                 IFX_GPIO_MODULE_TAPI_VMMC);
      if (ret)
         return ret;

      /* SmartSLIC interface SSI0 (GPIO36, P2.4) */
      ret = ifx_gpio_pin_reserve(IFX_GPIO_PIN_ID (2, 4),
                                 IFX_GPIO_MODULE_TAPI_VMMC);
      if (ret)
         return ret;

      lq_mps_xRX_flags |= SSLIC_GPIO_RESERVED;
   }

   return ret;
}

/**
 * Free GPIO lines used by SmartSLIC.
 * Called every time after VPE1 stopping.
 *
 * \param    none
 * \return   0         IFX_SUCCESS
 * \return   -1        IFX_ERROR
 * \ingroup  Internal
 */
static IFX_int32_t lq_mps_sslic_gpio_free(void)
{
   IFX_int32_t ret = IFX_SUCCESS;

   if (lq_mps_xRX_flags & SSLIC_GPIO_RESERVED)
   {
#if defined(SYSTEM_AR9) || defined(SYSTEM_VR9)
      /* SmartSLIC reset (GPIO31, P1.15) */
      const IFX_int32_t nPort = 1,
                        nPin  = 15;
#elif defined(SYSTEM_XRX300)
//#if defined(ARCADYAN_TARGET_kpn_VGV951ABWAC23) //for KPN-R0A
//#elif defined(ARCADYAN_TARGET_o2_VRV9518SWAC33) || defined(ARCADYAN_TARGET_zz_VRV9518SWAC33)
#if defined(ARCADYAN_TARGET_o2_VRV9518SWAC33) || defined(ARCADYAN_TARGET_zz_VRV9518SWAC33) || defined(ARCADYAN_TARGET_o2_VRV9518SWAC24)
      /* SmartSLIC reset (GPIO 25, P1.9) */
      const IFX_int32_t nPort = 1,
                        nPin  = 9;
#else /* else and KPN-R0A1 */
      /* SmartSLIC reset (GPIO 0, P0.0) */
      const IFX_int32_t nPort = 0,
                        nPin  = 0;
#endif
#else
#error no system selected
#endif /* SYSTEM_... */

//#if defined(SYSTEM_XRX300) && defined(ARCADYAN_TARGET_kpn_VGV951ABWAC23) //for KPN-R0A
//#else
      /* SmartSLIC reset */
      ret = ifx_gpio_pin_free(IFX_GPIO_PIN_ID (nPort, nPin),
                              IFX_GPIO_MODULE_TAPI_VMMC);
      if (ret)
         return ret;
//#endif

      /* SmartSLIC clock (GPIO8, P0.8) */
      ret = ifx_gpio_pin_free(IFX_GPIO_PIN_ID (0, 8),
                              IFX_GPIO_MODULE_TAPI_VMMC);
      if (ret)
         return ret;

      /* SmartSLIC interface SSI0 (GPIO34, P2.2) */
      ret = ifx_gpio_pin_free(IFX_GPIO_PIN_ID (2, 2),
                              IFX_GPIO_MODULE_TAPI_VMMC);
      if (ret)
         return ret;

      /* SmartSLIC interface SSI0 (GPIO35, P2.3) */
      ret = ifx_gpio_pin_free(IFX_GPIO_PIN_ID (2, 3),
                              IFX_GPIO_MODULE_TAPI_VMMC);
      if (ret)
         return ret;

      /* SmartSLIC interface SSI0 (GPIO36, P2.4) */
      ret = ifx_gpio_pin_free(IFX_GPIO_PIN_ID (2, 4),
                              IFX_GPIO_MODULE_TAPI_VMMC);
      if (ret)
         return ret;

      lq_mps_xRX_flags &= ~SSLIC_GPIO_RESERVED;
   }

   return ret;
}

/**
 * Start AR9 EDSP firmware watchdog mechanism.
 * Called after download and startup of VPE1.
 *
 * \param   none
 * \return  0         IFX_SUCCESS
 * \return  -1        IFX_ERROR
 * \ingroup Internal
 */
IFX_int32_t ifx_mps_fw_wdog_start_ar9()
{
   /* vpe1_wdog_ctr should be set up in u-boot as
      "vpe1_wdog_ctr_addr=0xBF2001B0"; protection from incorrect or missing
      setting */
   if (vpe1_wdog_ctr != VPE1_WDOG_CTR_ADDR)
   {
      vpe1_wdog_ctr = VPE1_WDOG_CTR_ADDR;
   }

   /* vpe1_wdog_timeout should be set up in u-boot as "vpe1_wdog_timeout =
      <value in ms>"; protection from insane setting */
   if (vpe1_wdog_timeout < VPE1_WDOG_TMOUT_MIN)
   {
      vpe1_wdog_timeout = VPE1_WDOG_TMOUT_MIN;
   }
   if (vpe1_wdog_timeout > VPE1_WDOG_TMOUT_MAX)
   {
      vpe1_wdog_timeout = VPE1_WDOG_TMOUT_MAX;
   }

   /* recalculate in jiffies */
   vpe1_wdog_timeout = vpe1_wdog_timeout * HZ / 1000;

   /* register BSP callback function */
   if (IFX_SUCCESS !=
       vpe1_sw_wdog_register_reset_handler (ifx_mps_wdog_callback))
   {
      TRACE (MPS, DBG_LEVEL_HIGH,
             (KERN_ERR "[%s %s %d]: Unable to register WDT callback.\r\n",
              __FILE__, __func__, __LINE__));
      return IFX_ERROR;;
   }

   /* start software watchdog timer */
   if (IFX_SUCCESS != vpe1_sw_wdog_start (0))
   {
      TRACE (MPS, DBG_LEVEL_HIGH,
             (KERN_ERR
              "[%s %s %d]: Error starting software watchdog timer.\r\n",
              __FILE__, __func__, __LINE__));
      return IFX_ERROR;
   }
   return IFX_SUCCESS;
}

/**
 * Firmware download to Voice CPU
 * This function performs a firmware download to the coprocessor.
 *
 * \param   pMBDev    Pointer to mailbox device structure
 * \param   pFWDwnld  Pointer to firmware structure
 * \return  0         IFX_SUCCESS, firmware ready
 * \return  -1        IFX_ERROR,   firmware not downloaded.
 * \ingroup Internal
 */
IFX_int32_t ifx_mps_download_firmware (mps_mbx_dev *pMBDev, mps_fw *pFWDwnld)
{
   IFX_uint32_t mem, cksum;
   IFX_boolean_t bMemReqNotPresent = IFX_FALSE;

#if 1 /* added by JamesPeng */
	printk("ifx_mps_download_firmware >>> start\n");
#endif

   IFX_MPS_UNUSED(pMBDev);

   /* copy FW footer from user space */
   if (IFX_NULL == IFXOS_CpyFromUser(pFW_img_data,
                           pFWDwnld->data+pFWDwnld->length/4-sizeof(*pFW_img_data)/4,
                           sizeof(*pFW_img_data)))
   {
      TRACE (MPS, DBG_LEVEL_HIGH,
                  (KERN_ERR "[%s %s %d]: copy_from_user error\r\n",
                   __FILE__, __func__, __LINE__));
      return IFX_ERROR;
   }

   if(FW_FORMAT_NEW)
   {
      IFX_uint32_t plt = pFW_img_data->fw_vers >> 8 & 0xf;

      /* platform check */
#if defined(SYSTEM_AR9)
      if (plt != FW_PLT_XRX100)
#elif defined(SYSTEM_VR9)
      if (plt != FW_PLT_XRX200)
#elif defined(SYSTEM_XRX300)
      if (plt != FW_PLT_XRX300)
#endif
      {
         TRACE (MPS, DBG_LEVEL_HIGH,("WRONG FIRMWARE PLATFORM!\n"));
         return IFX_ERROR;
      }
   }

   mem = pFW_img_data->mem;

   /* memory requirement sanity check - crc check */
   if ((0xFF & ~((mem >> 16) + (mem >> 8) + mem)) != (mem >> 24))
   {
      TRACE (MPS, DBG_LEVEL_HIGH,
          ("[%s %s %d]: warning, image does not contain size - assuming 1MB!\n",
           __FILE__, __func__, __LINE__));
      mem = 1 * 1024 * 1024;
      bMemReqNotPresent = IFX_TRUE;
   }
   else
   {
      mem &= 0x00FFFFFF;
   }

   /* check if FW image fits in available memory space */
   if (mem > vpe1_get_max_mem(0))
   {
      TRACE (MPS, DBG_LEVEL_HIGH,
      ("[%s %s %d]: error, firmware memory exceeds reserved space (%i > %i)!\n",
                 __FILE__, __func__, __LINE__, mem, vpe1_get_max_mem(0)));
      return IFX_ERROR;
   }

   /* reset the driver */
   ifx_mps_reset ();

   /* call BSP to get cpu1 base address */
   cpu1_base_addr = (IFX_uint32_t *)vpe1_get_load_addr(0);

   /* check if CPU1 base address is sane */
   if (cpu1_base_addr == IFX_NULL || !cpu1_base_addr)
   {
      TRACE (MPS, DBG_LEVEL_HIGH,
             (KERN_ERR "IFX_MPS: CPU1 base address is invalid!\r\n"));
      return IFX_ERROR;
   }
   else
   {
      /* check if CPU1 address is 1MB aligned */
      if ((IFX_uint32_t)cpu1_base_addr & 0xfffff)
      {
         TRACE (MPS, DBG_LEVEL_HIGH,
               (KERN_ERR "IFX_MPS: CPU1 base address is not 1MB aligned!\r\n"));
         return IFX_ERROR;
      }
   }

   /* further use uncached value */
   cpu1_base_addr = (IFX_uint32_t *)KSEG1ADDR(cpu1_base_addr);

   /* free all data buffers that might be currently used by FW */
   if (IFX_NULL != ifx_mps_bufman_freeall)
   {
      ifx_mps_bufman_freeall();
   }

   if(FW_FORMAT_NEW)
   {
      /* adjust download length */
      pFWDwnld->length -= (sizeof(*pFW_img_data)-sizeof(IFX_uint32_t));
   }
   else
   {
      pFWDwnld->length -= sizeof(IFX_uint32_t);

      /* handle unlikely case if FW image does not contain memory requirement -
         assumed for old format only */
      if (IFX_TRUE == bMemReqNotPresent)
         pFWDwnld->length += sizeof(IFX_uint32_t);

      /* in case of old FW format always assume that FW is encrypted;
         use compile switch USE_PLAIN_VOICE_FIRMWARE for plain FW */
#ifndef USE_PLAIN_VOICE_FIRMWARE
      pFW_img_data->enc = 1;
#else
#warning Using unencrypted firmware!
      pFW_img_data->enc = 0;
#endif /* USE_PLAIN_VOICE_FIRMWARE */
      /* initializations for the old format */
      pFW_img_data->st_addr_crc = 2*sizeof(IFX_uint32_t) +
                                  FW_AR9_OLD_FMT_XCPT_AREA_SZ;
      pFW_img_data->en_addr_crc = pFWDwnld->length;
      pFW_img_data->fw_vers = 0;
      pFW_img_data->magic = 0;
   }

   /* copy FW image to base address of CPU1 */
   if (IFX_NULL ==
       IFXOS_CpyFromUser ((IFX_void_t *)cpu1_base_addr,
                          (IFX_void_t *)pFWDwnld->data, pFWDwnld->length))
   {
      TRACE (MPS, DBG_LEVEL_HIGH,
             (KERN_ERR "[%s %s %d]: copy_from_user error\r\n", __FILE__,
              __func__, __LINE__));
      return IFX_ERROR;
   }

   /* process firmware decryption */
   if (pFW_img_data->enc == 1)
   {
      void (*decrypt_function)(unsigned int addr, int n);

      if(FW_FORMAT_NEW)
      {
         /* adjust decryption length (avoid decrypting CRC32 checksum) */
         pFWDwnld->length -= sizeof(IFX_uint32_t);
      }
      /* BootROM actually decrypts n+4 bytes if n bytes were passed for
         decryption. Subtract sizeof(u32) from length to avoid decryption
         of data beyond the FW image code */
      pFWDwnld->length -= sizeof(IFX_uint32_t);

      /* Workaround to get rid of warning "function with qualified void return
      type called" because the BSP defines ifx_bsp_basic_mps_decrypt to have
      a return type of "const void" which makes no sense. So here it is casted
      to a function pointer with just "void" return type. */
      decrypt_function = (void (*)(unsigned int, int))ifx_bsp_basic_mps_decrypt;
      /* Call the decrypt function in on-chip ROM. */
      decrypt_function((IFX_uint32_t)cpu1_base_addr, pFWDwnld->length);
   }

   /* calculate CRC32 checksum over downloaded image */
   cksum = ifx_mps_fw_crc32(cpu1_base_addr, pFW_img_data);

   /* verify the checksum */
   if(FW_FORMAT_NEW)
   {
      if (cksum != pFW_img_data->crc32)
      {
         TRACE (MPS, DBG_LEVEL_HIGH,
                ("MPS: FW checksum error: img=0x%08x calc=0x%08x\r\n",
                pFW_img_data->crc32, cksum));
         return IFX_ERROR;
      }
   }
   else
   {
      /* just store self-calculated checksum */
      pFW_img_data->crc32 = cksum;
   }

   /* start VPE1 */
   ifx_mps_release ();

   /* start FW watchdog mechanism */
   ifx_mps_fw_wdog_start_ar9();

#if 1 /* added by JamesPeng */
	printk("ifx_mps_download_firmware >>> finish\n");
#endif

   /* get FW version */
   return ifx_mps_get_fw_version (0);
}


/**
 * Restart CPU1
 * This function restarts CPU1 by accessing the reset request register and
 * reinitializes the mailbox.
 *
 * \return  0        IFX_SUCCESS, successful restart
 * \return  -1       IFX_ERROR, if reset failed
 * \ingroup Internal
 */
IFX_int32_t ifx_mps_restart (void)
{
   /* raise reset request for CPU1 and reset driver structures */
   ifx_mps_reset ();
   /* Disable GPTC Interrupt to CPU1 */
   ifx_mps_shutdown_gpt ();
   /* re-configure GPTC */
   ifx_mps_init_gpt ();
   /* let CPU1 run */
   ifx_mps_release ();
   /* start FW watchdog mechanism */
   ifx_mps_fw_wdog_start_ar9();
   TRACE (MPS, DBG_LEVEL_HIGH, ("IFX_MPS: Restarting firmware..."));
   return ifx_mps_get_fw_version (0);
}

/**
 * Shutdown MPS - stop VPE1
 * This function stops VPE1
 *
 * \ingroup Internal
 */
IFX_void_t ifx_mps_shutdown (void)
{
   if (lq_mps_xRX_flags & VPE1_STARTED)
   {
      /* stop software watchdog timer */
      vpe1_sw_wdog_stop (0);
      /* clean up the BSP callback function */
      vpe1_sw_wdog_register_reset_handler (IFX_NULL);
      /* stop VPE1 */
      vpe1_sw_stop (0);
      lq_mps_xRX_flags &= ~VPE1_STARTED;
      /* release SmartSLIC GPIO lines */
      if (lq_mps_sslic_gpio_free())
      {
         TRACE (MPS, DBG_LEVEL_HIGH,
                  ("IFX_MPS: error freeing SSLIC GPIO lines!\n"));
      }
   }
   /* free GPTC */
   ifx_mps_shutdown_gpt ();
}

/**
 * Reset CPU1
 * This function causes a reset of CPU1 by clearing the CPU0 boot ready bit
 * in the reset request register RCU_RST_REQ.
 * It does not change the boot configuration registers for CPU0 or CPU1.
 *
 * \return  0        IFX_SUCCESS, cannot fail
 * \ingroup Internal
 */
IFX_void_t ifx_mps_reset (void)
{
   /* if VPE1 is already started, stop it */
   if (lq_mps_xRX_flags & VPE1_STARTED)
   {
      /* stop software watchdog timer first */
      vpe1_sw_wdog_stop (0);
      vpe1_sw_stop (0);
      lq_mps_xRX_flags &= ~VPE1_STARTED;
      /* release SmartSLIC GPIO lines */
      if (lq_mps_sslic_gpio_free())
      {
         TRACE (MPS, DBG_LEVEL_HIGH,
                  ("IFX_MPS: error freeing SSLIC GPIO lines!\n"));
      }
   }

   /* reset driver */
   ifx_mps_reset_structures (pMPSDev);
   ifx_mps_bufman_close ();
   return;
}

/**
 * Let CPU1 run
 * This function starts VPE1
 *
 * \return  none
 * \ingroup Internal
 */
IFX_void_t ifx_mps_release (void)
{
   IFX_int_t ret;
   IFX_int32_t RetCode = 0;

   /* reserve SmartSLIC GPIO pins */
   if (lq_mps_sslic_gpio_reserve())
   {
      TRACE (MPS, DBG_LEVEL_HIGH,
               ("IFX_MPS: cannot reserve SSLIC GPIO lines!\n"));
      return;
   }

   /* Start VPE1 */
   if (IFX_SUCCESS !=
       vpe1_sw_start ((IFX_void_t *)cpu1_base_addr, 0, 0))
   {
      TRACE (MPS, DBG_LEVEL_HIGH, (KERN_ERR "Error starting VPE1\r\n"));
      return;
   }
   lq_mps_xRX_flags |= VPE1_STARTED;

   /* Sleep until FW is ready or a timeout after 3 seconds occured */
   ret = IFXOS_EventWait (&fw_ready_evt, 3000, &RetCode);
   if ((ret == IFX_ERROR) && (RetCode == 1))
   {
      /* timeout */
      TRACE (MPS, DBG_LEVEL_HIGH,
             (KERN_ERR "[%s %s %d]: Timeout waiting for firmware ready.\r\n",
              __FILE__, __func__, __LINE__));
      /* recalculate and compare the firmware checksum */
      ifx_mps_fw_crc_compare(cpu1_base_addr, pFW_img_data);
      /* dump exception area on a console */
      ifx_mps_dump_fw_xcpt(cpu1_base_addr, pFW_img_data);
   }
}

/**
 * WDT callback.
 * This function is called by BSP (module softdog_vpe) in case if software
 * watchdog timer expiration is detected by BSP.
 * This function needs to be registered at BSP as WDT callback using
 * vpe1_sw_wdog_register_reset_handler() API.
 *
 * \return  0        IFX_SUCCESS, cannot fail
 * \ingroup Internal
 */
IFX_int32_t ifx_mps_wdog_callback (IFX_uint32_t wdog_cleared_ok_count)
{
   IFXOS_INTSTAT flags;
#ifdef DEBUG
   TRACE (MPS, DBG_LEVEL_HIGH,
          ("MPS: watchdog callback! arg=0x%08x\r\n", wdog_cleared_ok_count));
#endif /* DEBUG */

   /* activate SmartSLIC RESET */
   if (lq_mps_xRX_flags & SSLIC_GPIO_RESERVED)
   {
      IFXOS_LOCKINT (flags);
#if defined(SYSTEM_AR9) || defined(SYSTEM_VR9)
      /* P1.15_OUT = 0 */
      if (ifx_gpio_output_clear
          (IFX_GPIO_PIN_ID (1, 15), IFX_GPIO_MODULE_TAPI_VMMC))
#elif defined(SYSTEM_XRX300)
#if defined(ARCADYAN_TARGET_o2_VRV9518SWAC33) || defined(ARCADYAN_TARGET_zz_VRV9518SWAC33) || defined(ARCADYAN_TARGET_o2_VRV9518SWAC24)
      /* P1.9_OUT = 0 */
      if (ifx_gpio_output_clear
          (IFX_GPIO_PIN_ID (1, 9), IFX_GPIO_MODULE_TAPI_VMMC))
#else /* else and KPN-R0A1 */
      /* P0.0_OUT = 0 */
      if (ifx_gpio_output_clear
          (IFX_GPIO_PIN_ID (0, 0), IFX_GPIO_MODULE_TAPI_VMMC))
#endif
#else
#error no system selected
#endif /* SYSTEM_... */
      {
         TRACE (MPS, DBG_LEVEL_HIGH,
                (KERN_ERR "[%s %s %d]: GPIO error clearing OUT.\r\n", __FILE__,
                 __func__, __LINE__));
      }
      IFXOS_UNLOCKINT (flags);
   }

   /* recalculate and compare the firmware checksum */
   ifx_mps_fw_crc_compare(cpu1_base_addr, pFW_img_data);

   /* dump exception area on a console */
   ifx_mps_dump_fw_xcpt(cpu1_base_addr, pFW_img_data);

   if (IFX_NULL != ifx_wdog_callback)
   {
      /* call VMMC driver */
      ifx_wdog_callback (wdog_cleared_ok_count);
   }
   else
   {
      TRACE (MPS, DBG_LEVEL_HIGH,
             (KERN_WARNING "MPS: VMMC watchdog timer callback is NULL.\r\n"));
   }
   return 0;
}

/**
 * Register WDT callback.
 * This function is called by VMMC driver to register its callback in
 * the MPS driver.
 *
 * \return  0        IFX_SUCCESS, cannot fail
 * \ingroup Internal
 */
IFX_int32_t
ifx_mps_register_wdog_callback (IFX_int32_t (*pfn) (IFX_uint32_t flags))
{
   ifx_wdog_callback = pfn;
   return 0;
}

#ifndef VMMC_WITH_MPS
EXPORT_SYMBOL (ifx_mps_register_wdog_callback);
#endif /* !VMMC_WITH_MPS */

#endif /* SYSTEM_AR9 || SYSTEM_VR9 || SYSTEM_XRX300 */
