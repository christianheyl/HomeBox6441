/******************************************************************************

                              Copyright (c) 2013
                            Lantiq Deutschland GmbH
                             http://www.lantiq.com

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

****************************************************************************
   Module      : drv_mps_vmmc_common.c
   Description : This file contains the implementation of the common MPS
                 driver functions.
*******************************************************************************/

/* ============================= */
/* Includes                      */
/* ============================= */
#include "drv_config.h"
#include "drv_mps_vmmc_bsp.h"

#if (defined(GENERIC_OS) && defined(GREENHILLS_CHECK))
   #include "drv_vmmc_ghs.h"
#endif /* (defined(GENERIC_OS) && defined(GREENHILLS_CHECK)) */

#undef USE_PLAIN_VOICE_FIRMWARE
#undef PRINT_ON_ERR_INTERRUPT
#undef FAIL_ON_ERR_INTERRUPT

#ifdef LINUX
#include <linux/version.h>
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,33))
   #include <linux/autoconf.h>
#else
   #include <generated/autoconf.h>
#endif /* < Linux 2.6.33 */
#include <linux/interrupt.h>
#include <linux/delay.h>
#endif /* LINUX */

/* lib_ifxos headers */
#include "ifx_types.h"
#include "ifxos_linux_drv.h"
#include "ifxos_lock.h"
#include "ifxos_select.h"
#include "ifxos_event.h"
#include "ifxos_memory_alloc.h"
#include "ifxos_interrupt.h"
#include "ifxos_time.h"

#ifdef LINUX
#if !defined(SYSTEM_FALCON)
   #if (BSP_API_VERSION < 3)
      #include <asm/ifx/ifx_regs.h>
      #include <asm/ifx/ifx_gptu.h>
   #else
      #include <ltq_regs.h>
      #include <ltq_gptu.h>
   #endif
#else /* SYSTEM_FALCON */
#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,28))
#include <lantiq.h>
#include <irq.h>
#include <lantiq_timer.h>

#define ifx_gptu_timer_request    lq_request_timer
#define ifx_gptu_timer_start      lq_start_timer
#define ifx_gptu_countvalue_get   lq_get_count_value
#define ifx_gptu_timer_free       lq_free_timer

#define bsp_mask_and_ack_irq      lq_mask_and_ack_irq
#else
#include <asm/ifx/ifx_regs.h>
#include <asm/ifx/ifx_gptu.h>
#endif
#include <sys1_reg.h>
#include <sysctrl.h>
#endif /* SYSTEM_FALCON */
#endif /* LINUX */

#include "drv_mps_vmmc.h"
#include "drv_mps_vmmc_dbg.h"
#include "drv_mps_vmmc_device.h"
#include "drv_mps_vmmc_crc32.h"

#ifdef VMMC_WITH_MPS
#ifdef TAPI_PACKET_OWNID
#include "drv_tapi_kpi_io.h"
#include "drv_vmmc_fw_data.h"
#endif /* TAPI_PACKET_OWNID */
#endif /* VMMC_WITH_MPS */

#ifdef CONFIG_DEBUG_MINI_BOOT
#define CONFIG_DANUBE_USE_IKOS
#endif /* */

/* ============================= */
/* Local Macros & Definitions    */
/* ============================= */
#define IFX_MPS_UNUSED(var) ((IFX_void_t)(var))

/* ============================= */
/* Global variable definition    */
/* ============================= */
mps_comm_dev ifx_mps_dev;

/* ============================= */
/* Global function declaration   */
/* ============================= */
IFX_int32_t ifx_mps_bufman_buf_provide (IFX_int32_t segments,
                                        IFX_uint32_t segment_size);
IFX_int32_t ifx_mps_bufman_close (void);
IFX_void_t ifx_mps_mbx_data_upstream (IFX_ulong_t dummy);
IFX_void_t ifx_mps_mbx_cmd_upstream (IFX_ulong_t dummy);
static IFX_int32_t ifx_mps_mbx_write_buffer_prov_message (mem_seg_t * mem_ptr,
                                                          IFX_uint8_t segments,
                                                          IFX_uint32_t
                                                          segment_size);
IFX_int32_t ifx_mps_mbx_read_message (mps_fifo * fifo, MbxMsg_s * msg,
                                      IFX_uint32_t * bytes);

#ifdef CONFIG_MPS_EVENT_MBX
IFX_void_t ifx_mps_mbx_event_upstream (IFX_ulong_t dummy);

#endif /* CONFIG_MPS_EVENT_MBX */
IFX_void_t *ifx_mps_fastbuf_malloc (IFX_size_t size, IFX_int32_t priority);
IFX_void_t ifx_mps_fastbuf_free (const IFX_void_t * ptr);
IFX_int32_t ifx_mps_fastbuf_init (void);
IFX_int32_t ifx_mps_fastbuf_close (void);
IFX_uint32_t ifx_mps_reset_structures (mps_comm_dev * pDev);
IFX_boolean_t ifx_mps_ext_bufman (void);
extern IFX_uint32_t danube_get_cpu_ver (void);
extern mps_mbx_dev *ifx_mps_get_device (mps_devices type);

#if   (BSP_API_VERSION == 1)
extern IFX_void_t mask_and_ack_danube_irq (IFX_uint32_t irq_nr);
#elif (BSP_API_VERSION == 2)
extern IFX_void_t bsp_mask_and_ack_irq (IFX_uint32_t irq_nr);
#endif

extern void sys_hw_setup (void);

extern IFXOS_event_t fw_ready_evt;
/* callback function to free all data buffers currently used by voice FW */
IFX_void_t (*ifx_mps_bufman_freeall)(void) = IFX_NULL;
/* ============================= */
/* Local function declaration    */
/* ============================= */

/* ============================= */
/* Local variable definition     */
/* ============================= */
/* global structure that holds VCPU buffer management data */
mps_buf_mng_t mps_buffer = {
   .buf_level = 0,
   .buf_size = MPS_MEM_SEG_DATASIZE,
   .buf_threshold = MPS_BUFFER_THRESHOLD,
   .buf_initial = MPS_BUFFER_INITIAL,
   .buf_state = MPS_BUF_EMPTY,
   /* fast buffer manager */
   .malloc = &ifx_mps_fastbuf_malloc,
   .free = &ifx_mps_fastbuf_free,
   .init = &ifx_mps_fastbuf_init,
   .close = &ifx_mps_fastbuf_close,
};

mps_comm_dev *pMPSDev = &ifx_mps_dev;

#if CONFIG_MPS_HISTORY_SIZE > 0
#if CONFIG_MPS_HISTORY_SIZE > 512
#error "MPS history buffer > 512 words (2kB)"
#endif /* */
#define MPS_HISTORY_BUFFER_SIZE (CONFIG_MPS_HISTORY_SIZE)
IFX_int32_t ifx_mps_history_buffer_freeze = 0;
IFX_uint32_t ifx_mps_history_buffer[MPS_HISTORY_BUFFER_SIZE] = { 0 };
IFX_int32_t ifx_mps_history_buffer_words = 0;

#ifdef DEBUG
IFX_int32_t ifx_mps_history_buffer_words_total = 0;
#endif /* */

IFX_int32_t ifx_mps_history_buffer_overflowed = 0;

#endif /* CONFIG_MPS_HISTORY_SIZE > 0 */

atomic_t ifx_mps_write_blocked = ATOMIC_INIT (0);
atomic_t ifx_mps_dd_mbx_int_enabled = ATOMIC_INIT (0);

/******************************************************************************
 * Fast bufferpool
 ******************************************************************************/
#define FASTBUF_USED     0x00000001
#define FASTBUF_FW_OWNED 0x00000002
#define FASTBUF_CMD_OWNED 0x00000004
#define FASTBUF_EVENT_OWNED 0x00000008
#define FASTBUF_WRITE_OWNED 0x00000010
#define FASTBUF_BUFS     (MPS_BUFFER_INITIAL * 2)
#define FASTBUF_BUFSIZE  MPS_MEM_SEG_DATASIZE
IFX_uint32_t *fastbuf_ptr;
volatile IFX_uint32_t fastbuf_pool[FASTBUF_BUFS] = { 0 };
IFX_uint32_t fastbuf_index = 0;
IFX_uint32_t fastbuf_initialized = 0;
/* firmware image footer */
FW_image_ftr_t *pFW_img_data;

/* cache operations */
#if defined(CONFIG_MIPS) && !defined(CONFIG_MIPS_UNCACHED)
#define CACHE_LINE_SZ 32
IFX_boolean_t bDoCacheOps = IFX_FALSE;
static IFX_void_t ifx_mps_cache_wb_inv (IFX_ulong_t addr, IFX_uint32_t len);
#endif /*defined(CONFIG_MIPS) && !defined(CONFIG_MIPS_UNCACHED)*/

/* D-Channel event counters */
IFX_uint32_t dchan_evt_cnt[16], dchan_evt_served_cnt[16];

/* ============================= */
/* Local function definition     */
/* ============================= */
/**
 * External buffer management check
 * Checks for external buffer manager (e.g. lib_bufferpool).
 *
 * \param   none
 *
 * \return  IFX_TRUE    External buffer manager is used (e.g. lib_bufferpool)
 * \return  IFX_FALSE   MPS internal buffer manager is used (fastbuf)
 * \ingroup Internal
 */
IFX_boolean_t ifx_mps_ext_bufman ()
{
   return (((mps_buffer.malloc != &ifx_mps_fastbuf_malloc) ||
            (mps_buffer.free != &ifx_mps_fastbuf_free)) ? IFX_TRUE : IFX_FALSE);
}

/**
 * Buffer allocate
 * Allocates and returns a buffer from the buffer pool.
 *
 * \param   size        Size of requested buffer
 * \param   priority    Ignored, always atomic
 *
 * \return  ptr    Address of the allocated buffer
 * \return  NULL   No buffer available
 * \ingroup Internal
 */
IFX_void_t *ifx_mps_fastbuf_malloc (IFX_size_t size, IFX_int32_t priority)
{
   IFXOS_INTSTAT flags;
   IFX_uint32_t ptr;
   IFX_int32_t findex = fastbuf_index;

   if (fastbuf_initialized == 0)
   {
      TRACE (MPS, DBG_LEVEL_HIGH,
             ("%s() - error, fast buffer not initialised\n", __FUNCTION__));
      return IFX_NULL;
   }

   if (size > FASTBUF_BUFSIZE)
   {
      TRACE (MPS, DBG_LEVEL_HIGH,
             ("%s() - error, buffer too large\n", __FUNCTION__));
      return IFX_NULL;
   }

   IFXOS_LOCKINT (flags);

   do
   {
      if (findex == FASTBUF_BUFS)
         findex = 0;
      if (fastbuf_pool[findex] & FASTBUF_USED)
         continue;
      ptr = fastbuf_pool[findex];
      fastbuf_pool[findex] |= FASTBUF_USED;
      if ((priority == FASTBUF_FW_OWNED) || (priority == FASTBUF_CMD_OWNED) ||
          (priority == FASTBUF_EVENT_OWNED) ||
          (priority == FASTBUF_WRITE_OWNED))
         fastbuf_pool[findex] |= priority;
      fastbuf_index = findex;
      IFXOS_UNLOCKINT (flags);
      return (IFX_void_t *) ptr;
   } while (++findex != fastbuf_index);
   IFXOS_UNLOCKINT (flags);
   TRACE (MPS, DBG_LEVEL_HIGH,
          ("%s() - error, buffer pool empty\n", __FUNCTION__));

   return IFX_NULL;
}


/**
 * Buffer free
 * Returns a buffer to the buffer pool.
 *
 * \param   ptr    Address of the allocated buffer
 *
 * \return  none
 * \ingroup Internal
 */
IFX_void_t ifx_mps_fastbuf_free (const IFX_void_t * ptr)
{
   IFXOS_INTSTAT flags;
   IFX_int32_t findex = fastbuf_index;

   IFXOS_LOCKINT (flags);

   do
   {
      if (findex < 0)
         findex = FASTBUF_BUFS - 1;
      if ((fastbuf_pool[findex] & ~(FASTBUF_FW_OWNED | FASTBUF_CMD_OWNED |
                                     FASTBUF_EVENT_OWNED | FASTBUF_WRITE_OWNED))
          == ((IFX_uint32_t) ptr | FASTBUF_USED))
      {
         fastbuf_pool[findex] &= ~FASTBUF_USED;
         fastbuf_pool[findex] &=
            ~(FASTBUF_FW_OWNED | FASTBUF_CMD_OWNED | FASTBUF_EVENT_OWNED |
              FASTBUF_WRITE_OWNED);
         IFXOS_UNLOCKINT (flags);
         return;
      }
   } while (--findex != fastbuf_index);
   IFXOS_UNLOCKINT (flags);
   TRACE (MPS, DBG_LEVEL_HIGH,
          ("%s() - error, buffer not inside pool (0x%p)\n", __FUNCTION__, ptr));
}


/**
 * Create MPS fastbuf proc file output.
 * This function creates the output for the fastbuf proc file
 *
 * \param   buf      Buffer to write the string to
 * \return  len      Lenght of data in buffer
 * \ingroup Internal
 */
IFX_int32_t ifx_mps_fastbuf_get_proc (char *buf)
{
   IFX_int32_t findex = fastbuf_index;
   IFX_int32_t len;
   len = 0;

   if (fastbuf_initialized == 0)
   {
      len += sprintf (buf + len, "Fastbuf not initialized.\n");
      return len;
   }

   len += sprintf (buf + len, "   Buffer   Owner  \n");

   do
   {
      if (findex == FASTBUF_BUFS)
         findex = 0;
      len += sprintf (buf + len, "0x%08x ", fastbuf_pool[findex] & 0xfffffffc);
      if (fastbuf_pool[findex] & FASTBUF_USED)
      {
         len += sprintf (buf + len, " used - ");
         if (fastbuf_pool[findex] & FASTBUF_FW_OWNED)
            len += sprintf (buf + len, " FW\n");

         else if (fastbuf_pool[findex] &
                  FASTBUF_EVENT_OWNED)
            len += sprintf (buf + len, " Event\n");

         else if (fastbuf_pool[findex] &
                  FASTBUF_CMD_OWNED)
            len += sprintf (buf + len, " Command\n");

         else if (fastbuf_pool[findex] &
                  FASTBUF_WRITE_OWNED)
            len += sprintf (buf + len, " Write\n");

         else
            len += sprintf (buf + len, " Linux\n");
      }
      else
         len += sprintf (buf + len, " free\n");
   } while (++findex != fastbuf_index);
   return len;
}


/**
 * Bufferpool init
 * Initializes a buffer pool of size FASTBUF_BUFSIZE * FASTBUF_BUFS and
 * separates it into FASTBUF_BUFS chunks. The 32byte alignment of the chunks
 * is guaranteed by increasing the buffer size accordingly. The pointer to
 * the pool is stored in fastbuf_ptr, while the pointers to the singles chunks
 * are maintained in fastbuf_pool.
 * Bit 0 of the address in fastbuf_pool is used as busy indicator.
 *
 * \return -ENOMEM  Memory allocation failed
 * \return  IFX_SUCCESS      Buffer pool initialized
 * \ingroup Internal
 */
IFX_int32_t ifx_mps_fastbuf_init (void)
{
   IFX_uint32_t *ptr, i;
   IFX_uint32_t bufsize = (FASTBUF_BUFSIZE + (FASTBUF_BUFSIZE % 32));

   if ((fastbuf_ptr = IFXOS_BlockAlloc (FASTBUF_BUFS * bufsize)) == IFX_NULL)
      return -ENOMEM;
   ptr = fastbuf_ptr;
   for (i = 0; i < FASTBUF_BUFS; i++)
   {
      fastbuf_pool[i] = (IFX_uint32_t) ptr;
      ptr = (IFX_uint32_t *) ((IFX_uint32_t) ptr + bufsize);
   }
   fastbuf_index = 0;
   fastbuf_initialized = 1;
   return IFX_SUCCESS;
}


/**
 * Bufferpool close
 * Frees the buffer pool allocated by ifx_mps_fastbuf_init and clears the
 * buffer pool.
 *
 * \return -ENOMEM  Memory allocation failed
 * \return  IFX_SUCCESS      Buffer pool initialized
 * \ingroup Internal
 */
IFX_return_t ifx_mps_fastbuf_close (void)
{
   IFX_int32_t i;

   if (fastbuf_initialized)
   {
      for (i = 0; i < FASTBUF_BUFS; i++)
         fastbuf_pool[i] = 0;
      IFXOS_BlockFree (fastbuf_ptr);
      fastbuf_initialized = 0;
   }
   return IFX_SUCCESS;
}


/******************************************************************************
 * Buffer manager
 ******************************************************************************/

/**
 * Get buffer fill level
 * This function return the current number of buffers provided to CPU1
 *
 * \return  level    The current number of buffers
 * \return  -1       The buffer state indicates an error
 * \ingroup Internal
 */
static IFX_int32_t ifx_mps_bufman_get_level (void)
{
   if (mps_buffer.buf_state != MPS_BUF_ERR)
      return mps_buffer.buf_level;
   return -1;
}


/**
 * Update buffer state
 * This function will set the buffer state according to the current buffer level
 * and the previous state.
 *
 * \return  state    The new buffer state
 * \ingroup Internal
 */
