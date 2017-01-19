/*
 *  linux/drivers/char/8250.c
 *
 *  Driver for wave400 serial ports
 *
 *  Based on drivers/char/serial.c, by Linus Torvalds, Theodore Ts'o.
 *
 *  Copyright (C) 2001 Russell King.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 *  $Id: wave400.c
 *
 * A note about mapbase / membase
 *
 *  mapbase is the physical address of the IO port.
 *  membase is an 'ioremapped' cookie.
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/ioport.h>
#include <linux/init.h>
#include <linux/console.h>
#include <linux/sysrq.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/tty.h>
#include <linux/tty_flip.h>
#include <linux/serial_reg.h>
//#include <linux/serial_core.h>
#include <linux/serial.h>
#include <linux/nmi.h>
#include <linux/mutex.h>

#include <asm/io.h>
#include <asm/irq.h>
#include <asm/serial.h>

#include <linux/serial_core.h>
/*change name: ?*/
#include <wave400_defs.h>
#include <wave400_cnfg.h>
#include <wave400_chadr.h>
#include <wave400_chipreg.h>
#include <wave400_emerald_env_regs.h>
#include <wave400_uart.h>
#include <wave400_time.h>
#include <wave400_interrupt.h>




#define UART_NR			1	/* only use one port */

int uart_done = 0;


static struct irqaction uart_irqaction = {
      .handler        = uart_interrupt,
    	.flags          = IRQF_DISABLED, /* for disable nested interrupts */ 
    	/* Lior.H - when we need to use-> IRQF_NOBALANCING ? */
      .name           = "wave400_uart",
};


static void wave400_console_write(struct console *co, const char *s, unsigned count);
static void wave400_start_tx(struct uart_port *port);

/*uart mux to host*/
#define REG_UART_CONTROL_MUX_MPU_BIT 0x100

#define uart_full_hw_buffer() (((MT_RdReg(UART_BASE, REG_UART_BYTES_IN_TX_FIFO) & UART_FWL_TX_MASK)>>5) == 16)
#define uart_get_char() (MT_RdReg(UART_BASE, REG_UART_READ_DATA) & UART_RD_DATA_MASK)
/****************************************************************************
* wave400_config_hw
*
* REG_UART_FIFO_WATER_LEVEL (0x18):
* REG_UART_RX_FIFO_WATER_LEVEL - [mask = 0x1f]
* REG_UART_TX_FIFO_WATER_LEVEL - [mask = 0x3e0]
*
* REG_UART_INT (0x1c):
* Those  registers define the UART water level for RX and TX FIFOs
* The register is split to 2 section, mask_int and status int (clear write 1)
* interesting mask:
*   UART_read_FIFO_water_level_IRQ_enable - [mask =  0x20]
*   UART_write_FIFO_water_level_IRQ_enable - [mask =  0x40] 
* interesting status:
*   UART_read_FIFO_water_level_IRQ - [mask =  0x2000]
*   UART_write_FIFO_water_level_IRQ - [mask = 0x4000]
*/
void wave400_config_hw(uint32 baud, uint8 data, uint8 parity, uint8 stop)
{
	uint32 temp = 0;
	
	/* disable interrupts */
    MT_WrReg(UART_BASE,REG_UART_INT, 0x00); 

	/* set up baud rate */
	{ 
		uint32 divisor;

		/* set divisor - clock 40mHz assumed */
#ifndef CONFIG_VBG400_CHIPIT
		divisor = ((WAVE400_SYSTEM_CLK) / (16*baud)) - 1;
#else
        divisor = ((WAVE400_SYSTEM_CLK/WAVE400_SCALE_VAL) / (16*baud)) - 1;
#endif
		MT_WrReg(UART_BASE,REG_UART_CLOCK_DIVISION_RATIO, divisor & 0xff);
 	}

	/* set data format */
	MT_WrReg(UART_BASE,REG_UART_CONTROL, data|parity|stop|REG_UART_CONTROL_MUX_MPU_BIT); 
	MT_WrReg(UART_BASE,REG_UART_RX_IDLE_COUNTER, 90); //10 time one byte duration

    /*============ Set WATER_L ======================================== */
    temp = 0 << REG_UART_TX_FIFO_WATER_LEVEL_SHIFT;  //TX
    temp |= (7 << REG_UART_RX_FIFO_WATER_LEVEL_SHIFT); //RX
    MT_WrReg(UART_BASE,REG_UART_RX_FIFO_WATER_LEVEL, temp); //same register as TX

	/*============ Set allowed interrupts ==================================== */
    temp = (1 << REG_UART_READ_FIFO_WATER_LEVEL_IRQ_ENABLE_SHIFT);   //RX
    temp |= 1 << REG_UART_WRITE_FIFO_WATER_LEVEL_IRQ_ENABLE_SHIFT; //TX
    temp |= (1 << REG_UART_IDLE_IRQ_ENABLE_SHIFT);                   //IDLE
	
	MT_WrReg(UART_BASE,REG_UART_INT,temp);

}


