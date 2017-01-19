#ifndef __CPU_STAT_H__
#define __CPU_STAT_H__


/*! 
  \file  cpu_stat.h
  \brief CPUStat module interface
*/

//#include "wave400_osal.h"
//#include "synopGMAC_plat.h"
//#include "synopGMAC_Dev.h"
#include "synopGMAC_network_interface.h"
#include "wave400_interrupt.h"

#ifdef DO_CPU_STAT
#include <linux/spinlock.h>

#define HANDLE_T(x)       ((wave400_handle_t)(x))
#define HANDLE_T_PTR(t,x) ((t*)(x))
#define HANDLE_T_INT(t,x) ((t)(x))

#define OSAL_LOCK_INIT(s)      spin_lock_init(s); return 0
#define OSAL_LOCK_ACQUIRE(s)   unsigned long res = 0; spin_lock_irqsave((s), res); return HANDLE_T(res)
#define OSAL_LOCK_RELEASE(s,a) spin_unlock_irqrestore(s, (unsigned long)(a))
#define OSAL_LOCK_CLEANUP(s)


#ifndef TR0
#define TR0(fmt, args...) printk(KERN_CRIT "SynopGMAC: " fmt, ##args)				

#ifdef DEBUG
#undef TR
#  define TR(fmt, args...) printk(KERN_CRIT "SynopGMAC: " fmt, ##args)
#else
# define TR(fmt, args...) /* not debugging: nothing */
#endif
#endif


typedef unsigned long wave400_handle_t;

/**********************************************************************
 * Spinlock abstraction
***********************************************************************/

#ifdef OSAL_LOCK_INIT
static inline int wave400_osal_lock_init(spinlock_t* spinlock)
{
  OSAL_LOCK_INIT(spinlock);
}
#else
int wave400_osal_lock_init(spinlock_t* spinlock);
#endif

#ifdef OSAL_LOCK_ACQUIRE
static inline wave400_handle_t wave400_osal_lock_acquire(spinlock_t* spinlock)
{
  OSAL_LOCK_ACQUIRE(spinlock);
}
#else
wave400_handle_t wave400_osal_lock_acquire(spinlock_t* spinlock);
#endif

#ifdef OSAL_LOCK_RELEASE
static inline void wave400_osal_lock_release(spinlock_t* spinlock, wave400_handle_t acquire_val)
{
  OSAL_LOCK_RELEASE(spinlock, acquire_val);
}
#else
void wave400_osal_lock_release(spinlock_t* spinlock, wave400_handle_t acquire_val);
#endif

#ifdef OSAL_LOCK_CLEANUP
static inline void wave400_osal_lock_cleanup(spinlock_t* spinlock)
{
  OSAL_LOCK_CLEANUP(spinlock);
}
#else
void wave400_osal_lock_cleanup(spinlock_t* spinlock);
#endif
/**********************************************************************/

/*! 
  \file  cpustat_osdep.h
  \brief Platform-dependent CPUStat module's part

  Must define the following:
    cpu_stat_ts_t                   - timestamp type
    cpu_stat_uint_t                 - int type to accumulate timestamp diffs
    CPU_STAT_GET_TIMESTAMP(ts)      - function/macro to get timestamp
    CPU_STAT_IS_VALID_TIMESTAMP(ts) - checks is timestamp is valid (not a zero)
    CPU_STAT_DIFF(type, from, to)   - calculates timestamps 
*/
#include "wave400_emerald_env_regs.h"
//#include "wave400_defs.h"
#include "wave400_cnfg.h"
#include "wave400_chadr.h"
#include "wave400_chipreg.h"
#include "cpustat_osdep.h"

#define CPU_STAT_MAX_PEEKS 100

/*! 
  \enum   _cpu_stat_track_id_e
  \brief  IDs of existing code tracks.
*/
//typedef s32 int32;
//typedef u32 uint32;
//typedef int32 cpu_stat_track_id_e;