static mps_buffer_state_e ifx_mps_bufman_update_state (void)
{
   if (mps_buffer.buf_state != MPS_BUF_ERR)
   {
      if (mps_buffer.buf_level == 0)
         mps_buffer.buf_state = MPS_BUF_EMPTY;
      if ((mps_buffer.buf_level > 0) &&
          (mps_buffer.buf_level < mps_buffer.buf_threshold))
         mps_buffer.buf_state = MPS_BUF_LOW;
      if ((mps_buffer.buf_level >= mps_buffer.buf_threshold) &&
          (mps_buffer.buf_level <= MPS_BUFFER_MAX_LEVEL))
         mps_buffer.buf_state = MPS_BUF_OK;
      if (mps_buffer.buf_level > MPS_BUFFER_MAX_LEVEL)
         mps_buffer.buf_state = MPS_BUF_OV;
   }
   return mps_buffer.buf_state;
}


/**
 * Increase buffer level
 * This function increments the buffer level by the passed value.
 *
 * \param   value    Increment value
 * \return  level    The new buffer level
 * \return  -1       Maximum value reached
 * \ingroup Internal
 */
static IFX_int32_t ifx_mps_bufman_inc_level (IFX_uint32_t value)
{
   IFXOS_INTSTAT flags;

   if (mps_buffer.buf_level + value > MPS_BUFFER_MAX_LEVEL)
   {
      TRACE (MPS, DBG_LEVEL_HIGH,
             ("ifx_mps_bufman_inc_level(): Maximum reached !\n"));
      return -1;
   }
   IFXOS_LOCKINT (flags);
   mps_buffer.buf_level += value;
   ifx_mps_bufman_update_state ();
   IFXOS_UNLOCKINT (flags);
   return mps_buffer.buf_level;
}


/**
 * Decrease buffer level
 * This function decrements the buffer level with the passed value.
 *
 * \param   value    Decrement value
 * \return  level    The new buffer level
 * \return  -1       Minimum value reached
 * \ingroup Internal
 */
static IFX_int32_t ifx_mps_bufman_dec_level (IFX_uint32_t value)
{
   IFXOS_INTSTAT flags;

   if (mps_buffer.buf_level < value)
   {
      TRACE (MPS, DBG_LEVEL_HIGH,
             ("ifx_mps_bufman_dec_level(): Minimum reached !\n"));
      return -1;
   }
   IFXOS_LOCKINT (flags);
   mps_buffer.buf_level -= value;
   ifx_mps_bufman_update_state ();
   IFXOS_UNLOCKINT (flags);
   return mps_buffer.buf_level;
}


/**
 * Init buffer management
 * This function initializes the buffer management data structures and
 * provides buffer segments to CPU1.
 *
 * \return  0        IFX_SUCCESS, initialized and message sent
 * \return  -1       Error during message transmission
 * \ingroup Internal
 */
IFX_int32_t ifx_mps_bufman_init (void)
{
   IFX_int32_t i;
   IFX_int32_t ret = IFX_ERROR;

   /* Initialize MPS fastbuf pool only in case of MPS internal buffer
      management. Initialization of MPS fastbuf pool is not required in case of
      external buffer pool management (e.g. lib_bufferpool). */
   if (IFX_FALSE == ifx_mps_ext_bufman ())
   {
      mps_buffer.init ();
   }

   for (i = 0; i < mps_buffer.buf_initial;
        i += MPS_DEFAULT_PROVISION_SEGMENTS_PER_MSG)
   {
      ret =
         ifx_mps_bufman_buf_provide (MPS_DEFAULT_PROVISION_SEGMENTS_PER_MSG,
                                     mps_buffer.buf_size);
   }
   return ret;
}


/**
 * Close buffer management
 * This function is called on termination of voice CPU firmware. The registered
 * close function has to take care of freeing buffers still left in VCPU.
 *
 * \return  0        IFX_SUCCESS, buffer manage shutdown correctly
 * \return  -1       Error during shutdown
 * \ingroup Internal
 */
IFX_int32_t ifx_mps_bufman_close (void)
{
   IFX_int32_t ret = IFX_ERROR;

   mps_buffer.close ();
   mps_buffer.buf_level = 0;
   mps_buffer.buf_size = MPS_MEM_SEG_DATASIZE;
   mps_buffer.buf_threshold = MPS_BUFFER_THRESHOLD;
   mps_buffer.buf_initial = MPS_BUFFER_INITIAL;
   mps_buffer.buf_state = MPS_BUF_EMPTY;
   return ret;
}


/**
 * Free buffer
 *
 * \ingroup Internal
 */
IFX_void_t ifx_mps_bufman_free (const IFX_void_t * ptr)
{
   mps_buffer.free ((IFX_void_t *) KSEG0ADDR (ptr));
}

/**
 * Allocate buffer
 *
 * \ingroup Internal
 */
IFX_void_t *ifx_mps_bufman_malloc (IFX_size_t size, IFX_int32_t priority)
{
   IFX_void_t *ptr;

   ptr = mps_buffer.malloc (size, priority);
   return ptr;
}


/**
 * Overwrite buffer management
 * Allows the upper layer to register its own malloc/free functions in order to do
 * its own buffer managment. To unregister driver needs to be re-initialized.
 *
 * \param   malloc      Buffer allocation - arguments and return value as kmalloc
 * \param   free        Buffer de-allocation - arguments and return value as kmalloc
 * \param   buf_size    Size of buffers provided to voice CPU
 * \param   treshold    Count of buffers provided to voice CPU
 */
IFX_void_t ifx_mps_bufman_register (IFX_void_t *
                                    (*malloc) (IFX_size_t size,
                                               IFX_int32_t priority),
                                    IFX_void_t (*free) (const IFX_void_t * ptr),
                                    IFX_uint32_t buf_size,
                                    IFX_uint32_t treshold)
{
   mps_buffer.buf_size = buf_size;
   mps_buffer.buf_threshold = treshold;
   mps_buffer.buf_initial = treshold + MPS_DEFAULT_PROVISION_SEGMENTS_PER_MSG;
   mps_buffer.malloc = malloc;
   mps_buffer.free = free;
}

/**
 * Send buffer provisioning message
 * This function sends a buffer provisioning message to CPU1 using the passed
 * segment parameters.
 *
 * \param   segments     Number of memory segments to be provided to CPU1
 * \param   segment_size Size of each memory segment in bytes
 * \return  0            IFX_SUCCESS, message sent
 * \return  -1           IFX_ERROR, if message could not be sent
 * \ingroup Internal
 */
IFX_int32_t ifx_mps_bufman_buf_provide (IFX_int32_t segments,
                                        IFX_uint32_t segment_size)
{
   IFX_int32_t i;
   IFX_int32_t mem_avail = 0;
   static mem_seg_t mem_seg_ptr;
   IFX_uint8_t seg_allocated = 0;

   /* Check available mailbox memory and adjust number of segments,
      if necessary */
   mem_avail = ifx_mps_fifo_mem_available (&pMPSDev->voice_dwstrm_fifo);
   if (mem_avail < ((segments + 2) * sizeof(IFX_uint32_t)))
   {
      /* available mailbox space is below requested */
      segments = (mem_avail >= (3 * sizeof(IFX_uint32_t))) ?
                  (mem_avail - 2 * sizeof(IFX_uint32_t)) >> 2 : 0;
   }

   if (segments)
   {
      memset (&mem_seg_ptr, 0, sizeof (mem_seg_t));
      for (i = 0; i < segments; i++)
      {
         mem_seg_ptr[i] = (IFX_uint32_t *) CPHYSADDR(
             (IFX_uint32_t) mps_buffer.malloc(segment_size, FASTBUF_FW_OWNED));
         if (mem_seg_ptr[i] == (IFX_uint32_t *)CPHYSADDR(IFX_NULL))
         {
            TRACE (MPS, DBG_LEVEL_HIGH,
                ("%s(): cannot allocate buffer\n", __FUNCTION__));
            break;
         }
         seg_allocated++;
         /* invalidate cache */
         ifx_mps_cache_inv (KSEG0ADDR(mem_seg_ptr[i]), segment_size);
      }

      if (seg_allocated)
      {
         if (ifx_mps_mbx_write_buffer_prov_message
             (&mem_seg_ptr, seg_allocated, segment_size) != IFX_SUCCESS)
         {
            atomic_inc (&ifx_mps_write_blocked);

            /* enable data downstream mailbox interrupt */
            ifx_mps_dd_mbx_int_enable ();
            /* free the segments */
            for (i = 0; i < seg_allocated; i++)
               mps_buffer.free ((IFX_void_t *) KSEG0ADDR (mem_seg_ptr[i]));
         }
         else
         {
            /* disable data downstream mailbox interrupt */
            ifx_mps_dd_mbx_int_disable ();
            atomic_set (&ifx_mps_write_blocked, 0);
            return IFX_SUCCESS;
         }
      }
   }

   return IFX_ERROR;
}


/******************************************************************************
 * FIFO Managment
 ******************************************************************************/

/**
 * Clear FIFO
 * This function clears the FIFO by resetting the pointers. The data itself is
 * not cleared.
 *
 * \param   fifo    Pointer to FIFO structure
 * \ingroup Internal
 */
IFX_void_t ifx_mps_fifo_clear (mps_fifo * fifo)
{
   *fifo->pread_off = fifo->size - 4;
   *fifo->pwrite_off = fifo->size - 4;
   return;
}


/**
 * Check FIFO for being not empty
 * This function checks whether the referenced FIFO contains at least
 * one unread data byte.
 *
 * \param   fifo     Pointer to FIFO structure
 * \return  1        TRUE if data to be read is available in FIFO,
 * \return  0        FALSE if FIFO is empty.
 * \ingroup Internal
 */
IFX_boolean_t ifx_mps_fifo_not_empty (mps_fifo * fifo)
{
   if (*fifo->pwrite_off == *fifo->pread_off)
      return IFX_FALSE;

   else
      return IFX_TRUE;
}


/**
 * Check FIFO for free memory
 * This function returns the amount of free bytes in FIFO.
 *
 * \param   fifo     Pointer to FIFO structure
 * \return  0        The FIFO is full,
 * \return  count    The number of available bytes
 * \ingroup Internal
 */
IFX_uint32_t ifx_mps_fifo_mem_available (mps_fifo * fifo)
{
   IFX_uint32_t retval;

   retval =
      (fifo->size - 1 - (*fifo->pread_off - *fifo->pwrite_off)) & (fifo->size -
                                                                   1);
   return (retval);
}


/**
 * Check FIFO for requested amount of memory
 * This function checks whether the requested FIFO is capable to store
 * the requested amount of data bytes.
 * The selected Fifo should be a downstream direction Fifo.
 *
 * \param   fifo     Pointer to mailbox structure to be checked
 * \param   bytes    Requested data bytes
 * \return  1        TRUE if space is available in FIFO,
 * \return  0        FALSE if not enough space in FIFO.
 * \ingroup Internal
 */
IFX_boolean_t ifx_mps_fifo_mem_request (mps_fifo * fifo, IFX_uint32_t bytes)
{
   IFX_uint32_t bytes_avail = ifx_mps_fifo_mem_available (fifo);

   if (bytes_avail > bytes)
   {
      return IFX_TRUE;
   }
   else
   {
      return IFX_FALSE;
   }
}


/**
 * Update FIFO read pointer
 * This function updates the position of the referenced FIFO.In case of
 * reaching the FIFO's end the pointer is set to the start position.
 *
 * \param   fifo      Pointer to FIFO structure
 * \param   increment Increment for read index
 * \ingroup Internal
 */
IFX_void_t ifx_mps_fifo_read_ptr_inc (mps_fifo * fifo, IFX_uint8_t increment)
{
   IFX_int32_t new_read_index =
      (IFX_int32_t) (*fifo->pread_off) - (IFX_int32_t) increment;

   if ((IFX_uint32_t) increment > fifo->size)
   {
      TRACE (MPS, DBG_LEVEL_HIGH,
             ("%s(): Invalid offset passed: %d !\n", __FUNCTION__, increment));
      return;
   }

   if (new_read_index >= 0)
   {
      *(fifo->pread_off) = (IFX_uint32_t) new_read_index;
   }
   else
   {
      *(fifo->pread_off) = (IFX_uint32_t) (new_read_index + (IFX_int32_t) (fifo->size));        /* overflow */
   }

   return;
}


/**
 * Update FIFO write pointer
 * This function updates the position of the write pointer of the referenced FIFO.
 * In case of reaching the FIFO's end the pointer is set to the start position.
 *
 * \param   fifo      Pointer to FIFO structure
 * \param   increment Increment of write index
 * \ingroup Internal
 */
IFX_void_t ifx_mps_fifo_write_ptr_inc (mps_fifo * fifo, u16 increment)
{
   /* calculate new index ignoring ring buffer overflow */
   IFX_int32_t new_write_index =
      (IFX_int32_t) (*fifo->pwrite_off) - (IFX_int32_t) increment;

   if ((IFX_uint32_t) increment > fifo->size)
   {
      TRACE (MPS, DBG_LEVEL_HIGH,
             ("%s(): Invalid offset passed: %d !\n", __FUNCTION__, increment));
      return;
   }

   if (new_write_index >= 0)
   {
      *fifo->pwrite_off = (IFX_uint32_t) new_write_index;       /* no overflow */
   }
   else
   {
      *fifo->pwrite_off =
         (IFX_uint32_t) (new_write_index + (IFX_int32_t) (fifo->size));
   }
   return;
}


/**
 * Write data word to FIFO
 * This function writes a data word (32bit) to the referenced FIFO. The word is
 * written to the position defined by the current write pointer index and the
 * offset being passed.
 *
 * \param   fifo           Pointer to FIFO structure
 * \param   data           Data word to be written
 * \param   offset         Byte offset to be added to write pointer position
 * \return  0              IFX_SUCCESS, word written
 * \return  -1             Invalid offset.
 * \ingroup Internal
 */
IFX_int32_t ifx_mps_fifo_write (mps_fifo * fifo, IFX_uint32_t data,
                                IFX_uint8_t offset)
{
   /* calculate write position */
   IFX_int32_t new_write_index =
      (IFX_int32_t) * fifo->pwrite_off - (IFX_int32_t) offset;
   IFX_uint32_t write_address;

   if (offset > fifo->size)
   {
      TRACE (MPS, DBG_LEVEL_HIGH,
             ("%s(): Invalid offset passed !\n", __FUNCTION__));
      return -1;
   }
   write_address =
      (IFX_uint32_t) fifo->pend + *fifo->pwrite_off - (IFX_uint32_t) offset;
   if (new_write_index < 0)
   {
      write_address += fifo->size;
   }
   *(IFX_uint32_t *) write_address = data;
   return 0;
}


#ifndef MODULE
/* extern IFX_void_t show_trace(long *sp); */
#endif /* */

/**
 * Read data word from FIFO
 * This function reads a data word (32bit) from the referenced FIFO. It first
 * calculates and checks the address defined by the FIFO's read index and passed
 * offset. The read pointer is not updated by this function.
 * It has to be updated after the complete message has been read.
 *
 * \param   fifo          Pointer to FIFO structure
 * \param   offset        Offset to read pointer position to be read from
 * \return  count         Number of data words read.
 * \return  -1            Invalid offset
 * \ingroup Internal
 */
IFX_int32_t ifx_mps_fifo_read (mps_fifo * fifo, IFX_uint8_t offset,
                               IFX_uint32_t * pData)
{
   IFX_uint32_t read_address;
   IFX_int32_t new_read_index =
      ((IFX_int32_t) * fifo->pread_off) - (IFX_int32_t) offset;
   IFX_int32_t ret;

   if (!ifx_mps_fifo_not_empty (fifo))
   {
#ifndef MODULE
/*         long *sp; */
#endif /* */
      TRACE (MPS, DBG_LEVEL_HIGH,
             ("%s(): FIFO %p is empty\n", __FUNCTION__, fifo));
#ifndef MODULE
/*
   __asm__("move %0, $29;":"=r"(sp));
   show_trace(sp);
*/
#endif /* */
      ret = IFX_ERROR;
   }
   else
   {
      if (offset > fifo->size)
      {
         TRACE (MPS, DBG_LEVEL_HIGH,
                ("%s(): Invalid offset passed: %d !\n", __FUNCTION__, offset));
         return -1;
      }
      read_address =
         (IFX_uint32_t) fifo->pend + (IFX_uint32_t) * fifo->pread_off -
         (IFX_uint32_t) offset;
      if (new_read_index < 0)
      {
         read_address += fifo->size;
      }
      *pData = *(IFX_uint32_t *) read_address;
      ret = IFX_SUCCESS;
   }
   return (ret);
}


/******************************************************************************
 * Global Routines
 ******************************************************************************/

/**
 * Open MPS device
 * Open routine for the MPS device driver.
 *
 * \param   mps_device  MPS communication device structure
 * \param   pMBDev      Pointer to mailbox device structure
 * \param   bcommand    voice/command selector, 1 -> command mailbox,
 *                      2 -> voice, 3 -> event mailbox
 * \return  0           IFX_SUCCESS, successfully opened
 * \return  -1          IFX_ERROR, Driver already installed
 * \ingroup Internal
 */