static void wave400_stop_tx(struct uart_port *port)
{
}

static void wave400_stop_rx(struct uart_port *port)
{
}

static void wave400_enable_ms(struct uart_port *port)
{
	return;
}

/********************************************************
* Name: wave400_uart_tx_isr
*
* Description: same handling as in start_tx clb.
* if there is data pending, send it out.
*
* TODO - if WL > 0, use timer in order to clear FIFO in polling mode.
*/
void wave400_uart_tx_isr(struct uart_port *port)
{
/*    struct console *co = NULL;
    const char *s = NULL;
    unsigned count = 0;
    wave400_start_tx(co, s, count); */
	 wave400_start_tx(port);
}

/********************************************************
* Name: wave400_uart_rx_isr
*
* Description: data in HW fifo.
* send it up to the core layer.
*
* UART_read_FIFO_water_level_IRQ_enable - [mask =  0x20]
*/
void wave400_uart_rx_isr(struct uart_port *port)
{
	unsigned char index, len;
	unsigned char char_in;
    char flag;
	unsigned int status;

	len = (unsigned char)UART_MCOR_RD_RX_BYTES;
	flag = TTY_NORMAL;
	/* TODO- error handling here...
	if ()..
	  flag = TTY_BREAK;
      etc...
    */
    status = 0;
	for (index=0; index <len; index++)
	{
		//char_in = (unsigned char) MT_RdReg( UART_BASE, REG_UART_READ_DATA) & UART_RD_DATA_MASK;
		char_in = (unsigned char) uart_get_char();
		port->icount.rx++;
	    uart_insert_char(port, status, UART_LSR_OE, char_in, flag);
	}

//	tty_flip_buffer_push(port->info->tty);
	tty_flip_buffer_push(port->state->port.tty);
}


static unsigned int wave400_tx_empty(struct uart_port *port)
{
	return 0;
}

static unsigned int wave400_get_mctrl(struct uart_port *port)
{
	return 0;
}

static void wave400_set_mctrl(struct uart_port *port, unsigned int mctrl)
{
	return;
}

static void wave400_break_ctl(struct uart_port *port, int break_state)
{
    return;
}

#define BOTH_EMPTY (UART_LSR_TEMT | UART_LSR_THRE)

static int wave400_startup(struct uart_port *port)
{
	if (uart_done)
        return 0;

	/*config uart*/
	printk(KERN_INFO "Serial: wave400 driver startup\n");
    wave400_config_hw(UART_RATE_115200,
                   UART_TX_OUT_ENABLE_YES/*0x1*/,
                   UART_PARITY_NONE/*0*/,
                   UART_STOP_ONEBIT/*0*/);

	/*config vector interrupt*/
	wave400_register_static_irq(WAVE400_SERIAL_IRQ_IN_INDEX, WAVE400_SERIAL_IRQ_OUT_INDEX, &uart_irqaction, wave400_uart_irq);
    wave400_enable_irq(WAVE400_SERIAL_IRQ_OUT_INDEX);
	uart_done = 1;

	return 0;
}

static void wave400_shutdown(struct uart_port *port)
{
    /*TODO*/
}

static void
wave400_set_termios(struct uart_port *port, struct ktermios *termios,
		       struct ktermios *old)
{
    return;
}

static void
wave400_pm(struct uart_port *port, unsigned int state,
	      unsigned int oldstate)
{
	return;
}

static void wave400_release_port(struct uart_port *port)
{
	return;
}

static int wave400_request_port(struct uart_port *port)
{
	return 0;
}

static void wave400_config_port(struct uart_port *port, int flags)
{
	return;
}

static int
wave400_verify_port(struct uart_port *port, struct serial_struct *ser)
{
	return 0;
}

static const char *
wave400_type(struct uart_port *port)
{
	return "wave400tty";
}

#define WAVE400_CONSOLE	&wave400_console

static struct uart_driver wave400_uart_reg;

static struct console wave400_console = {
	.name = "ttyS",
	.write = wave400_console_write,
	.device = uart_console_device,
	.data		= &wave400_uart_reg,
	.flags = CON_PRINTBUFFER | CON_ENABLED | CON_CONSDEV,
	.index = -1,
};