/*change CPU_STAT_MASK when add IDs*/
#define CPU_STAT_ID_NONE                  -1
#define CPU_STAT_ID_XMIT_START             0 /*!<  wave400_wrap_transmit() */
#define CPU_STAT_ID_RELEASE_TX_DATA        1 /*!<  TX DAT CFM handling */
#define CPU_STAT_ID_RX_DATA_FRAME          2 /*!<  RX DAT IND handling - Data Frames only */
#define CPU_STAT_ID_ISR_LATENCY            3 /*!<  ISR Latency */
/*sub tests:*/
#define CPU_STAT_ID_SET_TX_QPTR            4
#define CPU_STAT_ID_GET_RX_QPTR            5
#define CPU_STAT_ID_NETIF_RX               6
#define CPU_STAT_ID_ALLOC_SKB              7
#define CPU_STAT_ID_MAP                    8
#define CPU_STAT_ID_UNMAP                  9
#define CPU_STAT_ID_SKB_PUT                10
#define CPU_STAT_ID_GET_TEMP               11
/*last:*/
#define CPU_STAT_ID_LAST                   12 //increment when adding ID !!!

#ifndef CPU_STAT_MASK
#define CPU_STAT_MASK 0x00000FFF /* default mask- change mask when add IDs !!! */
#endif

#ifdef CPU_STAT_MASK
#define CPU_STAT_ID_IS_ENABLED(id) (CPU_STAT_MASK & (1L << (id)))
#else
#define CPU_STAT_ID_IS_ENABLED(id) 1
#endif

/*! 
  \struct  cpu_stat_node_t
  \brief   Structure for collectting CPU statistics for one track.
*/
typedef struct _cpu_stat_node_t
{
  uint32          count;   /*!< counter of iterations done */
  cpu_stat_uint_t total;   /*!< sum of measured per-iteration times */
  cpu_stat_uint_t peak;    /*!< maximal iteration time */
  uint32          peak_sn; /*!< peak's sequence number */
} cpu_stat_node_t;

/*! 
  \struct  cpu_stat_t
  \brief   Module's object. 

  \warning This structure is for internal usage only and must not be changed outside the module.
*/
typedef struct _cpu_stat_t
{
  cpu_stat_node_t      tracks[CPU_STAT_ID_LAST]; /*!< array of per-track statisticts */
  cpu_stat_ts_t        current_start;            /*!< current track's iteration start time */
  cpu_stat_track_id_e  current_id;               /*!< ID of the track we are measuring now */
  cpu_stat_track_id_e  enabled_id;               /*!< ID of enabled track */
  spinlock_t lock;                               /*!< Object's spinlock */
  u32                  pgmac;                    /*!< ptr to GMAC */
} cpu_stat_t;

/******************************************************************************
 * Auxiliary internal defines
 ******************************************************************************/
#define __CPU_STAT_LOCK_DECLARE          wave400_handle_t lock_val = HANDLE_T(0)
#define __CPU_STAT_LOCK(obj)             lock_val = wave400_osal_lock_acquire(&obj->lock)
#define __CPU_STAT_UNLOCK(obj)           wave400_osal_lock_release(&obj->lock, lock_val)

#define __CPU_STAT_ID_VALID(id)         ((id) > CPU_STAT_ID_NONE && (id) < CPU_STAT_ID_LAST)
#define __CPU_STAT_ID_ENABLED(obj, id)  (__CPU_STAT_ID_VALID(id) && (obj)->enabled_id == (id))

