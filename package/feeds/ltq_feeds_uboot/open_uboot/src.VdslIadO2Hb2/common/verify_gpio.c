
//#include <common.h>
//#include <command.h>

#include <config.h>
#include <common.h>
#include <command.h>
#include <asm/addrspace.h>
#include <asm/ar10.h>

#define AR10_LED                       0xBE100BB0
#define AR10_LED_CON0                  ((volatile unsigned int *)(AR10_LED + 0x0000))
#define AR10_LED_CON1                  ((volatile unsigned int *)(AR10_LED + 0x0004))
#define AR10_LED_CPU0                  ((volatile unsigned int *)(AR10_LED + 0x0008))
#define AR10_LED_CPU1                  ((volatile unsigned int *)(AR10_LED + 0x000C))
#define AR10_LED_AR                    ((volatile unsigned int *)(AR10_LED + 0x0010))

static int gpio_initialized = 0;
unsigned long long gpio_outmask = 0x200000000e0c4c7a;
unsigned long long gpio_inmask = 0x38200;

#if 1
static void gpio_init(void)
{
	 /* GPIO PINs Configuration */
	 #if 1
	 /*GPIO P0, GPIO0~GPIO15*/
	*(AR10_GPIO_P0_DIR) |= ((1<<0)|(1<<1)|(1<<10)|(1<<11));		/* Output */
	*(AR10_GPIO_P0_DIR) &= ~((1<<3)|(1<<14)); 					/* Input */
	*(AR10_GPIO_P0_ALTSEL0) &=~((1<<0)|(1<<1)|(1<<3)|(1<<10)|(1<<11)|(1<<14));
	*(AR10_GPIO_P0_ALTSEL1) &=~((1<<0)|(1<<1)|(1<<3)|(1<<10)|(1<<11)|(1<<14));
	
	*AR10_GPIO_P0_OD &= ~((1<<0)|(1<<1)|(1<<10)|(1<<11));
	*AR10_GPIO_P0_OUT |= ((1<<0)|(1<<1)|(1<<10)|(1<<11));
	//*AR10_GPIO_P0_OUT &= ~(1<<10);
	/*----------------------*/
	
	/*GPIO P1, GPIO16~GPIO31*/
	*(AR10_GPIO_P1_DIR) |= ((1<<10)|(1<<11));				/* Output */
	*(AR10_GPIO_P1_DIR) &= ~((1<<3)); 						/* Input */
	*(AR10_GPIO_P1_ALTSEL0) &=~((1<<3)|(1<<10)|(1<<11));
	*(AR10_GPIO_P1_ALTSEL1) &=~((1<<3)|(1<<10)|(1<<11));
	
	*AR10_GPIO_P1_OD &= ~((1<<10)|(1<<11));
	*AR10_GPIO_P1_OUT |= ((1<<11));
	*AR10_GPIO_P1_OUT &= ~(1<<10);
	/*----------------------*/
	
	/*GPIO P2, GPIO32~GPIO47*/
	*(AR10_GPIO_P2_DIR) |= ((1<<10)|(1<<11));				/* Output */
	// *(AR10_GPIO_P2_DIR) &= ~((1<<3)|(1<<14)); 			/* Input */
	*(AR10_GPIO_P2_ALTSEL0) &=~((1<<10)|(1<<11));
	*(AR10_GPIO_P2_ALTSEL1) &=~((1<<10)|(1<<11));
	
	*AR10_GPIO_P2_OD |= ((1<<10));							/* GPIO 42 is OD */
	*AR10_GPIO_P2_OD &= ~((1<<11));
	*AR10_GPIO_P2_OUT |= ((1<<10)|(1<<11));
	// *AR10_GPIO_P2_OUT &= ~(1<<10);
	/*----------------------*/
	
	/*GPIO P3, GPIO48~GPIO61*/
	*(AR10_GPIO_P3_DIR) |= ((1<<10));						/* Output */
	*(AR10_GPIO_P3_DIR) &= ~((1<<13)); 						/* Input */
	*(AR10_GPIO_P3_ALTSEL0) &=~((1<<10)|(1<<13));
	*(AR10_GPIO_P3_ALTSEL1) &=~((1<<10)|(1<<13));
	
	*AR10_GPIO_P3_OD &= ~((1<<10));
	*AR10_GPIO_P3_OUT |= ((1<<10));
	// *AR10_GPIO_P0_OUT &= ~(1<<10);
	/*----------------------*/
	
	 #else
	 /*p0.11 Internet LED*/
	 *(AR10_GPIO_P0_DIR) |= ((1<<1)|(1<<3)|(1<<10)|(1<<11)|(1<<14)); /* for ouput pin in port0 */
	 *(AR10_GPIO_P0_DIR) &= ~((1<<9)|(1<<15)); /* configure SIM dect & WPS button as input */
	 *(AR10_GPIO_P0_ALTSEL0) &=~((1<<1)|(1<<3)|(1<<9)|(1<<10)|(1<<11)|(1<<14)|(1<<15));
	 *(AR10_GPIO_P0_ALTSEL1) &=~((1<<1)|(1<<3)|(1<<9)|(1<<10)|(1<<11)|(1<<14)|(1<<15));
//	 *(AR10_GPIO_P0_ALTSEL0) |= (1<<14);
//	 *(AR10_GPIO_P0_ALTSEL1) |= (1<<14);
	 
	 *AR10_GPIO_P0_OD |= ((1<<1)|(1<<3)|(1<<9)|(1<<10)|(1<<11)|(1<<14)|(1<<15));
	 *AR10_GPIO_P0_OUT |= ((1<<1)|(1<<3)|(1<<10)|(1<<11)|(1<<14));
	 *AR10_GPIO_P0_OUT &= ~(1<<10);
	 #endif
	 
	 #if 1
	 // enable LED module in PMU
	 *AR10_PMU_PWDCR &= ~BSP_PMU_LEDC;
	 //while (--i && ((*IFX_PMU_SR) & (1 << 11)));
	
	 /* for LED Control function */
	 *(AR10_GPIO_P0_DIR) |= 0x70;
	 *(AR10_GPIO_P0_ALTSEL0) |= 0x70;
	 *(AR10_GPIO_P0_ALTSEL1) &= ~0x70;
	 
	 *AR10_GPIO_P0_OD |= 0x70;
	 *AR10_GPIO_P0_OUT |= 0x70;
	 
	// set LED related registers
	*AR10_LED_AR   = 0;
	//*AR10_LED_CPU0 = 0;
	/* Let VRX318 reset pin be low in initial stage */
	#if 1
	*AR10_LED_CPU0 = 0xfe;
	#else
	*AR10_LED_CPU0 = 0xff;
	#endif
	*AR10_LED_CPU1 = 0;
	*AR10_LED_CON1 = 0x81840001;	// FPID, 10Hz, LED 0~7
	*AR10_LED_CON0 = 0x84000000;	// falling edge & update to LED control module
	 #endif

#if 0
#if 1
#if 1
	 *(AR10_GPIO_P1_DIR) |= 0x0e0c;
	 *(AR10_GPIO_P1_DIR) &= ~0x3;
	 *(AR10_GPIO_P1_ALTSEL0) &=~0x0e0f;
	 *(AR10_GPIO_P1_ALTSEL1) &=~0x0e0f;
	 //*(AR10_GPIO_P1_ALTSEL0) |=~0x0008;
	 //*(AR10_GPIO_P1_ALTSEL1) |=~0x0008;
	 
	 *AR10_GPIO_P1_OD |= 0x0e0f;
	 *AR10_GPIO_P1_OUT |= 0x0e0c;

#else
	 *(AR10_GPIO_P1_DIR) |= 0x0204;
	 *(AR10_GPIO_P1_DIR) &= ~0x3;
	 *(AR10_GPIO_P1_ALTSEL0) &=~0x0207;
	 *(AR10_GPIO_P1_ALTSEL1) &=~0x0207;
	 //*(AR10_GPIO_P1_ALTSEL0) |=~0x0008;
	 //*(AR10_GPIO_P1_ALTSEL1) |=~0x0008;
	 
	 *AR10_GPIO_P1_OD |= 0x0207;
	 *AR10_GPIO_P1_OUT |= 0x0204;
#endif
#else
	 *(AR10_GPIO_P1_DIR) |= 0x0e0c;
	 *(AR10_GPIO_P1_DIR) &= ~0x3;
	 *(AR10_GPIO_P1_ALTSEL0) &=~0x0e0f;
	 *(AR10_GPIO_P1_ALTSEL1) &=~0x0e0f;
	 *(AR10_GPIO_P1_ALTSEL0) |=~0x0008;
	 *(AR10_GPIO_P1_ALTSEL1) |=~0x0008;
	 
	 *AR10_GPIO_P1_OD |= 0x0e0f;
	 *AR10_GPIO_P1_OUT |= 0x0e0c;
#endif
	 
	 *(AR10_GPIO_P3_DIR) |= 0x2000;
	 //*(AR10_GPIO_P3_DIR) &= ~0x2;
	 *(AR10_GPIO_P3_ALTSEL0) &=~0x2000;
	 *(AR10_GPIO_P3_ALTSEL1) &=~0x2000;
	 
	 *AR10_GPIO_P3_OD |= 0x2000;
	 *AR10_GPIO_P3_OUT |= 0x2000;
#endif 
}

