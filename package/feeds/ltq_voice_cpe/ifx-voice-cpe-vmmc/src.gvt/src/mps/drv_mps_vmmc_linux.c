/******************************************************************************

                              Copyright (c) 2013
                            Lantiq Deutschland GmbH
                             http://www.lantiq.com

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

/**
   \file drv_mps_vmmc_linux.c  Header file of the MPS driver Linux part.
   This file contains the implementation of the linux specific driver functions.
*/

/* ============================= */
/* Includes                      */
/* ============================= */
#include "drv_config.h"

#include "drv_mps_version.h"

#ifdef LINUX

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/init.h>
#include <linux/poll.h>
#include <linux/proc_fs.h>
#include <linux/vmalloc.h>
#include <linux/ioctl.h>
#include <linux/delay.h>
#include <linux/interrupt.h>

/* include for UTS_RELEASE */
#ifndef UTS_RELEASE
   #if   (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0))
      #include <linux/uts.h>
   #elif (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,33))
      #include <linux/utsrelease.h>
   #else
      #include <generated/utsrelease.h>
   #endif /* LINUX_VERSION_CODE */
#endif /* UTS_RELEASE */

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0))
#include <linux/cdev.h>
#else
#include <linux/moduleparam.h>
#endif /* LINUX_VERSION_CODE */

#if defined(SYSTEM_FALCON)
#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,28))
#include "drv_vmmc_init.h"
#include <lantiq.h>
#include <irq.h>
#ifdef CONFIG_SOC_LANTIQ_FALCON
#include <falcon_irq.h>
#endif
#else
#include <asm/ifx/irq.h>
#include <asm/ifx/ifx_regs.h>
#include <asm/ifx_vpe.h>
#endif
#endif /* SYSTEM_FALCON */

/* lib_ifxos headers */
#include "ifx_types.h"
#include "ifxos_lock.h"
#include "ifxos_select.h"
#include "ifxos_copy_user_space.h"
#include "ifxos_event.h"
#include "ifxos_interrupt.h"

#include "drv_mps_vmmc.h"
#include "drv_mps_vmmc_dbg.h"
#include "drv_mps_vmmc_device.h"

/* ============================= */
/* Local Macros & Definitions    */
/* ============================= */
#define IFX_MPS_DEV_NAME       "ltq_mps"

#ifndef CONFIG_MPS_HISTORY_SIZE
#define CONFIG_MPS_HISTORY_SIZE 128
#warning CONFIG_MPS_HISTORY_SIZE should have been set via cofigure - setting to default 128
#endif

/* first minor number */
#define LQ_MPS_FIRST_MINOR 1
/* total file descriptor number */
#define LQ_MPS_TOTAL_FD    10

/* ============================= */
/* Global variable definition    */
/* ============================= */
CREATE_TRACE_GROUP (MPS);
volatile IFX_uint32_t *cpu1_base_addr = IFX_NULL;

/* ============================= */
/* Global function declaration   */
/* ============================= */
extern irqreturn_t ifx_mps_ad0_irq (IFX_int32_t irq, IFX_void_t * pDev);
extern irqreturn_t ifx_mps_ad1_irq (IFX_int32_t irq, IFX_void_t * pDev);
extern irqreturn_t ifx_mps_vc_irq (IFX_int32_t irq, IFX_void_t * pDev);
extern IFX_void_t ifx_mps_shutdown (void);
extern IFX_int32_t ifx_mps_event_activation_poll (mps_devices type,
                                                  MbxEventRegs_s * act);
mps_mbx_dev *ifx_mps_get_device (mps_devices type);

#ifdef CONFIG_PROC_FS
IFX_int32_t ifx_mps_get_status_proc (IFX_char_t * buf);
#endif /* */

/* ============================= */
/* Local function declaration    */
/* ============================= */
#ifndef __KERNEL__
IFX_int32_t ifx_mps_open (struct inode *inode, struct file *file_p);
IFX_int32_t ifx_mps_close (struct inode *inode, struct file *file_p);
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,36))
IFX_int32_t ifx_mps_ioctl (struct inode *inode, struct file *file_p,
#else
long ifx_mps_ioctl (struct file *file_p,
#endif
                           IFX_uint32_t nCmd, IFX_ulong_t arg);
IFX_int32_t ifx_mps_read_mailbox (mps_devices type, mps_message * rw);
IFX_int32_t ifx_mps_write_mailbox (mps_devices type, mps_message * rw);
IFX_int32_t ifx_mps_register_data_callback (mps_devices type, IFX_uint32_t dir,
                                            IFX_void_t (*callback) (mps_devices
                                                                    type));
IFX_int32_t ifx_mps_unregister_data_callback (mps_devices type,
                                              IFX_uint32_t dir);
IFX_int32_t ifx_mps_register_event_callback (mps_devices type,
                                             MbxEventRegs_s * mask,
                                             IFX_void_t (*callback)
                                             (MbxEventRegs_s * events));
IFX_int32_t ifx_mps_unregister_event_callback (mps_devices type);
#endif  /*__KERNEL__*/
IFX_int32_t ifx_mps_register_event_poll (mps_devices type,
                                         MbxEventRegs_s * mask,
                                         IFX_void_t (*callback) (MbxEventRegs_s
                                                                 * events));
IFX_int32_t ifx_mps_unregister_event_poll (mps_devices type);
static IFX_uint32_t ifx_mps_poll (struct file *file_p, poll_table * wait);
extern IFX_int32_t ifx_mps_fastbuf_get_proc (IFX_char_t * buf);

#ifdef CONFIG_MPS_EVENT_MBX
IFX_int32_t ifx_mps_event_mbx_activation_poll (IFX_int32_t value);
#endif /* CONFIG_MPS_EVENT_MBX */

/* ============================= */
/* Local variable definition     */
/* ============================= */

/* ============================= */
/* Local function definition     */
/* ============================= */

/* external function declaration */

/* local function declaration */

#if (defined(MODULE) && !defined(VMMC_WITH_MPS))
MODULE_AUTHOR ("Lantiq Deutschland GmbH");
MODULE_DESCRIPTION ("MPS/DSP driver for DANUBE, XWAY(TM) XRX100 family, XWAY(TM) XRX200 family, XWAY(TM) XRX300 family");
#if defined(SYSTEM_AR9)
MODULE_SUPPORTED_DEVICE ("XWAY(TM) XRX100 family MIPS34KEc");
#elif  defined(SYSTEM_VR9)
MODULE_SUPPORTED_DEVICE ("XWAY(TM) XRX200 family MIPS34KEc");
#elif  defined(SYSTEM_XRX300)
MODULE_SUPPORTED_DEVICE ("XWAY(TM) XRX300 family MIPS34KEc");
#else /* Danube */
MODULE_SUPPORTED_DEVICE ("DANUBE MIPS24KEc");
#endif /* */
MODULE_LICENSE ("Dual BSD/GPL");
#endif /* */
static ushort ifx_mps_major_id = 0;
module_param (ifx_mps_major_id, ushort, 0);
MODULE_PARM_DESC (ifx_mps_major_id, "Major ID of device");
IFX_char_t ifx_mps_dev_name[10];
IFX_char_t voice_channel_int_name[NUM_VOICE_CHANNEL][15];

/* the driver callbacks */
static struct file_operations ifx_mps_fops = {
 owner:THIS_MODULE,
 poll:ifx_mps_poll,
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,36))
 ioctl:ifx_mps_ioctl,
#else
 unlocked_ioctl:ifx_mps_ioctl,
#endif /* LINUX < 2.6.36 */
 open:ifx_mps_open,
 release:ifx_mps_close
};


/* device structure */
extern mps_comm_dev ifx_mps_dev;

#ifdef CONFIG_PROC_FS
static struct proc_dir_entry *ifx_mps_proc_dir;
static struct proc_dir_entry *ifx_mps_proc_file;

#if CONFIG_MPS_HISTORY_SIZE > 0
static struct proc_dir_entry *ifx_mps_history_proc;
#endif /* */

#ifdef CONFIG_DANUBE_MPS_PROC_DEBUG

#define MPS_FIRMWARE_BUFFER_SIZE 512*1024
#define MPS_FW_START_TAG  "IFX-START-FW-NOW"
#define MPS_FW_INIT_TAG   "IFX-INITIALIZE-VCPU-HARDWARE"
#define MPS_FW_BUFFER_TAG "IFX-PROVIDE-BUFFERS"
#define MPS_FW_OPEN_VOICE_TAG "IFX-OPEN-VOICE0-MBX"
#define MPS_FW_REGISTER_CALLBACK_TAG "IFX-REGISTER-CALLBACK-VOICE0"
#define MPS_FW_SEND_MESSAGE_TAG "IFX-SEND-MESSAGE-VOICE0"
#define MPS_FW_RESTART_TAG "IFX-RESTART-VCPU-NOW"
#define MPS_FW_ENABLE_PACKET_LOOP_TAG "IFX-ENABLE-PACKET-LOOP"
#define MPS_FW_DISABLE_PACKET_LOOP_TAG "IFX-DISABLE-PACKET-LOOP"
static IFX_char_t ifx_mps_firmware_buffer[MPS_FIRMWARE_BUFFER_SIZE];
static mps_fw ifx_mps_firmware_struct;
static IFX_int32_t ifx_mps_firmware_buffer_pos = 0;
static IFX_int32_t ifx_mps_packet_loop = 0;
static IFX_uint32_t ifx_mps_rtp_voice_data_count = 0;
#endif /* CONFIG_DANUBE_MPS_PROC_DEBUG */

#if CONFIG_MPS_HISTORY_SIZE > 0
#define MPS_HISTORY_BUFFER_SIZE (CONFIG_MPS_HISTORY_SIZE)
extern IFX_int32_t ifx_mps_history_buffer_freeze;
extern IFX_uint32_t ifx_mps_history_buffer[];
extern IFX_int32_t ifx_mps_history_buffer_words;

#ifdef DEBUG
extern IFX_int32_t ifx_mps_history_buffer_words_total;
#endif /* */
extern IFX_int32_t ifx_mps_history_buffer_overflowed;
#endif /* CONFIG_DANUBE_MPS_PROC_HISTORY > 0 */
#endif /* CONFIG_PROC_FS */

static IFX_char_t ifx_mps_device_version[20];

#define DANUBE_MPS_VOICE_STATUS_CLEAR 0xC3FFFFFF

/* FW ready event */
IFXOS_event_t fw_ready_evt;

/**
   This function registers char device in kernel.
\param   pDev     pointer to mps_comm_dev structure
\return  0        success
\return  -ENOMEM
\return  -EPERM
*/
IFX_int32_t lq_mps_os_register (mps_comm_dev *pDev)
{
   IFX_int32_t ret;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0))
   IFX_uint8_t i, idx, minor;
   dev_t       dev;

   if (ifx_mps_major_id)
   {
      dev = MKDEV(ifx_mps_major_id, LQ_MPS_FIRST_MINOR);
      ret = register_chrdev_region(dev, LQ_MPS_TOTAL_FD, ifx_mps_dev_name);
   }
   else
   {
      /* dynamic major */
      ret = alloc_chrdev_region(&dev, LQ_MPS_FIRST_MINOR, LQ_MPS_TOTAL_FD, ifx_mps_dev_name);
      ifx_mps_major_id = MAJOR(dev);
   }
#else
   /* older way of char driver registration */
   ret = register_chrdev (ifx_mps_major_id, ifx_mps_dev_name, &ifx_mps_fops);
   if (ret >= 0 && ifx_mps_major_id == 0)
   {
      /* dynamic major */
      ifx_mps_major_id = ret;
   }
#endif
   if (ret < 0)
   {
      TRACE (MPS, DBG_LEVEL_HIGH,
             ("IFX_MPS: can't get major %d\n", ifx_mps_major_id));
      return ret;
   }
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0))
   for (i=0, idx=0, minor=LQ_MPS_FIRST_MINOR; i<LQ_MPS_TOTAL_FD; i++, minor++)
   {
      struct cdev *p_cdev = cdev_alloc();

      if (NULL == p_cdev)
         return -ENOMEM;

      cdev_init(p_cdev, &ifx_mps_fops);
      p_cdev->owner = THIS_MODULE;

      ret = cdev_add(p_cdev, MKDEV(ifx_mps_major_id, minor), 1);
      if (ret != 0)
      {
         cdev_del (p_cdev);
         return -EPERM;
      }

      if (minor == LQ_MPS_FIRST_MINOR)
      {
         pDev->command_mb.mps_cdev = p_cdev;
      }
      else if (minor == LQ_MPS_TOTAL_FD)
      {
#ifdef CONFIG_MPS_EVENT_MBX
         pDev->event_mbx.mps_cdev = p_cdev;
#endif /* CONFIG_MPS_EVENT_MBX */
      }
      else if (minor > LQ_MPS_FIRST_MINOR && minor < LQ_MPS_TOTAL_FD)
      {
         pDev->voice_mb[idx].mps_cdev = p_cdev;
         idx++;
      }
   }