IFX_int32_t ifx_mps_common_open (mps_comm_dev * mps_device,
                                 mps_mbx_dev * pMBDev, IFX_int32_t bcommand,
                                 IFX_boolean_t from_kernel)
{
   IFXOS_INTSTAT flags;

   IFXOS_LOCKINT (flags);

   /* device is already installed or unknown device ID used */
   if ((pMBDev->Installed == IFX_TRUE) || (pMBDev->devID == unknown))
   {
      IFXOS_UNLOCKINT (flags);
      return (IFX_ERROR);
   }
   pMBDev->Installed = IFX_TRUE;
   IFXOS_UNLOCKINT (flags);
   if (bcommand == 2)           /* voice */
   {
      if (from_kernel)
      {
         pMBDev->upstrm_fifo = &mps_device->voice_upstrm_fifo;
         pMBDev->dwstrm_fifo = &mps_device->voice_dwstrm_fifo;
      }
      else
      {
         pMBDev->upstrm_fifo = &mps_device->sw_upstrm_fifo[pMBDev->devID - 1];
         pMBDev->dwstrm_fifo = &mps_device->voice_dwstrm_fifo;
      }
   }

#ifdef CONFIG_MPS_EVENT_MBX
   else if (bcommand == 3)      /* event mailbox */
   {
      if (from_kernel)
      {
         pMBDev->upstrm_fifo = &mps_device->event_upstrm_fifo;
         pMBDev->dwstrm_fifo = IFX_NULL;
      }
      else
      {
         pMBDev->upstrm_fifo = &mps_device->sw_event_upstrm_fifo;
         pMBDev->dwstrm_fifo = IFX_NULL;
      }
   }
#endif /* CONFIG_MPS_EVENT_MBX */
   return (IFX_SUCCESS);
}


/**
 * Close routine for MPS device driver
 * This function closes the channel assigned to the passed mailbox
 * device structure.
 *
 * \param   pMBDev   Pointer to mailbox device structure
 * \return  0        IFX_SUCCESS, will never fail
 * \ingroup Internal
 */
IFX_int32_t ifx_mps_common_close (mps_mbx_dev * pMBDev,
                                  IFX_boolean_t from_kernel)
{
   IFX_MPS_UNUSED(from_kernel);

   /* clean data structures */
   if (pMBDev->Installed == IFX_FALSE)
   {
      return (IFX_ERROR);
   }
   pMBDev->Installed = IFX_FALSE;

   /* Clear the downstream queues for voice fds only */
#ifdef CONFIG_MPS_EVENT_MBX
   if ((pMBDev->devID != command) && (pMBDev->devID != event_mbx))
#else /* */
   if (pMBDev->devID != command)
#endif /* CONFIG_MPS_EVENT_MBX */
   {
#ifdef CONFIG_PROC_FS
      pMBDev->upstrm_fifo->min_space = MBX_DATA_UPSTRM_FIFO_SIZE;
      pMBDev->dwstrm_fifo->min_space = MBX_DATA_DNSTRM_FIFO_SIZE;
#endif /* */
      /* clean-up messages left in software fifo... */
      while (ifx_mps_fifo_not_empty (pMBDev->upstrm_fifo))
      {
         IFX_uint32_t bytes_read;
         MbxMsg_s msg;
         ifx_mps_mbx_read_message (pMBDev->upstrm_fifo, &msg, &bytes_read);
         ifx_mps_bufman_free ((IFX_void_t *)
                              KSEG0ADDR ((IFX_uint8_t *) msg.data[0]));
         pMBDev->upstrm_fifo->discards++;
      }
      /* reset software fifo... */
      *pMBDev->upstrm_fifo->pwrite_off = (pMBDev->upstrm_fifo->size - 4);
      *pMBDev->upstrm_fifo->pread_off = (pMBDev->upstrm_fifo->size - 4);
   }
   else
   {
#ifdef CONFIG_MPS_EVENT_MBX
      if (pMBDev->devID != event_mbx)
#endif /* CONFIG_MPS_EVENT_MBX */
      {
#ifdef CONFIG_PROC_FS
         pMBDev->upstrm_fifo->min_space = MBX_CMD_FIFO_SIZE;
         pMBDev->dwstrm_fifo->min_space = MBX_CMD_FIFO_SIZE;
#endif /* */
      }
   }
   return (IFX_SUCCESS);
}


/**
 * MPS Structure Release
 * This function releases the entire MPS data structure used for communication
 * between the CPUs.
 *
 * \param   pDev     Poiter to MPS communication structure
 * \ingroup Internal
 */
IFX_void_t ifx_mps_release_structures (mps_comm_dev * pDev)
{
   IFX_int32_t count;
   IFXOS_INTSTAT flags;

   IFXOS_LOCKINT (flags);
   IFXOS_BlockFree (pFW_img_data);
   IFXOS_BlockFree (pDev->command_mb.sem_dev);
#ifdef CONFIG_MPS_EVENT_MBX
   IFXOS_BlockFree ((IFX_void_t *) pDev->sw_event_upstrm_fifo.pend);
   IFXOS_BlockFree ((IFX_void_t *) pDev->event_mbx.sem_dev);
#endif /* CONFIG_MPS_EVENT_MBX */
   /* Initialize the Message queues for the voice packets */
   for (count = 0; count < NUM_VOICE_CHANNEL; count++)
   {
      IFXOS_BlockFree (pDev->voice_mb[count].sem_dev);
      IFXOS_BlockFree (pDev->voice_mb[count].sem_read_fifo);
#if 0
      This will crash.
         Please verify if this is allocated through kmalloc ! !!if (pDev->
                                                                    voice_mb
                                                                    [count].
                                                                    upstrm_fifo->
                                                                    pend)
              kfree ((IFX_void_t *) pDev->voice_mb[count].upstrm_fifo->pend);

#endif /* */
#ifdef MPS_FIFO_BLOCKING_WRITE
      IFXOS_BlockFree (pDev->voice_mb[count].sem_write_fifo);
#endif /* MPS_FIFO_BLOCKING_WRITE */
   }
   IFXOS_BlockFree (pDev->provide_buffer);
   IFXOS_UNLOCKINT (flags);
}

/**
 * MPS Structure Initialization
 * This function initializes the data structures of the Multi Processor System
 * that are necessary for inter processor communication
 *
 * \param   pDev     Pointer to MPS device structure to be initialized
 * \return  0        IFX_SUCCESS, if initialization was successful
 * \return  -1       IFX_ERROR, allocation or semaphore access problem
 * \ingroup Internal
 */
IFX_int32_t ifx_mps_init_structures (mps_comm_dev * pDev)
{
   mps_mbx_reg *MBX_Memory;
   IFX_int32_t i;

   /* Initialize MPS main structure */
   memset ((IFX_void_t *) pDev, 0, sizeof (mps_comm_dev));
#if !defined(SYSTEM_FALCON)
   pDev->base_global = (mps_mbx_reg *) IFX_MPS_SRAM;
#else
   pDev->base_global = (mps_mbx_reg *) IFXMIPS_MPS_SRAM;
#endif /* SYSTEM_FALCON */
   pDev->flags = 0x00000000;
   MBX_Memory = pDev->base_global;

   /* * Initialize common mailbox definition area which is used by both CPUs
      for MBX communication. These are: mailbox base address, mailbox size, *
      mailbox read index and mailbox write index. for command and voice
      mailbox, * upstream and downstream direction. */
#if !defined(SYSTEM_FALCON)
   memset ( /* avoid to overwrite CPU boot registers */
            (IFX_void_t *) MBX_Memory,
            0,
            sizeof (mps_mbx_reg) - 2 * sizeof (mps_boot_cfg_reg));
#else
   memset ( /* avoid to overwrite CPU boot registers */
            (IFX_void_t *) MBX_Memory + 2 * sizeof (mps_boot_cfg_reg),
            0,
            sizeof (mps_mbx_reg) - 2 * sizeof (mps_boot_cfg_reg));
#endif
   MBX_Memory->MBX_UPSTR_CMD_BASE =
      (IFX_uint32_t *) CPHYSADDR ((IFX_uint32_t) MBX_UPSTRM_CMD_FIFO_BASE);
   MBX_Memory->MBX_UPSTR_CMD_SIZE = MBX_CMD_FIFO_SIZE;
   MBX_Memory->MBX_DNSTR_CMD_BASE =
      (IFX_uint32_t *) CPHYSADDR ((IFX_uint32_t) MBX_DNSTRM_CMD_FIFO_BASE);
   MBX_Memory->MBX_DNSTR_CMD_SIZE = MBX_CMD_FIFO_SIZE;
   MBX_Memory->MBX_UPSTR_DATA_BASE =
      (IFX_uint32_t *) CPHYSADDR ((IFX_uint32_t) MBX_UPSTRM_DATA_FIFO_BASE);
   MBX_Memory->MBX_UPSTR_DATA_SIZE = MBX_DATA_UPSTRM_FIFO_SIZE;
   MBX_Memory->MBX_DNSTR_DATA_BASE =
      (IFX_uint32_t *) CPHYSADDR ((IFX_uint32_t) MBX_DNSTRM_DATA_FIFO_BASE);
   MBX_Memory->MBX_DNSTR_DATA_SIZE = MBX_DATA_DNSTRM_FIFO_SIZE;

   /* set read and write pointers below to the FIFO's uppermost address */
   MBX_Memory->MBX_UPSTR_CMD_READ = (MBX_Memory->MBX_UPSTR_CMD_SIZE - 4);
   MBX_Memory->MBX_UPSTR_CMD_WRITE = (MBX_Memory->MBX_UPSTR_CMD_READ);
   MBX_Memory->MBX_DNSTR_CMD_READ = (MBX_Memory->MBX_DNSTR_CMD_SIZE - 4);
   MBX_Memory->MBX_DNSTR_CMD_WRITE = MBX_Memory->MBX_DNSTR_CMD_READ;
   MBX_Memory->MBX_UPSTR_DATA_READ = (MBX_Memory->MBX_UPSTR_DATA_SIZE - 4);
   MBX_Memory->MBX_UPSTR_DATA_WRITE = MBX_Memory->MBX_UPSTR_DATA_READ;
   MBX_Memory->MBX_DNSTR_DATA_READ = (MBX_Memory->MBX_DNSTR_DATA_SIZE - 4);
   MBX_Memory->MBX_DNSTR_DATA_WRITE = MBX_Memory->MBX_DNSTR_DATA_READ;

#ifdef CONFIG_MPS_EVENT_MBX
   MBX_Memory->MBX_UPSTR_EVENT_BASE =
      (IFX_uint32_t *) CPHYSADDR ((IFX_uint32_t) MBX_UPSTRM_EVENT_FIFO_BASE);
   MBX_Memory->MBX_UPSTR_EVENT_SIZE = MBX_EVENT_FIFO_SIZE;
   MBX_Memory->MBX_UPSTR_EVENT_READ = (MBX_Memory->MBX_UPSTR_EVENT_SIZE - 4);
   MBX_Memory->MBX_UPSTR_EVENT_WRITE = MBX_Memory->MBX_UPSTR_EVENT_READ;
#endif /* CONFIG_MPS_EVENT_MBX */

   /* * Configure command mailbox sub structure pointers to global mailbox
      register addresses */
   /* * set command mailbox sub structure pointers to global mailbox register
      addresses */
   pDev->cmd_upstrm_fifo.pstart =
      (IFX_uint32_t *)
      KSEG1ADDR ((MBX_Memory->MBX_UPSTR_CMD_BASE +
                  ((MBX_Memory->MBX_UPSTR_CMD_SIZE - 4) >> 2)));
   pDev->cmd_upstrm_fifo.pend =
      (IFX_uint32_t *) KSEG1ADDR (MBX_Memory->MBX_UPSTR_CMD_BASE);
   pDev->cmd_upstrm_fifo.pwrite_off =
      (IFX_uint32_t *) & (MBX_Memory->MBX_UPSTR_CMD_WRITE);
   pDev->cmd_upstrm_fifo.pread_off =
      (IFX_uint32_t *) & (MBX_Memory->MBX_UPSTR_CMD_READ);
   pDev->cmd_upstrm_fifo.size = MBX_Memory->MBX_UPSTR_CMD_SIZE;

#ifdef CONFIG_PROC_FS
   pDev->cmd_upstrm_fifo.min_space = MBX_Memory->MBX_UPSTR_CMD_SIZE;
#endif /* */
   pDev->cmd_dwstrm_fifo.pstart =
      (IFX_uint32_t *)
      KSEG1ADDR ((MBX_Memory->MBX_DNSTR_CMD_BASE +
                  ((MBX_Memory->MBX_DNSTR_CMD_SIZE - 4) >> 2)));
   pDev->cmd_dwstrm_fifo.pend =
      (IFX_uint32_t *) KSEG1ADDR (MBX_Memory->MBX_DNSTR_CMD_BASE);
   pDev->cmd_dwstrm_fifo.pwrite_off =
      (IFX_uint32_t *) & (MBX_Memory->MBX_DNSTR_CMD_WRITE);
   pDev->cmd_dwstrm_fifo.pread_off =
      (IFX_uint32_t *) & (MBX_Memory->MBX_DNSTR_CMD_READ);
   pDev->cmd_dwstrm_fifo.size = MBX_Memory->MBX_DNSTR_CMD_SIZE;

#ifdef CONFIG_PROC_FS
   pDev->cmd_dwstrm_fifo.min_space = MBX_Memory->MBX_DNSTR_CMD_SIZE;
#endif /* */
   pDev->command_mb.dwstrm_fifo = &pDev->cmd_dwstrm_fifo;
   pDev->command_mb.upstrm_fifo = &pDev->cmd_upstrm_fifo;
   pDev->command_mb.pVCPU_DEV = pDev;   /* global pointer reference */
   pDev->command_mb.Installed = IFX_FALSE;      /* current installation status */

#ifdef CONFIG_MPS_EVENT_MBX
   pDev->event_upstrm_fifo.pstart =
      (IFX_uint32_t *)
      KSEG1ADDR ((MBX_Memory->MBX_UPSTR_EVENT_BASE +
                  ((MBX_Memory->MBX_UPSTR_EVENT_SIZE - 4) >> 2)));
   pDev->event_upstrm_fifo.pend =
      (IFX_uint32_t *) KSEG1ADDR (MBX_Memory->MBX_UPSTR_EVENT_BASE);
   pDev->event_upstrm_fifo.pwrite_off =
      (IFX_uint32_t *) & (MBX_Memory->MBX_UPSTR_EVENT_WRITE);
   pDev->event_upstrm_fifo.pread_off =
      (IFX_uint32_t *) & (MBX_Memory->MBX_UPSTR_EVENT_READ);
   pDev->event_upstrm_fifo.size = MBX_Memory->MBX_UPSTR_EVENT_SIZE;
#ifdef TEST_EVT_DISCARD
#define DEEP_EVT_SW_FIFO_SIZE 512
   pDev->sw_event_upstrm_fifo.pend =
      IFXOS_BlockAlloc (DEEP_EVT_SW_FIFO_SIZE + 8);
   pDev->sw_event_upstrm_fifo.pstart =
      (pDev->sw_event_upstrm_fifo.pend + ((DEEP_EVT_SW_FIFO_SIZE - 4) >> 2));
   pDev->sw_event_upstrm_fifo.pwrite_off =
      (pDev->sw_event_upstrm_fifo.pend + ((DEEP_EVT_SW_FIFO_SIZE) >> 2));
   *pDev->sw_event_upstrm_fifo.pwrite_off = (DEEP_EVT_SW_FIFO_SIZE - 4);
   pDev->sw_event_upstrm_fifo.pread_off =
      (pDev->sw_event_upstrm_fifo.pend + ((DEEP_EVT_SW_FIFO_SIZE + 4) >> 2));
   *pDev->sw_event_upstrm_fifo.pread_off = (DEEP_EVT_SW_FIFO_SIZE - 4);
   pDev->sw_event_upstrm_fifo.size = DEEP_EVT_SW_FIFO_SIZE;
#else
   pDev->sw_event_upstrm_fifo.pend =
      IFXOS_BlockAlloc (MBX_Memory->MBX_UPSTR_EVENT_SIZE + 8);
   pDev->sw_event_upstrm_fifo.pstart =
      (pDev->sw_event_upstrm_fifo.pend +
       ((MBX_Memory->MBX_UPSTR_EVENT_SIZE - 4) >> 2));
   pDev->sw_event_upstrm_fifo.pwrite_off =
      (pDev->sw_event_upstrm_fifo.pend +
       ((MBX_Memory->MBX_UPSTR_EVENT_SIZE) >> 2));
   *pDev->sw_event_upstrm_fifo.pwrite_off =
      (MBX_Memory->MBX_UPSTR_EVENT_SIZE - 4);
   pDev->sw_event_upstrm_fifo.pread_off =
      (pDev->sw_event_upstrm_fifo.pend +
       ((MBX_Memory->MBX_UPSTR_EVENT_SIZE + 4) >> 2));
   *pDev->sw_event_upstrm_fifo.pread_off =
      (MBX_Memory->MBX_UPSTR_EVENT_SIZE - 4);
   pDev->sw_event_upstrm_fifo.size = MBX_Memory->MBX_UPSTR_EVENT_SIZE;
#endif /* TEST_EVT_DISCARD */

   /* initialize the semaphores for multitasking access */
   pDev->event_mbx.sem_dev = IFXOS_BlockAlloc (sizeof (IFXOS_lock_t));
   memset (pDev->event_mbx.sem_dev, 0, sizeof (IFXOS_lock_t));
   /* before IFXOS porting: sema_init(pDev->event_mbx.sem_dev, 1); */
   IFXOS_LockInit (pDev->event_mbx.sem_dev);
   if (IFX_SUCCESS !=
       IFXOS_DrvSelectQueueInit (&pDev->event_mbx.mps_wakeuplist))
   {
      TRACE (MPS, DBG_LEVEL_HIGH,
             ("%s(): init_waitqueue_head error\r\n", __FUNCTION__));
   }
#endif /* */
   memset (&pDev->event, 0, sizeof (MbxEventRegs_s));

   /* initialize the semaphores for multitasking access */
   pDev->command_mb.sem_dev = IFXOS_BlockAlloc (sizeof (IFXOS_lock_t));
   memset (pDev->command_mb.sem_dev, 0, sizeof (IFXOS_lock_t));
   /* before IFXOS porting: sema_init(pDev->command_mb.sem_dev, 1); */
   IFXOS_LockInit (pDev->command_mb.sem_dev);

   /* select mechanism implemented for each queue */
   if (IFX_SUCCESS !=
       IFXOS_DrvSelectQueueInit (&pDev->command_mb.mps_wakeuplist))
   {
      TRACE (MPS, DBG_LEVEL_HIGH,
             ("%s(): init_waitqueue_head error\r\n", __FUNCTION__));
   }

   /* voice upstream data mailbox area */
   pDev->voice_upstrm_fifo.pstart =
      (IFX_uint32_t *) KSEG1ADDR (MBX_Memory->MBX_UPSTR_DATA_BASE +
                                  ((MBX_Memory->MBX_UPSTR_DATA_SIZE - 4) >> 2));
   pDev->voice_upstrm_fifo.pend =
      (IFX_uint32_t *) KSEG1ADDR (MBX_Memory->MBX_UPSTR_DATA_BASE);
   pDev->voice_upstrm_fifo.pwrite_off =
      (IFX_uint32_t *) & (MBX_Memory->MBX_UPSTR_DATA_WRITE);
   pDev->voice_upstrm_fifo.pread_off =
      (IFX_uint32_t *) & (MBX_Memory->MBX_UPSTR_DATA_READ);
   pDev->voice_upstrm_fifo.size = MBX_Memory->MBX_UPSTR_DATA_SIZE;

#ifdef CONFIG_PROC_FS
   pDev->voice_upstrm_fifo.min_space = MBX_Memory->MBX_UPSTR_DATA_SIZE;
   pDev->voice_upstrm_fifo.discards = 0;
#endif /* */

   /* voice downstream data mailbox area */
   pDev->voice_dwstrm_fifo.pstart =
      (IFX_uint32_t *) KSEG1ADDR (MBX_Memory->MBX_DNSTR_DATA_BASE +
                                  ((MBX_Memory->MBX_DNSTR_DATA_SIZE - 4) >> 2));
   pDev->voice_dwstrm_fifo.pend =
      (IFX_uint32_t *) KSEG1ADDR (MBX_Memory->MBX_DNSTR_DATA_BASE);
   pDev->voice_dwstrm_fifo.pwrite_off =
      (IFX_uint32_t *) & (MBX_Memory->MBX_DNSTR_DATA_WRITE);
   pDev->voice_dwstrm_fifo.pread_off =
      (IFX_uint32_t *) & (MBX_Memory->MBX_DNSTR_DATA_READ);
   pDev->voice_dwstrm_fifo.size = MBX_Memory->MBX_DNSTR_DATA_SIZE;

#ifdef CONFIG_PROC_FS
   pDev->voice_dwstrm_fifo.min_space = MBX_Memory->MBX_UPSTR_DATA_SIZE;
#endif /* */

   /* configure voice channel communication structure fields that are common to
      all voice channels */
   for (i = 0; i < NUM_VOICE_CHANNEL; i++)
   {
      /* voice upstream data software fifo */
      pDev->sw_upstrm_fifo[i].pend =
         IFXOS_BlockAlloc (MBX_Memory->MBX_UPSTR_DATA_SIZE + 8);
      pDev->sw_upstrm_fifo[i].pstart =
         (pDev->sw_upstrm_fifo[i].pend +
          ((MBX_Memory->MBX_UPSTR_DATA_SIZE - 4) >> 2));
      pDev->sw_upstrm_fifo[i].pwrite_off =
         (pDev->sw_upstrm_fifo[i].pend +
          ((MBX_Memory->MBX_UPSTR_DATA_SIZE) >> 2));
      *pDev->sw_upstrm_fifo[i].pwrite_off =
         (MBX_Memory->MBX_UPSTR_DATA_SIZE - 4);
      pDev->sw_upstrm_fifo[i].pread_off =
         (pDev->sw_upstrm_fifo[i].pend +
          ((MBX_Memory->MBX_UPSTR_DATA_SIZE + 4) >> 2));
      *pDev->sw_upstrm_fifo[i].pread_off =
         (MBX_Memory->MBX_UPSTR_DATA_SIZE - 4);
      pDev->sw_upstrm_fifo[i].size = MBX_Memory->MBX_UPSTR_DATA_SIZE;
#ifdef CONFIG_PROC_FS
      pDev->sw_upstrm_fifo[i].min_space = MBX_Memory->MBX_UPSTR_DATA_SIZE;
      pDev->sw_upstrm_fifo[i].discards = 0;
#endif /* */
      memset ((IFX_void_t *) pDev->sw_upstrm_fifo[i].pend, 0x00,
              MBX_Memory->MBX_UPSTR_DATA_SIZE);

      /* upstrm fifo pointer might be changed on open... */
      pDev->voice_mb[i].upstrm_fifo = &pDev->sw_upstrm_fifo[i];
      pDev->voice_mb[i].dwstrm_fifo = &pDev->voice_dwstrm_fifo;
      pDev->voice_mb[i].Installed = IFX_FALSE;  /* current mbx installation
                                                   status */
      pDev->voice_mb[i].base_global = (mps_mbx_reg *) VCPU_BASEADDRESS;
      pDev->voice_mb[i].pVCPU_DEV = pDev;       /* global pointer reference */
      pDev->voice_mb[i].down_callback = IFX_NULL;       /* callback functions
                                                           for */
      pDev->voice_mb[i].up_callback = IFX_NULL; /* down- and upstream dir. */

      /* initialize the semaphores for multitasking access */
      pDev->voice_mb[i].sem_dev = IFXOS_BlockAlloc (sizeof (IFXOS_lock_t));
      memset (pDev->voice_mb[i].sem_dev, 0, sizeof (IFXOS_lock_t));
      /* before IFXOS porting: sema_init(pDev->voice_mb[i].sem_dev, 1); */
      IFXOS_LockInit (pDev->voice_mb[i].sem_dev);

      /* initialize the semaphores to read from the fifo */
      pDev->voice_mb[i].sem_read_fifo =
         IFXOS_BlockAlloc (sizeof (IFXOS_lock_t));
      memset (pDev->voice_mb[i].sem_read_fifo, 0, sizeof (IFXOS_lock_t));
      /* before IFXOS porting: sema_init(pDev->voice_mb[i].sem_read_fifo, 0); */
      IFXOS_LockInit (pDev->voice_mb[i].sem_read_fifo);
      IFXOS_LockGet (pDev->voice_mb[i].sem_read_fifo);
#ifdef MPS_FIFO_BLOCKING_WRITE
      pDev->voice_mb[i].sem_write_fifo =
         IFXOS_BlockAlloc (sizeof (IFXOS_lock_t));
      memset (pDev->voice_mb[i].sem_write_fifo, 0, sizeof (IFXOS_lock_t));
      /* before IFXOS porting: sema_init(pDev->voice_mb[i].sem_write_fifo, 0); */
      IFXOS_LockInit (pDev->voice_mb[i].sem_write_fifo);
      IFXOS_LockGet (pDev->voice_mb[i].sem_write_fifo);
      pDev->voice_mb[i].bBlockWriteMB = TRUE;
#endif /* MPS_FIFO_BLOCKING_WRITE */
      if (pDev->voice_mb[i].sem_dev == IFX_NULL)
         return (IFX_ERROR);

      /* select mechanism implemented for each queue */
      if (IFX_SUCCESS !=
          IFXOS_DrvSelectQueueInit (&pDev->voice_mb[i].mps_wakeuplist))
      {
         TRACE (MPS, DBG_LEVEL_HIGH,
                ("%s(): init_waitqueue_head error\r\n", __FUNCTION__));
      }
      memset (&pDev->voice_mb[i].event_mask, 0, sizeof (MbxEventRegs_s));
   }

   /* set channel identifiers */
   pDev->command_mb.devID = command;
   pDev->voice_mb[0].devID = voice0;
   pDev->voice_mb[1].devID = voice1;
   pDev->voice_mb[2].devID = voice2;
   pDev->voice_mb[3].devID = voice3;
   pDev->voice_mb[4].devID = voice4;
   pDev->voice_mb[5].devID = voice5;
   pDev->voice_mb[6].devID = voice6;
   pDev->voice_mb[7].devID = voice7;
#ifdef CONFIG_MPS_EVENT_MBX
   pDev->event_mbx.devID = event_mbx;
#endif /* CONFIG_MPS_EVENT_MBX */
   pDev->provide_buffer = IFXOS_BlockAlloc (sizeof (IFXOS_lock_t));
   memset (pDev->provide_buffer, 0, sizeof (IFXOS_lock_t));
   /* before IFXOS porting: sema_init(pDev->provide_buffer, 0); */
   IFXOS_LockInit (pDev->provide_buffer);
   IFXOS_LockGet (pDev->provide_buffer);
   /* allocate buffer for firmware image data */
   pFW_img_data = IFXOS_BlockAlloc(sizeof(*pFW_img_data));
   if (IFX_NULL == pFW_img_data)
      return IFX_ERROR;

   /* debug code */
   memset (dchan_evt_cnt, 0, sizeof(dchan_evt_cnt));
   memset (dchan_evt_served_cnt, 0, sizeof(dchan_evt_served_cnt));
   /* end of debug code */

   return 0;
}


