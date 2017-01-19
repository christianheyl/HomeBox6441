#ifndef __WAVE400_TIME_H__
#define __WAVE400_TIME_H__

#ifdef  MT_GLOBAL
#define MT_EXTERN
#define MT_I(x) x
#else
#define MT_EXTERN extern
#define MT_I(x)
#endif

extern void __init setup_wave400_timer(void);

enum {
  MT_MIPS_TIMER_0,
  MT_MIPS_TIMER_1,
  MT_MIPS_TIMER_2,
  MT_MIPS_TIMER_3
};

#define MT_MIPS_TIMER_DISABLE      0
#define MT_MIPS_TIMER_ENABLE       1

#define MT_MIPS_TIMER_ONE_SHOT     0
#define MT_MIPS_TIMER_MULT_SHOT    1

#ifdef DO_CPU_STAT
extern void wave400_set_time_mips_timer(unsigned long timerId, unsigned long maxCount);	
extern void wave400_set_mode_mips_timer(unsigned long timerId, unsigned long enableTimer, unsigned long shotMode);	
extern void wave400_set_time_mips_prescaler(unsigned long  timerId, unsigned long  prescale);	
#endif

#undef MT_EXTERN
#undef MT_I

/*for real system undefine WAVE400_SCALE_DOWN*/
extern unsigned int system_to_cpu_multiplier;
#ifdef CONFIG_VBG400_CHIPIT
#define WAVE400_SCALE_DOWN
#endif

#ifdef WAVE400_SCALE_DOWN
#define WAVE400_SCALE_VAL 10
#else
#define WAVE400_SCALE_VAL 1
#endif

#if 1
#define WAVE400_SYSTEM_CLK  240000000
#define WAVE400_CPU_CLK     480000000
#else
#define WAVE400_SYSTEM_CLK  (24000000*WAVE400_SCALE_VAL)
#ifdef CONFIG_VBG400_CHIPIT
#define WAVE400_CPU_CLK     (48000000*WAVE400_SCALE_VAL)
#else
#define WAVE400_CPU_CLK     (WAVE400_SYSTEM_CLK *system_to_cpu_multiplier)
#endif
#endif

#define REG_NPU_SYS_INFO		0x60
#define CPU_FAST_MODE		0x00080000

/* Note - the timer used is WAVE400, run 24000000M
* If we use the MIPS timer, change to  WAVE400_CPU_CLK
*/
//#define TICK_DIV (WAVE400_CPU_CLK/HZ)
#define TICK_DIV (WAVE400_SYSTEM_CLK/HZ)


#endif /* __WAVE400_TIME_H__ */ 