#endif
   return ret;
}

/**
   This function unregisters char device from kernel.
\param   pDev     pointer to mps_comm_dev structure
*/
IFX_void_t lq_mps_os_unregister (mps_comm_dev *pDev)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0))
   IFX_uint8_t i, idx, minor;

   for (i=0, idx=0, minor=LQ_MPS_FIRST_MINOR; i<LQ_MPS_TOTAL_FD; i++, minor++)
   {
      if (minor == LQ_MPS_FIRST_MINOR)
      {
         cdev_del (pDev->command_mb.mps_cdev);
         pDev->command_mb.mps_cdev = IFX_NULL;
      }
      else if (minor == LQ_MPS_TOTAL_FD)
      {
#ifdef CONFIG_MPS_EVENT_MBX
         cdev_del (pDev->event_mbx.mps_cdev);
         pDev->event_mbx.mps_cdev = IFX_NULL;
#endif /* CONFIG_MPS_EVENT_MBX */
      }
      else if (minor > LQ_MPS_FIRST_MINOR && minor < LQ_MPS_TOTAL_FD)
      {
         cdev_del (pDev->voice_mb[idx].mps_cdev);
         pDev->voice_mb[idx].mps_cdev = IFX_NULL;
         idx++;
      }
   }
   unregister_chrdev_region (MKDEV(ifx_mps_major_id, LQ_MPS_FIRST_MINOR), LQ_MPS_TOTAL_FD);
#else
   /* older way of char driver deregistration */
   unregister_chrdev (ifx_mps_major_id, ifx_mps_dev_name);
#endif
}

/**
 * Get mailbox struct by type
 * This function returns the mailbox device structure pointer for the given
 * device.
 *
 * \param   type     DSP device entity
 * \ingroup Internal
 */
mps_mbx_dev *ifx_mps_get_device (mps_devices type)
{
   /* Get corresponding mailbox device structure */
   switch (type)
   {
      case command:
         return (&ifx_mps_dev.command_mb);
      case voice0:
         return (&ifx_mps_dev.voice_mb[0]);
      case voice1:
         return (&ifx_mps_dev.voice_mb[1]);
      case voice2:
         return (&ifx_mps_dev.voice_mb[2]);
      case voice3:
         return (&ifx_mps_dev.voice_mb[3]);
      case voice4:
         return (&ifx_mps_dev.voice_mb[4]);
      case voice5:
         return (&ifx_mps_dev.voice_mb[5]);
      case voice6:
         return (&ifx_mps_dev.voice_mb[6]);
      case voice7:
         return (&ifx_mps_dev.voice_mb[7]);

#ifdef CONFIG_MPS_EVENT_MBX
      case event_mbx:
         return (&ifx_mps_dev.event_mbx);
#endif /* CONFIG_MPS_EVENT_MBX */
      default:
         return (0);
   }
}


#ifdef CONFIG_DANUBE_MPS_PROC_DEBUG
/**
 * Dummy data callback
 * For test purposes this dummy receive handler function can be used.
 *
 * \param   type     DSP device entity ( 1 - command, 2 - voice0, 3 - voice1,
 *                   4 - voice2, 5 - voice3 )
 * \ingroup Internal
 */
IFX_void_t ifx_mbx_dummy_rcv_data_callback (mps_devices type)
{
   mps_message rw;

   memset (&rw, 0, sizeof (mps_message));
   if (ifx_mps_read_mailbox (type, &rw) != IFX_SUCCESS)
   {
      TRACE (MPS, DBG_LEVEL_HIGH, ("ifx_mps_read_mailbox failed\n"));
      return;
   }

   switch (rw.cmd_type)
   {
      case CMD_RTP_VOICE_DATA_PACKET:
         ifx_mps_rtp_voice_data_count++;
         if (ifx_mps_rtp_voice_data_count % 10000 == 0)
         {
            TRACE (MPS, DBG_LEVEL_HIGH,
                   ("%s - %d packets received\n", __FUNCTION__,
                    ifx_mps_rtp_voice_data_count));
         }

         if (ifx_mps_packet_loop == 1)
         {
            rw.cmd_type = CMD_RTP_VOICE_DATA_PACKET;
            rw.RTP_PaylOffset = 0x00000000;
            ifx_mps_write_mailbox (2, &rw);
         }
         else
            IFXOS_BlockFree ((IFX_char_t *) KSEG0ADDR (rw.pData));
         break;

      case CMD_ADDRESS_PACKET:
         IFXOS_BlockFree ((IFX_char_t *) KSEG0ADDR (rw.pData));
         break;

      case CMD_VOICEREC_STATUS_PACKET:
      case CMD_VOICEREC_DATA_PACKET:
      case CMD_RTP_EVENT_PACKET:
      case CMD_FAX_DATA_PACKET:
      case CMD_FAX_STATUS_PACKET:
      case CMD_P_PHONE_DATA_PACKET:
      case CMD_P_PHONE_STATUS_PACKET:
      case CMD_CID_DATA_PACKET:
         TRACE (MPS, DBG_LEVEL_HIGH,
                ("%s - unexpected packet (%x)\n", __FUNCTION__, rw.cmd_type));
         break;

      default:
         TRACE (MPS, DBG_LEVEL_HIGH,
                ("%s - received unknown packet (%x)\n", __FUNCTION__,
                 rw.cmd_type));
         break;
   }
   return;
}


#endif /* CONFIG_DANUBE_MPS_PROC_DEBUG */

/**
 * Open MPS device.
 * Open the device from user mode (e.g. application) or kernel mode. An inode
 * value of 0..7 indicates a kernel mode access. In such a case the inode value
 * is used as minor ID.
 *
 * \param   inode   Pointer to device inode
 * \param   file_p  Pointer to file descriptor
 * \return  0       IFX_SUCCESS, device opened
 * \return  EMFILE  Device already open
 * \return  EINVAL  Invalid minor ID
 * \ingroup API
 */
IFX_int32_t ifx_mps_open (struct inode * inode, struct file * file_p)
{
   mps_comm_dev *pDev = &ifx_mps_dev;
   mps_mbx_dev *pMBDev;
   IFX_int32_t bcommand = 2;
   IFX_int32_t from_kernel = 0;
   mps_devices num;

   /* Check whether called from user or kernel mode */

   /* a trick: VMMC driver passes the first parameter as a value of
      'mps_devices' enum type, which in fact is [0..8]; So, if inode value is
      [0..NUM_VOICE_CHANNEL+1], then we make sure that we are calling from
      kernel space. */
   if (((IFX_int32_t) inode >= 0) &&
       ((IFX_int32_t) inode < NUM_VOICE_CHANNEL + 1))
   {
      from_kernel = 1;
      num = (IFX_int32_t) inode;
   }
   else
   {
      num = (mps_devices) MINOR (inode->i_rdev);        /* the real device */
   }

   /* check the device number */
   switch (num)
   {
      case command:
         pMBDev = &(pDev->command_mb);
         bcommand = 1;
         break;
      case voice0:
         pMBDev = &(pDev->voice_mb[0]);
         break;
      case voice1:
         pMBDev = &pDev->voice_mb[1];
         break;
      case voice2:
         pMBDev = &pDev->voice_mb[2];
         break;
      case voice3:
         pMBDev = &pDev->voice_mb[3];
         break;
      case voice4:
         pMBDev = &pDev->voice_mb[4];
         break;
      case voice5:
         pMBDev = &pDev->voice_mb[5];
         break;
      case voice6:
         pMBDev = &pDev->voice_mb[6];
         break;
      case voice7:
         pMBDev = &pDev->voice_mb[7];
         break;
#ifdef CONFIG_MPS_EVENT_MBX
      case event_mbx:
         pMBDev = &pDev->event_mbx;
         bcommand = 3;
         break;
#endif /* CONFIG_MPS_EVENT_MBX */
      default:
         TRACE (MPS, DBG_LEVEL_HIGH,
                ("IFX_MPS ERROR: max. device number exceed!\n"));
         return -EINVAL;
   }

   if ((IFX_SUCCESS) ==
       ifx_mps_common_open (pDev, pMBDev, bcommand, from_kernel))
   {
      if (!from_kernel)
      {

         /* installation was successfull */
         /* and use file_p->private_data to point to the device data */
         file_p->private_data = pMBDev;
#ifdef MODULE
         /* increment module use counter */
         /* MOD_INC_USE_COUNT; */
#endif /* */
      }
      return 0;
   }
   else
   {
      /* installation failed */
      TRACE (MPS, DBG_LEVEL_HIGH,
             ("IFX_MPS ERROR: Device %d is already open!\n", num));
      return -EMFILE;
   }
}


/**
 * Close MPS device.
 * Close the device from user mode (e.g. application) or kernel mode. An inode
 * value of 0..7 indicates a kernel mode access. In such a case the inode value
 * is used as minor ID.
 *
 * \param   inode   Pointer to device inode
 * \param   file_p  Pointer to file descriptor
 * \return  0       IFX_SUCCESS, device closed
 * \return  ENODEV  Device invalid
 * \return  EINVAL  Invalid minor ID
 * \ingroup API
 */
IFX_int32_t ifx_mps_close (struct inode * inode, struct file * file_p)
{
   mps_mbx_dev *pMBDev;
   IFX_int32_t from_kernel = 0;
   mps_devices num;

   /* Check whether called from user or kernel mode */

   /* a trick: VMMC driver passes the first parameter as a value of
      'mps_devices' enum type, which in fact is [0..8]; So, if inode value is
      [0..NUM_VOICE_CHANNEL+1], then we make sure that we are calling from
      kernel space. */
   if (((IFX_int32_t) inode >= 0) &&
       ((IFX_int32_t) inode < NUM_VOICE_CHANNEL + 1))
   {
      from_kernel = 1;

      /* Get corresponding mailbox device structure */
      if ((pMBDev =
           ifx_mps_get_device ((mps_devices) ((IFX_int32_t) inode))) == 0)
         return (-EINVAL);
      num = (IFX_int32_t) inode;
   }
   else
   {
      pMBDev = file_p->private_data;
      num = (mps_devices) MINOR (inode->i_rdev);        /* the real device */
   }

   if (IFX_NULL != pMBDev)
   {
      /* device is still available */
      if (ifx_mps_common_close (pMBDev, from_kernel) != IFX_SUCCESS)
      {
         TRACE (MPS, DBG_LEVEL_HIGH,
                ("IFX_MPS ERROR: Device %d is not open!\n", num));
         return -ENODEV;
      }

#ifdef MODULE
      if (!from_kernel)
      {
         /* decrement module use counter */
         /* MOD_DEC_USE_COUNT; */
      }
#endif /* */
      return 0;
   }
   else
   {
      /* something went totally wrong */
      TRACE (MPS, DBG_LEVEL_HIGH, ("IFX_MPS ERROR: pMBDev pointer is NULL!\n"));
      return -ENODEV;
   }
}


/**
 * Poll handler.
 * The select function of the driver. A user space program may sleep until
 * the driver wakes it up.
 *
 * \param   file_p  File structure of device
 * \param   wait    Internal table of poll wait queues
 * \return  mask    If new data is available the POLLPRI bit is set,
 *                  triggering an exception indication. If the device pointer
 *                  is null POLLERR is set.
 * \ingroup API
 */
static IFX_uint32_t ifx_mps_poll (struct file *file_p, poll_table * wait)
{
   mps_mbx_dev *pMBDev = file_p->private_data;
   IFX_uint32_t mask;

   /* add to poll queue */
   IFXOS_DrvSelectQueueAddTask ((IFXOS_drvSelectOSArg_t *) file_p,
                                &(pMBDev->mps_wakeuplist),
                                (IFXOS_drvSelectTable_t *) wait);

   mask = 0;

   /* upstream queue */
   if (*pMBDev->upstrm_fifo->pwrite_off != *pMBDev->upstrm_fifo->pread_off)
   {
#ifdef CONFIG_MPS_EVENT_MBX
      if (pMBDev->devID == event_mbx)
      {
         mask = POLLPRI;
      }
      else
      {
         mask = POLLIN | POLLRDNORM;
      }
#else /* */
      mask = POLLIN | POLLRDNORM;
#endif /* CONFIG_MPS_EVENT_MBX */
   }
   /* no downstream queue in case of event mailbox */
   if (pMBDev->dwstrm_fifo == IFX_NULL)
      return mask;

   /* downstream queue */
   if (ifx_mps_fifo_mem_available (pMBDev->dwstrm_fifo) != 0)
   {
      /* queue is not full */
      mask |= POLLOUT | POLLWRNORM;
   }

   if ((ifx_mps_dev.event.MPS_Ad0Reg.val & pMBDev->event_mask.MPS_Ad0Reg.val) ||
       (ifx_mps_dev.event.MPS_Ad1Reg.val & pMBDev->event_mask.MPS_Ad1Reg.val) ||
       (ifx_mps_dev.event.MPS_VCStatReg[0].val & pMBDev->event_mask.
        MPS_VCStatReg[0].val) ||
       (ifx_mps_dev.event.MPS_VCStatReg[1].val & pMBDev->event_mask.
        MPS_VCStatReg[1].val) ||
       (ifx_mps_dev.event.MPS_VCStatReg[2].val & pMBDev->event_mask.
        MPS_VCStatReg[2].val) ||
       (ifx_mps_dev.event.MPS_VCStatReg[3].val & pMBDev->event_mask.
        MPS_VCStatReg[3].val))
   {
      mask |= POLLPRI;
   }

   return mask;
}