/**
 * MPS Structure Reset
 * This function resets the global structures into inital state
 *
 * \param   pDev     Pointer to MPS device structure
 * \return  0        IFX_SUCCESS, if initialization was successful
 * \return  -1       IFX_ERROR, allocation or semaphore access problem
 * \ingroup Internal
 */
IFX_uint32_t ifx_mps_reset_structures (mps_comm_dev * pDev)
{
   IFX_int32_t i;

#ifdef CONFIG_PROC_FS
   pDev->voice_dwstrm_fifo.min_space = pDev->voice_dwstrm_fifo.size;
   pDev->voice_dwstrm_fifo.bytes = 0;
   pDev->voice_dwstrm_fifo.pkts = 0;
   pDev->voice_dwstrm_fifo.discards = 0;
#endif /* */
   for (i = 0; i < NUM_VOICE_CHANNEL; i++)
   {
      ifx_mps_fifo_clear (pDev->voice_mb[i].dwstrm_fifo);
      ifx_mps_fifo_clear (pDev->voice_mb[i].upstrm_fifo);

#ifdef CONFIG_PROC_FS
      pDev->voice_mb[i].upstrm_fifo->min_space =
         pDev->voice_mb[i].upstrm_fifo->size;
      pDev->voice_mb[i].upstrm_fifo->bytes = 0;
      pDev->voice_mb[i].upstrm_fifo->pkts = 0;
      pDev->voice_mb[i].upstrm_fifo->discards = 0;
#endif /* */
   }
   ifx_mps_fifo_clear (pDev->command_mb.dwstrm_fifo);
   ifx_mps_fifo_clear (pDev->command_mb.upstrm_fifo);
#ifdef CONFIG_MPS_EVENT_MBX
   ifx_mps_fifo_clear (&pDev->event_upstrm_fifo);
#endif /* CONFIG_MPS_EVENT_MBX */
#ifdef CONFIG_PROC_FS
   pDev->command_mb.dwstrm_fifo->min_space = pDev->command_mb.dwstrm_fifo->size;
   pDev->command_mb.dwstrm_fifo->bytes = 0;
   pDev->command_mb.dwstrm_fifo->pkts = 0;
   pDev->command_mb.dwstrm_fifo->discards = 0;
   pDev->command_mb.upstrm_fifo->min_space = pDev->command_mb.upstrm_fifo->size;
   pDev->command_mb.upstrm_fifo->bytes = 0;
   pDev->command_mb.upstrm_fifo->pkts = 0;
   pDev->command_mb.upstrm_fifo->discards = 0;
#endif /* */
#if CONFIG_MPS_HISTORY_SIZE > 0
   ifx_mps_history_buffer_freeze = 0;
   ifx_mps_history_buffer_words = 0;
   ifx_mps_history_buffer_overflowed = 0;
#endif /* */
   IFXOS_LockTimedGet (pDev->provide_buffer, 0, IFX_NULL);
   return IFX_SUCCESS;
}


/******************************************************************************
 * Mailbox Managment
 ******************************************************************************/

/**
 * Gets channel ID field from message header
 * This function reads the data word at the read pointer position
 * of the mailbox FIFO pointed to by mbx and extracts the channel ID field
 * from the data word read.
 *
 * \param   mbx      Pointer to mailbox structure to be accessed
 * \return  ID       Voice channel identifier.
 * \ingroup Internal
 */
mps_devices ifx_mps_mbx_get_message_channel (mps_fifo * mbx)
{
   MbxMsgHd_u msg_hd;
   mps_devices retval = unknown;
   IFX_int32_t ret;

   ret = ifx_mps_fifo_read (mbx, 0, &msg_hd.val);
   if (ret == IFX_ERROR)
      return retval;
   switch (msg_hd.hd.chan)
   {
      case 0:
         retval = voice0;
         break;
      case 1:
         retval = voice1;
         break;
      case 2:
         retval = voice2;
         break;
      case 3:
         retval = voice3;
         break;
      case 4:
         retval = voice4;
         break;
      case 5:
         retval = voice5;
         break;
      case 6:
         retval = voice6;
         break;
      case 7:
         retval = voice7;
         break;
      default:
         retval = unknown;
         TRACE (MPS, DBG_LEVEL_HIGH,
                ("%s(): unknown channel ID %d\n", __FUNCTION__,
                 msg_hd.hd.chan));
         break;
   }
   return retval;
}


/**
 * Get message length
 * This function returns the length in bytes of the message located at read pointer
 * position. It reads the plength field of the message header (length in bytes)
 * adds the header length and returns the complete length in bytes.
 *
 * \param   mbx      Pointer to mailbox structure to be accessed
 * \return  length   Length of message in bytes.
 * \ingroup Internal
 */
IFX_int32_t ifx_mps_mbx_get_message_length (mps_fifo * mbx)
{
   MbxMsgHd_u msg_hd;
   IFX_int32_t ret;

   ret = ifx_mps_fifo_read (mbx, 0, &msg_hd.val);

   /* return payload + header length in bytes */
   if (ret == IFX_ERROR)        /* error */
      return 0;
   else
      return ((IFX_int32_t) msg_hd.hd.plength + 4);
}


/**
 * Read message from upstream data mailbox
 * This function reads a complete data message from the upstream data mailbox.
 * It reads the header checks how many payload words are included in the message
 * and reads the payload afterwards. The mailbox's read pointer is updated afterwards
 * by the amount of words read.
 *
 * \param   fifo        Pointer to mailbox structure to be read from
 * \param   msg         Pointer to message structure read from buffer
 * \param   bytes       Pointer to number of bytes included in read message
 * \return  0           IFX_SUCCESS, successful read operation,
 * \return  -1          Invalid length field read.
 * \ingroup Internal
 */
IFX_int32_t ifx_mps_mbx_read_message (mps_fifo * fifo, MbxMsg_s * msg,
                                      IFX_uint32_t * bytes)
{
   IFX_int32_t i, ret;
   IFXOS_INTSTAT flags;

   IFXOS_LOCKINT (flags);

   /* read message header from buffer */
   ret = ifx_mps_fifo_read (fifo, 0, &msg->header.val);
   if (ret == IFX_ERROR)
   {
      IFXOS_UNLOCKINT (flags);
      return ret;
   }

   if ((msg->header.hd.plength % 4) != 0)       /* check payload length */
   {
      TRACE (MPS, DBG_LEVEL_HIGH,
             ("%s(): Odd payload length %d\n", __FUNCTION__,
              msg->header.hd.plength));
      IFXOS_UNLOCKINT (flags);
      return IFX_ERROR;
   }

   if ((msg->header.hd.plength / 4) > MAX_UPSTRM_DATAWORDS)
   {
      TRACE (MPS, DBG_LEVEL_HIGH,
             ("%s(): header length %d exceeds the Mailbox size %d\n",
              __FUNCTION__, msg->header.hd.plength, MAX_UPSTRM_DATAWORDS*4));
      IFXOS_UNLOCKINT (flags);
      return IFX_ERROR;
   }

   for (i = 0; i < msg->header.hd.plength; i += 4)      /* read message payload
                                                         */
   {
      ret = ifx_mps_fifo_read (fifo, (IFX_uint8_t) (i + 4), &msg->data[i / 4]);
      if (ret == IFX_ERROR)
      {
         IFXOS_UNLOCKINT (flags);
         return ret;
      }
   }
   *bytes = msg->header.hd.plength + 4;
   ifx_mps_fifo_read_ptr_inc (fifo, (msg->header.hd.plength + 4));
   IFXOS_UNLOCKINT (flags);
   return IFX_SUCCESS;
}