/*! 
  \fn      void __CPU_STAT_END(cpu_stat_t         *obj,
                               cpu_stat_track_id_e id,
                               struct timeval     *start)
  \brief   Finilizes the measurement: takes the final timestamp, calculates diff and 
           updates the suitable track's node.

  \param   obj   module's object
  \param   id    ID of track to finalize
  \param   start pointer to start timestamp

  \warning This function is for internal usage only and must not be used outside the module.
*/
static inline void
__CPU_STAT_END (cpu_stat_t         *obj,
                cpu_stat_track_id_e id,
                cpu_stat_ts_t      *start)
{
  cpu_stat_node_t *node;
  cpu_stat_ts_t    now;

  /* Important:                                                */
  /*   always get NOW timestamp before ifs and calculations to */
  /*   avoid affecting the result                              */
  CPU_STAT_GET_TIMESTAMP(now);
  //TR0("time now = 0x%08x, id = %d \n", now, id);
  //TR0("time read = 0x%08x\n",MT_RdReg(MT_LOCAL_MIPS_BASE_ADDRESS, REG_TIMER_2_COUNT_VALUE));

  node = &obj->tracks[id];

  if (CPU_STAT_IS_VALID_TIMESTAMP(*start)) {
    __CPU_STAT_LOCK_DECLARE;
    cpu_stat_uint_t diff = CPU_STAT_DIFF(cpu_stat_ts_t/*cpu_stat_uint_t ??*/, *start, now);

    __CPU_STAT_LOCK(obj);
    if (diff > node->peak) {
      node->peak    = diff;
      node->peak_sn = node->count;
    }

    node->total += diff;
    node->count ++;
    __CPU_STAT_UNLOCK(obj);
  }
  else {
    TR("CPU_STAT_BEGIN() has not been called for Track#%d  \n", id);
  }
}

/*! 
  \fn      void _CPU_STAT_GET_DATA(cpu_stat_t         *obj, 
                                   cpu_stat_track_id_e id,
                                   cpu_stat_node_t    *stat)
  \brief   Fills the stat structure with the suitable track's data.

  \param   obj   module's object
  \param   id    track ID
  \param   stat  pointer to structure to fill

  \warning This function should not be used for the measurements. It provides functionality required
           for CPU_STAT UI exposing, for example via procfs   

*/
static inline void
_CPU_STAT_GET_DATA (cpu_stat_t         *obj, 
                    cpu_stat_track_id_e id,
                    cpu_stat_node_t    *stat)
{
  __CPU_STAT_LOCK_DECLARE;
  
  __CPU_STAT_LOCK(obj);
  if (__CPU_STAT_ID_VALID(id))
    *stat = obj->tracks[id];
  else
    stat->count = 0;
  __CPU_STAT_UNLOCK(obj);
}

/*! 
  \fn      const char* _CPU_STAT_GET_NAME(cpu_stat_t         *obj, 
                                          cpu_stat_track_id_e id);
  \brief   Returns track's symbolic name

  \param   obj   module's object
  \param   id    track ID

  \warning This function should not be used for the measurements. It provides functionality required
           for CPU_STAT UI exposing, for example via procfs   
*/
const char*
_CPU_STAT_GET_NAME (cpu_stat_t         *obj, 
                    cpu_stat_track_id_e id);

/*! 
  \fn      void _CPU_STAT_GET_NAME_EX(cpu_stat_t         *obj, 
                                      cpu_stat_track_id_e id,
                                      char               *buf,
                                      uint32              size)
  \brief   Fills buffer with the track's symbolic name or track's ID if no symbolic name found.

  \param   obj   module's object
  \param   id    track ID
  \param   buf   buffer to fill/format
  \param   size  size of buffer

  \warning This function should not be used for the measurements. It provides functionality required
           for CPU_STAT UI exposing, for example via procfs   
*/
void
_CPU_STAT_GET_NAME_EX (cpu_stat_t         *obj, 
                       cpu_stat_track_id_e id,
                       char               *buf,
                       uint32              size);

/*! 
  \fn      int _CPU_STAT_IS_ENABLED(cpu_stat_t         *obj, 
                                    cpu_stat_track_id_e id)
  \brief   Check if track ID is enabled.
           

  \param   obj  module's object
  \param   id   track ID

  \return  1 - enabled, 0 - disabled

  \warning This function should not be used for the measurements. It provides functionality required
           for CPU_STAT UI exposing, for example via procfs   
*/
static inline int
_CPU_STAT_IS_ENABLED (cpu_stat_t         *obj, 
                      cpu_stat_track_id_e id)
{
  return (id == obj->enabled_id);
}