/**
 * MPS IOCTL handler.
 * An inode value of 0..7 indicates a kernel mode access. In such a case the
 * inode value is used as minor ID.
 * The following IOCTLs are supported for the MPS device.
 * - #FIO_MPS_EVENT_REG
 * - #FIO_MPS_EVENT_UNREG
 * - #FIO_MPS_MB_READ
 * - #FIO_MPS_MB_WRITE
 * - #FIO_MPS_DOWNLOAD
 * - #FIO_MPS_GETVERSION
 * - #FIO_MPS_MB_RST_QUEUE
 * - #FIO_MPS_RESET
 * - #FIO_MPS_RESTART
 * - #FIO_MPS_GET_STATUS
 *
 * If MPS_FIFO_BLOCKING_WRITE is defined the following commands are also
 * available.
 * - #FIO_MPS_TXFIFO_SET
 * - #FIO_MPS_TXFIFO_GET
 *
 * \param   inode        Inode of device
 * \param   file_p       File structure of device
 * \param   nCmd         IOCTL command
 * \param   arg          Argument for some IOCTL commands
 * \return  0            Setting the LED bits was successfull
 * \return  -EINVAL      Invalid minor ID
 * \return  -ENOIOCTLCMD Invalid command
 * \ingroup API
 */
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,36))
IFX_int32_t ifx_mps_ioctl (struct inode * inode, struct file * file_p,
                           IFX_uint32_t nCmd, IFX_ulong_t arg)
#else
long ifx_mps_ioctl (struct file *file_p,
                           IFX_uint32_t nCmd, IFX_ulong_t arg)
#endif /* LINUX < 2.6.36 */
{
   IFX_int32_t retvalue = -EINVAL;
   mps_message rw_struct;
   mps_mbx_dev *pMBDev;
#if CONFIG_MPS_HISTORY_SIZE > 0
   mps_history cmd_history;
#endif /* */
   IFX_int32_t from_kernel = 0;

   /* a trick: VMMC driver passes the first parameter as a value of
      'mps_devices' enum type, which in fact is [0..8]; So, if inode value is
      [0..NUM_VOICE_CHANNEL+1], then we make sure that we are calling from
      kernel space. */
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,36))
   if (((IFX_int32_t) inode >= 0) &&
       ((IFX_int32_t) inode < NUM_VOICE_CHANNEL + 1))
#else
   if (((IFX_int32_t) file_p >= 0) &&
       ((IFX_int32_t) file_p < NUM_VOICE_CHANNEL + 1))
#endif
   {
      from_kernel = 1;

      /* Get corresponding mailbox device structure */
      if ((pMBDev =
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,36))
           ifx_mps_get_device ((mps_devices) ((IFX_int32_t) inode))) == 0)
#else
           ifx_mps_get_device ((mps_devices) ((IFX_int32_t) file_p))) == 0)
#endif
      {
         return (-EINVAL);
      }
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,36))
      inode = NULL;
#else
      file_p = NULL;
#endif
   }
   else
   {
      pMBDev = file_p->private_data;
   }

   switch (nCmd)
   {
      case FIO_MPS_EVENT_REG:
         {
            MbxEventRegs_s events;
            if (IFX_NULL ==
                IFXOS_CpyFromUser (&events, (IFX_void_t *) arg,
                                   sizeof (MbxEventRegs_s)))
            {
               TRACE (MPS, DBG_LEVEL_HIGH,
                      (KERN_ERR "[%s %s %d]: copy_from_user error\r\n",
                       __FILE__, __func__, __LINE__));
            }
            retvalue =
               ifx_mps_register_event_poll (pMBDev->devID, &events, IFX_NULL);
            if (retvalue == IFX_SUCCESS)
            {
               retvalue =
                  ifx_mps_event_activation_poll (pMBDev->devID, &events);
            }
            break;
         }
      case FIO_MPS_EVENT_UNREG:
         {
            MbxEventRegs_s events;
            events.MPS_Ad0Reg.val = 0;
            events.MPS_Ad1Reg.val = 0;
            events.MPS_VCStatReg[0].val = 0;
            events.MPS_VCStatReg[1].val = 0;
            events.MPS_VCStatReg[2].val = 0;
            events.MPS_VCStatReg[3].val = 0;
            ifx_mps_event_activation_poll (pMBDev->devID, &events);
            retvalue = ifx_mps_unregister_event_poll (pMBDev->devID);
            break;
         }
      case FIO_MPS_MB_READ:
         /* Read the data from mailbox stored in local FIFO */
         if (from_kernel)
         {
            retvalue = ifx_mps_mbx_read (pMBDev, (mps_message *) arg, 0);
         }
         else
         {
            IFX_uint32_t *pUserBuf;

            /* Initialize destination and copy mps_message from usermode */
            memset (&rw_struct, 0, sizeof (mps_message));
            if (IFX_NULL ==
                IFXOS_CpyFromUser (&rw_struct, (IFX_void_t *) arg,
                                   sizeof (mps_message)))
            {
               TRACE (MPS, DBG_LEVEL_HIGH,
                      (KERN_ERR "[%s %s %d]: copy_from_user error\r\n",
                       __FILE__, __func__, __LINE__));
            }
            pUserBuf = (IFX_uint32_t *) rw_struct.pData;        /* Remember
                                                                   usermode
                                                                   buffer */

            /* read data from upstream mailbox FIFO */
            retvalue = ifx_mps_mbx_read (pMBDev, &rw_struct, 0);
            if (retvalue != IFX_SUCCESS)
               return -ENOMSG;

            /* Copy data to usermode buffer... */
            if (IFX_NULL ==
                IFXOS_CpyToUser (pUserBuf, rw_struct.pData,
                                 rw_struct.nDataBytes))
            {
               TRACE (MPS, DBG_LEVEL_HIGH,
                      (KERN_ERR "[%s %s %d]: copy_to_user error\r\n", __FILE__,
                       __func__, __LINE__));
            }
            ifx_mps_bufman_free (rw_struct.pData);

            /* ... and finally restore the buffer pointer and copy mps_message
               back! */
            rw_struct.pData = (IFX_uint8_t *) pUserBuf;
            if (IFX_NULL ==
                IFXOS_CpyToUser ((IFX_void_t *) arg, &rw_struct,
                                 sizeof (mps_message)))
            {
               TRACE (MPS, DBG_LEVEL_HIGH,
                      (KERN_ERR "[%s %s %d]: copy_to_user error\r\n", __FILE__,
                       __func__, __LINE__));
            }
         }
         break;
      case FIO_MPS_MB_WRITE:
         /* Write data to send to the mailbox into the local FIFO */
         if (from_kernel)
         {
            if (pMBDev->devID == command)
            {
               return (ifx_mps_mbx_write_cmd (pMBDev, (mps_message *) arg));
            }
            else
            {
               return (ifx_mps_mbx_write_data (pMBDev, (mps_message *) arg));
            }
         }
         else
         {
            IFX_uint32_t *pUserBuf;
            if (IFX_NULL ==
                IFXOS_CpyFromUser (&rw_struct, (IFX_void_t *) arg,
                                   sizeof (mps_message)))
            {
               TRACE (MPS, DBG_LEVEL_HIGH,
                      (KERN_ERR "[%s %s %d]: copy_from_user error\r\n",
                       __FILE__, __func__, __LINE__));
            }

            /* Remember usermode buffer */
            pUserBuf = (IFX_uint32_t *) rw_struct.pData;

            /* Allocate kernelmode buffer for writing data */
            rw_struct.pData =
               ifx_mps_bufman_malloc (rw_struct.nDataBytes, 0x10);

            /* rw_struct.pData = ifx_mps_bufman_malloc(rw_struct.nDataBytes,
               GFP_KERNEL); */
            if (rw_struct.pData == IFX_NULL)
            {
               return (-ENOMEM);
            }

            /* copy data to kernelmode buffer and write to mailbox FIFO */
            if (IFX_NULL ==
                IFXOS_CpyFromUser (rw_struct.pData, pUserBuf,
                                   rw_struct.nDataBytes))
            {
               TRACE (MPS, DBG_LEVEL_HIGH,
                      (KERN_ERR "[%s %s %d]: copy_from_user error\r\n",
                       __FILE__, __func__, __LINE__));
            }
            if (pMBDev->devID == command)
            {
               retvalue = ifx_mps_mbx_write_cmd (pMBDev, &rw_struct);
               ifx_mps_bufman_free (rw_struct.pData);
            }
            else
            {
               if ((retvalue =
                    ifx_mps_mbx_write_data (pMBDev, &rw_struct)) != IFX_SUCCESS)
                  ifx_mps_bufman_free (rw_struct.pData);
            }

            /* ... and finally restore the buffer pointer and copy mps_message
               back! */
            rw_struct.pData = (IFX_uint8_t *) pUserBuf;
            if (IFX_NULL ==
                IFXOS_CpyToUser ((IFX_void_t *) arg, &rw_struct,
                                 sizeof (mps_message)))
            {
               TRACE (MPS, DBG_LEVEL_HIGH,
                      (KERN_ERR "[%s %s %d]: copy_to_user error\r\n", __FILE__,
                       __func__, __LINE__));
            }
         }
         break;
      case FIO_MPS_DOWNLOAD:
         {
            /* Download firmware file */
            if (pMBDev->devID == command)
            {
               mps_fw dwnld_struct;

               if (from_kernel)
               {
                  dwnld_struct.data = ((mps_fw *) arg)->data;
                  dwnld_struct.length = ((mps_fw *) arg)->length;
               }
               else
               {
                  if (IFX_NULL ==
                      IFXOS_CpyFromUser (&dwnld_struct, (IFX_void_t *) arg,
                                         sizeof (mps_fw)))
                  {
                     TRACE (MPS, DBG_LEVEL_HIGH,
                            (KERN_ERR "[%s %s %d]: copy_from_user error\r\n",
                             __FILE__, __func__, __LINE__));
                  }
               }

               retvalue = ifx_mps_download_firmware (pMBDev, &dwnld_struct);

               if (IFX_SUCCESS != retvalue)
               {
                  TRACE (MPS, DBG_LEVEL_HIGH,
                         (KERN_ERR "IFX_MPS: firmware download error (%i)!\n",
                          retvalue));
               }
               else
                  retvalue = ifx_mps_bufman_init ();
            }
            else
            {
               retvalue = -EINVAL;
            }
            break;
         }                      /* FIO_MPS_DOWNLOAD */
      case FIO_MPS_GETVERSION:
         if (from_kernel)
         {
            memcpy ((IFX_char_t *) arg, (IFX_char_t *) ifx_mps_device_version,
                    strlen (ifx_mps_device_version));
         }
         else
         {
            if (IFX_NULL ==
                IFXOS_CpyToUser ((IFX_void_t *) arg, ifx_mps_device_version,
                                 strlen (ifx_mps_device_version)))
            {
               TRACE (MPS, DBG_LEVEL_HIGH,
                      (KERN_ERR "[%s %s %d]: copy_to_user error\r\n", __FILE__,
                       __func__, __LINE__));
            }
         }
         retvalue = IFX_SUCCESS;
         break;
      case FIO_MPS_RESET:
         /* Reset of the DSP */
         ifx_mps_reset ();
         break;
      case FIO_MPS_RESTART:
         /* Restart of the DSP */
         if (!from_kernel)
         {
            TRACE (MPS, DBG_LEVEL_HIGH, ("IFX_MPS: Restarting firmware..."));
         }
         retvalue = ifx_mps_restart ();
         if (retvalue == IFX_SUCCESS)
         {
            if (!from_kernel)
               ifx_mps_get_fw_version (1);
            retvalue = ifx_mps_bufman_init ();
         }
         break;
#ifdef MPS_FIFO_BLOCKING_WRITE
      case FIO_MPS_TXFIFO_SET:
         /* Set the mailbox TX FIFO blocking mode */
         if (pMBDev->devID == command)
         {
            retvalue = -EINVAL; /* not supported for this command MB */
         }
         else
         {
            if (arg > 0)
            {
               pMBDev->bBlockWriteMB = IFX_TRUE;
            }
            else
            {
               pMBDev->bBlockWriteMB = IFX_FALSE;
               Sem_Unlock (pMBDev->sem_write_fifo);
            }
            retvalue = IFX_SUCCESS;
         }
         break;
      case FIO_MPS_TXFIFO_GET:
         /* Get the mailbox TX FIFO to blocking */
         if (pMBDev->devID == command)
         {
            retvalue = -EINVAL;
         }
         else
         {
            if (!from_kernel)
            {
               if (IFX_NULL ==
                   IFXOS_CpyToUser ((IFX_void_t *) arg, &pMBDev->bBlockWriteMB,
                                    sizeof (bool_t)))
               {
                  TRACE (MPS, DBG_LEVEL_HIGH,
                         (KERN_ERR "[%s %s %d]: copy_to_user error\r\n",
                          __FILE__, __func__, __LINE__));
               }
            }
            retvalue = IFX_SUCCESS;
         }
         break;
#endif /* MPS_FIFO_BLOCKING_WRITE */
      case FIO_MPS_GET_STATUS:
         {
            IFXOS_INTSTAT flags;

            /* get the status of the channel */
            if (!from_kernel)
            {
               if (IFX_NULL ==
                   IFXOS_CpyToUser ((IFX_void_t *) arg, &ifx_mps_dev.event,
                                    sizeof (MbxEventRegs_s)))
               {
                  TRACE (MPS, DBG_LEVEL_HIGH,
                         (KERN_ERR "[%s %s %d]: copy_to_user error\r\n",
                          __FILE__, __func__, __LINE__));
               }
            }
            IFXOS_LOCKINT (flags);
            ifx_mps_dev.event.MPS_Ad0Reg.val &=
               ~pMBDev->event_mask.MPS_Ad0Reg.val;
            ifx_mps_dev.event.MPS_Ad1Reg.val &=
               ~pMBDev->event_mask.MPS_Ad1Reg.val;
            ifx_mps_dev.event.MPS_VCStatReg[0].val &=
               ~pMBDev->event_mask.MPS_VCStatReg[0].val;
            ifx_mps_dev.event.MPS_VCStatReg[1].val &=
               ~pMBDev->event_mask.MPS_VCStatReg[1].val;
            ifx_mps_dev.event.MPS_VCStatReg[2].val &=
               ~pMBDev->event_mask.MPS_VCStatReg[2].val;
            ifx_mps_dev.event.MPS_VCStatReg[3].val &=
               ~pMBDev->event_mask.MPS_VCStatReg[3].val;
            IFXOS_UNLOCKINT (flags);
            retvalue = IFX_SUCCESS;
            break;
         }
#if CONFIG_MPS_HISTORY_SIZE > 0
      case FIO_MPS_GET_CMD_HISTORY:
         {
            IFXOS_INTSTAT flags;

            if (from_kernel)
            {

               /* TODO */
            }
            else
            {
               IFX_uint32_t *pUserBuf;
               IFX_uint32_t begin;

               /* Initialize destination and copy mps_message from usermode */
               memset (&cmd_history, 0, sizeof (mps_history));
               if (IFX_NULL ==
                   IFXOS_CpyFromUser (&cmd_history, (IFX_void_t *) arg,
                                      sizeof (mps_history)))
               {
                  TRACE (MPS, DBG_LEVEL_HIGH,
                         (KERN_ERR "[%s %s %d]: copy_from_user error\r\n",
                          __FILE__, __func__, __LINE__));
               }
               if (cmd_history.len < MPS_HISTORY_BUFFER_SIZE)
                  return -ENOBUFS;      /* not enough buffer space */
               pUserBuf = cmd_history.buf;      /* Remember usermode buffer */
               IFXOS_LOCKINT (flags);
               if (ifx_mps_history_buffer_overflowed == 0)
               {
                  cmd_history.len = ifx_mps_history_buffer_words;
                  IFXOS_UNLOCKINT (flags);
                  /* Copy data to usermode buffer... */
                  if (IFX_NULL ==
                      IFXOS_CpyToUser (pUserBuf, ifx_mps_history_buffer,
                                       cmd_history.len * 4))
                  {
                     TRACE (MPS, DBG_LEVEL_HIGH,
                            (KERN_ERR "[%s %s %d]: copy_to_user error\r\n",
                             __FILE__, __func__, __LINE__));
                  }
                  IFXOS_LOCKINT (flags);
               }
               else
               {
                  cmd_history.len = MPS_HISTORY_BUFFER_SIZE;
                  begin =
                     ifx_mps_history_buffer_words % MPS_HISTORY_BUFFER_SIZE;
                  IFXOS_UNLOCKINT (flags);
                  /* Copy data to usermode buffer... */
                  if (IFX_NULL ==
                      IFXOS_CpyToUser (pUserBuf,
                                       (&ifx_mps_history_buffer[begin]),
                                       (MPS_HISTORY_BUFFER_SIZE - begin) * 4))
                  {
                     TRACE (MPS, DBG_LEVEL_HIGH,
                            (KERN_ERR "[%s %s %d]: copy_to_user error\r\n",
                             __FILE__, __func__, __LINE__));
                  }
                  if (IFX_NULL ==
                      IFXOS_CpyToUser ((&pUserBuf
                                        [MPS_HISTORY_BUFFER_SIZE - begin]),
                                       (&ifx_mps_history_buffer[0]), begin * 4))
                  {
                     TRACE (MPS, DBG_LEVEL_HIGH,
                            (KERN_ERR "[%s %s %d]: copy_to_user error\r\n",
                             __FILE__, __func__, __LINE__));
                  }
                  IFXOS_LOCKINT (flags);
               }
               cmd_history.total_words = ifx_mps_history_buffer_words;
               cmd_history.freeze = ifx_mps_history_buffer_freeze;
               if (ifx_mps_history_buffer_freeze)
               {
                  /* restart history logging */
                  ifx_mps_history_buffer_freeze = 0;
                  ifx_mps_history_buffer_words = 0;
                  ifx_mps_history_buffer_overflowed = 0;
               }

               IFXOS_UNLOCKINT (flags);

               /* ... and finally restore the buffer pointer and copy
                  cmd_history back! */
               if (IFX_NULL ==
                   IFXOS_CpyToUser ((IFX_void_t *) arg, &cmd_history,
                                    sizeof (mps_history)))
               {
                  TRACE (MPS, DBG_LEVEL_HIGH,
                         (KERN_ERR "[%s %s %d]: copy_to_user error\r\n",
                          __FILE__, __func__, __LINE__));
               }
            }
            retvalue = IFX_SUCCESS;
            break;
         }
#endif /* CONFIG_MPS_HISTORY_SIZE > 0 */
#ifdef CONFIG_MPS_EVENT_MBX
      case FIO_MPS_EVENT_MBX_REG:
         {
            retvalue = ifx_mps_event_mbx_activation_poll (1);
            break;
         }
      case FIO_MPS_EVENT_MBX_UNREG:
         {
            ifx_mps_event_mbx_activation_poll (0);
            break;
         }
#endif /* CONFIG_MPS_EVENT_MBX */
      default:
         {
            TRACE (MPS, DBG_LEVEL_HIGH,
                   ("IFX_MPS_Ioctl: Invalid IOCTL handle %d passed.\n", nCmd));
            retvalue = -ENOIOCTLCMD;
            break;
         }
   }
   return retvalue;
}


