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
#include <linux/serial_core.h>
#include <linux/netdevice.h>

#include "wave400_emerald_env_regs.h"
#include "wave400_defs.h"
#include "wave400_cnfg.h"
#include "wave400_chadr.h"
#include "wave400_chipreg.h"
#include "wave400_uart.h"
#ifdef CONFIG_SYNOPGMACHOST_PCI
#include "synopGMAC_network_interface.h"
#endif
#include "wave400_interrupt.h"

#define GET_REG_IRQ_MASK MT_RdReg( MT_LOCAL_MIPS_BASE_ADDRESS, REG_IRQ_MASK)
#define SET_REG_IRQ_MASK(irq_mask) MT_WrReg( MT_LOCAL_MIPS_BASE_ADDRESS, REG_IRQ_MASK, irq_mask )

static uint32 irq_mask_save = 0;

//int wave400_register_static_irq_dummy(unsigned int irq_in, unsigned int irq_out);


/*inline*/ void wave400_disable_irq_all(void)
{
        uint32 irq_mask = 0x0;
        
        irq_mask_save = MT_RdReg( MT_LOCAL_MIPS_BASE_ADDRESS, REG_IRQ_MASK);
        MT_WrReg( MT_LOCAL_MIPS_BASE_ADDRESS, REG_IRQ_MASK, irq_mask );
}

/*inline*/ void wave400_disable_irq(unsigned int irq_nr)
{
        uint32 irq_mask = 0;
        
        irq_mask = MT_RdReg( MT_LOCAL_MIPS_BASE_ADDRESS, REG_IRQ_MASK);
    
        irq_mask &= ~(1 << (irq_nr));

        MT_WrReg( MT_LOCAL_MIPS_BASE_ADDRESS, REG_IRQ_MASK, irq_mask );

}

inline void wave400_ack_irq(unsigned int irq_nr)
{
    /* ack the relevant interrupt */
    switch(irq_nr) {
#if 1
	case 4:
	case 3:
	case 2:
#endif
	case 1:
        /* UART will reset the interrupt in the handler */
		break;
	default:
        while(1);
        BUG();
		break;
	}
}

#if 0
/*inline*/ void wave400_enable_irq_all(void)
{
	uint32 irq_mask = 0;

	irq_mask = (1<<WAVE400_SERIAL_IRQ_OUT_INDEX) |
	            (1<<WAVE400_TIMER_IRQ_OUT_INDEX) |
	            (1<<WAVE400_SYNOP_ETHER_IRQ_OUT_INDEX) |
	            (1<<WAVE400_SYNOP_ETHER_IRQ_GMAC2_OUT_INDEX) |
	            (1<<WAVE400_WIRELESS_IRQ_OUT_INDEX);
    

	SET_REG_IRQ_MASK(irq_mask/*irq_mask_save*/);
	//MT_WrReg( MT_LOCAL_MIPS_BASE_ADDRESS, REG_IRQ_MASK, irq_mask );
}
#endif

/*inline*/ void wave400_enable_irq(unsigned int irq_nr)
{
	uint32 irq_mask = 0;

	//irq_mask = MT_RdReg( MT_LOCAL_MIPS_BASE_ADDRESS, REG_IRQ_MASK);
	irq_mask = GET_REG_IRQ_MASK;
    
    irq_mask |= (1 << (irq_nr));

	SET_REG_IRQ_MASK(irq_mask);
	//MT_WrReg( MT_LOCAL_MIPS_BASE_ADDRESS, REG_IRQ_MASK, irq_mask );
}
#if 1
#define set_status(x)   __asm__ volatile("mtc0 %0,$12" :: "r" (x))

/*inline*/ void wave400_enable_irq_all_mips(unsigned long val)
{
	set_status(val);
/*	__asm__(
	"	li      $10,0xe0000001		\n"
	"	mtc0    $10,$12				\n"
	);
*/
}
#endif