/*! 
  \fn      cpu_stat_track_id_e _CPU_STAT_GET_ENABLED_ID(cpu_stat_t *obj)
  \brief   Returnes ID of enabled track. 
           

  \param   obj  module's object

  \return  track ID

  \warning This function should not be used for the measurements. It provides functionality required
           for CPU_STAT UI exposing, for example via procfs   
*/
static inline cpu_stat_track_id_e 
_CPU_STAT_GET_ENABLED_ID (cpu_stat_t *obj)
{
  return obj->enabled_id;
}

/*! 
  \def    _CPU_STAT_FOREACH_TRACK_IDX(obj, id)

  Iterates track IDs. Uses \a obj module's object and puts id into \a id variable.

  \warning This function should not be used for the measurements. It provides functionality required
           for CPU_STAT UI exposing, for example via procfs   
*/
#define _CPU_STAT_FOREACH_TRACK_IDX(obj, id) \
  for ((id) = CPU_STAT_ID_NONE; (id) < CPU_STAT_ID_LAST; (id)++)

/******************************************************************************
 * Interface
 ******************************************************************************/
/*! 
  \fn      void CPU_STAT_RESET(cpu_stat_t *obj)
  \brief   Resets the CPU Statistics module including collected data and Enabled Track ID

  \param   obj   module's object

  \warning This function is used to reset statistics upon user request, for example via pocfs.
*/
static inline void
CPU_STAT_RESET (cpu_stat_t *obj)
{
  __CPU_STAT_LOCK_DECLARE;
  
  __CPU_STAT_LOCK(obj);
  memset(&obj->tracks, 0, sizeof(obj->tracks));
  obj->current_id = CPU_STAT_ID_NONE;
  obj->enabled_id = CPU_STAT_ID_NONE;
  __CPU_STAT_UNLOCK(obj);
}

/*! 
  \fn      void CPU_STAT_INIT(cpu_stat_t *obj)
  \brief   Initializes the CPU Statistics module

  \param   obj   module's object

  \warning This function is usually called by HW layer during the initialization phase. 
           Pointer to CPU STAT module's object then is passed to Core via HAL 
           (wave400_core_set_prop).
*/
static inline void 
CPU_STAT_INIT (cpu_stat_t *obj) 
{
  wave400_osal_lock_init(&obj->lock);
  CPU_STAT_RESET(obj);
  CPU_STAT_ON_INIT();
}

/*! 
  \def     CPU_STAT_CLEANUP(obj)
  \brief   Cleans the CPU Statistics (\a obj) module up.
*/
static inline void 
CPU_STAT_CLEANUP (cpu_stat_t *obj) 
{
  CPU_STAT_ON_CLEANUP();
  wave400_osal_lock_cleanup(&obj->lock);
}

/*! 
  \fn      int CPU_STAT_BEGIN_TRACK(cpu_stat_t         *obj, 
                                    cpu_stat_track_id_e id)
  \brief   Begins measurement of specified track. 
           Note, this will do nothing (1 'if') if the specified track is not enabled 
           prior to this call.

  \param   obj   module's object
  \param   id    track ID
*/
static inline int 
CPU_STAT_BEGIN_TRACK (cpu_stat_t         *obj, 
                      cpu_stat_track_id_e id)
{
  int res = 0;

  TR("id = %d, obj->enabled_id = %d \n",id, obj->enabled_id);

  if (__CPU_STAT_ID_ENABLED(obj, id)) {
    TR("ID_ENABLED :-) \n");
    res = 1;
    wave400_disable_irq(irqOut[0]);
    wave400_disable_irq(irqOut[1]);
    //synopGMAC_disable_interrupt_all(gmacdev);
    CPU_STAT_GET_TIMESTAMP(obj->current_start);
  }
  else
    TR("ID #%d Not ENABLED :-( \n",id);

  return res;
}