/**
 * Register data callback.
 * Allows the upper layer to register a callback function either for
 * downstream (tranmsit mailbox space available) or for upstream (read data
 * available)
 *
 * \param   type     DSP device entity ( 1 - command, 2 - voice0, 3 - voice1,
 *                   4 - voice2, 5 - voice3, 6 - voice4 )
 * \param   dir      Direction (1 - upstream, 2 - downstream)
 * \param   callback Callback function to register
 * \return  0        IFX_SUCCESS, callback registered successfully
 * \return  ENXIO    Wrong DSP device entity (only 1-5 supported)
 * \return  EBUSY    Callback already registered
 * \return  EINVAL   Callback parameter null
 * \ingroup API
 */
IFX_int32_t ifx_mps_register_data_callback (mps_devices type, IFX_uint32_t dir,
                                            IFX_void_t (*callback) (mps_devices
                                                                    type))
{
   mps_mbx_dev *pMBDev;

   if (callback == IFX_NULL)
   {
      return (-EINVAL);
   }

   /* Get corresponding mailbox device structure */
   if ((pMBDev = ifx_mps_get_device (type)) == 0)
      return (-ENXIO);

   /* Enter the desired callback function */
   switch (dir)
   {
      case 1:                  /* register upstream callback function */
         if (pMBDev->up_callback != IFX_NULL)
         {
            return (-EBUSY);
         }
         else
         {
            pMBDev->up_callback = callback;
         }
         break;
      case 2:                  /* register downstream callback function */
         if (pMBDev->down_callback != IFX_NULL)
         {
            return (-EBUSY);
         }
         else
         {
            pMBDev->down_callback = callback;
         }
         break;
      default:
         break;
   }

   return (IFX_SUCCESS);
}


/**
 * Unregister data callback.
 * Allows the upper layer to unregister the callback function previously
 * registered.
 *
 * \param   type   DSP device entity ( 1 - command, 2 - voice0, 3 - voice1,
 *                 4 - voice2, 5 - voice3, 6 - voice4)
 * \param   dir    Direction (1 - upstream, 2 - downstream)
 * \return  0      IFX_SUCCESS, callback registered successfully
 * \return  ENXIO  Wrong DSP device entity (only 1-5 supported)
 * \return  EINVAL Nothing to unregister
 * \return  EINVAL Callback value null
 * \ingroup API
 */
IFX_int32_t ifx_mps_unregister_data_callback (mps_devices type,
                                              IFX_uint32_t dir)
{
   mps_mbx_dev *pMBDev;

   /* Get corresponding mailbox device structure */
   if ((pMBDev = ifx_mps_get_device (type)) == 0)
      return (-ENXIO);

   /* Delete the desired callback function */
   switch (dir)
   {
      case 1:
         if (pMBDev->up_callback == IFX_NULL)
         {
            return (-EINVAL);
         }
         else
         {
            pMBDev->up_callback = IFX_NULL;
         }
         break;
      case 2:
         if (pMBDev->down_callback == IFX_NULL)
         {
            return (-EINVAL);
         }
         else
         {
            pMBDev->down_callback = IFX_NULL;
         }
         break;
      default:
         {
            TRACE (MPS, DBG_LEVEL_HIGH,
                   ("DANUBE_MPS_DSP_UnregisterDataCallback: Invalid Direction %d\n",
                    dir));
            return (-ENXIO);
         }
   }

   return (IFX_SUCCESS);
}


/**
 * Register event callback.
 * Allows the upper layer to register a callback function either for events
 * specified by the mask parameter.
 *
 * \param   type     DSP device entity ( 1 - command, 2 - voice0, 3 - voice1,
 *                   4 - voice2, 5 - voice3, 6 - voice4 )
 * \param   mask     Mask according to MBC_ISR content
 * \param   callback Callback function to register
 * \return  0        IFX_SUCCESS, callback registered successfully
 * \return  ENXIO    Wrong DSP device entity (only 1-5 supported)
 * \return  EBUSY    Callback already registered
 * \ingroup API
 */
IFX_int32_t ifx_mps_register_event_poll (mps_devices type,
                                         MbxEventRegs_s * mask,
                                         IFX_void_t (*callback) (MbxEventRegs_s
                                                                 * events))
{
   mps_mbx_dev *pMBDev;

   callback = callback;

   /* Get corresponding mailbox device structure */
   if ((pMBDev = ifx_mps_get_device (type)) == 0)
      return (-ENXIO);
   memcpy ((IFX_char_t *) & pMBDev->event_mask, (IFX_char_t *) mask,
           sizeof (MbxEventRegs_s));
   return (IFX_SUCCESS);
}


/**
 * Unregister event callback.
 * Allows the upper layer to unregister the callback function previously
 * registered.
 *
 * \param   type   DSP device entity ( 1 - command, 2 - voice0, 3 - voice1,
 *                 4 - voice2, 5 - voice3, 6 - voice4 )
 * \return  0      IFX_SUCCESS, callback registered successfully
 * \return  ENXIO  Wrong DSP device entity (only 1-5 supported)
 * \ingroup API
 */
IFX_int32_t ifx_mps_unregister_event_poll (mps_devices type)
{
   mps_mbx_dev *pMBDev;

   /* Get corresponding mailbox device structure */
   if ((pMBDev = ifx_mps_get_device (type)) == 0)
      return (-ENXIO);

   /* Delete the desired callback function */
   memset ((IFX_char_t *) & pMBDev->event_mask, 0, sizeof (MbxEventRegs_s));
   return (IFX_SUCCESS);
}


