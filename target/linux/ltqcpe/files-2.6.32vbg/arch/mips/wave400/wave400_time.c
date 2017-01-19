#include <linux/clockchips.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <asm/irq_cpu.h>
#include <asm/mipsregs.h>
#include <asm/system.h>
#include <linux/sched.h>
#include <linux/ioport.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/kernel_stat.h>
#include <linux/kernel.h>
#include <linux/random.h>
#include <asm/mips-boards/generic.h>
#include <asm/mips-boards/prom.h>
#include <asm/bootinfo.h>

#include "wave400_emerald_env_regs.h"
#include "wave400_defs.h"
#include "wave400_cnfg.h"
#include "wave400_chadr.h"
#include "wave400_chipreg.h"
#include "wave400_interrupt.h"
#include "wave400_time.h"
#include <asm/time.h>

unsigned int system_to_cpu_multiplier = 1;

void wave400_clear_isr_mips_timer(uint32 timerId)
{
	uint32 timerId2RegExpAdr[] = { REG_TIMER0_EXPIRED, REG_TIMER1_EXPIRED,
		REG_TIMER2_EXPIRED, REG_TIMER3_EXPIRED
	};
	uint32 timerId2RegExpMask[] =
	    { REG_TIMER0_EXPIRED_MASK, REG_TIMER1_EXPIRED_MASK,
		REG_TIMER2_EXPIRED_MASK, REG_TIMER3_EXPIRED_MASK
	};
	//Clear interrupt source
	MT_WrRegMask(MT_LOCAL_MIPS_BASE_ADDRESS, timerId2RegExpAdr[timerId],
		     timerId2RegExpMask[timerId], timerId2RegExpMask[timerId]);
}

void wave400_set_mode_mips_timer(uint32 timerId, uint32 enableTimer,
			      uint32 shotMode)
{
	uint32 timerId2RegEnableAddr[] =
	    { REG_TIMER_0_ENABLE, REG_TIMER_1_ENABLE,
		REG_TIMER_2_ENABLE, REG_TIMER_3_ENABLE
	};

	uint32 timerId2RegEnableMask[] =
	    { REG_TIMER_0_ENABLE_MASK, REG_TIMER_1_ENABLE_MASK,
		REG_TIMER_2_ENABLE_MASK, REG_TIMER_3_ENABLE_MASK
	};

	uint32 timerId2RegShotMask[] =
	    { REG_TIMER_0_ONESHOT_MODE_MASK, REG_TIMER_1_ONESHOT_MODE_MASK,
		REG_TIMER_2_ONESHOT_MODE_MASK, REG_TIMER_3_ONESHOT_MODE_MASK
	};
	uint32 data;
printk("wave400_set_mode_mips_timer: timerId=%d\n",timerId);
	if (enableTimer == MT_MIPS_TIMER_ENABLE)
		if (shotMode == MT_MIPS_TIMER_MULT_SHOT)	/* enable timer in multi shot mode */
{
printk("wave400_set_mode_mips_timer: MT_MIPS_TIMER_MULT_SHOT INTERRUPT !!!\n");
			data =
			    timerId2RegEnableMask[timerId] |
			    timerId2RegShotMask[timerId];
}
		else 		/* enable timer in one shot mode */
{
			data = timerId2RegEnableMask[timerId];
printk("wave400_set_mode_mips_timer: one shot mode INTERRUPT !!!\n");
}
	else
{
		data = 0;	/* disable timer */
printk("wave400_set_mode_mips_timer: DISABLE INTERRUPT !!!\n");
}
printk("wave400_set_mode_mips_timer: data = 0x%08x\n",data);
	MT_WrRegMask(MT_LOCAL_MIPS_BASE_ADDRESS, timerId2RegEnableAddr[timerId],
		     (timerId2RegEnableMask[timerId] |
		      timerId2RegShotMask[timerId]), data);
printk("wave400_set_mode_mips_timer: read offset 0x%08x reg = 0x%08x\n",MT_RdReg(MT_LOCAL_MIPS_BASE_ADDRESS, timerId2RegEnableAddr[timerId]));
}