/*! 
  \fn       void CPU_STAT_END_TRACK(cpu_stat_t         *obj, 
                                    cpu_stat_track_id_e id)
  \brief   Finalizes measurement of specified track. 
           Note, this will do nothing (2 'if's) if the specified track is not enabled 
           or not started prior to this call.

  \param   obj   module's object
  \param   id    track ID
*/
static inline void 
CPU_STAT_END_TRACK (cpu_stat_t         *obj, 
                    cpu_stat_track_id_e id)
{
  if (__CPU_STAT_ID_ENABLED(obj, id)) {
    __CPU_STAT_END(obj, id, &obj->current_start);
    obj->current_id = CPU_STAT_ID_NONE;
    wave400_enable_irq(irqOut[0]);
    wave400_enable_irq(irqOut[1]);
    //synopGMAC_enable_interrupt(gmacdev);
  }
}

/*! 
  \fn      cpu_stat_track_id_e CPU_STAT_BEGIN_TRACK_SET(cpu_stat_t                *obj,
                                                        const cpu_stat_track_id_e *ids,
                                                        uint32                     nof_ids)
  \brief   Begins measurement of one of specified tracks. 
           Note, this will do nothing (1 'if' per track ID) if no one of specified tracks 
           is enabled prior to this call. Current track ID can be specified after this call.
           

  \param   obj     module's object
  \param   ids     array of track IDs
  \param   nof_ids array size

  \return  Value to be passed to CPU_STAT_END_TRACK_SET.
*/
static inline cpu_stat_track_id_e
CPU_STAT_BEGIN_TRACK_SET (cpu_stat_t                *obj,
                          const cpu_stat_track_id_e *ids,
                          uint32                     nof_ids)
{
  cpu_stat_track_id_e res = CPU_STAT_ID_NONE;
  uint32              i   = 0;

  for (; i < nof_ids; i++) {
    if (CPU_STAT_BEGIN_TRACK(obj, ids[i])) {
      res = ids[i];
      break;
    }
  }

  return res;
}

/*! 
  \fn      void CPU_STAT_SPECIFY_TRACK(cpu_stat_t         *obj, 
                                       cpu_stat_track_id_e id)
  \brief   Specifies current track ID. 

  \param   obj     module's object
  \param   id    track ID
*/
static inline void 
CPU_STAT_SPECIFY_TRACK (cpu_stat_t         *obj, 
                        cpu_stat_track_id_e id) 
{
  if (__CPU_STAT_ID_ENABLED(obj, id))
    obj->current_id = id;
}

/*! 
  \fn      void CPU_STAT_END_TRACK_SET (cpu_stat_t         *obj,
                                        cpu_stat_track_id_e begin_ret_val)
  \brief   Finalizes measurement of the current track previously specified by 
           CPU_STAT_SPECIFY_TRACK(). 
           Note, this will do nothing (1 'if') if the specified track is not started or
           it is not enabled prior to this call.
           

  \param   obj           module's object
  \param   begin_ret_val value returned by CPU_STAT_BEGIN_TRACK_SET
*/
static inline void
CPU_STAT_END_TRACK_SET (cpu_stat_t         *obj,
                        cpu_stat_track_id_e begin_ret_val)
{
  if (begin_ret_val == obj->current_id) /* current TRACK is started */
    CPU_STAT_END_TRACK(obj, begin_ret_val);
}

/*! 
  \fn      void CPU_STAT_ENABLE(cpu_stat_t         *obj, 
                                cpu_stat_track_id_e id)
  \brief   Enanble measurements for specified track. Disables measurement if id == CPU_STAT_PT_NONE
           

  \param   obj  module's object
  \param   id   track ID
*/
static inline void 
CPU_STAT_ENABLE (cpu_stat_t         *obj, 
                 cpu_stat_track_id_e id)
{
  if (__CPU_STAT_ID_VALID(id) || id == CPU_STAT_ID_NONE) {
      TR("ID #%d valid \n",id);
      obj->enabled_id = id;
  }
  else
      TR0("ID #%d Not valid ! :-( , try hex ?\n",id);
}

#else /* DO_CPU_STAT */

#define CPU_STAT_ID_IS_ENABLED(id) 0

#endif /* DO_CPU_STAT */

#endif /* __CPU_STAT_H__ */