/**
 * Register event callback.
 * Allows the upper layer to register a callback function either for events
 * specified by the mask parameter.
 *
 * \param   type     DSP device entity ( 1 - command, 2 - voice0, 3 - voice1,
 *                   4 - voice2, 5 - voice3, 6 - voice4 )
 * \param   mask     Mask according to MBC_ISR content
 * \param   callback Callback function to register
 * \return  0        IFX_SUCCESS, callback registered successfully
 * \return  ENXIO    Wrong DSP device entity (only 1-5 supported)
 * \return  EBUSY    Callback already registered
 * \ingroup API
 */
IFX_int32_t ifx_mps_register_event_callback (mps_devices type,
                                             MbxEventRegs_s * mask,
                                             IFX_void_t (*callback)
                                             (MbxEventRegs_s * events))
{
   mps_mbx_dev *pMBDev;

   /* Get corresponding mailbox device structure */
   if ((pMBDev = ifx_mps_get_device (type)) == 0)
      return (-ENXIO);

   /* Enter the desired callback function */
   if (pMBDev->event_callback != IFX_NULL)
   {
      return (-EBUSY);
   }
   else
   {
      memcpy ((IFX_char_t *) & pMBDev->callback_event_mask, (IFX_char_t *) mask,
              sizeof (MbxEventRegs_s));
      pMBDev->event_callback = callback;
   }
   return (IFX_SUCCESS);
}


/**
 * Unregister event callback.
 * Allows the upper layer to unregister the callback function previously
 * registered.
 *
 * \param   type   DSP device entity ( 1 - command, 2 - voice0, 3 - voice1,
 *                 4 - voice2, 5 - voice3, 6 - voice4 )
 * \return  0      IFX_SUCCESS, callback registered successfully
 * \return  ENXIO  Wrong DSP device entity (only 1-5 supported)
 * \ingroup API
 */
IFX_int32_t ifx_mps_unregister_event_callback (mps_devices type)
{
   mps_mbx_dev *pMBDev;

   /* Get corresponding mailbox device structure */
   if ((pMBDev = ifx_mps_get_device (type)) == 0)
      return (-ENXIO);

   /* Delete the desired callback function */
   memset ((IFX_char_t *) & pMBDev->callback_event_mask, 0,
           sizeof (MbxEventRegs_s));
   pMBDev->event_callback = IFX_NULL;
   return (IFX_SUCCESS);
}


#ifdef CONFIG_MPS_EVENT_MBX
/**
 * Register event mailbox callback.
 * Allows the upper layer to register a callback function for events in the
 * specified by the mask parameter.
 *
 * \param   callback Callback function to register
 * \return  0        IFX_SUCCESS, callback registered successfully
 * \return  EBUSY    Callback already registered
 * \ingroup API
 */
IFX_int32_t ifx_mps_register_event_mbx_callback (IFX_uint32_t pDev,
                                                 IFX_void_t (*callback)
                                                 (IFX_uint32_t pDev,
                                                  mps_event_msg * msg))
{
   mps_mbx_dev *pMBDev;

   /* Get corresponding mailbox device structure */
   pMBDev = ifx_mps_get_device (event_mbx);

   /* Enter the desired callback function */
   if (pMBDev->event_mbx_callback != IFX_NULL)
   {
      return (-EBUSY);
   }
   else
   {
      pMBDev->event_callback_handle = pDev;
      pMBDev->event_mbx_callback = callback;
   }

   return (ifx_mps_event_mbx_activation_poll (1));
}


/**
 * Unregister event mailbox callback.
 * Allows the upper layer to unregister the callback function previously
 * registered.
 *
 * \return  0      IFX_SUCCESS, callback registered successfully
 * \ingroup API
 */
IFX_int32_t ifx_mps_unregister_event_mbx_callback (void)
{
   mps_mbx_dev *pMBDev;

   /* Get corresponding mailbox device structure */
   pMBDev = ifx_mps_get_device (event_mbx);
   pMBDev->event_callback_handle = 0;
   pMBDev->event_mbx_callback = IFX_NULL;
   ifx_mps_event_mbx_activation_poll (0);
   return (IFX_SUCCESS);
}
#endif /* CONFIG_MPS_EVENT_MBX */

/**
 * Read from mailbox upstream FIFO.
 * This function reads from the mailbox upstream FIFO selected by type.
 *
 * \param   type  DSP device entity ( 1 - command, 2 - voice0, 3 - voice1,
 *                4 - voice2, 5 - voice3 )
 * \param   rw    Pointer to message structure for received data
 * \return  0     IFX_SUCCESS, successful read operation
 * \return  ENXIO Wrong DSP device entity (only 1-5 supported)
 * \return  -1    ERROR, in case of read error.
 * \ingroup API
 */
IFX_int32_t ifx_mps_read_mailbox (mps_devices type, mps_message * rw)
{
   IFX_int32_t ret;

   switch (type)
   {
      case command:
         ret = ifx_mps_mbx_read (&ifx_mps_dev.command_mb, rw, 0);
         break;
      case voice0:
         ret = ifx_mps_mbx_read (&ifx_mps_dev.voice_mb[0], rw, 0);
         break;
      case voice1:
         ret = ifx_mps_mbx_read (&ifx_mps_dev.voice_mb[1], rw, 0);
         break;
      case voice2:
         ret = ifx_mps_mbx_read (&ifx_mps_dev.voice_mb[2], rw, 0);
         break;
      case voice3:
         ret = ifx_mps_mbx_read (&ifx_mps_dev.voice_mb[3], rw, 0);
         break;
      case voice4:
         ret = ifx_mps_mbx_read (&ifx_mps_dev.voice_mb[4], rw, 0);
         break;
      case voice5:
         ret = ifx_mps_mbx_read (&ifx_mps_dev.voice_mb[5], rw, 0);
         break;
      case voice6:
         ret = ifx_mps_mbx_read (&ifx_mps_dev.voice_mb[6], rw, 0);
         break;
      case voice7:
         ret = ifx_mps_mbx_read (&ifx_mps_dev.voice_mb[7], rw, 0);
         break;
      default:
         ret = -ENXIO;
   }
   return (ret);
}


/**
 * Write to downstream mailbox buffer.
 * This function writes data to either the command or to the voice FIFO
 *
 * \param   type  DSP device entity ( 1 - command, 2 - voice0, 3 - voice1,
 *                4 - voice2, 5 - voice3 )
 * \param   rw    Pointer to message structure
 * \return  0       IFX_SUCCESS, successful write operation
 * \return  -ENXIO  Wrong DSP device entity (only 1-5 supported)
 * \return  -EAGAIN ERROR, in case of FIFO overflow.
 * \ingroup API
 */
IFX_int32_t ifx_mps_write_mailbox (mps_devices type, mps_message * rw)
{
   IFX_int32_t ret;

   switch (type)
   {
      case command:
         ret = ifx_mps_mbx_write_cmd (&ifx_mps_dev.command_mb, rw);
         break;
      case voice0:
         ret = ifx_mps_mbx_write_data (&ifx_mps_dev.voice_mb[0], rw);
         break;
      case voice1:
         ret = ifx_mps_mbx_write_data (&ifx_mps_dev.voice_mb[1], rw);
         break;
      case voice2:
         ret = ifx_mps_mbx_write_data (&ifx_mps_dev.voice_mb[2], rw);
         break;
      case voice3:
         ret = ifx_mps_mbx_write_data (&ifx_mps_dev.voice_mb[3], rw);
         break;
      case voice4:
         ret = ifx_mps_mbx_write_data (&ifx_mps_dev.voice_mb[4], rw);
         break;
      case voice5:
         ret = ifx_mps_mbx_write_data (&ifx_mps_dev.voice_mb[5], rw);
         break;
      case voice6:
         ret = ifx_mps_mbx_write_data (&ifx_mps_dev.voice_mb[6], rw);
         break;
      case voice7:
         ret = ifx_mps_mbx_write_data (&ifx_mps_dev.voice_mb[7], rw);
         break;
      default:
         ret = -ENXIO;
   }

   return (ret);
}


#ifdef CONFIG_PROC_FS
/**
 * Create MPS version proc file output.
 * This function creates the output for the MPS version proc file
 *
 * \param   buf      Buffer to write the string to
 * \return  len      Lenght of data in buffer
 * \ingroup Internal
 */
static IFX_int32_t ifx_mps_get_version_proc (IFX_char_t * buf)
{
   IFX_int32_t len;

   len = sprintf (buf, "%s%s\n", IFX_MPS_INFO_STR, ifx_mps_device_version);
   len +=
      sprintf (buf + len, "Compiled on %s, %s for Linux kernel %s\n", __DATE__,
               __TIME__, UTS_RELEASE);

#ifdef CONFIG_DANUBE_MPS_PROC_DEBUG
   len += sprintf (buf + len, "Supported debug tags:\n");
   len +=
      sprintf (buf + len, "%s = Initialize hardware for voice CPU\n",
               MPS_FW_INIT_TAG);
   len += sprintf (buf + len, "%s = Start firmware\n", MPS_FW_START_TAG);
   len +=
      sprintf (buf + len, "%s = Send buffer provisioning message\n",
               MPS_FW_BUFFER_TAG);
   len +=
      sprintf (buf + len, "%s = Opening voice0 mailbox\n",
               MPS_FW_OPEN_VOICE_TAG);
   len +=
      sprintf (buf + len, "%s = Register voice0 data callback\n",
               MPS_FW_REGISTER_CALLBACK_TAG);
   len +=
      sprintf (buf + len, "%s = Send message to voice0 mailbox\n",
               MPS_FW_SEND_MESSAGE_TAG);
   len += sprintf (buf + len, "%s = Restart voice CPU\n", MPS_FW_RESTART_TAG);
   len +=
      sprintf (buf + len, "%s = Packet loop enable\n",
               MPS_FW_ENABLE_PACKET_LOOP_TAG);
   len +=
      sprintf (buf + len, "%s = Packet loop disable\n",
               MPS_FW_DISABLE_PACKET_LOOP_TAG);
   len +=
      sprintf (buf + len, "%d Packets received\n",
               ifx_mps_rtp_voice_data_count);
#endif /* CONFIG_DANUBE_MPS_PROC_DEBUG */
   return len;
}


/**
 * Create MPS status proc file output.
 * This function creates the output for the MPS status proc file
 *
 * \param   buf      Buffer to write the string to
 * \return  len      Lenght of data in buffer
 * \ingroup Internal
 */
