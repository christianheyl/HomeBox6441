/******************************************************************************
**
** FILE NAME    : ar10_ref_board.c
** PROJECT      : IFX UEIP
** MODULES      : BSP Basic
**
** DATE         : 27 May 2009
** AUTHOR       : Xu Liang
** DESCRIPTION  : source file for AR10 
** COPYRIGHT    :       Copyright (c) 2009
**                      Infineon Technologies AG
**                      Am Campeon 1-12, 85579 Neubiberg, Germany
**
**    This program is free software; you can redistribute it and/or modify
**    it under the terms of the GNU General Public License as published by
**    the Free Software Foundation; either version 2 of the License, or
**    (at your option) any later version.
**
** HISTORY
** $Date        $Author         $Comment
** 27 May 2009   Xu Liang        The first UEIP release
*******************************************************************************/



#ifndef AUTOCONF_INCLUDED
#include <linux/config.h>
#endif /* AUTOCONF_INCLUDED */

#include <linux/module.h>
#include <linux/types.h>
#include <linux/kallsyms.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>

#include <asm/ifx/ifx_regs.h>
#include <asm/ifx/ifx_types.h>
#include <asm/ifx/ifx_board.h>
#include <asm/ifx/irq.h>
#include <asm/ifx/ifx_gpio.h>
#include <asm/ifx/ifx_ledc.h>
#include <asm/ifx/ifx_led.h>