void wave400_set_time_mips_prescaler(MT_UINT32 timerId, uint32 prescale)
{
	MT_UINT32 timerId2RegMaxCountAdr[] =
	    { 0, 0, REG_TIMER_2_PRESCALER, REG_TIMER_3_PRESCALER };
	//Configure the timer period
	if ((timerId >= REG_TIMER_PRESCALER_MIN)
	    && (timerId <= REG_TIMER_PRESCALER_MAX)) {
		MT_WrReg(MT_LOCAL_MIPS_BASE_ADDRESS,
			 timerId2RegMaxCountAdr[timerId], prescale);
	}
else
	printk("wave400_set_time_mips_prescaler: timerId error 0x%08x\n",timerId);
printk("wave400_set_time_mips_prescaler: prescale = 0x%08x\n",prescale);
}

void wave400_set_time_mips_timer(MT_UINT32 timerId, uint32 maxCount)
{
	MT_UINT32 timerId2RegMaxCountAdr[] =
	    { REG_TIMER_0_MAX_COUNT, REG_TIMER_1_MAX_COUNT,
		REG_TIMER_2_MAX_COUNT, REG_TIMER_3_MAX_COUNT
	};
	//Configure the timer period
	MT_WrReg(MT_LOCAL_MIPS_BASE_ADDRESS, timerId2RegMaxCountAdr[timerId],
		 maxCount);
printk("wave400_set_time_mips_timer: maxCount 0x%08x\n",maxCount);
}

static void wave400_timer_irq(void)
{
	do_IRQ(WAVE400_TIMER_IRQ_OUT_INDEX /*0x2 */ );
}