IFX_int32_t ifx_mps_get_status_proc (IFX_char_t * buf)
{
   IFX_int32_t len = 0, i;

   /* len = sprintf(buf, "Open files: %d\n", MOD_IN_USE); */
   len += sprintf (buf + len, "Status registers:\n");
   len += sprintf (buf + len, "   AD0ENR = 0x%08x\n", *IFX_MPS_AD0ENR);
   len += sprintf (buf + len, "   RAD0SR = 0x%08x\n", *IFX_MPS_RAD0SR);
   len += sprintf (buf + len, "   AD1ENR = 0x%08x\n", *IFX_MPS_AD1ENR);
   len += sprintf (buf + len, "   RAD1SR = 0x%08x\n", *IFX_MPS_RAD1SR);
   for (i = 0; i < 4; i++)
   {
      len += sprintf (buf + len, "   VC%iENR = 0x%08x\n", i, IFX_MPS_VC0ENR[i]);
      len += sprintf (buf + len, "   RVC%iSR = 0x%08x\n", i, IFX_MPS_RVC0SR[i]);
   }

   /* Print internals of the command mailbox fifo */
   len +=
      sprintf (buf + len, "\n * CMD *\t\tUP\t\tDO\t%s\n",
               (ifx_mps_dev.command_mb.Installed ==
                IFX_TRUE) ? "(active)" : "(idle)");
   len +=
      sprintf (buf + len, "   Size: \t  %8d\t  %8d\n",
               ifx_mps_dev.cmd_upstrm_fifo.size,
               ifx_mps_dev.cmd_dwstrm_fifo.size);
   len +=
      sprintf (buf + len, "   Fill: \t  %8d\t  %8d\n",
               ifx_mps_dev.cmd_upstrm_fifo.size - 1 -
               ifx_mps_fifo_mem_available (&ifx_mps_dev.cmd_upstrm_fifo),
               ifx_mps_dev.cmd_dwstrm_fifo.size - 1 -
               ifx_mps_fifo_mem_available (&ifx_mps_dev.cmd_dwstrm_fifo));
   len +=
      sprintf (buf + len, "   Free: \t  %8d\t  %8d\n",
               ifx_mps_fifo_mem_available (&ifx_mps_dev.cmd_upstrm_fifo),
               ifx_mps_fifo_mem_available (&ifx_mps_dev.cmd_dwstrm_fifo));

   /* Printout the number of interrupts and fifo misses */
   len +=
      sprintf (buf + len, "   Pkts: \t  %8d\t  %8d\n",
               ifx_mps_dev.cmd_upstrm_fifo.pkts,
               ifx_mps_dev.cmd_dwstrm_fifo.pkts);
   len +=
      sprintf (buf + len, "   Bytes:\t  %8d\t  %8d\n",
               ifx_mps_dev.cmd_upstrm_fifo.bytes,
               ifx_mps_dev.cmd_dwstrm_fifo.bytes);
   len += sprintf (buf + len, "\n * VOICE *\t\tUP\t\tDO\n");
   len +=
      sprintf (buf + len, "   Size: \t  %8d\t  %8d\n",
               ifx_mps_dev.voice_upstrm_fifo.size,
               ifx_mps_dev.voice_dwstrm_fifo.size);
   len +=
      sprintf (buf + len, "   Fill: \t  %8d\t  %8d\n",
               ifx_mps_dev.voice_upstrm_fifo.size - 1 -
               ifx_mps_fifo_mem_available (&ifx_mps_dev.voice_upstrm_fifo),
               ifx_mps_dev.voice_dwstrm_fifo.size - 1 -
               ifx_mps_fifo_mem_available (&ifx_mps_dev.voice_dwstrm_fifo));
   len +=
      sprintf (buf + len, "   Free: \t  %8d\t  %8d\n",
               ifx_mps_fifo_mem_available (&ifx_mps_dev.voice_upstrm_fifo),
               ifx_mps_fifo_mem_available (&ifx_mps_dev.voice_dwstrm_fifo));
   len +=
      sprintf (buf + len, "   Pkts: \t  %8d\t  %8d\n",
               ifx_mps_dev.voice_upstrm_fifo.pkts,
               ifx_mps_dev.voice_dwstrm_fifo.pkts);
   len +=
      sprintf (buf + len, "   Bytes: \t  %8d\t  %8d\n",
               ifx_mps_dev.voice_upstrm_fifo.bytes,
               ifx_mps_dev.voice_dwstrm_fifo.bytes);
   len +=
      sprintf (buf + len, "   Discd: \t  %8d\n",
               ifx_mps_dev.voice_upstrm_fifo.discards);
   len +=
      sprintf (buf + len, "   minLv: \t  %8d\t  %8d\n",
               ifx_mps_dev.voice_upstrm_fifo.min_space,
               ifx_mps_dev.voice_dwstrm_fifo.min_space);

   for (i = 0; i < (NUM_VOICE_CHANNEL - 1); i++)
   {
      len +=
         sprintf (buf + len, "\n * CH%i *\t\tUP\t\tDO\t%s\n", i,
                  (ifx_mps_dev.voice_mb[i].Installed ==
                   IFX_FALSE) ? "(idle)" : "(active)");
      len +=
         sprintf (buf + len, "   Size: \t  %8d\n",
                  ifx_mps_dev.voice_mb[i].upstrm_fifo->size);
      len +=
         sprintf (buf + len, "   Fill: \t  %8d\n",
                  ifx_mps_dev.voice_mb[i].upstrm_fifo->size - 1 -
                  ifx_mps_fifo_mem_available (ifx_mps_dev.voice_mb[i].
                                              upstrm_fifo));
      len +=
         sprintf (buf + len, "   Free: \t  %8d\n",
                  ifx_mps_fifo_mem_available (ifx_mps_dev.voice_mb[i].
                                              upstrm_fifo));
      len +=
         sprintf (buf + len, "   Pkts: \t  %8d\n",
                  ifx_mps_dev.voice_mb[i].upstrm_fifo->pkts);
      len +=
         sprintf (buf + len, "   Bytes: \t  %8d\n",
                  ifx_mps_dev.voice_mb[i].upstrm_fifo->bytes);
      len +=
         sprintf (buf + len, "   Discd: \t  %8d\n",
                  ifx_mps_dev.voice_mb[i].upstrm_fifo->discards);
      len +=
         sprintf (buf + len, "   minLv: \t  %8d\n",
                  ifx_mps_dev.voice_mb[i].upstrm_fifo->min_space);
   }
   return len;
}


/**
 * Create MPS mailbox proc file output.
 * This function creates the output for the MPS mailbox proc file
 *
 * \param   buf      Buffer to write the string to
 * \return  len      Lenght of data in buffer
 * \ingroup Internal
 */
static IFX_int32_t ifx_mps_get_mailbox_proc (IFX_char_t * buf)
{
   IFX_int32_t len;
   IFX_uint32_t i;

   len = sprintf (buf, " * CMD * UP");
   len +=
      sprintf (buf + len, " (wr: 0x%08x, rd: 0x%08x)\n",
               (IFX_uint32_t) ifx_mps_dev.cmd_upstrm_fifo.pend +
               (IFX_uint32_t) * ifx_mps_dev.cmd_upstrm_fifo.pwrite_off,
               (IFX_uint32_t) ifx_mps_dev.cmd_upstrm_fifo.pend +
               (IFX_uint32_t) * ifx_mps_dev.cmd_upstrm_fifo.pread_off);

   for (i = 0; i < ifx_mps_dev.cmd_upstrm_fifo.size; i += 16)
   {
      len +=
         sprintf (buf + len, "   0x%08x: %08x %08x %08x %08x\n",
                  (IFX_uint32_t) (ifx_mps_dev.cmd_upstrm_fifo.pend + (i / 4)),
                  *(ifx_mps_dev.cmd_upstrm_fifo.pend + (i / 4)),
                  *(ifx_mps_dev.cmd_upstrm_fifo.pend + 1 + (i / 4)),
                  *(ifx_mps_dev.cmd_upstrm_fifo.pend + 2 + (i / 4)),
                  *(ifx_mps_dev.cmd_upstrm_fifo.pend + 3 + (i / 4)));
   }
   len += sprintf (buf + len, "\n * CMD * DO");
   len +=
      sprintf (buf + len, " (wr: 0x%08x, rd: 0x%08x)\n",
               (IFX_uint32_t) ifx_mps_dev.cmd_dwstrm_fifo.pend +
               (IFX_uint32_t) * ifx_mps_dev.cmd_dwstrm_fifo.pwrite_off,
               (IFX_uint32_t) ifx_mps_dev.cmd_dwstrm_fifo.pend +
               (IFX_uint32_t) * ifx_mps_dev.cmd_dwstrm_fifo.pread_off);
   for (i = 0; i < ifx_mps_dev.cmd_dwstrm_fifo.size; i += 16)
   {
      len +=
         sprintf (buf + len, "   0x%08x: %08x %08x %08x %08x\n",
                  (IFX_uint32_t) (ifx_mps_dev.cmd_dwstrm_fifo.pend + (i / 4)),
                  *(ifx_mps_dev.cmd_dwstrm_fifo.pend + (i / 4)),
                  *(ifx_mps_dev.cmd_dwstrm_fifo.pend + 1 + (i / 4)),
                  *(ifx_mps_dev.cmd_dwstrm_fifo.pend + 2 + (i / 4)),
                  *(ifx_mps_dev.cmd_dwstrm_fifo.pend + 3 + (i / 4)));
   }
   len += sprintf (buf + len, "\n * VOICE * UP");
   len +=
      sprintf (buf + len, " (wr:0x%08x, rd: 0x%08x)\n",
               (IFX_uint32_t) ifx_mps_dev.voice_upstrm_fifo.pend +
               (IFX_uint32_t) * ifx_mps_dev.voice_upstrm_fifo.pwrite_off,
               (IFX_uint32_t) ifx_mps_dev.voice_upstrm_fifo.pend +
               (IFX_uint32_t) * ifx_mps_dev.voice_upstrm_fifo.pread_off);
   for (i = 0; i < ifx_mps_dev.voice_upstrm_fifo.size; i += 16)
   {
      len +=
         sprintf (buf + len, "   0x%08x: %08x %08x %08x %08x\n",
                  (IFX_uint32_t) (ifx_mps_dev.voice_upstrm_fifo.pend + (i / 4)),
                  *(ifx_mps_dev.voice_upstrm_fifo.pend + (i / 4)),
                  *(ifx_mps_dev.voice_upstrm_fifo.pend + 1 + (i / 4)),
                  *(ifx_mps_dev.voice_upstrm_fifo.pend + 2 + (i / 4)),
                  *(ifx_mps_dev.voice_upstrm_fifo.pend + 3 + (i / 4)));
   }
   len += sprintf (buf + len, "\n * VOICE * DO");
   len +=
      sprintf (buf + len, " (wr: 0x%08x, rd: 0x%08x)\n",
               (IFX_uint32_t) ifx_mps_dev.voice_dwstrm_fifo.pend +
               (IFX_uint32_t) * ifx_mps_dev.voice_dwstrm_fifo.pwrite_off,
               (IFX_uint32_t) ifx_mps_dev.voice_dwstrm_fifo.pend +
               (IFX_uint32_t) * ifx_mps_dev.voice_dwstrm_fifo.pread_off);
   for (i = 0; i < ifx_mps_dev.voice_dwstrm_fifo.size; i += 16)
   {
      len +=
         sprintf (buf + len, "   0x%08x: %08x %08x %08x %08x\n",
                  (IFX_uint32_t) (ifx_mps_dev.voice_dwstrm_fifo.pend + (i / 4)),
                  *(ifx_mps_dev.voice_dwstrm_fifo.pend + (i / 4)),
                  *(ifx_mps_dev.voice_dwstrm_fifo.pend + 1 + (i / 4)),
                  *(ifx_mps_dev.voice_dwstrm_fifo.pend + 2 + (i / 4)),
                  *(ifx_mps_dev.voice_dwstrm_fifo.pend + 3 + (i / 4)));
   }

#ifdef CONFIG_MPS_EVENT_MBX
   len += sprintf (buf + len, "\n * EVENT * UP");
   len +=
      sprintf (buf + len, " (wr: 0x%08x, rd: 0x%08x)\n",
               (IFX_uint32_t) ifx_mps_dev.event_upstrm_fifo.pend +
               (IFX_uint32_t) * ifx_mps_dev.event_upstrm_fifo.pwrite_off,
               (IFX_uint32_t) ifx_mps_dev.event_upstrm_fifo.pend +
               (IFX_uint32_t) * ifx_mps_dev.event_upstrm_fifo.pread_off);
   for (i = 0; i < ifx_mps_dev.event_upstrm_fifo.size; i += 16)
   {
      len +=
         sprintf (buf + len, "   0x%08x: %08x %08x %08x %08x\n",
                  (IFX_uint32_t) (ifx_mps_dev.event_upstrm_fifo.pend + (i / 4)),
                  *(ifx_mps_dev.event_upstrm_fifo.pend + (i / 4)),
                  *(ifx_mps_dev.event_upstrm_fifo.pend + 1 + (i / 4)),
                  *(ifx_mps_dev.event_upstrm_fifo.pend + 2 + (i / 4)),
                  *(ifx_mps_dev.event_upstrm_fifo.pend + 3 + (i / 4)));
   }
#endif /* CONFIG_MPS_EVENT_MBX */
   return len;
}


/**
 * Create MPS sw fifo proc file output.
 * This function creates the output for the sw fifo proc file
 *
 * \param   buf      Buffer to write the string to
 * \return  len      Lenght of data in buffer
 * \ingroup Internal
 */
static IFX_int32_t ifx_mps_get_swfifo_proc (IFX_char_t * buf)
{
   IFX_int32_t len, i, chan;
   len = 0;

   for (chan = 0; chan < (NUM_VOICE_CHANNEL - 1); chan++)
   {
      len += sprintf (buf + len, "\n * CH%i * UP", chan);
      len +=
         sprintf (buf + len, " (wr:0x%08x, rd: 0x%08x)\n",
                  (IFX_uint32_t) ifx_mps_dev.sw_upstrm_fifo[chan].pend +
                  (IFX_uint32_t) * ifx_mps_dev.sw_upstrm_fifo[chan].pwrite_off,
                  (IFX_uint32_t) ifx_mps_dev.sw_upstrm_fifo[chan].pend +
                  (IFX_uint32_t) * ifx_mps_dev.sw_upstrm_fifo[chan].pread_off);

      for (i = 0; i < ifx_mps_dev.sw_upstrm_fifo[chan].size; i += 16)
      {
         len +=
            sprintf (buf + len, "   0x%08x: %08x %08x %08x %08x\n",
                     (IFX_uint32_t) (ifx_mps_dev.sw_upstrm_fifo[chan].pend +
                                     (i / 4)),
                     *(ifx_mps_dev.sw_upstrm_fifo[chan].pend + (i / 4)),
                     *(ifx_mps_dev.sw_upstrm_fifo[chan].pend + 1 + (i / 4)),
                     *(ifx_mps_dev.sw_upstrm_fifo[chan].pend + 2 + (i / 4)),
                     *(ifx_mps_dev.sw_upstrm_fifo[chan].pend + 3 + (i / 4)));
      }
   }
   return len;
}