/* GPIO PIN to Module Mapping and default PIN configuration */
struct ifx_gpio_ioctl_pin_config g_board_gpio_pin_map[] = {
    //  module_id of last item must be IFX_GPIO_PIN_AVAILABLE
    /*
     *  LED Controller
     */
#ifndef CONFIG_QUALCOMM_AP_PPA
#ifdef CONFIG_AR10_FAMILY_BOARD_2
    {IFX_GPIO_MODULE_LED, IFX_GPIO_PIN_ID(0, 4), IFX_GPIO_IOCTL_PIN_CONFIG_DIR_OUT | IFX_GPIO_IOCTL_PIN_CONFIG_ALTSEL0_CLEAR | IFX_GPIO_IOCTL_PIN_CONFIG_ALTSEL1_CLEAR | IFX_GPIO_IOCTL_PIN_CONFIG_OD_SET},
    {IFX_GPIO_MODULE_LED, IFX_GPIO_PIN_ID(0, 5), IFX_GPIO_IOCTL_PIN_CONFIG_DIR_OUT | IFX_GPIO_IOCTL_PIN_CONFIG_ALTSEL0_CLEAR | IFX_GPIO_IOCTL_PIN_CONFIG_ALTSEL1_CLEAR | IFX_GPIO_IOCTL_PIN_CONFIG_OD_SET},
    {IFX_GPIO_MODULE_LED, IFX_GPIO_PIN_ID(0, 6), IFX_GPIO_IOCTL_PIN_CONFIG_DIR_OUT | IFX_GPIO_IOCTL_PIN_CONFIG_ALTSEL0_CLEAR | IFX_GPIO_IOCTL_PIN_CONFIG_ALTSEL1_CLEAR | IFX_GPIO_IOCTL_PIN_CONFIG_OD_SET},
    //{IFX_GPIO_MODULE_LED, IFX_GPIO_PIN_ID(0, 3), IFX_GPIO_IOCTL_PIN_CONFIG_DIR_OUT | IFX_GPIO_IOCTL_PIN_CONFIG_ALTSEL0_CLEAR | IFX_GPIO_IOCTL_PIN_CONFIG_ALTSEL1_CLEAR | IFX_GPIO_IOCTL_PIN_CONFIG_OD_SET},
#else
    {IFX_GPIO_MODULE_LEDC, IFX_GPIO_PIN_ID(0, 4), IFX_GPIO_IOCTL_PIN_CONFIG_DIR_OUT | IFX_GPIO_IOCTL_PIN_CONFIG_ALTSEL0_SET | IFX_GPIO_IOCTL_PIN_CONFIG_ALTSEL1_CLEAR | IFX_GPIO_IOCTL_PIN_CONFIG_OD_SET},
    {IFX_GPIO_MODULE_LEDC, IFX_GPIO_PIN_ID(0, 5), IFX_GPIO_IOCTL_PIN_CONFIG_DIR_OUT | IFX_GPIO_IOCTL_PIN_CONFIG_ALTSEL0_SET | IFX_GPIO_IOCTL_PIN_CONFIG_ALTSEL1_CLEAR | IFX_GPIO_IOCTL_PIN_CONFIG_OD_SET},
    {IFX_GPIO_MODULE_LEDC, IFX_GPIO_PIN_ID(0, 6), IFX_GPIO_IOCTL_PIN_CONFIG_DIR_OUT | IFX_GPIO_IOCTL_PIN_CONFIG_ALTSEL0_SET | IFX_GPIO_IOCTL_PIN_CONFIG_ALTSEL1_CLEAR | IFX_GPIO_IOCTL_PIN_CONFIG_OD_SET},
#endif
#endif
    /*
     *  USB
     */
/* Terry 20140801, since native USB driver used we need to disable code here */
#if 1
	{IFX_GPIO_MODULE_USB, IFX_GPIO_PIN_ID(2, 10), IFX_GPIO_IOCTL_PIN_CONFIG_DIR_OUT | IFX_GPIO_IOCTL_PIN_CONFIG_ALTSEL0_CLEAR | IFX_GPIO_IOCTL_PIN_CONFIG_ALTSEL1_CLEAR | IFX_GPIO_IOCTL_PIN_CONFIG_PUDSEL_SET | IFX_GPIO_IOCTL_PIN_CONFIG_PUDEN_SET},
	{IFX_GPIO_MODULE_USB, IFX_GPIO_PIN_ID(2, 11), IFX_GPIO_IOCTL_PIN_CONFIG_DIR_OUT | IFX_GPIO_IOCTL_PIN_CONFIG_ALTSEL0_CLEAR | IFX_GPIO_IOCTL_PIN_CONFIG_ALTSEL1_CLEAR | IFX_GPIO_IOCTL_PIN_CONFIG_PUDSEL_SET | IFX_GPIO_IOCTL_PIN_CONFIG_PUDEN_SET},
#else
#if defined(CONFIG_USB_HOST_IFX) || defined(CONFIG_USB_HOST_IFX_MODULE)
	#if defined(CONFIG_AR10_FAMILY_BOARD_1_2)
    	{IFX_GPIO_MODULE_LED, IFX_GPIO_PIN_ID(1, 10), IFX_GPIO_IOCTL_PIN_CONFIG_DIR_OUT | IFX_GPIO_IOCTL_PIN_CONFIG_ALTSEL0_CLEAR | IFX_GPIO_IOCTL_PIN_CONFIG_ALTSEL1_CLEAR | IFX_GPIO_IOCTL_PIN_CONFIG_OD_SET},
    	{IFX_GPIO_MODULE_LED, IFX_GPIO_PIN_ID(1, 11), IFX_GPIO_IOCTL_PIN_CONFIG_DIR_OUT | IFX_GPIO_IOCTL_PIN_CONFIG_ALTSEL0_CLEAR | IFX_GPIO_IOCTL_PIN_CONFIG_ALTSEL1_CLEAR | IFX_GPIO_IOCTL_PIN_CONFIG_OD_SET},
	#endif
	#if   defined(IFX_LEDGPIO_USB_VBUS1) 
			{IFX_GPIO_MODULE_LED, IFX_LEDGPIO_USB_VBUS1, IFX_GPIO_IOCTL_PIN_CONFIG_DIR_OUT | IFX_GPIO_IOCTL_PIN_CONFIG_ALTSEL0_CLEAR | IFX_GPIO_IOCTL_PIN_CONFIG_ALTSEL1_CLEAR | IFX_GPIO_IOCTL_PIN_CONFIG_PUDSEL_SET | IFX_GPIO_IOCTL_PIN_CONFIG_PUDEN_SET | IFX_GPIO_IOCTL_PIN_CONFIG_OD_SET},
	#endif
	#if   defined(IFX_GPIO_USB_VBUS)
			{IFX_GPIO_MODULE_USB, IFX_GPIO_USB_VBUS, IFX_GPIO_IOCTL_PIN_CONFIG_DIR_OUT | IFX_GPIO_IOCTL_PIN_CONFIG_ALTSEL0_CLEAR | IFX_GPIO_IOCTL_PIN_CONFIG_ALTSEL1_CLEAR | IFX_GPIO_IOCTL_PIN_CONFIG_PUDSEL_SET | IFX_GPIO_IOCTL_PIN_CONFIG_PUDEN_SET},
	#endif
#endif
#if   (defined(CONFIG_USB_HOST_IFX) || defined(CONFIG_USB_HOST_IFX_MODULE)) && defined(CONFIG_USB_HOST_IFX_LED)
		{IFX_GPIO_MODULE_LED, IFX_LEDGPIO_USB_LED, IFX_GPIO_IOCTL_PIN_CONFIG_DIR_OUT | IFX_GPIO_IOCTL_PIN_CONFIG_ALTSEL0_CLEAR | IFX_GPIO_IOCTL_PIN_CONFIG_ALTSEL1_CLEAR | IFX_GPIO_IOCTL_PIN_CONFIG_OD_SET},
#elif (defined(CONFIG_USB_GADGET_IFX) || defined(CONFIG_USB_GADGET_IFX_MODULE)) && defined(CONFIG_USB_GADGET_IFX_LED)
//		{IFX_GPIO_MODULE_LED, IFX_LEDGPIO_USB_LED, IFX_GPIO_IOCTL_PIN_CONFIG_DIR_OUT | IFX_GPIO_IOCTL_PIN_CONFIG_ALTSEL0_CLEAR | IFX_GPIO_IOCTL_PIN_CONFIG_ALTSEL1_CLEAR | IFX_GPIO_IOCTL_PIN_CONFIG_OD_SET},	
#endif
#endif /* #if 0 */
		/* 
		 *	LED
		 */
    //{IFX_GPIO_MODULE_LED, IFX_GPIO_PIN_ID(0, 14), IFX_GPIO_IOCTL_PIN_CONFIG_DIR_OUT | IFX_GPIO_IOCTL_PIN_CONFIG_ALTSEL0_CLEAR | IFX_GPIO_IOCTL_PIN_CONFIG_ALTSEL1_CLEAR | IFX_GPIO_IOCTL_PIN_CONFIG_PUDSEL_SET | IFX_GPIO_IOCTL_PIN_CONFIG_PUDEN_SET | IFX_GPIO_IOCTL_PIN_CONFIG_OD_SET},
    //{IFX_GPIO_MODULE_LED, IFX_GPIO_PIN_ID(1, 3), IFX_GPIO_IOCTL_PIN_CONFIG_DIR_OUT | IFX_GPIO_IOCTL_PIN_CONFIG_ALTSEL0_CLEAR | IFX_GPIO_IOCTL_PIN_CONFIG_ALTSEL1_CLEAR | IFX_GPIO_IOCTL_PIN_CONFIG_PUDSEL_SET | IFX_GPIO_IOCTL_PIN_CONFIG_PUDEN_SET | IFX_GPIO_IOCTL_PIN_CONFIG_OD_SET},
    //{IFX_GPIO_MODULE_LED, IFX_GPIO_PIN_ID(0, 1), IFX_GPIO_IOCTL_PIN_CONFIG_DIR_OUT | IFX_GPIO_IOCTL_PIN_CONFIG_ALTSEL0_CLEAR | IFX_GPIO_IOCTL_PIN_CONFIG_ALTSEL1_CLEAR | IFX_GPIO_IOCTL_PIN_CONFIG_PUDSEL_SET | IFX_GPIO_IOCTL_PIN_CONFIG_PUDEN_SET | IFX_GPIO_IOCTL_PIN_CONFIG_OD_SET},
    //{IFX_GPIO_MODULE_LED, IFX_GPIO_PIN_ID(1, 9), IFX_GPIO_IOCTL_PIN_CONFIG_DIR_OUT | IFX_GPIO_IOCTL_PIN_CONFIG_ALTSEL0_CLEAR | IFX_GPIO_IOCTL_PIN_CONFIG_ALTSEL1_CLEAR | IFX_GPIO_IOCTL_PIN_CONFIG_PUDSEL_SET | IFX_GPIO_IOCTL_PIN_CONFIG_PUDEN_SET | IFX_GPIO_IOCTL_PIN_CONFIG_OD_SET},
	/* Telefon_LED */
    {IFX_GPIO_MODULE_LED, IFX_GPIO_PIN_ID(0, 10), IFX_GPIO_IOCTL_PIN_CONFIG_DIR_OUT | IFX_GPIO_IOCTL_PIN_CONFIG_ALTSEL0_CLEAR | IFX_GPIO_IOCTL_PIN_CONFIG_ALTSEL1_CLEAR | IFX_GPIO_IOCTL_PIN_CONFIG_PUDSEL_SET | IFX_GPIO_IOCTL_PIN_CONFIG_PUDEN_SET | IFX_GPIO_IOCTL_PIN_CONFIG_OD_SET},
	/* DECT_LED */
	/* Terry 20150317, need to hack for new/old boards, should change (0, 11) -> (0, 3) */
    {IFX_GPIO_MODULE_LED, IFX_GPIO_PIN_ID(0, 3), IFX_GPIO_IOCTL_PIN_CONFIG_DIR_OUT | IFX_GPIO_IOCTL_PIN_CONFIG_ALTSEL0_CLEAR | IFX_GPIO_IOCTL_PIN_CONFIG_ALTSEL1_CLEAR | IFX_GPIO_IOCTL_PIN_CONFIG_PUDSEL_SET | IFX_GPIO_IOCTL_PIN_CONFIG_PUDEN_SET | IFX_GPIO_IOCTL_PIN_CONFIG_OD_SET},
	/* QTN_RESET */
	/* Terry 20150317, need to hack for new/old boards, should change (3, 10) -> (0, 14) */
	{IFX_GPIO_MODULE_LED, IFX_GPIO_PIN_ID(0, 14), IFX_GPIO_IOCTL_PIN_CONFIG_DIR_OUT | IFX_GPIO_IOCTL_PIN_CONFIG_ALTSEL0_CLEAR | IFX_GPIO_IOCTL_PIN_CONFIG_ALTSEL1_CLEAR | IFX_GPIO_IOCTL_PIN_CONFIG_PUDSEL_SET | IFX_GPIO_IOCTL_PIN_CONFIG_PUDEN_SET | IFX_GPIO_IOCTL_PIN_CONFIG_OD_SET},
    //{IFX_GPIO_MODULE_LED, IFX_GPIO_PIN_ID(1, 2), IFX_GPIO_IOCTL_PIN_CONFIG_DIR_OUT | IFX_GPIO_IOCTL_PIN_CONFIG_ALTSEL0_CLEAR | IFX_GPIO_IOCTL_PIN_CONFIG_ALTSEL1_CLEAR | IFX_GPIO_IOCTL_PIN_CONFIG_PUDSEL_SET | IFX_GPIO_IOCTL_PIN_CONFIG_PUDEN_SET | IFX_GPIO_IOCTL_PIN_CONFIG_OD_SET},
    //{IFX_GPIO_MODULE_LED, IFX_GPIO_PIN_ID(3, 13), IFX_GPIO_IOCTL_PIN_CONFIG_DIR_OUT | IFX_GPIO_IOCTL_PIN_CONFIG_ALTSEL0_CLEAR | IFX_GPIO_IOCTL_PIN_CONFIG_ALTSEL1_CLEAR | IFX_GPIO_IOCTL_PIN_CONFIG_PUDSEL_SET | IFX_GPIO_IOCTL_PIN_CONFIG_PUDEN_SET | IFX_GPIO_IOCTL_PIN_CONFIG_OD_SET},
    /*
     * ARCADYAN HAL input
     */
    	/* SIM detection */
    //{IFX_GPIO_MODULE_ARC_HAL, IFX_GPIO_PIN_ID(0, 9), IFX_GPIO_IOCTL_PIN_CONFIG_DIR_IN | IFX_GPIO_IOCTL_PIN_CONFIG_ALTSEL0_SET | IFX_GPIO_IOCTL_PIN_CONFIG_ALTSEL1_SET | IFX_GPIO_IOCTL_PIN_CONFIG_PUDEN_SET | IFX_GPIO_IOCTL_PIN_CONFIG_PUDSEL_SET},
    /* WPS button */
    //{IFX_GPIO_MODULE_ARC_HAL, IFX_GPIO_PIN_ID(0, 15), IFX_GPIO_IOCTL_PIN_CONFIG_DIR_IN | IFX_GPIO_IOCTL_PIN_CONFIG_ALTSEL0_SET | IFX_GPIO_IOCTL_PIN_CONFIG_ALTSEL1_SET | IFX_GPIO_IOCTL_PIN_CONFIG_PUDEN_SET | IFX_GPIO_IOCTL_PIN_CONFIG_PUDSEL_SET},
    /* default button */
    //{IFX_GPIO_MODULE_ARC_HAL, IFX_GPIO_PIN_ID(1, 1), IFX_GPIO_IOCTL_PIN_CONFIG_DIR_IN | IFX_GPIO_IOCTL_PIN_CONFIG_ALTSEL0_SET | IFX_GPIO_IOCTL_PIN_CONFIG_ALTSEL1_SET | IFX_GPIO_IOCTL_PIN_CONFIG_PUDEN_SET | IFX_GPIO_IOCTL_PIN_CONFIG_PUDSEL_SET},
	/* dect button */
	/* Terry 20150317, need to hack for new/old boards, should change (0, 3) -> (0, 11) */
	{IFX_GPIO_MODULE_ARC_HAL, IFX_GPIO_PIN_ID(0, 11), IFX_GPIO_IOCTL_PIN_CONFIG_DIR_IN | IFX_GPIO_IOCTL_PIN_CONFIG_ALTSEL0_CLEAR | IFX_GPIO_IOCTL_PIN_CONFIG_ALTSEL1_CLEAR | IFX_GPIO_IOCTL_PIN_CONFIG_PUDEN_SET | IFX_GPIO_IOCTL_PIN_CONFIG_PUDSEL_SET},
	/* wlan/wps button */
	/* Terry 20150317, need to hack for new/old boards, should change (0, 14) -> (3, 10) */
	{IFX_GPIO_MODULE_ARC_HAL, IFX_GPIO_PIN_ID(3, 10), IFX_GPIO_IOCTL_PIN_CONFIG_DIR_IN | IFX_GPIO_IOCTL_PIN_CONFIG_ALTSEL0_CLEAR | IFX_GPIO_IOCTL_PIN_CONFIG_ALTSEL1_CLEAR | IFX_GPIO_IOCTL_PIN_CONFIG_PUDEN_SET | IFX_GPIO_IOCTL_PIN_CONFIG_PUDSEL_SET},
	/* reset button */
	{IFX_GPIO_MODULE_ARC_HAL, IFX_GPIO_PIN_ID(3, 13), IFX_GPIO_IOCTL_PIN_CONFIG_DIR_IN | IFX_GPIO_IOCTL_PIN_CONFIG_ALTSEL0_CLEAR | IFX_GPIO_IOCTL_PIN_CONFIG_ALTSEL1_CLEAR | IFX_GPIO_IOCTL_PIN_CONFIG_PUDEN_SET | IFX_GPIO_IOCTL_PIN_CONFIG_PUDSEL_SET},
    /*
     * ARCADYAN HAL output
     */
    /* Power_Sratrt_up */
    {IFX_GPIO_MODULE_LED, IFX_GPIO_PIN_ID(1, 10), IFX_GPIO_IOCTL_PIN_CONFIG_DIR_OUT | IFX_GPIO_IOCTL_PIN_CONFIG_ALTSEL0_CLEAR | IFX_GPIO_IOCTL_PIN_CONFIG_ALTSEL1_CLEAR | IFX_GPIO_IOCTL_PIN_CONFIG_PUDSEL_SET | IFX_GPIO_IOCTL_PIN_CONFIG_PUDEN_SET | IFX_GPIO_IOCTL_PIN_CONFIG_OD_SET},
    /* Power_Update */
    {IFX_GPIO_MODULE_LED, IFX_GPIO_PIN_ID(1, 11), IFX_GPIO_IOCTL_PIN_CONFIG_DIR_OUT | IFX_GPIO_IOCTL_PIN_CONFIG_ALTSEL0_CLEAR | IFX_GPIO_IOCTL_PIN_CONFIG_ALTSEL1_CLEAR | IFX_GPIO_IOCTL_PIN_CONFIG_PUDSEL_SET | IFX_GPIO_IOCTL_PIN_CONFIG_PUDEN_SET | IFX_GPIO_IOCTL_PIN_CONFIG_OD_SET},
    /* lte wake up */
    //{IFX_GPIO_MODULE_ARC_HAL, IFX_GPIO_PIN_ID(1, 0), IFX_GPIO_IOCTL_PIN_CONFIG_DIR_OUT | IFX_GPIO_IOCTL_PIN_CONFIG_ALTSEL0_CLEAR | IFX_GPIO_IOCTL_PIN_CONFIG_ALTSEL1_CLEAR | IFX_GPIO_IOCTL_PIN_CONFIG_PUDSEL_SET | IFX_GPIO_IOCTL_PIN_CONFIG_PUDEN_SET | IFX_GPIO_IOCTL_PIN_CONFIG_OD_SET},
    /* usb power on */
    //{IFX_GPIO_MODULE_ARC_HAL, IFX_GPIO_PIN_ID(0, 3), IFX_GPIO_IOCTL_PIN_CONFIG_DIR_OUT | IFX_GPIO_IOCTL_PIN_CONFIG_ALTSEL0_CLEAR | IFX_GPIO_IOCTL_PIN_CONFIG_ALTSEL1_CLEAR | IFX_GPIO_IOCTL_PIN_CONFIG_PUDSEL_SET | IFX_GPIO_IOCTL_PIN_CONFIG_PUDEN_SET | IFX_GPIO_IOCTL_PIN_CONFIG_OD_SET},

