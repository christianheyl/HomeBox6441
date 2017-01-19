/******************************************************************************
**
** FILE NAME    : devices.c
** MODULES      : BSP Basic
**
** DATE         : 15 Feb. 2012
** AUTHOR       : Yinglei Huang
** DESCRIPTION  : device file for HN1
** COPYRIGHT    :       Copyright (c) 2009
**                      Lantiq Deutschland GmbH
**                      Am Campeon 3, 85579 Neubiberg, Germany
**
**    This program is free software; you can redistribute it and/or modify
**    it under the terms of the GNU General Public License as published by
**    the Free Software Foundation; either version 2 of the License, or
**    (at your option) any later version.
**
** HISTORY
** $Date        $Author         $Comment
** 15 Feb 2012   Yinglei     First version HNX
*******************************************************************************/

#include <linux/init.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/string.h>
#include <linux/mtd/physmap.h>
#include <linux/kernel.h>
#include <linux/reboot.h>
#include <linux/platform_device.h>
#include <linux/time.h>
#include <linux/io.h>
#include <linux/gpio.h>
#include <asm/irq.h>

/*
 *  Chip Specific Head File
 */
#include <asm/ifx/ifx_types.h>
#include <asm/ifx/ifx_regs.h>
#include <asm/ifx/common_routines.h>
#include <asm/ifx/ifx_gpio.h>
#include <asm/ifx/i2c-lantiq-platform.h>

//#include <irq.h>
//#include "devices.h"

#define REG32(addr) *((volatile u32 *)(addr))

#define IRQ_RES(resname,irq) {.name=resname,.start=(irq),.flags=IORESOURCE_IRQ}
#define MEM_RES(resname,adr_start,adr_end) \
    { .name=resname, .flags=IORESOURCE_MEM, \
      .start=((adr_start)&~KSEG1),.end=((adr_end)&~KSEG1) }

// I2C clk can be st to 50/100/200/400 kH
#define HNX_I2C_IN_CLK_MHZ  200
#define HNX_I2C_CLK_KHZ     50

extern  void __init hn1_register_i2c(void);

static struct resource hn1_i2c_resources[] = {
	MEM_RES("i2c", HNX_I2C_BASE,HNX_I2C_END),
	IRQ_RES("i2c_lb", IFX_HN1_I2C_IR2),
	IRQ_RES("i2c_b", IFX_HN1_I2C_IR3),
	IRQ_RES("i2c_err", IFX_HN1_I2C_IR4),
	IRQ_RES("i2c_p", IFX_HN1_I2C_IR5),
};

static int setup_i2c_pins(void *data)
{
	int ret = 0;

    ret = ifx_gpio_register(IFX_GPIO_MODULE_I2C);

	return ret;
}

static void unset_i2c_pins(void *data)
{
    ifx_gpio_deregister(IFX_GPIO_MODULE_I2C);
}

static struct platform_device hn1_i2c_device;
static struct i2c_lantiq_platform_data hn1_i2c_platform_data = {
	.i2c_clock	= HNX_I2C_CLK_KHZ,
	.clock_khz	= HNX_I2C_IN_CLK_MHZ*1000,
	.setup_pins	= setup_i2c_pins,
	.unset_pins	= unset_i2c_pins,
	.data		= &hn1_i2c_device,
};

static struct platform_device hn1_i2c_device = {
	.name		= "i2c-lantiq",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(hn1_i2c_resources),
	.resource	= hn1_i2c_resources,
	.dev		= {
		.platform_data = &hn1_i2c_platform_data,
	},
};

static void enable_AHB_bus(void) {
    REG32(IFX_PMU_PWDCR) = REG32(IFX_PMU_PWDCR) & 0xFFFF7FFF;
}

 void __init hn1_register_i2c(void)
 {
	platform_device_register(&hn1_i2c_device);
    enable_AHB_bus();
 }