#else
static void gpio_init(void)
{
	 /*p0.11 Internet LED*/
	 *(AR10_GPIO_P0_DIR) |= ((1<<1)|(1<<3)|(1<<10)|(1<<11)|(1<<14)); /* for ouput pin in port0 */
	 *(AR10_GPIO_P0_DIR) &= ~((1<<9)|(1<<15)); /* configure SIM dect & WPS button as input */
	 *(AR10_GPIO_P0_ALTSEL0) &=~((1<<1)|(1<<3)|(1<<9)|(1<<10)|(1<<11)|(1<<14)|(1<<15));
	 *(AR10_GPIO_P0_ALTSEL1) &=~((1<<1)|(1<<3)|(1<<9)|(1<<10)|(1<<11)|(1<<14)|(1<<15));
	 
	 *AR10_GPIO_P0_OD |= ((1<<1)|(1<<3)|(1<<9)|(1<<10)|(1<<11)|(1<<14)|(1<<15));
	 *AR10_GPIO_P0_OUT |= ((1<<1)|(1<<3)|(1<<10)|(1<<11)|(1<<14));
	 
	 #if 1
	 // enable LED module in PMU
	 *AR10_PMU_PWDCR &= ~BSP_PMU_LEDC;
	 //while (--i && ((*IFX_PMU_SR) & (1 << 11)));
	
	 /* for LED Control function */
	 *(AR10_GPIO_P0_DIR) |= 0x70;
	 *(AR10_GPIO_P0_ALTSEL0) |= 0x70;
	 *(AR10_GPIO_P0_ALTSEL1) &= ~0x70;
	 
	 *AR10_GPIO_P0_OD |= 0x70;
	 *AR10_GPIO_P0_OUT |= 0x70;
	 
	// set LED related registers
	*AR10_LED_AR   = 0;
	//*AR10_LED_CPU0 = 0;
	*AR10_LED_CPU0 = 0xff;
	*AR10_LED_CPU1 = 0;
	*AR10_LED_CON1 = 0x81840001;	// FPID, 10Hz, LED 0~7
	*AR10_LED_CON0 = 0x84000000;	// falling edge & update to LED control module
	 #endif
	 
	 
	 *(AR10_GPIO_P1_DIR) |= 0x0e0c;
	 *(AR10_GPIO_P1_DIR) &= ~0x3;
	 *(AR10_GPIO_P1_ALTSEL0) &=~0x0e0f;
	 *(AR10_GPIO_P1_ALTSEL1) &=~0x0e0f;
	 
	 *AR10_GPIO_P1_OD |= 0x0e0f;
	 *AR10_GPIO_P1_OUT |= 0x0e0c;
	 
	 
	 *(AR10_GPIO_P3_DIR) |= 0x2000;
	 //*(AR10_GPIO_P3_DIR) &= ~0x2;
	 *(AR10_GPIO_P3_ALTSEL0) &=~0x2000;
	 *(AR10_GPIO_P3_ALTSEL1) &=~0x2000;
	 
	 *AR10_GPIO_P3_OD |= 0x2000;
	 *AR10_GPIO_P3_OUT |= 0x2000;
	 
	 
}
#endif
//__attribute__((weak))
void arc_gpio_init(void)
{
	gpio_init();
}