	/*
	 *  SSC - SPI
	 */
	/* SPI DIN - input */
	{IFX_GPIO_MODULE_SSC, IFX_GPIO_PIN_ID(1, 0), IFX_GPIO_IOCTL_PIN_CONFIG_DIR_IN | IFX_GPIO_IOCTL_PIN_CONFIG_ALTSEL0_SET | IFX_GPIO_IOCTL_PIN_CONFIG_ALTSEL1_CLEAR},
	/* SPI DOUT - output */
	{IFX_GPIO_MODULE_SSC, IFX_GPIO_PIN_ID(1, 1), IFX_GPIO_IOCTL_PIN_CONFIG_DIR_OUT | IFX_GPIO_IOCTL_PIN_CONFIG_ALTSEL0_SET | IFX_GPIO_IOCTL_PIN_CONFIG_ALTSEL1_CLEAR | IFX_GPIO_IOCTL_PIN_CONFIG_OD_SET},
	/* SPI_CLK used as I/O */
	{IFX_GPIO_MODULE_SSC, IFX_GPIO_PIN_ID(1, 2), IFX_GPIO_IOCTL_PIN_CONFIG_DIR_OUT | IFX_GPIO_IOCTL_PIN_CONFIG_ALTSEL0_SET | IFX_GPIO_IOCTL_PIN_CONFIG_ALTSEL1_CLEAR | IFX_GPIO_IOCTL_PIN_CONFIG_OD_SET},
	/*
	 *  COSIC DRIVER for DECT
	 */
	/* SPI_CS1 used as output */
	{IFX_GPIO_MODULE_DECT, IFX_GPIO_PIN_ID(0, 15), IFX_GPIO_IOCTL_PIN_CONFIG_DIR_OUT | IFX_GPIO_IOCTL_PIN_CONFIG_ALTSEL0_CLEAR | IFX_GPIO_IOCTL_PIN_CONFIG_ALTSEL1_CLEAR | IFX_GPIO_IOCTL_PIN_CONFIG_OD_SET},
	/* EXIN 5 used as input */
	{IFX_GPIO_MODULE_DECT, IFX_GPIO_PIN_ID(0, 9), IFX_GPIO_IOCTL_PIN_CONFIG_DIR_IN | IFX_GPIO_IOCTL_PIN_CONFIG_ALTSEL0_SET | IFX_GPIO_IOCTL_PIN_CONFIG_ALTSEL1_SET | IFX_GPIO_IOCTL_PIN_CONFIG_PUDEN_SET | IFX_GPIO_IOCTL_PIN_CONFIG_PUDSEL_SET},
	/* GPIO 3 used as input - the real o2hb2 Target Board's Button */
	//{IFX_GPIO_MODULE_PAGE, IFX_GPIO_PIN_ID(0, 3), IFX_GPIO_IOCTL_PIN_CONFIG_DIR_IN | IFX_GPIO_IOCTL_PIN_CONFIG_ALTSEL0_CLEAR | IFX_GPIO_IOCTL_PIN_CONFIG_ALTSEL1_CLEAR | IFX_GPIO_IOCTL_PIN_CONFIG_PUDSEL_SET | IFX_GPIO_IOCTL_PIN_CONFIG_PUDEN_SET},
	/* GPIO 19 used as output - DECT_RESET uses GPIO19 as DECT reset */
	{IFX_GPIO_MODULE_DECT, IFX_GPIO_PIN_ID(1, 3), IFX_GPIO_IOCTL_PIN_CONFIG_DIR_OUT | IFX_GPIO_IOCTL_PIN_CONFIG_ALTSEL0_CLEAR | IFX_GPIO_IOCTL_PIN_CONFIG_ALTSEL1_CLEAR | IFX_GPIO_IOCTL_PIN_CONFIG_OD_SET | IFX_GPIO_IOCTL_PIN_CONFIG_OUTPUT_SET},
	/* GPIO11 - DECT_LED */
	//{IFX_GPIO_MODULE_LED, IFX_GPIO_PIN_ID(0, 11), IFX_GPIO_IOCTL_PIN_CONFIG_DIR_OUT | IFX_GPIO_IOCTL_PIN_CONFIG_ALTSEL0_CLEAR | IFX_GPIO_IOCTL_PIN_CONFIG_ALTSEL1_CLEAR | IFX_GPIO_IOCTL_PIN_CONFIG_OD_SET},
	/*
	 *  SLIC_RESET uses GPIO25 for DECT
	 */
	{IFX_GPIO_MODULE_TAPI_VMMC, IFX_GPIO_PIN_ID(1, 9), IFX_GPIO_IOCTL_PIN_CONFIG_DIR_OUT | IFX_GPIO_IOCTL_PIN_CONFIG_ALTSEL0_CLEAR | IFX_GPIO_IOCTL_PIN_CONFIG_ALTSEL1_CLEAR | IFX_GPIO_IOCTL_PIN_CONFIG_OD_SET},

	
    {IFX_GPIO_PIN_AVAILABLE, 0, 0},
};
EXPORT_SYMBOL(g_board_gpio_pin_map);