/**
 * Read message from FIFO
 * This function reads a message from the upstream data mailbox and passes it
 * to the calling function. A call to the notify_upstream function will trigger
 * another wakeup in case there is already more data available.
 *
 * \param   pMBDev   Pointer to mailbox device structure
 * \param   pPkg     Pointer to data transfer structure (output parameter)
 * \param   timeout  Currently unused
 * \return  0        IFX_SUCCESS, successful read operation,
 * \return  -1       IFX_ERROR, in case of read error.
 * \return  -ENODATA No data was available
 * \return  -EBADMSG Accidential read of buffer message
 * \ingroup Internal
 */
IFX_int32_t ifx_mps_mbx_read (mps_mbx_dev * pMBDev, mps_message * pPkg,
                              IFX_int32_t timeout)
{
   MbxMsg_s msg;
   IFX_uint32_t bytes = 0;
   mps_fifo *fifo;
   IFX_int32_t retval = IFX_ERROR;

   IFX_MPS_UNUSED(timeout);

   fifo = pMBDev->upstrm_fifo;
   memset (&msg, 0, sizeof (msg));      /* initialize msg pointer */
   if (!ifx_mps_fifo_not_empty (fifo))
   {
      /* Nothing available for this channel... */
      return -ENODATA;
   }

   /* read message from mailbox */
   if (ifx_mps_mbx_read_message (fifo, &msg, &bytes) == 0)
   {
      switch (pMBDev->devID)
      {
         case command:

            /* command messages are completely passed to the caller. The
               mps_message structure comprises a pointer to the * message start
               and the message size in bytes */
            pPkg->pData = mps_buffer.malloc (bytes, FASTBUF_CMD_OWNED);
            if (pPkg->pData == IFX_NULL)
               return -1;
            memcpy ((IFX_uint8_t *) pPkg->pData, (IFX_uint8_t *) & msg, bytes);
            pPkg->cmd_type = msg.header.hd.type;
            pPkg->nDataBytes = bytes;
            pPkg->RTP_PaylOffset = 0;
            retval = IFX_SUCCESS;
#ifdef CONFIG_PROC_FS
            pMBDev->upstrm_fifo->bytes += bytes;
#endif /* */

            /* do another wakeup in case there is more data available... */
            ifx_mps_mbx_cmd_upstream (0);
            break;

#ifdef CONFIG_MPS_EVENT_MBX
         case event_mbx:

            /* event messages are completely passed to the caller. The
               mps_message structure comprises a pointer to the * message start
               and the message size in bytes */
            pPkg->pData = mps_buffer.malloc (bytes, FASTBUF_EVENT_OWNED);
            if (pPkg->pData == IFX_NULL)
               return -1;
            memcpy ((IFX_uint8_t *) pPkg->pData, (IFX_uint8_t *) & msg, bytes);
            pPkg->cmd_type = msg.header.hd.type;
            pPkg->nDataBytes = bytes;
            pPkg->RTP_PaylOffset = 0;
            retval = IFX_SUCCESS;

            /* do another wakeup in case there is more data available... */
            ifx_mps_mbx_event_upstream (0);
            break;

#endif /* CONFIG_MPS_EVENT_MBX */
         case voice0:
         case voice1:
         case voice2:
         case voice3:
         case voice4:
         case voice5:
         case voice6:
         case voice7:

            /* data messages are passed as mps_message pointer that comprises a
               pointer to the payload start address and the payload size in
               bytes. The message header is removed and the payload pointer,
               payload size, payload type and and RTP payload offset are passed
               to CPU0. */
            pPkg->cmd_type = msg.header.hd.type;
            pPkg->pData = (IFX_uint8_t *) KSEG0ADDR ((IFX_uint8_t *) msg.data[0]);      /* get
                                                                                           payload
                                                                                           pointer */
            pPkg->nDataBytes = msg.data[1];     /* get payload size */
            /* invalidate cache */
            ifx_mps_cache_inv ((IFX_ulong_t)pPkg->pData, pPkg->nDataBytes);
            /* set RTP payload offset for RTP messages to be clarified how this
               should look like exactly */
            pPkg->RTP_PaylOffset = 0;
            retval = IFX_SUCCESS;
#ifdef CONFIG_PROC_FS
            pMBDev->upstrm_fifo->bytes += bytes;
#endif /* */
            if (IFX_SUCCESS ==
                IFXOS_LockTimedGet (pMPSDev->provide_buffer, 0, IFX_NULL))
            {
               if (ifx_mps_bufman_buf_provide
                   (MPS_DEFAULT_PROVISION_SEGMENTS_PER_MSG,
                    mps_buffer.buf_size) != IFX_SUCCESS)
               {
                  TRACE (MPS, DBG_LEVEL_HIGH,
                         ("%s(): Warning - provide buffer failed...\n",
                          __FUNCTION__));
                  IFXOS_LockRelease (pMPSDev->provide_buffer);
               }
            }
            break;
         default:
            break;
      }
   }
   return retval;
}


/**
 * Build 32 bit word starting at byte_ptr.
 * This function builds a 32 bit word out of 4 consecutive bytes
 * starting at byte_ptr position.
 *
 * \param   byte_ptr  Pointer to first byte (most significat 8 bits) of word calculation
 * \return  value     Returns value of word starting at byte_ptr position
 * \ingroup Internal
 */
IFX_uint32_t ifx_mps_mbx_build_word (IFX_uint8_t * byte_ptr)
{
   IFX_uint32_t result = 0x00000000;
   IFX_int32_t i;

   for (i = 0; i < 4; i++)
   {
      result += (IFX_uint32_t) (*(byte_ptr + i)) << ((3 - i) * 8);
   }
   return (result);
}


/**
 * Write to Downstream Mailbox of MPS.
 * This function writes messages into the downstream mailbox to be read
 * by CPU1
 *
 * \param   pMBDev    Pointer to mailbox device structure
 * \param   msg_ptr   Pointer to message
 * \param   msg_bytes Number of bytes in message
 * \return  0         Returns IFX_SUCCESS in case of successful write operation
 * \return  -EAGAIN   in case of access fails with FIFO overflow while in irq
 * \return  -EIO      in case of access fails with FIFO overflow in task context
 * \ingroup Internal
 */
IFX_int32_t ifx_mps_mbx_write_message (mps_mbx_dev * pMBDev,
                                       IFX_uint8_t * msg_ptr,
                                       IFX_uint32_t msg_bytes)
{
   mps_fifo *mbx;
   IFX_uint32_t i;
   IFXOS_INTSTAT flags;
   IFX_int32_t retval = -EAGAIN;
   IFX_int32_t retries = 0;
   IFX_uint32_t word = 0;
   IFX_boolean_t word_aligned = IFX_TRUE;
   static IFX_uint32_t trace_fag;

   IFXOS_LOCKINT (flags);
   mbx = pMBDev->dwstrm_fifo;   /* set pointer to downstream mailbox FIFO
                                   structure */
   if ((IFX_uint32_t) msg_ptr & 0x00000003)
   {
      word_aligned = IFX_FALSE;
      TRACE (MPS, DBG_LEVEL_HIGH,
             ("%s(): Passed message not word aligned !!!\n", __FUNCTION__));
   }

   /* request for downstream mailbox buffer memory, make MAX_FIFO_WRITE_RETRIES
      attempts in case not enough memory is not available */
   while (++retries <= MAX_FIFO_WRITE_RETRIES)
   {
      if (ifx_mps_fifo_mem_request (mbx, msg_bytes) == IFX_TRUE)
      {
         trace_fag = 0;
         break;
      }

      if (in_interrupt ())
      {
         retries = MAX_FIFO_WRITE_RETRIES + 1;
         break;
      }
      else
      {
#ifdef TEST_NO_CMD_MBX_WRITE_RETRIES
         if(pMBDev == &ifx_mps_dev.command_mb)
         {
            retries = MAX_FIFO_WRITE_RETRIES + 1;
            trace_fag = 1;
            break;
         }
         else
#endif /* TEST_NO_CMD_MBX_WRITE_RETRIES */
         {
            IFXOS_UNLOCKINT (flags);
            udelay (125);
            IFXOS_LOCKINT (flags);
         }
      }
   }

   if (retries <= MAX_FIFO_WRITE_RETRIES)
   {
      /* write message words to mailbox buffer starting at write pointer
         position and update the write pointer index by the amount of written
         data afterwards */
      for (i = 0; i < msg_bytes; i += 4)
      {
         if (word_aligned)
            ifx_mps_fifo_write (mbx, *(IFX_uint32_t *) (msg_ptr + i), i);
         else
         {
            word = ifx_mps_mbx_build_word (msg_ptr + i);
            ifx_mps_fifo_write (mbx, word, i);
         }
      }
#ifdef VMMC_WITH_MPS
#ifdef TAPI_PACKET_OWNID
      if(pMBDev != &(ifx_mps_dev.command_mb))
      {
         switch (((mps_message *)msg_ptr)->cmd_type)
         {
            case DAT_PAYL_PTR_MSG_HDLC_PACKET:
               IFX_TAPI_VoiceBufferChOwn (
                  (IFX_void_t *)KSEG0ADDR (((mps_message *)msg_ptr)->pData),
                  IFX_TAPI_BUFFER_OWNER_HDLC_FW);
               break;
            case DAT_PAYL_PTR_MSG_FAX_DATA_PACKET:
            case DAT_PAYL_PTR_MSG_VOICE_PACKET:
            case DAT_PAYL_PTR_MSG_EVENT_PACKET:
            case DAT_PAYL_PTR_MSG_FAX_T38_PACKET:
               IFX_TAPI_VoiceBufferChOwn (
                  (IFX_void_t *)KSEG0ADDR (((mps_message *)msg_ptr)->pData),
                  IFX_TAPI_BUFFER_OWNER_COD_FW);
               break;
         }
      }
#endif /* TAPI_PACKET_OWNID */
#endif /* MMC_WITH_MPS */

      ifx_mps_fifo_write_ptr_inc (mbx, msg_bytes);

      retval = IFX_SUCCESS;

#ifdef CONFIG_PROC_FS
      pMBDev->dwstrm_fifo->pkts++;
      pMBDev->dwstrm_fifo->bytes += msg_bytes;
      if (mbx->min_space > ifx_mps_fifo_mem_available (mbx))
         mbx->min_space = ifx_mps_fifo_mem_available (mbx);
#endif /* CONFIG_PROC_FS */
   }
   else
   {
      /* insufficient space in the mailbox for writing the data */

      /** \todo update error statistics */

      if (!trace_fag)           /* protect from trace flood */
      {
         TRACE (MPS, DBG_LEVEL_LOW,
                ("%s(): write message timeout\n", __FUNCTION__));

         if (pMBDev->devID == command)
         {
            /* dump the command downstream mailbox */
            TRACE (MPS, DBG_LEVEL_HIGH,
                   (" (wr: 0x%08x, rd: 0x%08x)\n",
                    (IFX_uint32_t) ifx_mps_dev.cmd_dwstrm_fifo.pend +
                    (IFX_uint32_t) * ifx_mps_dev.cmd_dwstrm_fifo.pwrite_off,
                    (IFX_uint32_t) ifx_mps_dev.cmd_dwstrm_fifo.pend +
                    (IFX_uint32_t) * ifx_mps_dev.cmd_dwstrm_fifo.pread_off));
            for (i = 0; i < ifx_mps_dev.cmd_dwstrm_fifo.size; i += 16)
            {
               TRACE (MPS, DBG_LEVEL_HIGH,
                      ("   0x%08x: %08x %08x %08x %08x\n",
                       (IFX_uint32_t) (ifx_mps_dev.cmd_dwstrm_fifo.pend +
                                       (i / 4)),
                       *(ifx_mps_dev.cmd_dwstrm_fifo.pend + (i / 4)),
                       *(ifx_mps_dev.cmd_dwstrm_fifo.pend + 1 + (i / 4)),
                       *(ifx_mps_dev.cmd_dwstrm_fifo.pend + 2 + (i / 4)),
                       *(ifx_mps_dev.cmd_dwstrm_fifo.pend + 3 + (i / 4))));
            }
         }

         /* trace only once until write succeeds at least one time */
         trace_fag = 1;
      }

      /* If the command downstream mailbox stays full for several milliseconds,
         a fatal error has occurred and the voice CPU should be restarted */
      if (!in_interrupt ())
      {
         /* If not in interrupt we already waited some milliseconds for the
            voice firmware. Do a reset now. */

#if 0                           /* disabled until reset concept is implemented */
         IFX_int32_t status;

         TRACE (MPS, DBG_LEVEL_HIGH,
                ("%s(): Restarting the voice firmware now\n", __FUNCTION__));

         status = ifx_mps_restart ();
         if (status == IFX_SUCCESS)
         {
            status = ifx_mps_get_fw_version (1);
         }
         if (status == IFX_SUCCESS)
         {
            status = ifx_mps_bufman_init ();
         }
         if (status != IFX_SUCCESS)
         {
            TRACE (MPS, DBG_LEVEL_HIGH,
                   ("%s(): Restarting the voice firmware failed\n",
                    __FUNCTION__));
         }
         else
         {
            /* firmware was restarted so reset the trace output flag */
            trace_fag = 0;
         }
#endif

         /* -> return fatal error */
         retval = -EIO;
      }
   }
   IFXOS_UNLOCKINT (flags);
   return retval;
}


/**
 * Write to Downstream Data Mailbox of MPS.
 * This function writes the passed message into the downstream data mailbox.
 *
 * \param   pMBDev     Pointer to mailbox device structure
 * \param   readWrite  Pointer to message structure
 * \return  0          IFX_SUCCESS in case of successful write operation
 * \return  -1         IFX_ERROR in case of access fails with FIFO overflow
 * \ingroup Internal
 */
IFX_int32_t ifx_mps_mbx_write_data (mps_mbx_dev * pMBDev,
                                    mps_message * readWrite)
{
   IFX_int32_t retval = IFX_ERROR;
   MbxMsg_s msg;

   if ((pMBDev->devID >= voice0) && (pMBDev->devID <= voice7))
   {
#ifdef FAIL_ON_ERR_INTERRUPT
      /* check status not worth going on if voice CPU has indicated an error */
      if (pMPSDev->event.MPS_Ad0Reg.fld.data_err)
      {
         return retval;
      }
#endif /* */
      if (atomic_read (&ifx_mps_write_blocked) != 0)
      {
         /* no more messages can be sent until more buffers have been provided */
         return -1;
      }
      memset (&msg, 0, sizeof (msg));   /* initialize msg structure */

      /* build data message from passed payload data structure */
      msg.header.hd.plength = 0x8;
      switch (pMBDev->devID)
      {
         case voice0:
            msg.header.hd.chan = 0;
            break;
         case voice1:
            msg.header.hd.chan = 1;
            break;
         case voice2:
            msg.header.hd.chan = 2;
            break;
         case voice3:
            msg.header.hd.chan = 3;
            break;
         case voice4:
            msg.header.hd.chan = 4;
            break;
         case voice5:
            msg.header.hd.chan = 5;
            break;
         case voice6:
            msg.header.hd.chan = 6;
            break;
         case voice7:
            msg.header.hd.chan = 7;
            break;
         default:
            return retval;
      }
      msg.header.hd.type = readWrite->cmd_type;
	  /* debug code */
      if ((msg.header.val & 0x1f000000) == 0x09000000)
      {
         IFX_uint8_t ch = (msg.header.val & 0x000f0000) >> 16;

         dchan_evt_served_cnt[ch]++;
      }
      /* end of debug code */
      msg.data[0] = CPHYSADDR ((IFX_uint32_t) readWrite->pData);
      msg.data[1] = readWrite->nDataBytes;
#if defined(CONFIG_MIPS) && !defined(CONFIG_MIPS_UNCACHED)
      if (IFX_TRUE == bDoCacheOps)
      {
         ifx_mps_cache_wb_inv ((IFX_uint32_t) readWrite->pData, readWrite->nDataBytes);
      }
#endif /*defined(CONFIG_MIPS) && !defined(CONFIG_MIPS_UNCACHED)*/
      if ((retval =
           ifx_mps_mbx_write_message (pMBDev, (IFX_uint8_t *) & msg,
                                      12)) != IFX_SUCCESS)
      {
         TRACE (MPS, DBG_LEVEL_LOW,
                ("%s(): Writing data failed ! *\n", __FUNCTION__));
      }
   }
   else
   {
      TRACE (MPS, DBG_LEVEL_HIGH,
             ("%s(): Invalid device ID %d !\n", __FUNCTION__, pMBDev->devID));
   }

   if (IFX_SUCCESS == IFXOS_LockTimedGet (pMPSDev->provide_buffer, 0, IFX_NULL))
   {
      if (ifx_mps_bufman_buf_provide
          (MPS_DEFAULT_PROVISION_SEGMENTS_PER_MSG,
           mps_buffer.buf_size) != IFX_SUCCESS)
      {
         TRACE (MPS, DBG_LEVEL_HIGH,
                ("%s(): Warning - provide buffer failed...\n", __FUNCTION__));
         IFXOS_LockRelease (pMPSDev->provide_buffer);
      }
   }
   return retval;
}