unsigned long setExIO_data(unsigned long reg,unsigned long mask)
{
	unsigned long cpu0, con0;
	unsigned long eXIO_data;
	//int s;

	//s = mips_int_lock();
	//BLOCK_PREEMPTION;

	cpu0 = *AR10_LED_CPU0;
	con0 = *AR10_LED_CON0;

	eXIO_data = (cpu0 & (~mask))|(reg & mask);
	if(eXIO_data != cpu0) {
		//iprintf("eXIO_data = %08X\n", eXIO_data);
		*AR10_LED_CPU0 = eXIO_data;
		*AR10_LED_CON0 = con0 | 0x80000000;
	}

	//UNBLOCK_PREEMPTION;
	//mips_int_unlock(s);
	return eXIO_data;
}

int do_gpioReg ( cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	
	printf("AR10_GPIO_P0_OUT      %08x\n", *AR10_GPIO_P0_OUT);	  
	printf("AR10_GPIO_P0_IN       %08x\n", *AR10_GPIO_P0_IN);
	printf("AR10_GPIO_P0_DIR      %08x\n", *AR10_GPIO_P0_DIR);
	printf("AR10_GPIO_P0_ALTSEL0  %08x\n", *AR10_GPIO_P0_ALTSEL0);
	printf("AR10_GPIO_P0_ALTSEL1  %08x\n", *AR10_GPIO_P0_ALTSEL1);
	printf("AR10_GPIO_P0_OD       %08x\n", *AR10_GPIO_P0_OD);
	printf("AR10_GPIO_P0_STOFF    %08x\n", *AR10_GPIO_P0_STOFF);
	printf("AR10_GPIO_P0_PUDSEL   %08x\n", *AR10_GPIO_P0_PUDSEL);
	printf("AR10_GPIO_P0_PUDEN    %08x\n", *AR10_GPIO_P0_PUDEN);
	
	printf("AR10_GPIO_P1_OUT      %08x\n", *AR10_GPIO_P1_OUT);	  
	printf("AR10_GPIO_P1_IN       %08x\n", *AR10_GPIO_P1_IN);
	printf("AR10_GPIO_P1_DIR      %08x\n", *AR10_GPIO_P1_DIR);
	printf("AR10_GPIO_P1_ALTSEL0  %08x\n", *AR10_GPIO_P1_ALTSEL0);
	printf("AR10_GPIO_P1_ALTSEL1  %08x\n", *AR10_GPIO_P1_ALTSEL1);
	printf("AR10_GPIO_P1_OD       %08x\n", *AR10_GPIO_P1_OD);
	printf("AR10_GPIO_P1_STOFF    %08x\n", *AR10_GPIO_P1_STOFF);
	printf("AR10_GPIO_P1_PUDSEL   %08x\n", *AR10_GPIO_P1_PUDSEL);
	printf("AR10_GPIO_P1_PUDEN    %08x\n", *AR10_GPIO_P1_PUDEN);
	
	printf("AR10_GPIO_P2_OUT      %08x\n", *AR10_GPIO_P2_OUT);	  
	printf("AR10_GPIO_P2_IN       %08x\n", *AR10_GPIO_P2_IN);
	printf("AR10_GPIO_P2_DIR      %08x\n", *AR10_GPIO_P2_DIR);
	printf("AR10_GPIO_P2_ALTSEL0  %08x\n", *AR10_GPIO_P2_ALTSEL0);
	printf("AR10_GPIO_P2_ALTSEL1  %08x\n", *AR10_GPIO_P2_ALTSEL1);
	printf("AR10_GPIO_P2_OD       %08x\n", *AR10_GPIO_P2_OD);
	printf("AR10_GPIO_P2_STOFF    %08x\n", *AR10_GPIO_P2_STOFF);
	printf("AR10_GPIO_P2_PUDSEL   %08x\n", *AR10_GPIO_P2_PUDSEL);
	printf("AR10_GPIO_P2_PUDEN    %08x\n", *AR10_GPIO_P2_PUDEN);
	
	printf("AR10_GPIO_P3_OUT      %08x\n", *AR10_GPIO_P3_OUT);	  
	printf("AR10_GPIO_P3_IN       %08x\n", *AR10_GPIO_P3_IN);
	printf("AR10_GPIO_P3_DIR      %08x\n", *AR10_GPIO_P3_DIR);
	printf("AR10_GPIO_P3_ALTSEL0  %08x\n", *AR10_GPIO_P3_ALTSEL0);
	printf("AR10_GPIO_P3_ALTSEL1  %08x\n", *AR10_GPIO_P3_ALTSEL1);
	printf("AR10_GPIO_P3_OD       %08x\n", *AR10_GPIO_P3_OD);
	//printf("AR10_GPIO_P3_STOFF    %08x\n", *AR10_GPIO_P3_STOFF);
	printf("AR10_GPIO_P3_PUDSEL   %08x\n", *AR10_GPIO_P3_PUDSEL);
	printf("AR10_GPIO_P3_PUDEN    %08x\n", *AR10_GPIO_P3_PUDEN);
	
	printf("\n\nAR10_LED_AR           %08x\n", *AR10_LED_AR);
	printf("AR10_LED_CPU0         %08x\n", *AR10_LED_CPU0);
	printf("AR10_LED_CPU1         %08x\n", *AR10_LED_CPU1);
	printf("AR10_LED_CON1         %08x\n", *AR10_LED_CON1);	
	printf("AR10_LED_CON0         %08x\n", *AR10_LED_CON0);
	
	return 0;
}