struct ifx_ledc_config_param g_board_ledc_hw_config = {
#ifdef CONFIG_QUALCOMM_AP_PPA
#elif 1 /* Terry 20131219, TODO, for HB2 H/W */
    .operation_mask         = IFX_LEDC_CFG_OP_NUMBER_OF_LED,
    .number_of_enabled_led  = 8,
#else
    .operation_mask         = IFX_LEDC_CFG_OP_UPDATE_SOURCE | IFX_LEDC_CFG_OP_BLINK | IFX_LEDC_CFG_OP_UPDATE_CLOCK | IFX_LEDC_CFG_OP_STORE_MODE | IFX_LEDC_CFG_OP_SHIFT_CLOCK | IFX_LEDC_CFG_OP_DATA_OFFSET | IFX_LEDC_CFG_OP_NUMBER_OF_LED | IFX_LEDC_CFG_OP_DATA | IFX_LEDC_CFG_OP_MIPS0_ACCESS | IFX_LEDC_CFG_OP_DATA_CLOCK_EDGE,
    .source_mask            = 0x7ff,
    .source                 = 0x76c, //  LEDs controlled by EXT Src
    .blink_mask             = (1 << 24) - 1,
    .blink                  = 0,    //  disable blink for all LEDs
    .update_clock           = LED_CON1_UPDATE_SRC_FPI,
    .fpid                   = 3,
    .store_mode             = 0,    //  single store
    .fpis                   = 2,
    .data_offset            = 0,
    .number_of_enabled_led  = 24,
    .data_mask              = (1 << 24) - 1,
    .data                   = 0,
    .mips0_access_mask      = (1 << 24) - 1,
    .mips0_access           = (1 << 24) - 1,
    .f_data_clock_on_rising = 0,    //  falling edge
#endif
};
EXPORT_SYMBOL(g_board_ledc_hw_config);