/**
 * Write buffer provisioning message to mailbox.
 * This function writes a buffer provisioning message to the downstream data
 * mailbox that provides the specified amount of memory segments .
 *
 * \param   mem_ptr      Pointer to segment pointer array
 * \param   segments     Number of valid segment pointers in array
 * \param   segment_size Size of segements in array
 * \return  0            IFX_SUCCESS in case of successful write operation
 * \return  -1           IFX_ERROR in case of access fails with FIFO overflow
 * \ingroup Internal
 */
IFX_int32_t ifx_mps_mbx_write_buffer_prov_message (mem_seg_t * mem_ptr,
                                                   IFX_uint8_t segments,
                                                   IFX_uint32_t segment_size)
{
   IFX_int32_t retval = IFX_ERROR;
   IFX_int32_t i;
   MbxMsg_s msg;

   memset (&msg, 0, sizeof (msg));      /* initialize msg structure */
   /* build data message from passed payload data structure */
   msg.header.hd.plength = (segments * 4) + 4;
   msg.header.hd.type = CMD_ADDRESS_PACKET;
   for (i = 0; i < segments; i++)
   {
      msg.data[i] = *((IFX_uint32_t *) mem_ptr + i);
   }
   msg.data[segments] = segment_size;

   /* send buffer provision message and update buffer management */
   retval =
      ifx_mps_mbx_write_message ((&pMPSDev->voice_mb[0]), (IFX_uint8_t *) & msg,
                                 (IFX_uint32_t) (segments + 2) * 4);
   if (retval == IFX_SUCCESS)
   {
      ifx_mps_bufman_inc_level (segments);
   }
   else
   {
      TRACE (MPS, DBG_LEVEL_HIGH,
             ("%s() - failed to write message\n", __FUNCTION__));
   }
   return retval;
}


/**
 * Write to downstream command mailbox.
 * This is the function to write commands into the downstream command mailbox
 * to be read by CPU1
 *
 * \param   pMBDev     Pointer to mailbox device structure
 * \param   readWrite  Pointer to transmission data container
 * \return  0          IFX_SUCCESS in case of successful write operation
 * \return  -1         IFX_ERROR in case of access fails with FIFO overflow
 * \ingroup Internal
 */
IFX_int32_t ifx_mps_mbx_write_cmd (mps_mbx_dev * pMBDev,
                                   mps_message * readWrite)
{
   IFX_int32_t retval = IFX_ERROR;

   if (pMBDev->devID == command)
   {
#ifdef FAIL_ON_ERR_INTERRUPT
      /* check status not worth going on if voice CPU has indicated an error */
      if (pMPSDev->event.MPS_Ad0Reg.fld.cmd_err)
      {
         return retval;
      }
#endif /*DEBUG*/
         if ((readWrite->nDataBytes) % 4)
      {
         TRACE (MPS, DBG_LEVEL_HIGH,
                ("%s(): invalid number of bytes %d\n", __FUNCTION__,
                 readWrite->nDataBytes));
      }
      if ((IFX_uint32_t) (readWrite->pData) & 0x00000003)
         TRACE (MPS, DBG_LEVEL_HIGH,
                ("%s(): non word aligned data passed to mailbox\n",
                 __FUNCTION__));
      if (readWrite->nDataBytes > (MBX_CMD_FIFO_SIZE - 4))
         TRACE (MPS, DBG_LEVEL_HIGH,
                ("%s(): command size too large!\n", __FUNCTION__));

#if CONFIG_MPS_HISTORY_SIZE > 0
      if (!ifx_mps_history_buffer_freeze)
      {
         IFX_int32_t i, pos;
         for (i = 0; i < (readWrite->nDataBytes / 4); i++)
         {
            pos = ifx_mps_history_buffer_words;
            ifx_mps_history_buffer[pos] =
               ((IFX_uint32_t *) readWrite->pData)[i];
            ifx_mps_history_buffer_words++;

#ifdef DEBUG
            ifx_mps_history_buffer_words_total++;
#endif /* */
            if (ifx_mps_history_buffer_words == MPS_HISTORY_BUFFER_SIZE)
            {
               ifx_mps_history_buffer_words = 0;
               ifx_mps_history_buffer_overflowed = 1;
            }
         }
      }
#endif /* */
      retval =
         ifx_mps_mbx_write_message (pMBDev, (IFX_uint8_t *) readWrite->pData,
                                    readWrite->nDataBytes);
      if (retval != IFX_SUCCESS)
      {
         TRACE (MPS, DBG_LEVEL_HIGH,
                ("%s() - failed to write message!\n", __FUNCTION__));
      }
   }
   else
   {
      /* invalid device id read from mailbox FIFO structure */
      TRACE (MPS, DBG_LEVEL_HIGH,
             ("%s(): Invalid device ID %d !\n", __FUNCTION__, pMBDev->devID));
   }
   return retval;
}


/**
 * Notify queue about upstream data reception
 * This function checks the channel identifier included in the header
 * of the message currently pointed to by the upstream data mailbox's
 * read pointer. It wakes up the related queue to read the received data message
 * out of the mailbox for further processing. The process is repeated
 * as long as upstream messages are avaiilable in the mailbox.
 * The function is attached to the driver's poll/select functionality.
 *
 * \param   dummy    Tasklet parameter, not used.
 * \ingroup Internal
 */
IFX_void_t ifx_mps_mbx_data_upstream (IFX_ulong_t dummy)
{
   mps_devices channel;
   mps_fifo *mbx;
   mps_mbx_dev *mbx_dev;
   MbxMsg_s msg;
   IFX_uint32_t bytes_read = 0;
   IFXOS_INTSTAT flags;
   IFX_int32_t ret;

   IFX_MPS_UNUSED(dummy);

   /* set pointer to data upstream mailbox, no matter if 0,1,2 or 3 because
      they point to the same shared mailbox memory */
   mbx = &pMPSDev->voice_upstrm_fifo;
   while (ifx_mps_fifo_not_empty (mbx))
   {
      IFXOS_LOCKINT (flags);
      channel = ifx_mps_mbx_get_message_channel (mbx);

      /* select mailbox device structure acc. to channel ID read from current
         msg */
      switch (channel)
      {
         case voice0:
            mbx_dev = (mps_mbx_dev *) & (pMPSDev->voice_mb[0]);
            break;
         case voice1:
            mbx_dev = (mps_mbx_dev *) & (pMPSDev->voice_mb[1]);
            break;
         case voice2:
            mbx_dev = (mps_mbx_dev *) & (pMPSDev->voice_mb[2]);
            break;
         case voice3:
            mbx_dev = (mps_mbx_dev *) & (pMPSDev->voice_mb[3]);
            break;
         case voice4:
            mbx_dev = (mps_mbx_dev *) & (pMPSDev->voice_mb[4]);
            break;
         case voice5:
            mbx_dev = (mps_mbx_dev *) & (pMPSDev->voice_mb[5]);
            break;
         case voice6:
            mbx_dev = (mps_mbx_dev *) & (pMPSDev->voice_mb[6]);
            break;
         case voice7:
            mbx_dev = (mps_mbx_dev *) & (pMPSDev->voice_mb[7]);
            break;
         default:
            TRACE (MPS, DBG_LEVEL_HIGH,
                   ("%s(): Invalid channel ID %d read from mailbox\n",
                    __FUNCTION__, channel));
            IFXOS_UNLOCKINT (flags);
            return;
      }

#ifdef CONFIG_PROC_FS
      if (mbx->min_space > ifx_mps_fifo_mem_available (mbx))
         mbx->min_space = ifx_mps_fifo_mem_available (mbx);
#endif /* */

      /* read message header from buffer */
      ret = ifx_mps_fifo_read (mbx, 0, &msg.header.val);
      if (ret == IFX_ERROR)     /* fifo error (empty) */
         return;

#ifdef CONFIG_PROC_FS
      mbx->pkts++;
      mbx->bytes += msg.header.hd.plength + 4;
#endif /* */
      if (msg.header.hd.type == CMD_ADDRESS_PACKET)
      {
         IFX_int32_t i;
         ifx_mps_mbx_read_message (mbx, &msg, &bytes_read);
         for (i = 0; i < (msg.header.hd.plength / 4 - 1); i++)
         {
            mps_buffer.free ((IFX_void_t *) KSEG0ADDR (msg.data[i]));
         }
         IFXOS_UNLOCKINT (flags);
         continue;
      }
      else
      {
         /* discard packet in case no one is listening... */
         if (mbx_dev->Installed == IFX_FALSE)
         {
          data_upstream_discard:
            ifx_mps_bufman_dec_level (1);
            ifx_mps_mbx_read_message (mbx, &msg, &bytes_read);
            ifx_mps_bufman_free ((IFX_void_t *)
                                 KSEG0ADDR ((IFX_uint8_t *) msg.data[0]));
            mbx_dev->upstrm_fifo->discards++;
            IFXOS_UNLOCKINT (flags);
            continue;
         }

         if (mbx_dev->up_callback != IFX_NULL)
         {
            ifx_mps_bufman_dec_level (1);
            if ((ifx_mps_bufman_get_level () <= mps_buffer.buf_threshold)
#ifdef LINUX
                 &&
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,26))
                (atomic_read (&pMPSDev->provide_buffer->object.count) == 0)
#else
                ((volatile unsigned int)pMPSDev->provide_buffer->object.count == 0)
#endif
#endif /* LINUX */
            )
            {
               IFXOS_LockRelease (pMPSDev->provide_buffer);
            }

            /* use callback function to notify about data reception */
            mbx_dev->up_callback (channel);
            IFXOS_UNLOCKINT (flags);
            continue;
         }
         else
         {
            IFX_int32_t i, msg_bytes;

            msg_bytes = (msg.header.hd.plength + 4);
            if (ifx_mps_fifo_mem_request (mbx_dev->upstrm_fifo, msg_bytes) !=
                IFX_TRUE)
            {
               goto data_upstream_discard;
            }

            /* Copy message into sw fifo */
            for (i = 0; i < msg_bytes; i += 4)
            {
               IFX_uint32_t data;

               ret = ifx_mps_fifo_read (mbx, (IFX_uint8_t) i, &data);
               if (ret == IFX_ERROR)
                  return;
               ifx_mps_fifo_write (mbx_dev->upstrm_fifo, data, (IFX_uint8_t) i);
            }
            ifx_mps_fifo_read_ptr_inc (mbx, msg_bytes);
            ifx_mps_fifo_write_ptr_inc (mbx_dev->upstrm_fifo, msg_bytes);

#ifdef CONFIG_PROC_FS
            if (mbx_dev->upstrm_fifo->min_space >
                ifx_mps_fifo_mem_available (mbx_dev->upstrm_fifo))
               mbx_dev->upstrm_fifo->min_space =
                  ifx_mps_fifo_mem_available (mbx_dev->upstrm_fifo);
            mbx_dev->upstrm_fifo->pkts++;
#endif /* CONFIG_PROC_FS */
            ifx_mps_bufman_dec_level (1);
            if ((ifx_mps_bufman_get_level () <= mps_buffer.buf_threshold)
#ifdef LINUX
                &&
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,26))
                (atomic_read (&pMPSDev->provide_buffer->object.count) == 0)
#else
                (pMPSDev->provide_buffer->object.count == 0)
#endif
#endif /* LINUX */
            )
            {
               IFXOS_LockRelease (pMPSDev->provide_buffer);
            }

            /* use queue wake up to notify about data reception */
            IFXOS_DrvSelectQueueWakeUp (&(mbx_dev->mps_wakeuplist), 0);
            IFXOS_UNLOCKINT (flags);
         }
      }
   }
   return;
}


/**
 * Notify queue about upstream command reception
 * This function checks the channel identifier included in the header
 * of the message currently pointed to by the upstream command mailbox's
 * read pointer. It wakes up the related queue to read the received command
 * message out of the mailbox for further processing. The process is repeated
 * as long as upstream messages are avaiilable in the mailbox.
 * The function is attached to the driver's poll/select functionality.
 *
 * \param   dummy    Tasklet parameter, not used.
 * \ingroup Internal
 */
IFX_void_t ifx_mps_mbx_cmd_upstream (IFX_ulong_t dummy)
{
   mps_fifo *mbx;
   IFXOS_INTSTAT flags;

   IFX_MPS_UNUSED(dummy);

   /* set pointer to upstream command mailbox */
   mbx = &(pMPSDev->cmd_upstrm_fifo);
   IFXOS_LOCKINT (flags);
   if (ifx_mps_fifo_not_empty (mbx))
   {
#ifdef CONFIG_PROC_FS
      if (mbx->min_space > ifx_mps_fifo_mem_available (mbx))
         mbx->min_space = ifx_mps_fifo_mem_available (mbx);
#endif /* */
      if (pMPSDev->command_mb.Installed == IFX_FALSE)
      {
         /* TODO: What to do with this?? */
         TRACE (MPS, DBG_LEVEL_HIGH,
                ("%s() - wheee, unmaintained command message...\n",
                 __FUNCTION__));
      }
      if (pMPSDev->command_mb.up_callback != IFX_NULL)
      {
         pMPSDev->command_mb.up_callback (command);
      }
      else
      {
         /* wake up sleeping process for further processing of received command
          */
         IFXOS_DrvSelectQueueWakeUp (&(pMPSDev->command_mb.mps_wakeuplist), 0);
      }
   }
   IFXOS_UNLOCKINT (flags);
   return;
}


#ifdef CONFIG_MPS_EVENT_MBX
/**
 * Notify queue about upstream event reception
 * The function will deliver an incoming event to the registered handler.
 *
 * \param   dummy    Tasklet parameter, not used.
 * \ingroup Internal
 */
IFX_void_t ifx_mps_mbx_event_upstream (IFX_ulong_t dummy)
{
   mps_fifo *mbx;
   mps_event_msg msg;
   IFX_int32_t length = 0;
   IFX_uint32_t read_length = 0;
   IFXOS_INTSTAT flags;

   IFX_MPS_UNUSED(dummy);

   /* set pointer to upstream event mailbox */
   mbx = &(pMPSDev->event_upstrm_fifo);
   IFXOS_LOCKINT (flags);
   while (ifx_mps_fifo_not_empty (mbx))
   {
      length = ifx_mps_mbx_get_message_length (mbx);
#ifdef TEST_EVT_DISCARD
      /* VoIP firmware test feature - if there is no space in event sw fifo,
         discard the event */
      if (length >= ifx_mps_fifo_mem_available (&pMPSDev->sw_event_upstrm_fifo))
      {
         IFXOS_UNLOCKINT (flags);
         return;
      }
#endif /* TEST_EVT_DISCARD */
      if (length > sizeof (mps_event_msg))
      {
         TRACE (MPS, DBG_LEVEL_HIGH,
                ("MPS Event message too large for buffer. Skipping\n"));
         ifx_mps_fifo_read_ptr_inc (mbx, length);
         IFXOS_UNLOCKINT (flags);
         return;
      }
      ifx_mps_mbx_read_message (mbx, (MbxMsg_s *) & msg, &read_length);
	  /* debug code */
      if ((msg.data[0] & 0xfff0ffff) == 0x09000404)
      {
         IFX_uint8_t ch = (msg.data[0] & 0x000f0000) >> 16;

         dchan_evt_cnt[ch]++;
      }
      /* end of debug code */
      if (pMPSDev->event_mbx.event_mbx_callback != IFX_NULL)
      {
         pMPSDev->event_mbx.event_mbx_callback (pMPSDev->event_mbx.
                                                event_callback_handle, &msg);
      }
      else
      {
         if (ifx_mps_fifo_write (&pMPSDev->sw_event_upstrm_fifo, msg.data[0], 0)
             == -1)
         {
            IFXOS_UNLOCKINT (flags);
            return;
         }
         if (ifx_mps_fifo_write (&pMPSDev->sw_event_upstrm_fifo, msg.data[1], 4)
             == -1)
         {
            IFXOS_UNLOCKINT (flags);
            return;
         }
         ifx_mps_fifo_write_ptr_inc (&pMPSDev->sw_event_upstrm_fifo, length);

         /* wake up sleeping process for further processing of received event */
         IFXOS_DrvSelectQueueWakeUp (&(pMPSDev->event_mbx.mps_wakeuplist), 0);
      }
   }
   IFXOS_UNLOCKINT (flags);
   return;
}

IFX_int32_t ifx_mps_event_mbx_activation_poll (IFX_int32_t value)
{
   MPS_Ad0Reg_u Ad0Reg;

   /* Enable necessary MPS interrupts */
   Ad0Reg.val = *IFX_MPS_AD0ENR;
   Ad0Reg.fld.evt_mbx = value;
   *IFX_MPS_AD0ENR = Ad0Reg.val;
   return (IFX_SUCCESS);
}


#endif /* CONFIG_MPS_EVENT_MBX */

/**
 * Change event interrupt activation.
 * Allows the upper layer enable or disable interrupt generation of event previously
 * registered. Note that
 *
 * \param   type   DSP device entity ( 1 - command, 2 - voice0, 3 - voice1,
 *                 4 - voice2, 5 - voice3, 6 - voice4 )
 * \param   act    Register values according to MbxEvent_Regs, whereas bit=1 means
 *                 active, bit=0 means inactive
 * \return  0      IFX_SUCCESS, interrupt masked changed accordingly
 * \return  ENXIO  Wrong DSP device entity (only 1-5 supported)
 * \return  EINVAL Callback value null
 * \ingroup API
 */