int do_gpioRd ( cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	int pin;
	unsigned long addr;
	int off;
	
	if (argc != 2) {
		cmd_usage(cmdtp);
		return 1;
	}
	
	pin = simple_strtoul(argv[1], NULL, 16);
	
	if ( (pin > 64) || ( (gpio_inmask & (1 << pin) == 0) && (gpio_outmask & (1 << pin) == 0) ) ) {
		cmd_usage(cmdtp);
		return 1;
	}
	
	
#if 0
	switch (argc) {
	case 2:
  
		/* FALL TROUGH */
	case 1:			/* get status */
		//printf ("Data (writethrough) Cache is %s\n",
		//	dcache_status() ? "ON" : "OFF");
		return 0;
	default:
		cmd_usage(cmdtp);
		return 1;
	}
#endif
	
	if ( !gpio_initialized ) {
		gpio_init();
		gpio_initialized = 1;
	}
	
	addr = AR10_GPIO+0x0014+(pin >> 4)*0x30;
	off = pin & 0xf;
	//printf("pin %d addr %08x, val %08x : %d\n", pin, addr, *((unsigned long*)addr), (*((unsigned long*)addr)&(1<<off))>>off);
	printf("pin %x : %d\n", pin, (*((unsigned long*)addr)&(1<<off))>>off);
	
	return 0;

}