struct ifx_led_device g_board_led_hw_config[] = {
/* Terry 20140801, since native USB driver used we need to disable code here */
#if 1
	    {
	        .name               = "USB_VBUS1",
	        .default_trigger    = "USB_VBUS1",
	        .phys_id            = 42,
	        .value_on           = 1,
	        .value_off          = 0,
	        .flags              = IFX_LED_DEVICE_FLAG_PHYS_GPIO
	    },
	    {
	        .name               = "USB_VBUS2",
	        .default_trigger    = "USB_VBUS2",
	        .phys_id            = 43,
	        .value_on           = 1,
	        .value_off          = 0,
	        .flags              = IFX_LED_DEVICE_FLAG_PHYS_GPIO
	    },
#else
#if defined(CONFIG_USB_HOST_IFX) || defined(CONFIG_USB_HOST_IFX_MODULE)
#ifdef CONFIG_AR10_FAMILY_BOARD_2
	#if   defined(IFX_LEDGPIO_USB_VBUS1)
	    {
	        .name               = "USB_VBUS1",
	        .default_trigger    = "USB_VBUS1",
	        .phys_id            = IFX_LEDGPIO_USB_VBUS1,
	        .value_on           = 1,
	        .value_off          = 0,
	        .flags              = IFX_LED_DEVICE_FLAG_PHYS_GPIO
	    },
	#endif

#else
	#if   defined(IFX_LEDLED_USB_VBUS1)
	    {
	        .name               = "USB_VBUS1",
	        .default_trigger    = "USB_VBUS1",
	        .phys_id            = IFX_LEDLED_USB_VBUS1,
	        .value_on           = 1,
	        .value_off          = 0,
	        .flags              = IFX_LED_DEVICE_FLAG_PHYS_LEDC,
	    },
	#endif