/**
 * Create MPS proc file output.
 * This function creates the output for the MPS proc file according to the
 * function specified in the data parameter, which is setup during registration.
 *
 * \param   buf      Buffer to write the string to
 * \param   start    not used (Linux internal)
 * \param   offset   not used (Linux internal)
 * \param   count    not used (Linux internal)
 * \param   eof      Set to 1 when all data is stored in buffer
 * \param   data     not used (Linux internal)
 * \return  len      Lenght of data in buffer
 * \ingroup Internal
 */
static IFX_int32_t ifx_mps_read_proc (IFX_char_t * page, IFX_char_t ** start,
                                      off_t off, IFX_int32_t count,
                                      IFX_int32_t * eof, IFX_void_t * data)
{
   IFX_int32_t len;
   IFX_int32_t (*fn) (IFX_char_t * buf);

   if (data != IFX_NULL)
   {
      fn = data;
      len = fn (page);
   }
   else
      return 0;

   if (len <= off + count)
      *eof = 1;

   *start = page + off;
   len -= off;

   if (len > count)
      len = count;

   if (len < 0)
      len = 0;

   return len;
}


#ifdef CONFIG_DANUBE_MPS_PROC_DEBUG
/**
 * Process MPS proc file input.
 * This function evaluates the input of the MPS formware proc file and downloads
 * the given firmware into the voice CPU.
 *
 * \param   file    file structure for proc file
 * \param   buffer  buffer holding the data
 * \param   count   number of characters in buffer
 * \param   data    unused
 * \return  count   Number of processed characters
 * \ingroup Internal
 */
static IFX_int32_t ifx_mps_write_fw_procmem (struct file *file,
                                             const IFX_char_t * buffer,
                                             IFX_ulong_t count,
                                             IFX_void_t * data)
{
   IFX_int32_t t = 0;

   if (count == (sizeof (MPS_FW_INIT_TAG)))
   {
      if (!strncmp (buffer, MPS_FW_INIT_TAG, sizeof (MPS_FW_INIT_TAG) - 1))
      {
         TRACE (MPS, DBG_LEVEL_HIGH, ("Initializing Voice CPU...\n"));

         if (ifx_mps_init_gpt ())
            return 0;
         return count;
      }
   }
   if (count == (sizeof (MPS_FW_START_TAG)))
   {
      if (!strncmp (buffer, MPS_FW_START_TAG, sizeof (MPS_FW_START_TAG) - 1))
      {
         TRACE (MPS, DBG_LEVEL_HIGH,
                ("Starting FW (size=%d)...\n", ifx_mps_firmware_buffer_pos));
         ifx_mps_firmware_struct.data =
            (IFX_uint32_t *) ifx_mps_firmware_buffer;
         ifx_mps_firmware_struct.length = ifx_mps_firmware_buffer_pos;
         ifx_mps_download_firmware (IFX_NULL, &ifx_mps_firmware_struct);
         return count;
      }
   }
   if (count == (sizeof (MPS_FW_BUFFER_TAG)))
   {
      if (!strncmp (buffer, MPS_FW_BUFFER_TAG, sizeof (MPS_FW_BUFFER_TAG) - 1))
      {
         TRACE (MPS, DBG_LEVEL_HIGH, ("Providing Buffers...\n"));
         ifx_mps_bufman_init ();
         return count;
      }
   }
   if (count == (sizeof (MPS_FW_OPEN_VOICE_TAG)))
   {
      if (!strncmp
          (buffer, MPS_FW_OPEN_VOICE_TAG, sizeof (MPS_FW_OPEN_VOICE_TAG) - 1))
      {
         TRACE (MPS, DBG_LEVEL_HIGH, ("Opening voice0 mailbox...\n"));
         ifx_mps_open ((struct inode *) 2, IFX_NULL);
         return count;
      }
   }
   if (count == (sizeof (MPS_FW_REGISTER_CALLBACK_TAG)))
   {
      if (!strncmp
          (buffer, MPS_FW_REGISTER_CALLBACK_TAG,
           sizeof (MPS_FW_REGISTER_CALLBACK_TAG) - 1))
      {
         TRACE (MPS, DBG_LEVEL_HIGH, ("Opening voice0 mailbox...\n"));
         ifx_mps_register_data_callback (2, 1, ifx_mbx_dummy_rcv_data_callback);
         return count;
      }
   }
   if (count == (sizeof (MPS_FW_SEND_MESSAGE_TAG)))
   {
      if (!strncmp
          (buffer, MPS_FW_SEND_MESSAGE_TAG,
           sizeof (MPS_FW_SEND_MESSAGE_TAG) - 1))
      {
         mps_message wrt_data;
         IFX_int32_t i;
         TRACE (MPS, DBG_LEVEL_HIGH,
                ("Sending message to voice0 mailbox...\n"));
         wrt_data.pData = (IFX_uint8_t *) IFXOS_BlockAlloc (100);
         wrt_data.nDataBytes = 100;
         wrt_data.cmd_type = CMD_RTP_VOICE_DATA_PACKET;
         wrt_data.RTP_PaylOffset = 0x00000000;
         for (i = 0; i < (100 / 4); i++)
            *((IFX_uint32_t *) KSEG1ADDR ((wrt_data.pData + (i << 2)))) =
               0xdeadbeef;
         ifx_mps_write_mailbox (2, &wrt_data);
         return count;
      }
   }
   if (count == (sizeof (MPS_FW_RESTART_TAG)))
   {
      if (!strncmp
          (buffer, MPS_FW_RESTART_TAG, sizeof (MPS_FW_RESTART_TAG) - 1))
      {
         TRACE (MPS, DBG_LEVEL_HIGH, ("Restarting voice CPU...\n"));
         ifx_mps_restart ();
         return count;
      }
   }
   if (count == (sizeof (MPS_FW_ENABLE_PACKET_LOOP_TAG)))
   {
      if (!strncmp
          (buffer, MPS_FW_ENABLE_PACKET_LOOP_TAG,
           sizeof (MPS_FW_ENABLE_PACKET_LOOP_TAG) - 1))
      {
         TRACE (MPS, DBG_LEVEL_HIGH, ("Enabling packet loop...\n"));
         ifx_mps_packet_loop = 1;
         return count;
      }
   }
   if (count == (sizeof (MPS_FW_DISABLE_PACKET_LOOP_TAG)))
   {
      if (!strncmp
          (buffer, MPS_FW_DISABLE_PACKET_LOOP_TAG,
           sizeof (MPS_FW_DISABLE_PACKET_LOOP_TAG) - 1))
      {
         TRACE (MPS, DBG_LEVEL_HIGH, ("Disabling packet loop...\n"));
         ifx_mps_packet_loop = 0;
         return count;
      }
   }
   if ((count + ifx_mps_firmware_buffer_pos) < MPS_FIRMWARE_BUFFER_SIZE)
   {
      for (t = 0; t < count; t++)
         ifx_mps_firmware_buffer[ifx_mps_firmware_buffer_pos + t] = buffer[t];
      ifx_mps_firmware_buffer_pos += count;
      return count;
   }
   return 0;
}
#endif /* CONFIG_DANUBE_MPS_PROC_DEBUG */

#if CONFIG_MPS_HISTORY_SIZE > 0
/**
 * Process MPS proc file output.
 * This function outputs the history buffer showing the messages
 * sent to command mailbox so far.
 *
 * \param   buf     buffer for the data
 * \param   start   buffer start pointer
 * \param   offset  buffer offset
 * \param   count   buffer size
 * \param   eof     end of file flag
 * \param   data    unused
 * \return  buflen  number of characters in buffer
 * \ingroup Internal
 */
static IFX_int32_t ifx_mps_read_history_proc (IFX_char_t * buf,
                                              IFX_char_t ** start, off_t offset,
                                              IFX_int32_t count,
                                              IFX_int32_t * eof,
                                              IFX_void_t * data)
{
   IFX_long_t buflen = 0;
   IFX_int32_t i, begin;
   static IFX_int32_t print_pos = 0;
   static IFX_int32_t buffer_pos = 0;
   static IFX_int32_t printing = 0;
   static IFX_int32_t len = 0;

   /* Sanity check: return eof if current file posision index is greater than
      requested byte count */
   if ((printing == 0) && (offset > count))
   {
      *eof = 1;
      return 0;
   }

   /* ifx_mps_history_buffer_overflowed is set to 1 if number of written
      command words is >= MPS_HISTORY_BUFFER_SIZE; See drv_mps_vmmc_common.c:
      ifx_mps_mbx_write_cmd() ifx_mps_history_buffer_words is in the range [0
      ... (MPS_HISTORY_BUFFER_SIZE-1)] */
   if (ifx_mps_history_buffer_overflowed == 0)
   {
      /* print ifx_mps_history_buffer_words words from 0 index to
         ifx_mps_history_buffer_words */
      len = ifx_mps_history_buffer_words;
      begin = 0;
   }
   else
   {
      /* print MPS_HISTORY_BUFFER_SIZE words from ifx_mps_history_buffer_words
         index to the end of buffer, then from 0 to the remaining index (ring
         operation) */
      len = MPS_HISTORY_BUFFER_SIZE;
      begin = ifx_mps_history_buffer_words;
   }

   if (printing == 1)
   {
      begin = buffer_pos;
      *start = buf;
   }
   else
   {
      printing = 1;
#ifdef DEBUG
      /* Total count of received words may go out of range, as it's being only
         incremented. Left for debugging purposes only. */
      buflen +=
         sprintf (buf + buflen, "MPS command mailbox received %d words so far. \
Printing last %d words...\n", ifx_mps_history_buffer_words_total, len);
#else /* */
      buflen += sprintf (buf + buflen, "Printing last %d words...\n", len);
#endif /* */
   }

   if (ifx_mps_history_buffer_words)
   {
      i = begin;

      do
      {
         buflen +=
            sprintf (buf + buflen, "%5d: %08x\n", print_pos,
                     ifx_mps_history_buffer[i]);
         i++;
         print_pos++;

         if (i == MPS_HISTORY_BUFFER_SIZE)
            i = 0;

         if (buflen > (count - 80))
         {
            buffer_pos = i;
            break;
         }
      } while (print_pos != len);
   }

   if (ifx_mps_history_buffer_freeze)
      buflen +=
         sprintf (buf + buflen,
                  "---- FREEZE ----\nTo restart logging write '1' to this proc file.\n");
   *eof = 1;
   /* No more data available? */
   if (print_pos == len)
   {
      buffer_pos = printing = print_pos = len = 0;
   }

   return buflen;
}


/**
 * Process MPS proc file input.
 * This function unlocks the command history logging.
 *
 * \param   file    file structure for proc file
 * \param   buffer  buffer holding the data
 * \param   count   number of characters in buffer
 * \param   data    unused
 * \return  count   Number of processed characters
 * \ingroup Internal
 */
static IFX_int32_t ifx_mps_write_history_proc (struct file *pfile,
                                               const IFX_char_t * buffer,
                                               IFX_ulong_t count,
                                               IFX_void_t * data)
{
   pfile = pfile;
   buffer = buffer;
   data = data;

   ifx_mps_history_buffer_freeze = 0;
   ifx_mps_history_buffer_words = 0;
   ifx_mps_history_buffer_overflowed = 0;
   TRACE (MPS, DBG_LEVEL_HIGH, ("MPS command history logging restarted!\n"));
   return count;
}
#endif /* CONFIG_MPS_HISTORY_SIZE > 0 */

/**
 * Create MPS CPU1 load address proc file output.
 * This function creates the output for the CPU1 load address proc file
 *
 * \param   buf      Buffer to write the string to
 * \return  len      Lenght of data in buffer
 * \ingroup Internal
 */
static IFX_int32_t ifx_mps_loadaddr_get_proc(IFX_char_t *buf)
{
   IFX_int32_t len;

   len = sprintf (buf, "CPU1 LOAD ADDRESS = 0x%p\n", cpu1_base_addr);

   return len;
}

#endif /* CONFIG_PROC_FS */