static void wave400_timer_set_mode(enum clock_event_mode mode,
				struct clock_event_device *evt)
{
	switch (mode) {
	case CLOCK_EVT_MODE_PERIODIC:
		wave400_set_time_mips_timer(MT_MIPS_TIMER_0, TICK_DIV);
		wave400_set_mode_mips_timer(MT_MIPS_TIMER_0, MT_MIPS_TIMER_ENABLE,
					 MT_MIPS_TIMER_MULT_SHOT);
printk("set CLOCK_EVT_MODE_PERIODIC, TICK_DIV=0x%08x\n",TICK_DIV);
#ifdef WAVE400_TEST_TIMER_PERIOD
		wave400_set_time_mips_timer(MT_MIPS_TIMER_1, 0xffffffff);
		wave400_set_mode_mips_timer(MT_MIPS_TIMER_1, MT_MIPS_TIMER_ENABLE,
					 MT_MIPS_TIMER_MULT_SHOT);
#endif
		break;
	case CLOCK_EVT_MODE_ONESHOT:
		wave400_set_time_mips_timer(MT_MIPS_TIMER_0, TICK_DIV);
		wave400_set_mode_mips_timer(MT_MIPS_TIMER_0, MT_MIPS_TIMER_ENABLE,
					 MT_MIPS_TIMER_ONE_SHOT);
printk("set CLOCK_EVT_MODE_ONESHOT, TICK_DIV=0x%08x\n",TICK_DIV);
#ifdef WAVE400_TEST_TIMER_PERIOD
		wave400_set_time_mips_timer(MT_MIPS_TIMER_1, 0xffffffff);
		wave400_set_mode_mips_timer(MT_MIPS_TIMER_1, MT_MIPS_TIMER_ENABLE,
					 MT_MIPS_TIMER_MULT_SHOT);
#endif
		break;
	case CLOCK_EVT_MODE_UNUSED:
		break;
	case CLOCK_EVT_MODE_SHUTDOWN:
		wave400_set_mode_mips_timer(MT_MIPS_TIMER_0, MT_MIPS_TIMER_DISABLE,
					 MT_MIPS_TIMER_ONE_SHOT);
printk("set CLOCK_EVT_MODE_SHUTDOWN !!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
		break;
	case CLOCK_EVT_MODE_RESUME:
printk("set CLOCK_EVT_MODE_SHUTDOWN !!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
		break;
	}
}

struct clock_event_device wave400_clockevent = {
	.name = "wave400_timer",
	.features = CLOCK_EVT_FEAT_PERIODIC,
	.rating = 300,		/* Lior Hadad will need to adjust */
	.set_mode = wave400_timer_set_mode,
	.irq = /*2 */ WAVE400_TIMER_IRQ_OUT_INDEX,	/* Lior Hadad will need to adjust */
};

#ifdef WAVE400_TEST_TIMER_PERIOD
static unsigned int timer_enter[11];
static unsigned int timer_out[11];
static unsigned int timer_index = 0;
static unsigned int count_index = 0;
#endif

static irqreturn_t wave400_timer_interrupt0(int irq, void *dev_id)
{
	struct clock_event_device *cd = dev_id;
#ifdef WAVE400_TEST_TIMER_PERIOD
	int i = 0;

	timer_enter[timer_index] =
	    MT_RdReg(MT_LOCAL_MIPS_BASE_ADDRESS, REG_TIMER_1_COUNT_VALUE);
	wave400_clear_isr_mips_timer(MT_MIPS_TIMER_0);
	cd->event_handler(cd);	/* will call tick_handle_periodic() */

	timer_out[timer_index] =
	    MT_RdReg(MT_LOCAL_MIPS_BASE_ADDRESS, REG_TIMER_1_COUNT_VALUE);
	if ((count_index >= 100) && (timer_index == 10)) {
		if (timer_index == 10) {
			for (i = 0; i < 10; i++) {
				printk("%d %08x\n", i, timer_enter[i]);
				printk("%d %08x\n", i, timer_out[i]);
			}
		}
		count_index = 0;
	}
	count_index++;
	timer_index++;
	timer_index = (timer_index % 11);
#else
	wave400_clear_isr_mips_timer(MT_MIPS_TIMER_0);
	cd->event_handler(cd);	/* will call tick_handle_periodic() */

#endif

	return IRQ_HANDLED;
}

static struct irqaction wave400_timer_irqaction = {
	.handler = wave400_timer_interrupt0,
	.flags = IRQF_DISABLED /*disable nested interrupts */  | IRQF_PERCPU,
	.name = "wave400_timer"
};

void __init setup_wave400_timer(void)
{
	struct clock_event_device *cd = &wave400_clockevent;
	struct irqaction *action = &wave400_timer_irqaction;
	unsigned int cpu = smp_processor_id();
#ifndef CONFIG_VBG400_CHIPIT
	unsigned int fast_mode;
#endif
	cd->cpumask = get_cpu_mask(cpu);
		//cpumask_of_cpu(cpu);
	clockevent_set_clock(&wave400_clockevent, WAVE400_SYSTEM_CLK);

#ifndef CONFIG_VBG400_CHIPIT
    //read CPU clock configuration from register:
    fast_mode = MT_RdReg(MT_LOCAL_MIPS_BASE_ADDRESS, REG_NPU_SYS_INFO);
    if (fast_mode & CPU_FAST_MODE)
        system_to_cpu_multiplier = 2; //change to fast cpu clock
printk("setup_wave400_timer: system_to_cpu_multiplier=%d\n",system_to_cpu_multiplier);
#endif
	clockevents_register_device(cd);
	action->dev_id = cd;

	wave400_register_static_irq(WAVE400_TIMER_IRQ_IN_INDEX,
				 WAVE400_TIMER_IRQ_OUT_INDEX,
				 &wave400_timer_irqaction, wave400_timer_irq);
	wave400_enable_irq(WAVE400_TIMER_IRQ_OUT_INDEX);
//wave400_enable_irq_all_mips(0xe0000001);
}