    {
        .name               = "usb0_led",
        .default_trigger    = NULL,
        .phys_id            = 26,
        .value_on           = 1,
        .value_off          = 0,
        .flags              = IFX_LED_DEVICE_FLAG_PHYS_GPIO,
    },
    {
        .name               = "usb1_led",
        .default_trigger    = NULL,
        .phys_id            = 27,
        .value_on           = 1,
        .value_off          = 0,
        .flags              = IFX_LED_DEVICE_FLAG_PHYS_GPIO,
    },
#endif
	#if   defined(IFX_LEDLED_USB_VBUS2)
	    {
	        .name               = "USB_VBUS2",
	        .default_trigger    = "USB_VBUS2",
	        .phys_id            = IFX_LEDLED_USB_VBUS2,
	        .value_on           = 1,
	        .value_off          = 0,
	        .flags              = IFX_LED_DEVICE_FLAG_PHYS_LEDC,
	    },
	#endif
#endif
#if   (defined(CONFIG_USB_HOST_IFX) || defined(CONFIG_USB_HOST_IFX_MODULE)) && defined(CONFIG_USB_HOST_IFX_LED)
	    {
	        .default_trigger    = IFX_LED_TRIGGER_USB_LINK,
	        .phys_id            = IFX_LEDGPIO_USB_LED,
	        .value_on           = 0,
	        .value_off          = 1,
	        .flags              = IFX_LED_DEVICE_FLAG_PHYS_GPIO,
	    },
#endif
#endif /* #if 0 */
/*
 * leds defined below will be controlled by arcadyan HAL.
 * HAL use "name" as index, name should be inside the range from "100" to "179".
 * */

/* Terry 20140129, for O2-HB2 GPIO */
#if 1
	{
		.name               = "104", //"power_led",
		.default_trigger    = "power_led",
		.phys_id            = 26,
		.value_on           = 1,
		.value_off          = 0,
		.flags              = IFX_LED_DEVICE_FLAG_PHYS_GPIO,
	},
	{
		.name               = "105", //"power_update",
		.default_trigger    = NULL,
		.phys_id            = 27,
		.value_on           = 1,
		.value_off          = 0,
		.flags              = IFX_LED_DEVICE_FLAG_PHYS_GPIO,
	},
	{
		.name               = "106", //"qtn_reset",
		.default_trigger    = NULL,
		.phys_id            = 14,	 /* Terry 20150317, need to hack for new/old boards, should change 58 -> 14 */
		.value_on           = 1,
		.value_off          = 0,
		.flags              = IFX_LED_DEVICE_FLAG_PHYS_GPIO,
	},
    {
        .name               = "107", //"phone_led",
        .default_trigger    = NULL,
        .phys_id            = 10,
        .value_on           = 1,
        .value_off          = 0,
        .flags              = IFX_LED_DEVICE_FLAG_PHYS_GPIO,
    },
    {
        .name               = "108", //"dect_led",
        .default_trigger    = NULL,
        .phys_id            = 3,	 /* Terry 20150317, need to hack for new/old boards, should change 11 -> 3 */
        .value_on           = 1,
        .value_off          = 0,
        .flags              = IFX_LED_DEVICE_FLAG_PHYS_GPIO,
    },
#else
    {
        .name               = "100", //"lan1_led",
        .default_trigger    = NULL,
        .phys_id            = 14,
        .value_on           = 1,
        .value_off          = 0,
        .flags              = IFX_LED_DEVICE_FLAG_PHYS_GPIO,
    },
    {
        .name               = "101", //"lan2_led",
        .default_trigger    = NULL,
        .phys_id            = 19,
        .value_on           = 1,
        .value_off          = 0,
        .flags              = IFX_LED_DEVICE_FLAG_PHYS_GPIO,
    },
    {
        .name               = "102", //"lan3_led",
        .default_trigger    = NULL,
        .phys_id            = 1,
        .value_on           = 1,
        .value_off          = 0,
        .flags              = IFX_LED_DEVICE_FLAG_PHYS_GPIO,
    },
    {
        .name               = "103", //"lan4_led",
        .default_trigger    = NULL,
        .phys_id            = 25,
        .value_on           = 1,
        .value_off          = 0,
        .flags              = IFX_LED_DEVICE_FLAG_PHYS_GPIO,
    },
    {
        .name               = "104", //"power_led",
        .default_trigger    = NULL,
        .phys_id            = 10,
        .value_on           = 1,
        .value_off          = 0,
        .flags              = IFX_LED_DEVICE_FLAG_PHYS_GPIO,
    },
    {
        .name               = "105", //"internet_led",
        .default_trigger    = NULL,
        .phys_id            = 11,
        .value_on           = 1,
        .value_off          = 0,
        .flags              = IFX_LED_DEVICE_FLAG_PHYS_GPIO,
    },
    {
        .name               = "106", //"wlan_led",
        .default_trigger    = NULL,
        .phys_id            = 18,
        .value_on           = 1,
        .value_off          = 0,
        .flags              = IFX_LED_DEVICE_FLAG_PHYS_GPIO,
    },
    {
        .name               = "107", //"phone_led",
        .default_trigger    = NULL,
        .phys_id            = 61,
        .value_on           = 1,
        .value_off          = 0,
        .flags              = IFX_LED_DEVICE_FLAG_PHYS_GPIO,
    },
#endif
/* Terry 20140129, for O2-HB2 LEDs */
#if 1
    {
		.name		    						= "109", //"info_unpin",
        .default_trigger    = NULL,
        .phys_id            = 7,
        .value_on           = 1,
        .value_off          = 0,
        .flags              = IFX_LED_DEVICE_FLAG_PHYS_LEDC,
    },
    {
		.name		    						= "110", //"info_pin",
        .default_trigger    = NULL,
        .phys_id            = 6,
        .value_on           = 1,
        .value_off          = 0,
        .flags              = IFX_LED_DEVICE_FLAG_PHYS_LEDC,
    },
    {
		.name		    						= "111", //"dsl_led",
        .default_trigger    = NULL,
        .phys_id            = 5,
        .value_on           = 1,
        .value_off          = 0,
        .flags              = IFX_LED_DEVICE_FLAG_PHYS_LEDC,
    },
    {
		.name		    						= "112", //"internet_unconnect",
        .default_trigger    = NULL,
        .phys_id            = 4,
        .value_on           = 1,
        .value_off          = 0,
        .flags              = IFX_LED_DEVICE_FLAG_PHYS_LEDC,
    },
    {
		.name		    						= "113", //"internet_dt",
        .default_trigger    = NULL,
        .phys_id            = 3,
        .value_on           = 1,
        .value_off          = 0,
        .flags              = IFX_LED_DEVICE_FLAG_PHYS_LEDC,
    },
    {
		.name		    						= "114", //"lan_led",
        .default_trigger    = NULL,
        .phys_id            = 2,
        .value_on           = 1,
        .value_off          = 0,
        .flags              = IFX_LED_DEVICE_FLAG_PHYS_LEDC,
    },
    {
		.name		    						= "115", //"wlan_led",
        .default_trigger    = NULL,
        .phys_id            = 1,
        .value_on           = 1,
        .value_off          = 0,
        .flags              = IFX_LED_DEVICE_FLAG_PHYS_LEDC,
    },
    {
		.name		    						= "116", //"vrx318_reset",
        .default_trigger    = NULL,
        .phys_id            = 0,
        .value_on           = 1,
        .value_off          = 0,
        .flags              = IFX_LED_DEVICE_FLAG_PHYS_LEDC,
    },
#else
    {
				.name		    				= "108", //"sim_led",
        .default_trigger    = NULL,
        .phys_id            = 16,
        .value_on           = 1,
        .value_off          = 0,
        .flags              = IFX_LED_DEVICE_FLAG_PHYS_LEDC,
    },
    {
				.name		    				= "109", //"utms",
        .default_trigger    = NULL,
        .phys_id            = 17,
        .value_on           = 1,
        .value_off          = 0,
        .flags              = IFX_LED_DEVICE_FLAG_PHYS_LEDC,
    },
    {
				.name		    				= "110", //"lte_led",
        .default_trigger    = NULL,
        .phys_id            = 18,
        .value_on           = 1,
        .value_off          = 0,
        .flags              = IFX_LED_DEVICE_FLAG_PHYS_LEDC,
    },
    {
				.name		    				= "111", //"lte_h1",
        .default_trigger    = NULL,
        .phys_id            = 19,
        .value_on           = 1,
        .value_off          = 0,
        .flags              = IFX_LED_DEVICE_FLAG_PHYS_LEDC,
    },
    {
				.name		    				= "112", //"lte_m2",
        .default_trigger    = NULL,
        .phys_id            = 20,
        .value_on           = 1,
        .value_off          = 0,
        .flags              = IFX_LED_DEVICE_FLAG_PHYS_LEDC,
    },
    {
				.name		    				= "113", //"lte_m1",
        .default_trigger    = NULL,
        .phys_id            = 21,
        .value_on           = 1,
        .value_off          = 0,
        .flags              = IFX_LED_DEVICE_FLAG_PHYS_LEDC,
    },
    {
				.name		    				= "114", //"lte_l2",
        .default_trigger    = NULL,
        .phys_id            = 22,
        .value_on           = 1,
        .value_off          = 0,
        .flags              = IFX_LED_DEVICE_FLAG_PHYS_LEDC,
    },
    {
				.name		    				= "115", //"lte_l1",
        .default_trigger    = NULL,
        .phys_id            = 23,
        .value_on           = 1,
        .value_off          = 0,
        .flags              = IFX_LED_DEVICE_FLAG_PHYS_LEDC,
    },
#endif
    {
        .flags              = IFX_LED_DEVICE_FLAG_INVALID,
    }
};
EXPORT_SYMBOL(g_board_led_hw_config);

