#include <linux/interrupt.h>
#include "cpu_stat.h"
#include "wave400_time.h"

u32 prescale = 23 /*0: tick=clk*/ /*23 (+1): tick=micro*/;

u32 CPU_clock;
u32 AHB_clock;

#if defined (CONFIG_ARCH_STR9100) /* Star boards */

static void str9100_counter_init( void ) {
  u32 control_value,mask_value;
  control_value = TIMER_CONTROL_REG;
  control_value &= ~(1 << TIMER2_CLOCK_SOURCE_BIT_INDEX);
  control_value &= ~(1 << TIMER2_UP_DOWN_COUNT_BIT_INDEX);
  TIMER_CONTROL_REG = control_value;

  mask_value = TIMER_INTERRUPT_MASK_REG;
  mask_value |= (0x7 << 3);
  TIMER_INTERRUPT_MASK_REG = mask_value;
}

static void str9100_counter_enable ( void ) {
  u32 volatile control_value;
  TIMER2_COUNTER_REG=0;
  control_value = TIMER_CONTROL_REG;
  control_value |= (1 << TIMER2_ENABLE_BIT_INDEX);
  TIMER_CONTROL_REG = control_value;
}

static void str9100_counter_disable ( void ) { 
  u32 volatile control_value;
  control_value = TIMER_CONTROL_REG;
  control_value &= ~(1 << TIMER2_ENABLE_BIT_INDEX);
  TIMER_CONTROL_REG = control_value;
}

void str9100_counter_on_init( void ) {
  str9100_counter_init();
  str9100_counter_enable();
}

void str9100_counter_on_cleanup( void ) {
  str9100_counter_disable();
}

#elif defined (CONFIG_ARCH_NPU) /* NPU boards */
#ifdef DO_CPU_STAT
/* ?????????
* how to handle the wrapping? should do it in the CPU_STAT_DIFF macro !?
*/
void npu_counter_on_init( void )
{
  u32 maxValue = 0xffffffff;
  
  TR0("Init timer2 \n");
  wave400_set_time_mips_timer(MT_MIPS_TIMER_2, maxValue/*TICK_DIV*/);
  wave400_set_time_mips_prescaler(MT_MIPS_TIMER_2, prescale);
  wave400_set_mode_mips_timer(MT_MIPS_TIMER_2, MT_MIPS_TIMER_ENABLE, MT_MIPS_TIMER_MULT_SHOT/*MT_MIPS_TIMER_ONE_SHOT*/);
}

void npu_counter_on_cleanup( void )
{
  TR0("Clean timer2 \n");
  wave400_set_mode_mips_timer(MT_MIPS_TIMER_2, MT_MIPS_TIMER_DISABLE, MT_MIPS_TIMER_MULT_SHOT/*MT_MIPS_TIMER_ONE_SHOT*/);
}

void npu_counter_enable( uint32 data )
{
  TR0("wrie timer2 val = %d, prescale = %d\n",data, prescale);
  wave400_set_time_mips_prescaler(MT_MIPS_TIMER_2, prescale);
  wave400_set_mode_mips_timer(MT_MIPS_TIMER_2, data, MT_MIPS_TIMER_MULT_SHOT/*MT_MIPS_TIMER_ONE_SHOT*/);
}

#endif
#endif