IFX_int32_t ifx_mps_event_activation_poll (mps_devices type,
                                           MbxEventRegs_s * act)
{
   mps_mbx_dev *pMBDev;
   MPS_Ad0Reg_u Ad0Reg;
   MPS_Ad1Reg_u Ad1Reg;
   MPS_VCStatReg_u VCStatReg;
   IFX_int32_t i;

   /* Get corresponding mailbox device structure */
   if ((pMBDev = ifx_mps_get_device (type)) == 0)
      return (-ENXIO);

   /* Enable necessary MPS interrupts */
   Ad0Reg.val = *IFX_MPS_AD0ENR;
   Ad0Reg.val =
      (Ad0Reg.val & ~pMBDev->event_mask.MPS_Ad0Reg.val) | (act->MPS_Ad0Reg.
                                                           val & pMBDev->
                                                           event_mask.
                                                           MPS_Ad0Reg.val);
   *IFX_MPS_AD0ENR = Ad0Reg.val;

   atomic_set (&ifx_mps_dd_mbx_int_enabled, Ad0Reg.fld.dd_mbx);

   Ad1Reg.val = *IFX_MPS_AD1ENR;
   Ad1Reg.val =
      (Ad1Reg.val & ~pMBDev->event_mask.MPS_Ad1Reg.val) | (act->MPS_Ad1Reg.
                                                           val & pMBDev->
                                                           event_mask.
                                                           MPS_Ad1Reg.val);
   *IFX_MPS_AD1ENR = Ad1Reg.val;
   for (i = 0; i < 4; i++)
   {
      VCStatReg.val = IFX_MPS_VC0ENR[i];
      VCStatReg.val =
         (VCStatReg.val & ~pMBDev->event_mask.MPS_VCStatReg[i].val) | (act->
                                                                       MPS_VCStatReg
                                                                       [i].
                                                                       val &
                                                                       pMBDev->
                                                                       event_mask.
                                                                       MPS_VCStatReg
                                                                       [i].val);
      IFX_MPS_VC0ENR[i] = VCStatReg.val;
   }
   return (IFX_SUCCESS);
}


/**
 * Change event interrupt activation.
 * Allows the upper layer enable or disable interrupt generation of event previously
 * registered. Note that
 *
 * \param   type   DSP device entity ( 1 - command, 2 - voice0, 3 - voice1,
 *                 4 - voice2, 5 - voice3 )
 * \param   act    Register values according to MbxEvent_Regs, whereas bit=1 means
 *                 active, bit=0 means skip
 * \return  0      IFX_SUCCESS, interrupt masked changed accordingly
 * \return  ENXIO  Wrong DSP device entity (only 1-5 supported)
 * \return  EINVAL Callback value null
 * \ingroup API
 */
IFX_int32_t ifx_mps_event_activation (mps_devices type, MbxEventRegs_s * act)
{
   mps_mbx_dev *pMBDev;
   MPS_Ad0Reg_u Ad0Reg;
   MPS_Ad1Reg_u Ad1Reg;
   MPS_VCStatReg_u VCStatReg;
   IFX_int32_t i;

   /* Get corresponding mailbox device structure */
   if ((pMBDev = ifx_mps_get_device (type)) == 0)
      return (-ENXIO);

   /* Enable necessary MPS interrupts */
   Ad0Reg.val = *IFX_MPS_AD0ENR;
   Ad0Reg.val =
      (Ad0Reg.val & ~pMBDev->callback_event_mask.MPS_Ad0Reg.val) | (act->
                                                                    MPS_Ad0Reg.
                                                                    val &
                                                                    pMBDev->
                                                                    callback_event_mask.
                                                                    MPS_Ad0Reg.
                                                                    val);
   *IFX_MPS_AD0ENR = Ad0Reg.val;

   atomic_set (&ifx_mps_dd_mbx_int_enabled, Ad0Reg.fld.dd_mbx);

   Ad1Reg.val = *IFX_MPS_AD1ENR;
   Ad1Reg.val =
      (Ad1Reg.val & ~pMBDev->callback_event_mask.MPS_Ad1Reg.val) | (act->
                                                                    MPS_Ad1Reg.
                                                                    val &
                                                                    pMBDev->
                                                                    callback_event_mask.
                                                                    MPS_Ad1Reg.
                                                                    val);
   *IFX_MPS_AD1ENR = Ad1Reg.val;
   for (i = 0; i < 4; i++)
   {
      VCStatReg.val = IFX_MPS_VC0ENR[i];
      VCStatReg.val =
         (VCStatReg.val & ~pMBDev->callback_event_mask.MPS_VCStatReg[i].
          val) | (act->MPS_VCStatReg[i].val & pMBDev->callback_event_mask.
                  MPS_VCStatReg[i].val);
      IFX_MPS_VC0ENR[i] = VCStatReg.val;
   }
   return (IFX_SUCCESS);
}


/**
   This function enables mailbox interrupts on Danube.
\param
   None.
\return
   None.
*/
IFX_void_t ifx_mps_enable_mailbox_int ()
{
   MPS_Ad0Reg_u Ad0Reg;

   Ad0Reg.val = *IFX_MPS_AD0ENR;
   Ad0Reg.fld.cu_mbx = 1;
   Ad0Reg.fld.du_mbx = 1;
   Ad0Reg.fld.dl_end = 1;
#if defined(SYSTEM_DANUBE)
   Ad0Reg.fld.wd_fail = 1;
#endif

   *IFX_MPS_AD0ENR = Ad0Reg.val;
}

/**
   This function disables mailbox interrupts on Danube.
\param
   None.
\return
   None.
*/
IFX_void_t ifx_mps_disable_mailbox_int ()
{
   MPS_Ad0Reg_u Ad0Reg;

   Ad0Reg.val = *IFX_MPS_AD0ENR;
   Ad0Reg.fld.cu_mbx = 0;
   Ad0Reg.fld.du_mbx = 0;
   *IFX_MPS_AD0ENR = Ad0Reg.val;
}

/**
   This function enables dd_mbx interrupts on Danube.
\param
   None.
\return
   None.
*/
IFX_void_t ifx_mps_dd_mbx_int_enable (void)
{
   IFXOS_INTSTAT flags;
   MPS_Ad0Reg_u Ad0Reg;

   IFXOS_LOCKINT (flags);

   if (atomic_read (&ifx_mps_dd_mbx_int_enabled) == 0)
   {
      Ad0Reg.val = *IFX_MPS_AD0ENR;
      Ad0Reg.fld.dd_mbx = 1;
      *IFX_MPS_AD0ENR = Ad0Reg.val;
   }

   atomic_inc (&ifx_mps_dd_mbx_int_enabled);

   IFXOS_UNLOCKINT (flags);
}

/**
   This function disables dd_mbx interrupts on Danube.
\param
   None.
\return
   None.
*/
IFX_void_t ifx_mps_dd_mbx_int_disable (void)
{
   IFXOS_INTSTAT flags;
   MPS_Ad0Reg_u Ad0Reg;

   IFXOS_LOCKINT (flags);

   if (atomic_read (&ifx_mps_dd_mbx_int_enabled) > 0)
   {
      atomic_dec (&ifx_mps_dd_mbx_int_enabled);

      if (atomic_read (&ifx_mps_dd_mbx_int_enabled) == 0)
      {
         Ad0Reg.val = *IFX_MPS_AD0ENR;
         Ad0Reg.fld.dd_mbx = 0;
         *IFX_MPS_AD0ENR = Ad0Reg.val;
      }
   }

   IFXOS_UNLOCKINT (flags);
}

/**
   This function disables all MPS interrupts on Danube.
\param
   None.
\return
   None.
*/
IFX_void_t ifx_mps_disable_all_int ()
{
   *IFX_MPS_SAD0SR = 0x00000000;
}

/******************************************************************************
 * Interrupt service routines
 ******************************************************************************/

/**
 * Upstream data interrupt handler
 * This function is called on occurence of an data upstream interrupt.
 * Depending on the occured interrupt either the command or data upstream
 * message processing is started via tasklet
 *
 * \param   irq      Interrupt number
 * \param   pDev     Pointer to MPS communication device structure
 * \ingroup Internal
 */
irqreturn_t ifx_mps_ad0_irq (IFX_int32_t irq, mps_comm_dev * pDev)
{
   MbxEventRegs_s events;
   MPS_Ad0Reg_u MPS_Ad0StatusReg;
   mps_mbx_dev *mbx_dev = (mps_mbx_dev *) & (pMPSDev->command_mb);

   IFX_MPS_UNUSED(pDev);

   /* read interrupt status */
   MPS_Ad0StatusReg.val = *IFX_MPS_RAD0SR;
   /* acknowledge interrupt */
   *IFX_MPS_CAD0SR = MPS_Ad0StatusReg.val;
   /* handle only enabled interrupts */
   MPS_Ad0StatusReg.val &= *IFX_MPS_AD0ENR;

#if !defined(SYSTEM_FALCON)
#if   (BSP_API_VERSION == 1)
   mask_and_ack_danube_irq (irq);
#elif (BSP_API_VERSION == 2)
   bsp_mask_and_ack_irq (irq);
#endif
#endif /* !defined(SYSTEM_FALCON) */

   /* FW is up and ready to process commands */
   if (MPS_Ad0StatusReg.fld.dl_end)
   {
      IFXOS_EventWakeUp (&fw_ready_evt);
   }

#if defined(SYSTEM_DANUBE)
   /* watchdog timer expiration */
   if (MPS_Ad0StatusReg.fld.wd_fail)
   {
      ifx_mps_wdog_expiry();
   }
#endif

#ifdef PRINT_ON_ERR_INTERRUPT
   if (MPS_Ad0StatusReg.fld.data_err)
   {
      TRACE (MPS, DBG_LEVEL_HIGH, ("\n%s() - data_err\n", __FUNCTION__));
   }

   if (MPS_Ad0StatusReg.fld.cmd_err)
   {
      TRACE (MPS, DBG_LEVEL_HIGH, ("\n%s() - cmd_err\n", __FUNCTION__));
   }

   if (MPS_Ad0StatusReg.fld.pcm_crash)
   {
      TRACE (MPS, DBG_LEVEL_HIGH, ("\n%s() - pcm_crash\n", __FUNCTION__));
   }

   if (MPS_Ad0StatusReg.fld.mips_ol)

   {
      TRACE (MPS, DBG_LEVEL_HIGH, ("\n%s() - mips_ol\n", __FUNCTION__));
   }

   if (MPS_Ad0StatusReg.fld.evt_ovl)
   {
      TRACE (MPS, DBG_LEVEL_HIGH, ("\n%s() - evt_ovl\n", __FUNCTION__));
   }

   if (MPS_Ad0StatusReg.fld.rcv_ov)
   {
      TRACE (MPS, DBG_LEVEL_HIGH, ("\n%s() - rcv_ov\n", __FUNCTION__));
   }
#endif /* */

   if (MPS_Ad0StatusReg.fld.dd_mbx)
   {
      if (atomic_read (&ifx_mps_write_blocked) != 0)
      {
         if (ifx_mps_bufman_buf_provide
             (MPS_DEFAULT_PROVISION_SEGMENTS_PER_MSG,
              mps_buffer.buf_size) == IFX_SUCCESS)
         {
            /* mark interrupt as handled, skip handling on hight level */
            MPS_Ad0StatusReg.fld.dd_mbx = 0;
         }
      }
   }

   if (MPS_Ad0StatusReg.fld.du_mbx)
   {
#ifdef CONFIG_PROC_FS
      pMPSDev->voice_mb[0].upstrm_fifo->pkts++;
#endif /* CONFIG_PROC_FS */
      ifx_mps_mbx_data_upstream (0);
   }

   if (MPS_Ad0StatusReg.fld.cu_mbx)
   {
#ifdef CONFIG_PROC_FS
      pMPSDev->command_mb.upstrm_fifo->pkts++;
#endif /* CONFIG_PROC_FS */
      ifx_mps_mbx_cmd_upstream (0);
   }

#ifdef CONFIG_MPS_EVENT_MBX
   if (MPS_Ad0StatusReg.fld.evt_mbx)
   {
      ifx_mps_mbx_event_upstream (0);
   }
#endif /* CONFIG_MPS_EVENT_MBX */

#if CONFIG_MPS_HISTORY_SIZE > 0
   if (MPS_Ad0StatusReg.fld.cmd_err)
   {
      ifx_mps_history_buffer_freeze = 1;
      TRACE (MPS, DBG_LEVEL_HIGH, ("MPS cmd_err interrupt!\n"));
   }
#endif /* */
   pMPSDev->event.MPS_Ad0Reg.val =
      MPS_Ad0StatusReg.val | (pMPSDev->event.MPS_Ad0Reg.val & mbx_dev->
                              event_mask.MPS_Ad0Reg.val);

   /* use callback function or queue wake up to notify about data reception */
   if (mbx_dev->event_callback != IFX_NULL)
   {
      if (mbx_dev->callback_event_mask.MPS_Ad0Reg.val & MPS_Ad0StatusReg.val)
      {
         events.MPS_Ad0Reg.val = MPS_Ad0StatusReg.val;
         /* pass only requested bits */
         events.MPS_Ad0Reg.val &= mbx_dev->callback_event_mask.MPS_Ad0Reg.val;

         events.MPS_Ad1Reg.val = 0;
         events.MPS_VCStatReg[0].val = 0;
         events.MPS_VCStatReg[1].val = 0;
         events.MPS_VCStatReg[2].val = 0;
         events.MPS_VCStatReg[3].val = 0;

         mbx_dev->event_callback (&events);
      }
   }

   if (mbx_dev->event_mask.MPS_Ad0Reg.val & MPS_Ad0StatusReg.val)
   {
      IFXOS_DrvSelectQueueWakeUp (&(mbx_dev->mps_wakeuplist), 0);
   }

   return IRQ_HANDLED;
}


/**
 * Upstream data interrupt handler
 * This function is called on occurence of an data upstream interrupt.
 * Depending on the occured interrupt either the command or data upstream
 * message processing is started via tasklet
 *
 * \param   irq      Interrupt number
 * \param   pDev     Pointer to MPS communication device structure
 * \ingroup Internal
 */
irqreturn_t ifx_mps_ad1_irq (IFX_int32_t irq, mps_comm_dev * pDev)
{
   MbxEventRegs_s events;
   MPS_Ad1Reg_u MPS_Ad1StatusReg;
   mps_mbx_dev *mbx_dev = (mps_mbx_dev *) & (pMPSDev->command_mb);

   IFX_MPS_UNUSED(pDev);

   /* read interrupt status */
   MPS_Ad1StatusReg.val = *IFX_MPS_RAD1SR;
   /* acknowledge interrupt */
   *IFX_MPS_CAD1SR = MPS_Ad1StatusReg.val;
   /* handle only enabled interrupts */
   MPS_Ad1StatusReg.val &= *IFX_MPS_AD1ENR;

#if !defined(SYSTEM_FALCON)
#if   (BSP_API_VERSION == 1)
   mask_and_ack_danube_irq (irq);
#elif (BSP_API_VERSION == 2)
   bsp_mask_and_ack_irq (irq);
#endif
#endif /* !defined(SYSTEM_FALCON) */

   pMPSDev->event.MPS_Ad1Reg.val = MPS_Ad1StatusReg.val;

   /* use callback function or queue wake up to notify about data reception */
   if (mbx_dev->event_callback != IFX_NULL)
   {
      if (mbx_dev->callback_event_mask.MPS_Ad1Reg.val & MPS_Ad1StatusReg.val)
      {
         events.MPS_Ad0Reg.val = 0;

         events.MPS_Ad1Reg.val = MPS_Ad1StatusReg.val;
         /* pass only requested bits */
         events.MPS_Ad1Reg.val &= mbx_dev->callback_event_mask.MPS_Ad1Reg.val;

         events.MPS_VCStatReg[0].val = 0;
         events.MPS_VCStatReg[1].val = 0;
         events.MPS_VCStatReg[2].val = 0;
         events.MPS_VCStatReg[3].val = 0;

         mbx_dev->event_callback (&events);
      }
   }

   if (mbx_dev->event_mask.MPS_Ad1Reg.val & MPS_Ad1StatusReg.val)
   {
      IFXOS_DrvSelectQueueWakeUp (&(mbx_dev->mps_wakeuplist), 0);
   }

   return IRQ_HANDLED;
}


/**
 * Voice channel status interrupt handler
 * This function is called on occurence of an status interrupt.
 *
 * \param   irq      Interrupt number
 * \param   pDev     Pointer to MPS communication device structure
 * \ingroup Internal
 */