#ifdef CONFIG_MTD_IFX_NOR

/* NOR flash partion table */
#if (CONFIG_MTD_IFX_NOR_FLASH_SIZE == 2)
#define IFX_MTD_NOR_PARTITION_SIZE    0x001B0000
const struct mtd_partition g_ifx_mtd_nor_partitions[] = {
    {
        .name       = "U-Boot",       /* U-Boot firmware */
        .offset     = 0x00000000,
        .size       = 0x00020000, //128K
/*      .mask_flags = MTD_WRITEABLE,  force read-only */
    },
    {
        .name       = "firmware", /* firmware */
        .offset     = 0x00020000,
        .size       = 0x00030000, //192K
/*      mask_flags  = MTD_WRITEABLE,    force read-only */
    },
    {
        .name       = "rootfs,kernel,Data,Environment",       /* default partition */
        .offset     = 0x00050000,
        .size       = IFX_MTD_NOR_PARTITION_SIZE,
/*      mask_flags  = MTD_WRITEABLE,   force read-only */
    },
};
#elif (CONFIG_MTD_IFX_NOR_FLASH_SIZE == 4)
#define IFX_MTD_NOR_PARTITION_SIZE    0x003A0000
const struct mtd_partition g_ifx_mtd_nor_partitions[] = {
    {
        .name       = "U-Boot",       /* U-Boot firmware */
        .offset     = 0x00000000,
        .size       = 0x00020000, //128K
/*      mask_flags  = MTD_WRITEABLE,    force read-only */
    },
    {
        .name       = "firmware", /* firmware */
        .offset     = 0x00020000,
        .size       = 0x00040000, //256K
/*      mask_flags  = MTD_WRITEABLE,    force read-only */
    },
    {
        .name       = "rootfs,kernel,Data,Environment",       /* default partition */
        .offset     = 0x00060000,
        .size       = IFX_MTD_NOR_PARTITION_SIZE,
/*      mask_flags  = MTD_WRITEABLE,    force read-only */
    },
};
#elif (CONFIG_MTD_IFX_NOR_FLASH_SIZE == 8)
#define IFX_MTD_NOR_PARTITION_SIZE    0x007A0000
const struct mtd_partition g_ifx_mtd_nor_partitions[] = {
    {
	     .name    = "U-Boot",
		 .offset  = 0,
		 .size    = 0x00080000,
    },
    {
          .name    = "Linux",
          .offset  = 0x00080000,
          .size    = 0x00200000,
    },
    {
          .name    = "Rootfs",
          .offset  = 0x00380000,
          .size    = 0x00200000,
    },

};
#elif (CONFIG_MTD_IFX_NOR_FLASH_SIZE == 16)
#define IFX_MTD_NOR_PARTITION_SIZE    0x00FA0000
const struct mtd_partition g_ifx_mtd_nor_partitions[] = {
    {
        .name       = "U-Boot",        /* U-Boot firmware */
        .offset     = 0x00000000,
        .size       = 0x00020000, //128K
/*      mask_flags  = MTD_WRITEABLE,    force read-only */
    },
    {
        .name       = "firmware", /* firmware */
        .offset     = 0x00020000,
        .size       = 0x00040000, //256K
/*      mask_flags  = MTD_WRITEABLE,    force read-only */
    },
    {
        .name       = "rootfs,kernel,Data,Environment",       /* default partition */
        .offset     = 0x00060000,
        .size       = IFX_MTD_NOR_PARTITION_SIZE,
/*      mask_flags  = MTD_WRITEABLE,    force read-only */
    },
};
#else
#error  "Configure IFX MTD NOR flash size first!!"
#endif
const int g_ifx_mtd_partion_num = ARRAY_SIZE(g_ifx_mtd_nor_partitions);
EXPORT_SYMBOL(g_ifx_mtd_partion_num);
EXPORT_SYMBOL(g_ifx_mtd_nor_partitions);
#endif /* CONFIG_IFX_MTD_NOR */

#if defined(CONFIG_MTD_IFX_NAND) && !defined(CONFIG_MTD_CMDLINE_PARTS)

const struct mtd_partition g_ifx_mtd_nand_partitions[] = {
#if (CONFIG_MTD_IFX_NAND_FLASH_SIZE == 4)
    {
	   .name    = "U-Boot",
	   .offset  = 0x00000000,
	   .size    = 0x00080000,
	},
	{
	   .name    = "kernel",
	   .offset  = 0x00080000,
	   .size    = 0x00100000,
    },
    {
       .name    = "rootfs",
       .offset  = 0x00180000,
       .size    = 0x00280000,
    },
#elif (CONFIG_MTD_IFX_NAND_FLASH_SIZE == 8)
    {
       .name    = "U-Boot",
	   .offset  = 0x00000000,
	   .size    = 0x00080000,
    },
    {
       .name    = "kernel",
       .offset  = 0x00080000,
       .size    = 0x00200000,
    },
    {
       .name    = "rootfs",
       .offset  = 0x00280000,
       .size    = 0x00580000,
    },
#endif
};
const int g_ifx_mtd_nand_partion_num = ARRAY_SIZE(g_ifx_mtd_nand_partitions);
EXPORT_SYMBOL(g_ifx_mtd_nand_partion_num);
EXPORT_SYMBOL(g_ifx_mtd_nand_partitions);
#endif /* CONFIG_MTD_IFX_NAND */

#if  defined(CONFIG_MTD_IFX_MLCNAND) && !defined(CONFIG_MTD_CMDLINE_PARTS)
const struct mtd_partition g_ifx_mtd_nand_partitions[] = {
#if ((CONFIG_MTD_IFX_MLCNAND_FLASH_SIZE == 4))
    {
           .name    = "U-Boot",
           .offset  = 0x00000000,
           .size    = 0x00080000,
        },
        {
           .name    = "kernel",
           .offset  = 0x00080000,
           .size    = 0x00100000,
    },
    {
       .name    = "rootfs",
       .offset  = 0x00180000,
       .size    = 0x00280000,
    },
#elif (CONFIG_MTD_IFX_MLCNAND_FLASH_SIZE == 8)
    {
       .name    = "U-Boot",
           .offset  = 0x00000000,
           .size    = 0x00080000,
    },
    {
       .name    = "kernel",
       .offset  = 0x00080000,
       .size    = 0x00200000,
    },
    {
       .name    = "rootfs",
       .offset  = 0x00280000,
       .size    = 0x00580000,
    },
#endif
};

