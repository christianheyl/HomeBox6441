#ifndef __CPUSTAT_OSDEP_H__
#define __CPUSTAT_OSDEP_H__

/******************************************************************************
 * Auxiliary external defines - can be redefined to platform-dependent
 ******************************************************************************/
typedef s32 int32;
typedef u32 uint32;
typedef int32 cpu_stat_track_id_e;

extern u32 prescale;

extern u32 CPU_clock;
extern u32 AHB_clock;

#define uSECS_PER_TICK       (1000000 / APB_clock)
#define TICKS_PER_uSEC       (APB_clock / 1000000)



#if defined (CONFIG_ARCH_STR9100) /* Star boards */
/******************** For Star boards ****************************/
#include <linux/time.h>

typedef int32 wave400_dbg_hres_ts_t;

void str9100_counter_on_init( void );
void str9100_counter_on_cleanup( void );

typedef u32                             cpu_stat_ts_t;
typedef u64                             cpu_stat_uint_t;
#define CPU_STAT_GET_TIMESTAMP(ts)      (ts) = TIMER2_COUNTER_REG
#define CPU_STAT_IS_VALID_TIMESTAMP(ts) ((ts) != 0)
#define CPU_STAT_DIFF(type, from, to)   ((type)((to) - (from)))/1
#define CPU_STAT_UNIT                   "tick"
#define CPU_STAT_ON_INIT()              str9100_counter_on_init()
#define CPU_STAT_ON_CLEANUP()           str9100_counter_on_cleanup()

#define WAVE400_DBG_HRES_TS_WRAPAROUND_PERIOD_US() \
  (((wave400_dbg_hres_ts_t)-1)/TICKS_PER_uSEC)

#define WAVE400_DBG_HRES_TS(ts)                        \
  do {                                              \
    (ts) = (wave400_dbg_hres_ts_t)TIMER2_COUNTER_REG;  \
  } while (0)

#define WAVE400_DBG_HRES_TS_TO_US(ts)              \
  ((ts)/TICKS_PER_uSEC)

#elif defined (CONFIG_ARCH_NPU) /* NPU boards */
/******************** For NPU boards ****************************/

typedef int32 wave400_dbg_hres_ts_t;

void npu_counter_on_init( void );
void npu_counter_on_cleanup( void );
void npu_counter_enable( uint32 data );

typedef u32                             cpu_stat_ts_t;
typedef u64                             cpu_stat_uint_t;
#define CPU_STAT_GET_TIMESTAMP(ts)      (ts) = MT_RdReg(MT_LOCAL_MIPS_BASE_ADDRESS, REG_TIMER_2_COUNT_VALUE);
#define CPU_STAT_IS_VALID_TIMESTAMP(ts) ((ts) != 0)
#define CPU_STAT_DIFF(type, from, to)   ((type)((to) - (from)))/1
#define CPU_STAT_UNIT                   "tick"
#define CPU_STAT_ON_INIT()              npu_counter_on_init()
#define CPU_STAT_ON_CLEANUP()           npu_counter_on_cleanup()
#define CPU_STAT_PRINT_TIMESTAMP()                                                         \
  do {                                                                                     \
    TR0("time = 0x%08x \n",MT_RdReg(MT_LOCAL_MIPS_BASE_ADDRESS, REG_TIMER_2_COUNT_VALUE)); \
  } while (0)


#define WAVE400_DBG_HRES_TS_WRAPAROUND_PERIOD_US() \
  (((wave400_dbg_hres_ts_t)-1)/TICKS_PER_uSEC)

#define WAVE400_DBG_HRES_TS(ts)                    \
  do {                                          \
    CPU_STAT_GET_TIMESTAMP(ts);                  \
  } while (0)

#define WAVE400_DBG_HRES_TS_TO_US(ts)              \
  ((ts)/TICKS_PER_uSEC)

#else /* Using Linux default - do_gettimeofday() */
/******************** Using Linux default ****************************/
#include <linux/time.h>

typedef u64                             cpu_stat_uint_t;
typedef struct timeval                  cpu_stat_ts_t;

#define CPU_STAT_GET_TIMESTAMP(ts)      do_gettimeofday(&ts)
#define CPU_STAT_IS_VALID_TIMESTAMP(ts) ((ts).tv_sec != 0 || (ts).tv_usec != 0)
#define __CPU_STAT_NUM_TIMESTAMP(ts)    (((s64)(ts).tv_sec * USEC_PER_SEC) + (ts).tv_usec)
#define CPU_STAT_DIFF(type, from, to)   (type)(__CPU_STAT_NUM_TIMESTAMP(to) - __CPU_STAT_NUM_TIMESTAMP(from))
#define CPU_STAT_UNIT                   "us"
#define CPU_STAT_ON_INIT()
#define CPU_STAT_ON_CLEANUP()

typedef struct timeval wave400_dbg_hres_ts_t;

#define WAVE400_DBG_HRES_TS_WRAPAROUND_PERIOD_US() \
  (-1) /* wraparound is handled by do_gettimeofday() API internally */

#define WAVE400_DBG_HRES_TS(ts)                    \
  do_gettimeofday(&(ts))

#define WAVE400_DBG_HRES_TS_TO_US(ts)              \
  ((((uint64)(ts).tv_sec) * USEC_PER_SEC) + (ts).tv_usec)

#endif

#endif /* __CPUSTAT_OSDEP_H__ */