irqreturn_t ifx_mps_vc_irq (IFX_int32_t irq, mps_comm_dev * pDev)
{
   IFX_uint32_t chan = irq - INT_NUM_IM4_IRL14;
   mps_mbx_dev *mbx_dev = (mps_mbx_dev *) & (pMPSDev->voice_mb[chan]);
   MPS_VCStatReg_u MPS_VCStatusReg;
   MbxEventRegs_s events;

   IFX_MPS_UNUSED(pDev);

   /* read interrupt status */
   MPS_VCStatusReg.val = IFX_MPS_RVC0SR[chan];
   /* acknowledge interrupt */
   IFX_MPS_CVC0SR[chan] = MPS_VCStatusReg.val;
   /* handle only enabled interrupts */
   MPS_VCStatusReg.val &= IFX_MPS_VC0ENR[chan];

#if !defined(SYSTEM_FALCON)
#if   (BSP_API_VERSION == 1)
   mask_and_ack_danube_irq (irq);
#elif (BSP_API_VERSION == 2)
   bsp_mask_and_ack_irq (irq);
#endif
#endif /* !defined(SYSTEM_FALCON) */

   pMPSDev->event.MPS_VCStatReg[chan].val = MPS_VCStatusReg.val;
#ifdef PRINT_ON_ERR_INTERRUPT
   if (MPS_VCStatusReg.fld.rcv_ov)
   {
      TRACE (MPS, DBG_LEVEL_HIGH,
             ("\n%s() - rcv_ov chan=%d\n", __FUNCTION__, chan));
   }

   if (MPS_VCStatusReg.fld.vpou_jbl)
   {
      TRACE (MPS, DBG_LEVEL_HIGH,
             ("\n%s() - vpou_jbl chan=%d\n", __FUNCTION__, chan));
   }

   if (MPS_VCStatusReg.fld.vpou_jbh)
   {
      TRACE (MPS, DBG_LEVEL_HIGH,
             ("\n%s() - vpou_jbh chan=%d\n", __FUNCTION__, chan));
   }

   if (MPS_VCStatusReg.fld.epq_st)
   {
      TRACE (MPS, DBG_LEVEL_HIGH,
             ("\n%s() - epq_st chan=%d\n", __FUNCTION__, chan));
   }

   if (MPS_VCStatusReg.fld.vpou_st)
   {
      TRACE (MPS, DBG_LEVEL_HIGH,
             ("\n%s() - vpou_st chan=%d\n", __FUNCTION__, chan));
   }

   if (MPS_VCStatusReg.fld.lin_req)
   {
      TRACE (MPS, DBG_LEVEL_HIGH,
             ("\n%s() - lin_req chan=%d\n", __FUNCTION__, chan));
   }

   if (MPS_VCStatusReg.fld.dec_chg)
   {
      TRACE (MPS, DBG_LEVEL_HIGH,
             ("\n%s() - dec_chg chan=%d\n", __FUNCTION__, chan));
   }
#endif /* */

   /* use callback function or queue wake up to notify about data reception */
   if (mbx_dev->event_callback != IFX_NULL)
   {
      if (mbx_dev->callback_event_mask.MPS_VCStatReg[chan].
          val & MPS_VCStatusReg.val)
      {
         events.MPS_Ad0Reg.val = 0;
         events.MPS_Ad1Reg.val = 0;
         events.MPS_VCStatReg[0].val = 0;
         events.MPS_VCStatReg[1].val = 0;
         events.MPS_VCStatReg[2].val = 0;
         events.MPS_VCStatReg[3].val = 0;

         events.MPS_VCStatReg[chan].val = MPS_VCStatusReg.val;
         /* pass only requested bits */
         events.MPS_VCStatReg[chan].val &=
            mbx_dev->callback_event_mask.MPS_VCStatReg[chan].val;

         mbx_dev->event_callback (&events);
      }
   }

   if (mbx_dev->event_mask.MPS_VCStatReg[chan].val & MPS_VCStatusReg.val)
   {
      IFXOS_DrvSelectQueueWakeUp (&(mbx_dev->mps_wakeuplist), 0);
   }

   return IRQ_HANDLED;
}


/**
 * Print firmware version.
 * This function queries the current firmware version and prints it.
 *
 * \return  0        IFX_SUCCESS
 * \return  -EFAULT  Error while fetching version
 * \ingroup Internal
 */
IFX_int32_t ifx_mps_get_fw_version (IFX_int32_t print)
{
   MbxMsg_s msg;
   MbxMsg_s msg2;
   IFX_uint32_t *ptmp;
   mps_fifo *fifo;
   IFX_int32_t timeout = 300;   /* 3s timeout */
   IFX_int32_t retval;
   IFX_uint32_t bytes_read = 0;

   fifo = &(ifx_mps_dev.cmd_upstrm_fifo);
   /* build message */
   ptmp = (IFX_uint32_t *) & msg;
   ptmp[0] = 0x8600e604;
   ptmp[1] = 0x00000000;

   /* send message */
   retval =
      ifx_mps_mbx_write_message (&(ifx_mps_dev.command_mb),
                                 (IFX_uint8_t *) & msg, 4);
   while (!ifx_mps_fifo_not_empty (fifo) && timeout > 0)
   {
      /* Sleep for 10ms */
      IFXOS_MSecSleep(10);
      timeout--;
   }
   if (timeout == 0)
   {
      TRACE (MPS, DBG_LEVEL_HIGH,
             ("[%s %s %d]: timeout\n", __FILE__, __func__, __LINE__));
      return -EFAULT;
   }
   retval = ifx_mps_mbx_read_message (fifo, &msg2, &bytes_read);
   if ((retval != 0) || (bytes_read != 8))
   {
      TRACE (MPS, DBG_LEVEL_HIGH,
             ("[%s %s %d]: error!\n", __FILE__, __func__, __LINE__));
      return -EFAULT;
   }
   ptmp = (IFX_uint32_t *) & msg2;

   if (print)
      TRACE (MPS, DBG_LEVEL_HIGH,
             ("ok!\nVersion %d.%d.%d.%d.%d up and running...\n",
              (ptmp[1] >> 24) & 0xff, (ptmp[1] >> 16) & 0xff,
              (ptmp[1] >> 12) & 0xf, (ptmp[1] >> 8) & 0xf, (ptmp[1] & 0xff)));

   return 0;
}

/**
 * Initialize Timer
 * This function initializes the general purpose timer, which
 * is used by the voice firmware.
 *
 * \ingroup Internal
 */
IFX_return_t ifx_mps_init_gpt ()
{
#if !defined(SYSTEM_FALCON)
   IFX_uint32_t timer_flags, timer, loops = 0;
   IFXOS_INTSTAT flags;
   IFX_ulong_t count;
#if defined(SYSTEM_AR9) || defined(SYSTEM_VR9) || defined(SYSTEM_XRX300)
   timer = TIMER1A;
#else /* Danube */
   timer = TIMER1B;
#endif /* SYSTEM_AR9 || SYSTEM_VR9 || SYSTEM_XRX300 */
#endif

#if defined(SYSTEM_FALCON)
   sys_hw_setup ();
#else

   /* calibration loop - required to syncronize GPTC interrupt with falling
      edge of FSC clock */
   timer_flags =
      TIMER_FLAG_16BIT | TIMER_FLAG_COUNTER | TIMER_FLAG_CYCLIC |
      TIMER_FLAG_DOWN | TIMER_FLAG_FALL_EDGE | TIMER_FLAG_SYNC
#if defined(__KERNEL__)
      | TIMER_FLAG_CALLBACK_IN_IRQ
#endif /* defined(__KERNEL__) */
      ;
   if (0 >
       ifx_gptu_timer_request (timer, timer_flags, 1, (IFX_ulong_t) IFX_NULL,
                               (IFX_ulong_t) IFX_NULL))
   {
      TRACE (MPS, DBG_LEVEL_HIGH,
             (KERN_ERR "[%s %s %d]: failed to reserve GPTC timer. \
Probably already in use.\r\n", __FILE__, __func__, __LINE__));
      return IFX_ERROR;
   }
   if (0 > ifx_gptu_timer_start (timer, 0))
   {
      TRACE (MPS, DBG_LEVEL_HIGH,
             (KERN_ERR "[%s %s %d]: failed to start GPTC timer.\r\n", __FILE__,
              __func__, __LINE__));
      return IFX_ERROR;
   }
   do
   {
      loops++;
      ifx_gptu_countvalue_get (timer, &count);
   } while (count);

   IFXOS_LOCKINT (flags);
/* temporary workaround - to be removed */
#if defined(SYSTEM_AR9) || defined(SYSTEM_VR9) || defined(SYSTEM_XRX300)
   /* CON_1_A register */
   *((volatile IFX_uint32_t *) (KSEG1 + 0x1e100a00 + 0x0010)) = 0x000005c5;
#else /* Danube */
   /* CON_1_B register */
   *((volatile IFX_uint32_t *) (KSEG1 + 0x1e100a00 + 0x0014)) = 0x000005c5;
#endif /* SYSTEM_AR9 || SYSTEM_VR9 || SYSTEM_XRX300*/

#ifdef DEBUG
   TRACE (MPS, DBG_LEVEL_HIGH,
          ("%s() - calibration done, waited %i loops!\n", __func__, loops));
#endif /* DEBUG */

   IFXOS_UNLOCKINT (flags);

#endif
   return IFX_SUCCESS;
}

/**
 * Stop timer
 * This function frees the general purpose timer, which
 * is used by the voice firmware.
 * \param   none
 * \return  none
 *
 * \ingroup Internal
 */
IFX_void_t ifx_mps_shutdown_gpt (void)
{
#if defined(SYSTEM_FALCON)
   sys1_hw_deactivate (ACTS_MPS);
#else
   IFX_uint32_t timer;
#if defined(SYSTEM_AR9) || defined(SYSTEM_VR9) || defined(SYSTEM_XRX300)
   timer = TIMER1A;
#else /* Danube */
   timer = TIMER1B;
#endif /* SYSTEM_AR9 || SYSTEM_VR9 || SYSTEM_XRX300 */
   ifx_gptu_timer_free (timer);
#endif
}

/**
 * Register callback function to free all data buffers currently used by
 * voice firmware. Called by VMMC driver.
 * \param   none
 * \return  none
 *
 * \ingroup Internal
 */
IFX_void_t ifx_mps_register_bufman_freeall_callback(IFX_void_t (*pfn)(void))
{
   ifx_mps_bufman_freeall = pfn;
}

/**
 * Calculate CRC-32 checksum on voice EDSP firmware image located
 * at cpu1_base addr.
 *
 * \param   l_cpu1_base_addr Base address of CPU1, obtained from BSP.
 * \param   l_pFW_img_data   ptr to FW image metadata such as range
 *                           for checksum calculation etc.
 *
 * \return  crc32            The CRC-32 checksum.
 *
 * \ingroup Internal
 */
IFX_uint32_t ifx_mps_fw_crc32(volatile IFX_uint32_t *l_cpu1_base_addr,
                                   FW_image_ftr_t *l_pFW_img_data)
{
   IFX_uint32_t crc32;

   crc32 = ifx_mps_calc_crc32(0xffffffff, (IFX_uint8_t *)l_cpu1_base_addr,
                              2*sizeof(IFX_uint32_t), IFX_FALSE);
   crc32 = ifx_mps_calc_crc32(~crc32, ((IFX_uint8_t *)l_cpu1_base_addr) +
                                                      l_pFW_img_data->st_addr_crc,
                              l_pFW_img_data->en_addr_crc -
                                                      l_pFW_img_data->st_addr_crc,
                              IFX_TRUE);
   return crc32;
}

/**
 * Re-calculate and compare the calculated EDSP firmware checksum with
 * stored EDSP firmware checksum. Print on console whether verification
 * is passed or not. Called in case of FW watchdog or timeout waiting
 * for FW ready event.
 *
 * \param   l_cpu1_base_addr Base address of CPU1, obtained from BSP.
 * \param   l_pFW_img_data   ptr to FW image metadata such as range
 *                           for checksum calculation etc.
 * \return  none
 *
 * \ingroup Internal
 */
IFX_void_t ifx_mps_fw_crc_compare(volatile IFX_uint32_t *l_cpu1_base_addr,
                                  FW_image_ftr_t *l_pFW_img_data)
{
   IFX_uint32_t stored_cksum = l_pFW_img_data->crc32;
   IFX_uint32_t calc_cksum = ifx_mps_fw_crc32(l_cpu1_base_addr,
                                                   l_pFW_img_data);

   if (stored_cksum != calc_cksum)
   {
      TRACE (MPS, DBG_LEVEL_HIGH,
             ("MPS: FW checksum error: calculated=0x%08x stored=0x%08x\r\n",
              calc_cksum, stored_cksum));
   }
   else
   {
      TRACE (MPS, DBG_LEVEL_HIGH, ("MPS: FW checksum OK\r\n"));
   }
}

/**
 * Dump the EDSP firmware exception area on console. The size of the
 * exception area is different on different platforms. The information
 * on exception area size is stored in pFW_img_data buffer.
 * Called in case of FW watchdog or timeout waiting for FW ready event.
 *
 * \param   l_cpu1_base_addr Base address of CPU1, obtained from BSP.
 * \param   l_pFW_img_data   ptr to FW image metadata such as range
 *                           for checksum calculation etc.
 * \return  none
 *
 * \ingroup Internal
 */
IFX_void_t ifx_mps_dump_fw_xcpt(volatile IFX_uint32_t *l_cpu1_base_addr,
                                FW_image_ftr_t *l_pFW_img_data)
{
   IFX_uint32_t offset, *pTmp;
   IFX_boolean_t bPrintout = IFX_FALSE;

   for(offset=FW_XCPT_AREA_OFFSET;
                offset<l_pFW_img_data->st_addr_crc; offset += 4)
   {
      pTmp = (IFX_uint32_t *)(((IFX_uint8_t*)l_cpu1_base_addr)+offset);
      if (0 != *pTmp)
      {
         bPrintout = IFX_TRUE;
         break;
      }
   }

   if (IFX_TRUE == bPrintout)
   {
      for(offset=FW_XCPT_AREA_OFFSET;
                   offset<l_pFW_img_data->st_addr_crc; offset += 4)
      {
         pTmp = (IFX_uint32_t *)(((IFX_uint8_t*)l_cpu1_base_addr)+offset);
         TRACE (MPS, DBG_LEVEL_HIGH, ("%08x: %08x\r\n", offset, *pTmp));
      }
   }
}

#ifndef VMMC_WITH_MPS
EXPORT_SYMBOL (ifx_mps_bufman_register);
EXPORT_SYMBOL (ifx_mps_event_activation);
EXPORT_SYMBOL (ifx_mps_register_bufman_freeall_callback);
#endif

#if defined(CONFIG_MIPS) && !defined(CONFIG_MIPS_UNCACHED)
/**
 * Issue a Hit_Invalidate_D operation on a data segment of a
 * given size at given address. Address range is aligned to the
 * multiple of a cache line size.
 *
 * \param   addr    Address of a data segment
 * \param   len     Length in bytes of a data segment
 * \return  none
 * \ingroup Internal
 * \remarks addr parameter must be in KSEG0
 */
IFX_void_t ifx_mps_cache_inv (IFX_ulong_t addr, IFX_uint32_t len)
{
   IFX_ulong_t aline = addr & ~(CACHE_LINE_SZ - 1);
   IFX_ulong_t end   = (addr + len - 1) & ~(CACHE_LINE_SZ - 1);

   if (IFX_FALSE == bDoCacheOps)
      return;

   while (1)
   {
      __asm__ __volatile__(
         " .set    push        \n"
         " .set    noreorder   \n"
         " .set    mips3\n\t   \n"
         " cache   0x11, 0(%0) \n"
         " .set    pop         \n"
         : :"r"(aline));

      if (aline == end)
         break;

      aline += CACHE_LINE_SZ;
   }
}

/**
 * Issue a Hit_Writeback_Inv_D operation on a data segment of a
 * given size at given address. Address range is aligned to the
 * multiple of a cache line size.
 *
 * \param   addr    Address of a data segment
 * \param   len     Length in bytes of a data segment
 * \return  none
 * \ingroup Internal
 * \remarks addr parameter must be in KSEG0
 */
static IFX_void_t ifx_mps_cache_wb_inv (IFX_ulong_t addr, IFX_uint32_t len)
{
   IFX_ulong_t aline = addr & ~(CACHE_LINE_SZ - 1);
   IFX_ulong_t end   = (addr + len - 1) & ~(CACHE_LINE_SZ - 1);

   while (1)
   {
      __asm__ __volatile__(
         " .set    push            \n"
         " .set    noreorder       \n"
         " .set    mips3\n\t       \n"
         " cache   0x15, 0(%0)     \n"
         " .set    pop             \n"
         : :"r" (aline));

      if (aline == end)
         break;

      aline += CACHE_LINE_SZ;
   }
   /* MIPS multicore write reordering workaround:
      writing to on-chip SRAM and off-chip SDRAM can be reordered in time on
      MIPS multicore, in other words, there is no guarantee that write
      operation to SDRAM is finished at the moment of passing a data pointer to
      voice CPU  through data mailbox in SRAM.
      Workaround sequence:
      1) Write back (and invalidate) all used cache lines
      2) SYNC
      3) Read-back uncahed one word
      4) SYNC
      5) Write data pointer message to the mailbox in the on-chip SRAM */
   __asm__ __volatile__(" sync \n");
   /* dummy read back uncached */
   *((volatile IFX_uint32_t *)KSEG1ADDR(aline));
   __asm__ __volatile__(" sync \n");
}
#endif /*defined(CONFIG_MIPS) && !defined(CONFIG_MIPS_UNCACHED)*/