/*static */irqreturn_t uart_interrupt(int irq, void *dev_id)
{
	uint32 UartStatus;
#ifdef WAVE400_DEBUG_UART
    uint32 temp1,temp2;

    temp1 = MT_RdReg(MT_LOCAL_MIPS_BASE_ADDRESS, REG_UNMAPPED_IRQS_TO_MAPPER);
    temp2 = MT_RdReg(MT_LOCAL_MIPS_BASE_ADDRESS, REG_MAPPED_IRQS_FROM_MAPPER);
#endif

	UartStatus = MT_RdReg(MT_LOCAL_MIPS_BASE_ADDRESS, REG_UART_INT);
    //UartStatus |=0x7f00;
    
    //printk("enter: UNMAPPED_IRQS_TO_MAPPER = %x, MAPPED_IRQS_FROM_MAPPER = %x, stat b=%x ", temp1, temp2, UartStatus);

	// clear interrupt
	MT_WrReg(MT_LOCAL_MIPS_BASE_ADDRESS, REG_UART_INT, UartStatus);
#ifdef WAVE400_DEBUG_UART
    temp1 = MT_RdReg(MT_LOCAL_MIPS_BASE_ADDRESS, REG_UNMAPPED_IRQS_TO_MAPPER);
    temp2 = MT_RdReg(MT_LOCAL_MIPS_BASE_ADDRESS, REG_MAPPED_IRQS_FROM_MAPPER);

	UartStatus = MT_RdReg(MT_LOCAL_MIPS_BASE_ADDRESS, REG_UART_INT);
    printk("exit: UNMAPPED_IRQS_TO_MAPPER = %x, MAPPED_IRQS_FROM_MAPPER = %x, stat b=%x ", temp1, temp2, UartStatus);
#endif
	// TX Part
	if (UartStatus && REG_UART_WRITE_FIFO_WATER_LEVEL_IRQ_MASK)
	    wave400_uart_tx_isr(dev_id); /*needed???*/

	// RX Part
	if ((UartStatus && REG_UART_READ_FIFO_WATER_LEVEL_IRQ_MASK) ||
	    (UartStatus && REG_UART_IDLE_IRQ_MASK))
        wave400_uart_rx_isr(dev_id);
	 
    return IRQ_HANDLED;
}

static struct irq_chip external_irq_type = {
	.name     = "Lantiq WAVE400 Ext IRQ Controller",
	.mask_ack = wave400_disable_irq,  
	.mask     = wave400_disable_irq,
	.unmask   = wave400_enable_irq,
};

#if 0
static struct irqaction uart_irqaction = {
        .handler        = uart_interrupt,
    	.flags          = IRQF_DISABLED, /* for disable nested interrupts */ 
    	/* Lior.H - when we need to use-> IRQF_NOBALANCING ? */
        .name           = "wave400_uart",
};
#endif

asmlinkage void plat_irq_dispatch(void)
{
	while(1);
}

static inline int clz(unsigned long x)
{
	__asm__(
	"	.set	push					\n"
	"	.set	mips32					\n"
	"	clz	%0, %1					\n"
	"	.set	pop					\n"
	: "=r" (x)
	: "r" (x));

	return x;
}

void EXCEP_remap_line( MT_UINT32 int_line, MT_UINT32 map_address, MT_UINT32 num)
{
	MT_UINT32 org_map, result_map, t1 = 0, t2 = 0;


    printk("EXCEP_remap_line: int_line = 0x%08lx, map_address = 0x%08lx, num = 0x%08lx \n",int_line,map_address, num);
	org_map = MT_RdReg( MT_LOCAL_MIPS_BASE_ADDRESS, map_address );
	t1 = ~(0x1f << (num * 5));
	t2 = int_line << (num * 5);
	result_map = org_map & t1;
	result_map |= t2;
    printk("EXCEP_remap_line: org_map = 0x%08lx, result_map = 0x%08lx \n",org_map,result_map);
	MT_WrReg( MT_LOCAL_MIPS_BASE_ADDRESS, map_address, result_map );
#if 0
{
	int i=0;
    for(i=REG_IRQ_SOURCE_01_MAPPING ; i <= REG_IRQ_SOURCE_31_MAPPING ; i+=4)
    {
        printk("EXCEP_remap_line: map[%d] = 0x%08lx \n",i,MT_RdReg(MT_LOCAL_MIPS_BASE_ADDRESS, i));
    }
}
#endif
}

/*static */void wave400_uart_irq(void)
{
	do_IRQ(WAVE400_SERIAL_IRQ_OUT_INDEX/*0x1*/);
}

/*static */void wave400_net_irq(void)
{
	do_IRQ(WAVE400_SYNOP_ETHER_IRQ_OUT_INDEX/*0x3*/);
}
EXPORT_SYMBOL(wave400_net_irq);

/*static */void wave400_net_g2_irq(void)
{
	do_IRQ(WAVE400_SYNOP_ETHER_IRQ_GMAC2_OUT_INDEX/*0x4*/);
}
EXPORT_SYMBOL(wave400_net_g2_irq);

void wave400_wls_irq(void)
{
	do_IRQ(WAVE400_WIRELESS_IRQ_OUT_INDEX);
}

EXPORT_SYMBOL(wave400_wls_irq);