/**
   This function initializes the module.
\param
   None.
\return  IFX_SUCCESS, module initialized
\return  EPERM    Reset of CPU1 failed
\return  ENOMEM   No memory left for structures
*/
#ifndef IKOS_MINI_BOOT
#ifdef VMMC_WITH_MPS
int
#else
static IFX_int32_t __init
#endif
ifx_mps_init_module (void)
#else /* */
IFX_int32_t __init ifx_mps_init_module (void)
#endif                          /* */
{
   IFX_int32_t result;
   IFX_int32_t i;

   sprintf (ifx_mps_device_version, "%d.%d.%d.%d", MAJORSTEP, MINORSTEP,
            VERSIONSTEP, VERS_TYPE);

   TRACE (MPS, DBG_LEVEL_HIGH,
          ("%s%s, (c) 2006-2010 Lantiq Deutschland GmbH\n", IFX_MPS_INFO_STR,
           ifx_mps_device_version));

   sprintf (ifx_mps_dev_name, IFX_MPS_DEV_NAME);

   /* setup cache operations */
#if defined(CONFIG_MIPS) && !defined(CONFIG_MIPS_UNCACHED)
#if defined(SYSTEM_DANUBE)
   bDoCacheOps = IFX_TRUE; /* on Danube always perform cache ops */
#elif defined(SYSTEM_AR9) || defined(SYSTEM_VR9) || \
      defined(SYSTEM_FALCON) || defined(SYSTEM_XRX300)
   /* on AR9/VR9 cache is configured by BSP;
      here we check whether the D-cache is shared or partitioned;
      1) in case of shared D-cache all cache operations are omitted;
      2) in case of partitioned D-cache the cache operations are performed,
      the same way as on Danube */
   if(read_c0_mvpcontrol() & MVPCONTROL_CPA_BIT)
   {
      if (read_vpe_c0_vpeopt() & VPEOPT_DWX_MASK &&
          (read_vpe_c0_vpeopt() & VPEOPT_DWX_MASK) != VPEOPT_DWX_MASK)
      {
         bDoCacheOps = IFX_TRUE;
      }
   }
#else
#error unknown system
#endif /* SYSTEM_ */
#endif /*defined(CONFIG_MIPS) && !defined(CONFIG_MIPS_UNCACHED)*/

   /* Configure GPTC timer */
   result = ifx_mps_init_gpt ();
   if (result)
      return result;

   /* init the device driver structure */
   if (0 != ifx_mps_init_structures (&ifx_mps_dev))
      return -ENOMEM;

   /* register char module in kernel */
   result = lq_mps_os_register (&ifx_mps_dev);
   if (result)
      return result;

   /* reset the device before initializing the device driver */
   ifx_mps_reset ();
   result = request_irq (INT_NUM_IM4_IRL18,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0))
                         ifx_mps_ad0_irq, IRQF_DISABLED
#else /* */
                         (irqreturn_t (*)(int, IFX_void_t *, struct pt_regs *))
                         ifx_mps_ad0_irq, SA_INTERRUPT
#endif /* */
                         , "mps_mbx ad0", &ifx_mps_dev);
   if (result)
      return result;
   result = request_irq (INT_NUM_IM4_IRL19,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0))
                         ifx_mps_ad1_irq, IRQF_DISABLED
#else /* */
                         (irqreturn_t (*)(int, IFX_void_t *, struct pt_regs *))
                         ifx_mps_ad1_irq, SA_INTERRUPT
#endif /* */
                         , "mps_mbx ad1", &ifx_mps_dev);
   if (result)
      return result;

   /* register status interrupts for voice channels */
   for (i = 0; i < 4; ++i)
   {
      sprintf (&voice_channel_int_name[i][0], "mps_mbx vc%d", i);
      result = request_irq (INT_NUM_IM4_IRL14 + i,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0))
                            ifx_mps_vc_irq, IRQF_DISABLED
#else /* */
                            (irqreturn_t (*)
                             (int, IFX_void_t *,
                              struct pt_regs *)) ifx_mps_vc_irq, SA_INTERRUPT
#endif /* */
                            , &voice_channel_int_name[i][0], &ifx_mps_dev);
      if (result)
         return result;
   }

#if !defined(CONFIG_LANTIQ)
   /** \todo This is handled already with request_irq, remove */

   /* Enable all MPS Interrupts at ICU0 */
   MPS_INTERRUPTS_ENABLE (0x0000FF80);
#endif

   /* enable mailbox interrupts */
   ifx_mps_enable_mailbox_int ();
   /* init FW ready event */
   IFXOS_EventInit (&fw_ready_evt);

#ifdef CONFIG_PROC_FS
   /* install the proc entry */
#ifdef DEBUG
   TRACE (MPS, DBG_LEVEL_HIGH, (KERN_INFO "IFX_MPS: using proc fs\n"));
#endif /* */
   ifx_mps_proc_dir = proc_mkdir ("driver/" IFX_MPS_DEV_NAME, IFX_NULL);
   if (ifx_mps_proc_dir != IFX_NULL)
   {
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,30))
      ifx_mps_proc_dir->owner = THIS_MODULE;
#endif /* < Linux 2.6.30 */
#ifdef CONFIG_DANUBE_MPS_PROC_DEBUG
      ifx_mps_proc_file = create_proc_entry ("firmware", 0, ifx_mps_proc_dir);
      if (ifx_mps_proc_file != IFX_NULL)
      {
         ifx_mps_proc_file->write_proc = ifx_mps_write_fw_procmem;
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,30))
         ifx_mps_proc_file->owner = THIS_MODULE;
#endif /* < Linux 2.6.30 */
      }
#endif /* CONFIG_DANUBE_MPS_PROC_DEBUG */
      ifx_mps_proc_file =
         create_proc_read_entry ("version", S_IFREG | S_IRUGO, ifx_mps_proc_dir,
                                 ifx_mps_read_proc,
                                 (IFX_void_t *) ifx_mps_get_version_proc);
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,30))
      if (ifx_mps_proc_file != IFX_NULL)
         ifx_mps_proc_file->owner = THIS_MODULE;
#endif /* < Linux 2.6.30 */
      ifx_mps_proc_file =
         create_proc_read_entry ("status", S_IFREG | S_IRUGO, ifx_mps_proc_dir,
                                 ifx_mps_read_proc,
                                 (IFX_void_t *) ifx_mps_get_status_proc);
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,30))
      if (ifx_mps_proc_file != IFX_NULL)
         ifx_mps_proc_file->owner = THIS_MODULE;
#endif /* < Linux 2.6.30 */
      ifx_mps_proc_file =
         create_proc_read_entry ("mailbox", S_IFREG | S_IRUGO, ifx_mps_proc_dir,
                                 ifx_mps_read_proc,
                                 (IFX_void_t *) ifx_mps_get_mailbox_proc);
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,30))
      if (ifx_mps_proc_file != IFX_NULL)
         ifx_mps_proc_file->owner = THIS_MODULE;
#endif /* < Linux 2.6.30 */
      ifx_mps_proc_file =
         create_proc_read_entry ("swfifo", S_IFREG | S_IRUGO, ifx_mps_proc_dir,
                                 ifx_mps_read_proc,
                                 (IFX_void_t *) ifx_mps_get_swfifo_proc);
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,30))
      if (ifx_mps_proc_file != IFX_NULL)
         ifx_mps_proc_file->owner = THIS_MODULE;
#endif /* < Linux 2.6.30 */
      ifx_mps_proc_file =
         create_proc_read_entry ("fastbuf", S_IFREG | S_IRUGO, ifx_mps_proc_dir,
                                 ifx_mps_read_proc,
                                 (IFX_void_t *) ifx_mps_fastbuf_get_proc);
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,30))
      if (ifx_mps_proc_file != IFX_NULL)
         ifx_mps_proc_file->owner = THIS_MODULE;
#endif /* < Linux 2.6.30 */
#if CONFIG_MPS_HISTORY_SIZE > 0
      if ((ifx_mps_history_proc =
           create_proc_entry ("history", 0, ifx_mps_proc_dir)))
      {
         ifx_mps_history_proc->read_proc = ifx_mps_read_history_proc;
         ifx_mps_history_proc->write_proc = ifx_mps_write_history_proc;
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,30))
         ifx_mps_history_proc->owner = THIS_MODULE;
#endif /* < Linux 2.6.30 */
      }
#endif /* CONFIG_MPS_HISTORY_SIZE > 0 */
      ifx_mps_proc_file =
         create_proc_read_entry ("loadaddr", S_IFREG | S_IRUGO, ifx_mps_proc_dir,
                                 ifx_mps_read_proc,
                                 (IFX_void_t *) ifx_mps_loadaddr_get_proc);
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,30))
      if (ifx_mps_proc_file != IFX_NULL)
         ifx_mps_proc_file->owner = THIS_MODULE;
#endif /* < Linux 2.6.30 */
   }
   else
   {
      TRACE (MPS, DBG_LEVEL_HIGH, ("IFX_MPS: cannot create proc entry\n"));
   }
#endif /* */

#ifdef IKOS_MINI_BOOT
#ifdef SYSTEM_DANUBE
   TRACE (MPS, DBG_LEVEL_HIGH, ("Enabling GPTC...\n"));
   if (result = ifx_mps_init_gpt_danube ())
      return result;
#endif /*DANUBE*/
      TRACE (MPS, DBG_LEVEL_HIGH, ("Downloading Firmware...\n"));
   ifx_mps_download_firmware (IFX_NULL, (mps_fw *) 0xa0a00000);
   udelay (500);
   TRACE (MPS, DBG_LEVEL_HIGH, ("Providing Buffers...\n"));
   ifx_mps_bufman_init ();
#endif /* */
   return IFX_SUCCESS;
}


/**
   This function cleans up the module.
\param
   None.
\return
   None.
*/
#ifdef VMMC_WITH_MPS
void
#else
static IFX_void_t __exit
#endif
ifx_mps_cleanup_module (void)
{
   IFX_uint32_t i;

   /* disable mailbox interrupts */
   ifx_mps_disable_mailbox_int ();

#if !defined(CONFIG_LANTIQ)
   /* disable Interrupts at ICU0 */
   /* Disable DFE/AFE 0 Interrupts */
   MPS_INTERRUPTS_DISABLE (DANUBE_MPS_AD0_IR4);
#endif

   /* disable all MPS interrupts */
   ifx_mps_disable_all_int ();
   ifx_mps_shutdown ();

   /* unregister char module from kernel */
   lq_mps_os_unregister (&ifx_mps_dev);

   /* release the memory usage of the device driver structure */
   ifx_mps_release_structures (&ifx_mps_dev);

   /* release all interrupts at the system */
   free_irq (INT_NUM_IM4_IRL18, &ifx_mps_dev);
   free_irq (INT_NUM_IM4_IRL19, &ifx_mps_dev);

   /* register status interrupts for voice channels */
   for (i = 0; i < 4; ++i)
   {
      free_irq (INT_NUM_IM4_IRL14 + i, &ifx_mps_dev);
   }
#ifdef CONFIG_PROC_FS
#if CONFIG_MPS_HISTORY_SIZE > 0
   remove_proc_entry ("history", ifx_mps_proc_dir);
#endif /* CONFIG_MPS_HISTORY_SIZE > 0 */
#ifdef CONFIG_DANUBE_MPS_PROC_DEBUG
   remove_proc_entry ("firmware", ifx_mps_proc_dir);
#endif /* CONFIG_DANUBE_MPS_PROC_DEBUG */
   remove_proc_entry ("mailbox", ifx_mps_proc_dir);
   remove_proc_entry ("swfifo", ifx_mps_proc_dir);
   remove_proc_entry ("fastbuf", ifx_mps_proc_dir);
   remove_proc_entry ("version", ifx_mps_proc_dir);
   remove_proc_entry ("status", ifx_mps_proc_dir);
   remove_proc_entry ("loadaddr", ifx_mps_proc_dir);
   remove_proc_entry ("driver/" IFX_MPS_DEV_NAME, IFX_NULL);
#endif /* CONFIG_PROC_FS */
   TRACE (MPS, DBG_LEVEL_HIGH, (KERN_INFO "IFX_MPS: cleanup done\n"));
}

#ifndef VMMC_WITH_MPS
module_init (ifx_mps_init_module);
module_exit (ifx_mps_cleanup_module);

#ifndef IKOS_MINI_BOOT
#ifndef DEBUG
EXPORT_SYMBOL (ifx_mps_write_mailbox);
EXPORT_SYMBOL (ifx_mps_register_data_callback);
EXPORT_SYMBOL (ifx_mps_unregister_data_callback);
EXPORT_SYMBOL (ifx_mps_register_event_callback);
EXPORT_SYMBOL (ifx_mps_unregister_event_callback);
EXPORT_SYMBOL (ifx_mps_read_mailbox);

EXPORT_SYMBOL (ifx_mps_dd_mbx_int_enable);
EXPORT_SYMBOL (ifx_mps_dd_mbx_int_disable);

#ifdef CONFIG_MPS_EVENT_MBX
EXPORT_SYMBOL (ifx_mps_register_event_mbx_callback);
EXPORT_SYMBOL (ifx_mps_unregister_event_mbx_callback);

#endif /* CONFIG_MPS_EVENT_MBX */
EXPORT_SYMBOL (ifx_mps_ioctl);
EXPORT_SYMBOL (ifx_mps_open);
EXPORT_SYMBOL (ifx_mps_close);

#endif /* */
#endif /* */
#endif /* !VMMC_WITH_MPS */
#endif /* LINUX */