static struct uart_ops wave400_ops = {
	.tx_empty	    = wave400_tx_empty,     /*NULL*/
	.set_mctrl	    = wave400_set_mctrl,    /*NULL*/
	.get_mctrl	    = wave400_get_mctrl,    /*NULL*/
	.stop_tx	    = wave400_stop_tx,
	.start_tx	    = wave400_start_tx,
	.stop_rx        = wave400_stop_rx,
	.enable_ms	    = wave400_enable_ms,    /*NULL*/
	.break_ctl	    = wave400_break_ctl,    /*NULL*/
	.startup	    = wave400_startup,
	.shutdown	    = wave400_shutdown,
	.set_termios    = wave400_set_termios,  /*NULL*/
	.pm		        = wave400_pm,           /*NULL*/
	.type		    = wave400_type,
	.release_port	= wave400_release_port, /*NULL*/
	.request_port	= wave400_request_port, /*NULL*/
	.config_port	= wave400_config_port,  /*NULL*/
	.verify_port	= wave400_verify_port,  /*NULL*/
};

static struct uart_port wave400_port = {
	.ops		= &wave400_ops,
   .type       = PORT_WAVE400,
};

//static struct uart_driver wave400_uart_reg;

static struct uart_driver wave400_uart_reg = {
	.owner			= THIS_MODULE,
	.driver_name	= "serial",
	.dev_name		= "ttyS",
	.major			= TTY_MAJOR,
	.minor			= 64,
	.nr			    = UART_NR,
	.cons			= WAVE400_CONSOLE,
};


static void wave400_console_write(struct console *co, const char *s, unsigned count)
{
	struct uart_port *port = &wave400_port;
	wave400_start_tx(port);
}

static void wave400_start_tx(struct uart_port *port)
{
//	struct uart_port *port = &wave400_port;
//	struct circ_buf *xmit = &port->info->xmit; //ptr to xmit buff
	int count_local;
	struct circ_buf *xmit = NULL;

	if (!port->state)
		return;

	xmit = &port->state->xmit; //ptr to xmit buff

	if (port->x_char) {
		port->icount.tx++;
        //printk("wave400_start_tx: xon-xoff non-zero=%c\n", port->x_char); 
        port->x_char = 0;
	}

	/*buff handling:*/
	count_local = uart_circ_chars_pending(xmit);
	if (!count_local)
    {
	    return;
    }

	do {
		//if ( uart_circ_empty(xmit) || (((MT_RdReg( UART_BASE, REG_UART_BYTES_IN_TX_FIFO) & UART_FWL_TX_MASK)>>5) == 16) )
		if ( uart_circ_empty(xmit) || uart_full_hw_buffer() )
			break; //who trigger tx again?
	    MT_WrReg(UART_BASE ,WR_ADDR, (volatile unsigned long)xmit->buf[xmit->tail]);
	    //MT_WrReg(UART_BASE ,WR_ADDR, ((volatile unsigned long)xmit->buf[xmit->tail] & 0x000000FF));
		xmit->tail = (xmit->tail + 1) & (UART_XMIT_SIZE - 1);
		port->icount.tx++;
	} while (--count_local > 0);
    
    if (uart_circ_chars_pending(xmit))
        uart_write_wakeup(port);
    /*? can get port->x_char ??*/
    
}


static int __init wave400_uart_init(void)
{
    struct irqaction *action = &uart_irqaction;
    struct uart_port *up = &wave400_port;
    int ret;

	printk(KERN_INFO "Serial: wave400 driver init\n");

    /*register uart_port for use in interrupt handler*/
    action->dev_id = up;

	ret = uart_register_driver(&wave400_uart_reg);
	if (ret)
		goto out;

    ret = uart_add_one_port(&wave400_uart_reg, &wave400_port);
	if (ret)
	    uart_unregister_driver(&wave400_uart_reg);
	
out:
	return ret;
}

static int __init wave400_console_init(void)
{
	printk(KERN_INFO ": wave400_console_init: call register\n");
	register_console(&wave400_console);
    /*no need to unregister early console if set flag=CON_CONSDEV in new console,
     done in register_console().
    unregister_console(&early_lntq_console);*/

	return 0;
}

console_initcall(wave400_console_init);


static void __exit wave400_uart_exit(void)
{

	uart_unregister_driver(&wave400_uart_reg);
}

module_init(wave400_uart_init);
module_exit(wave400_uart_exit);