const int g_ifx_mtd_nand_partion_num = ARRAY_SIZE(g_ifx_mtd_nand_partitions);
EXPORT_SYMBOL(g_ifx_mtd_nand_partion_num);
EXPORT_SYMBOL(g_ifx_mtd_nand_partitions);
#endif /* CONFIG_MTD_IFX_MLCNAND */

#if defined(CONFIG_IFX_SPI_FLASH) || defined (CONFIG_IFX_SPI_FLASH_MODULE) \
    || defined(CONFIG_IFX_USIF_SPI_FLASH) || defined (CONFIG_IFX_USIF_SPI_FLASH_MODULE)
/*
 * spi flash partition information
 * Here are partition information for all known series devices.
 * See include/linux/mtd/partitions.h for definition of the mtd_partition
 * structure.
 */
#define IFX_MTD_SPI_PARTITION_2MB_SIZE    0x001B0000
#define IFX_MTD_SPI_PARTITION_4MB_SIZE    0x003A0000
#define IFX_MTD_SPI_PARTITION_8MB_SIZE    0x007A0000
#define IFX_MTD_SPI_PARTITION_16MB_SIZE   0x00FA0000

const struct mtd_partition g_ifx_mtd_spi_partitions[IFX_SPI_FLASH_MAX][IFX_MTD_SPI_PART_NB] = {
    {{0, 0, 0}},

/* 256K Byte */
    {{
        .name   =      "spi-boot",      /* U-Boot firmware */
        .offset =      0x00000000,
        .size   =      0x00040000,         /* 256 */
    /*  mask_flags:   MTD_WRITEABLE,    force read-only */
    }, {0}, {0},
    },

/* 512K Byte */
    {{0, 0, 0}},

/* 1M Byte */
    {{
        .name   =       "spi-boot",     /* U-Boot firmware */
        .offset =       0x00000000,
        .size   =       0x00010000,        /* 64K */
    /*  mask_flags:   MTD_WRITEABLE,    force read-only */
    },
    {
        .name   =       "spi-firmware", /* firmware */
        .offset =       0x00010000,
        .size   =       0x00030000,        /* 64K */
    /*  mask_flags:   MTD_WRITEABLE,    force read-only */
    },
    {
        .name   =       "spi-rootfs,kernel,Data,Environment",       /* default partition */
        .offset =       0x00030000,
        .size   =       0x000C0000,
    /*  mask_flags:   MTD_WRITEABLE,    force read-only */
    }},

/* 2M Byte */
    {{
        .name   =       "spi-boot",     /* U-Boot firmware */
        .offset =       0x00000000,
        .size   =       0x00020000,        /* 128K */
    /*  mask_flags:   MTD_WRITEABLE,    force read-only */
    },
    {
        .name   =       "spi-firmware", /* firmware */
        .offset =       0x00020000,
        .size   =       0x00030000,        /* 192K */
    /*  mask_flags:   MTD_WRITEABLE,    force read-only */
    },
    {
        .name   =       "spi-rootfs,kernel,Data,Environment",       /* default partition */
        .offset =       0x00050000,
        .size   =       IFX_MTD_SPI_PARTITION_2MB_SIZE,
    /*  mask_flags:   MTD_WRITEABLE,    force read-only */
    }},

/* 4M Byte */
    {{
        .name   =       "spi-boot",     /* U-Boot firmware */
        .offset =       0x00000000,
        .size   =       0x00020000,        /* 128K */
    /*  mask_flags:   MTD_WRITEABLE,    force read-only */
    },
    {
        .name   =       "spi-firmware", /* firmware */
        .offset =       0x00020000,
        .size   =       0x00040000,        /* 256K */
    /*  mask_flags:   MTD_WRITEABLE,    force read-only */
    },
    {
        .name   =       "spi-rootfs,kernel,Data,Environment",       /* default partition */
        .offset =       0x00060000,
        .size   =       IFX_MTD_SPI_PARTITION_4MB_SIZE,
    /*  mask_flags:   MTD_WRITEABLE,    force read-only */
    }},

/* 8M Byte */
    {{
        .name   =       "spi-boot",     /* U-Boot firmware */
        .offset =       0x00000000,
        .size   =       0x00020000,        /* 128K */
    /*  mask_flags:   MTD_WRITEABLE,    force read-only */
    },
    {
        .name   =      "spi-firmware",  /* firmware */
        .offset =      0x00020000,
        .size   =      0x00030000,         /* 192K */
    /*  mask_flags:   MTD_WRITEABLE,    force read-only */
    },
    {
        .name   =       "spi-rootfs,kernel,Data,Environment",       /* default partition */
        .offset =       0x00050000,
        .size   =       IFX_MTD_SPI_PARTITION_8MB_SIZE,
    /*  mask_flags:   MTD_WRITEABLE,    force read-only */
    }},

/* 16M Byte */
    {{
        .name   =       "spi-boot",     /* U-Boot firmware */
        .offset =       0x00000000,
        .size   =       0x00020000,        /* 128K */
    /*  mask_flags:   MTD_WRITEABLE,    force read-only */
    },
    {
        .name   =      "spi-firmware",  /* firmware */
        .offset =      0x00020000,
        .size   =      0x00030000,         /* 192K */
    /*  mask_flags:   MTD_WRITEABLE,    force read-only */
    },
    {
        .name   =       "spi-rootfs,kernel,Data,Environment",       /* default partition */
        .offset =       0x00050000,
        .size   =       IFX_MTD_SPI_PARTITION_16MB_SIZE,
    /*  mask_flags:   MTD_WRITEABLE,    force read-only */
    }},
/* 32M Byte */
    {{
        .name   =       "spi-boot",     /* U-Boot firmware */
        .offset =       0x00000000,
        .size   =       0x00020000,        /* 128K */
    /*  mask_flags:   MTD_WRITEABLE,    force read-only */
    },
    {
        .name   =      "spi-firmware",  /* firmware */
        .offset =      0x00020000,
        .size   =      0x00030000,         /* 192K */
    /*  mask_flags:   MTD_WRITEABLE,    force read-only */
    },
    {
        .name   =       "spi-rootfs,kernel,Data,Environment",       /* default partition */
        .offset =       0x00050000,
        .size   =       0x01000000,
    /*  mask_flags:   MTD_WRITEABLE,    force read-only */
    },
    {
        .name   =       "test",       /* default partition */
        .offset =       0x01050000,
        .size   =       0x00fb0000,
    /*  mask_flags:   MTD_WRITEABLE,    force read-only */
    }
    },
};
EXPORT_SYMBOL(g_ifx_mtd_spi_partitions);

#endif /* defined(CONFIG_IFX_SPI_FLASH) || defined (CONFIG_IFX_SPI_FLASH_MODULE) */