void wave400_init_irq(void)
{
	uint32 irq;
	/*
	31 - SI_TimerInt
	30 - SI_SWInt[1]
	29 - SI_SWInt[0]
	28 - timer0_irq
	27 - timer1_irq
	26 - timer2_irq
	25 - timer3_irq
	24 - N/A
	23 - N/A
	...
	05 - N/A 
	04 - N/A
	03 - N/A
	02 - UART
	01 - ETH
	00 - N/A - (Never used)
	*/

	/* Remap all interrupts to an unused line - then open only those that are used */ 
#if 1
    for(irq=REG_IRQ_SOURCE_01_MAPPING ; irq <= REG_IRQ_SOURCE_31_MAPPING ; irq+=4)
	{
        MT_WrReg(MT_LOCAL_MIPS_BASE_ADDRESS, irq, 0 );
	}

	for (irq = 1; irq <= 31; irq++) {
		set_irq_chip_and_handler(irq, &external_irq_type,
					 handle_level_irq);
		wave400_disable_irq(irq);
#else
	for (irq = 1; irq <= 31; irq++) {
		if ((irq!= WAVE400_SYNOP_ETHER_IRQ_IN_INDEX) |
		        (irq!= WAVE400_SERIAL_IRQ_IN_INDEX) |
		            (irq!= WAVE400_SYNOP_ETHER_IRQ_GMAC2_IN_INDEX) |
		                (irq!= WAVE400_WIRELESS_IRQ_IN_INDEX) |
		                    (irq!= WAVE400_TIMER_IRQ_IN_INDEX))
        {
	        wave400_register_static_irq_dummy(irq,0);

		    set_irq_chip_and_handler(irq, &external_irq_type,
					     handle_level_irq);

		    wave400_disable_irq(irq);
        }
#endif
	}
    
#if 0
    wave400_register_irq(2 /* in */,1 /* out */,&uart_irqaction, wave400_uart_irq); 
#endif

//wave400_enable_irq_all_mips(/*0xe0000001*/0x00000001);
}

#if 0
int wave400_register_static_irq_dummy(unsigned int irq_in, unsigned int irq_out)
{
  uint32 mapper_reg;
  uint32 mapper_offset;
  int res;

  mapper_reg = REG_IRQ_SOURCE_01_MAPPING + (((irq_in - 1) / 6) * 4);
  mapper_offset = ((irq_in - 1) % 6);
  printk("wave400_register_static_irq: irq_in = 0x%08x, irq_out = 0x%08x\n",irq_in,irq_out);
  printk("wave400_register_static_irq: mapper_reg = 0x%08lx, mapper_offset = 0x%08lx\n",mapper_reg,mapper_offset);
  EXCEP_remap_line( irq_out, mapper_reg, mapper_offset);
}
#endif

int wave400_register_static_irq(unsigned int irq_in, unsigned int irq_out, 
                             struct irqaction *action, vi_handler_t addr)
{
  uint32 mapper_reg;
  uint32 mapper_offset;
  int res;

  mapper_reg = REG_IRQ_SOURCE_01_MAPPING + (((irq_in - 1) / 6) * 4);
  mapper_offset = ((irq_in - 1) % 6);
  printk("wave400_register_static_irq: irq_in = 0x%08x, irq_out = 0x%08x\n",irq_in,irq_out);
  printk("wave400_register_static_irq: mapper_reg = 0x%08lx, mapper_offset = 0x%08lx\n",mapper_reg,mapper_offset);
  EXCEP_remap_line( irq_out, mapper_reg, mapper_offset);

  set_vi_handler(irq_out, addr);

  res = setup_irq(irq_out, action);
  if(0 != res) {
	 printk(KERN_INFO "wave400_irq: setup_irq() failed!!! **********\n");
    return res;
  }
#if 0
  wave400_enable_irq(irq_out);
#endif
  return 0;
}

EXPORT_SYMBOL(wave400_register_static_irq);
EXPORT_SYMBOL(wave400_enable_irq);
EXPORT_SYMBOL(wave400_disable_irq);

int wave400_register_irq(unsigned int irq_in, unsigned int irq_out, 
                       irq_handler_t handler, unsigned long flags,
                       const char *name, void *dev_id,
                       vi_handler_t addr)
{
    struct irqaction *action;
    int res;

    action = kzalloc(sizeof(struct irqaction), GFP_KERNEL);
    if (!action)
      return -ENOMEM;

    action->handler = handler;
    action->flags = flags;
    action->name = name;
    action->dev_id = dev_id;

    res = wave400_register_static_irq(irq_in, irq_out, action, addr);

    if(0 != res)
      kfree(action);

    return res;
}

EXPORT_SYMBOL(wave400_register_irq);

void wave400_unregister_irq(unsigned int irq_in, unsigned int irq_out, void* dev_id)
{
    uint32 mapper_reg;
    uint32 mapper_offset;

    wave400_disable_irq(irq_out);

    mapper_reg = REG_IRQ_SOURCE_01_MAPPING + (((irq_in - 1) / 6) * 4);
    mapper_offset = ((irq_in - 1) % 6);
    EXCEP_remap_line(0, mapper_reg, mapper_offset);

    set_vi_handler(irq_out, NULL);
    free_irq(irq_out, dev_id);
}

EXPORT_SYMBOL(wave400_unregister_irq);