static int LED_cnt;
int do_gpioSet ( cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{

	int pin;
	unsigned long addr, val;
	int off;
	
	if (argc != 3) {
		cmd_usage(cmdtp);
		return 1;
	}
	
	pin = simple_strtoul(argv[1], NULL, 16);
	val = simple_strtoul(argv[2], NULL, 16);
	
	if ( (pin > 64) || (gpio_outmask & (1 << pin) == 0)  ) {
		cmd_usage(cmdtp);
		return 1;
	}
	
	if ( !gpio_initialized ) {
		gpio_init();
		gpio_initialized = 1;
	}
	
	addr = AR10_GPIO+0x0010+(pin >> 4)*0x30;
	off = pin & 0xf;
	if ( val ) *((unsigned long*)addr) |= 1<<off;
	else *((unsigned long*)addr) &= ~(1<<off);
	//printf("pin %d addr %08x, val %08x : %d\n", pin, addr, *((unsigned long*)addr), (*((unsigned long*)addr)&(1<<off))>>off);
	printf("pin %x : %d\n", pin, (*((unsigned long*)addr)&(1<<off))>>off);
	
	return 0;

}


int do_LEDtest ( cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	int i;
	int mask;

	if (argc != 2) {
		cmd_usage(cmdtp);
		return 1;
	}
	
	mask = simple_strtoul(argv[1], NULL, 16);
	mask &= 0xff;
	
	if ( !gpio_initialized ) {
		gpio_init();
		gpio_initialized = 1;
	}
	
	//*AR10_LED_CON0 = 0x84000000 | mask;
	*AR10_LED_CPU0 = mask;
	return 0;

}


U_BOOT_CMD(
	gpioReg,   1,   1,     do_gpioReg,
	"show GPIO registers",
	"\n"
	"    - show GPIO registers"
);

U_BOOT_CMD(
	gpioRd,   2,   1,     do_gpioRd,
	"Read GPIO",
	"GPIO_number\n"
	"    - Read GPIO"
);

U_BOOT_CMD(
	gpioSet,   3,   1,     do_gpioSet,
	"Set GPIO",
	"GPIO_number [1, 0]\n"
	"    - Set GPIO"
);

U_BOOT_CMD(
	LEDtest,   2,   1,     do_LEDtest,
	"test serial LED controller",
	"LED_bitmask\n"
	"    - test serial LED controller"
);
