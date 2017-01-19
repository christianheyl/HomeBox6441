/*
 * Copyright (C) 1999, 2000, 2004, 2005  MIPS Technologies, Inc.
 *	All rights reserved.
 *	Authors: Carsten Langgaard <carstenl@mips.com>
 *		 Maciej W. Rozycki <macro@mips.com>
 *
 *  This program is free software; you can distribute it and/or modify it
 *  under the terms of the GNU General Public License (Version 2) as
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 *  for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  59 Temple Place - Suite 330, Boston MA 02111-1307, USA.
 *
 * PROM library initialisation code.
 */
#include <linux/init.h>
#include <linux/string.h>
#include <linux/kernel.h>

#include <asm/bootinfo.h>
#include <asm/io.h>
#include <asm/system.h>
#include <asm/cacheflush.h>
#include <asm/traps.h>

#include <asm/mips-boards/prom.h>
#include <asm/mips-boards/generic.h>
#include <asm/mips-boards/bonito64.h>
#include <asm/mips-boards/msc01_pci.h>

#include <linux/module.h>

#ifdef CONFIG_KGDB
extern int rs_kgdb_hook(int, int);
extern int rs_putDebugChar(char);
extern char rs_getDebugChar(void);
extern int saa9730_kgdb_hook(int);
extern int saa9730_putDebugChar(char);
extern char saa9730_getDebugChar(void);
extern int wave400_kgdb_hook(int speed);
extern int wave400_putDebugChar(char byte);
extern char wave400_getDebugChar(void);
int kgdb_flag = 0;
#endif

int prom_argc;
int *_prom_argv, *_prom_envp;

/*
 * YAMON (32-bit PROM) pass arguments and environment as 32-bit pointer.
 * This macro take care of sign extension, if running in 64-bit mode.
 */
#define prom_envp(index) ((char *)(long)_prom_envp[(index)])

int init_debug = 0;

int mips_revision_corid;
int mips_revision_sconid;

/* Bonito64 system controller register base. */
unsigned long _pcictrl_bonito;
unsigned long _pcictrl_bonito_pcicfg;

/* GT64120 system controller register base */
unsigned long _pcictrl_gt64120;

/* MIPS System controller register base */
unsigned long _pcictrl_msc;

char *prom_getenv(char *envname)
{
	/*
	 * Return a pointer to the given environment variable.
	 * In 64-bit mode: we're using 64-bit pointers, but all pointers
	 * in the PROM structures are only 32-bit, so we need some
	 * workarounds, if we are running in 64-bit mode.
	 */
	int len = 0, index=0;
	char *elem = NULL;
	
	len = strlen(envname);

	while (1) {
		if(!(elem = prom_envp(index)))
			break;
		if(!strncmp(envname, elem, len)) {
			elem = strstr(elem, "=");
			elem++;
			return elem;
		}
		index++;
	}
	return NULL;
}

static inline unsigned char str2hexnum(unsigned char c)
{
	if (c >= '0' && c <= '9')
		return c - '0';
	if (c >= 'a' && c <= 'f') 
		return c - 'a' + 10;
	else if(c >= 'A' && c <= 'F')
		return c - 'A' + 10;
	return 0; /* foo */
}

static inline void str2eaddr(unsigned char *ea, unsigned char *str)
{
	int i;

	for (i = 0; i < 6; i++) {
		unsigned char num;

		if((*str == '.') || (*str == ':'))
			str++;
		num = str2hexnum(*str++) << 4;
		num |= (str2hexnum(*str++));
		ea[i] = num;
	}
}

int get_ethernet_addr(char *ethernet_addr, int ifnum)
{
    char ethaddr_str[32];
	char *val = NULL;

	if(!ifnum)
		strcpy(ethaddr_str, "ethaddr");
	else
		sprintf(ethaddr_str,"eth%daddr", ifnum);

        val = prom_getenv(ethaddr_str);
	printk("MAC Addr:%s\n", val);
	if (!val) {
	        printk("ethaddr not set in boot prom\n");
		return -1;
	}
	str2eaddr(ethernet_addr, val);

	if (init_debug > 1) {
	        int i;
		printk("get_ethernet_addr: ");
	        for (i=0; i<5; i++)
		        printk("%02x:", (unsigned char)*(ethernet_addr+i));
		printk("%02x\n", *(ethernet_addr+i));
	}

	return 0;
}

#ifdef CONFIG_SERIAL_8250_CONSOLE
static void __init console_config(void)
{
	char console_string[40];
	int baud = 0;
	char parity = '\0', bits = '\0', flow = '\0';
	char *s;

	if ((strstr(prom_getcmdline(), "console=")) == NULL) {
		s = prom_getenv("modetty0");
		if (s) {
			while (*s >= '0' && *s <= '9')
				baud = baud*10 + *s++ - '0';
			if (*s == ',') s++;
			if (*s) parity = *s++;
			if (*s == ',') s++;
			if (*s) bits = *s++;
			if (*s == ',') s++;
			if (*s == 'h') flow = 'r';
		}
		if (baud == 0)
			baud = 38400;
		if (parity != 'n' && parity != 'o' && parity != 'e')
			parity = 'n';
		if (bits != '7' && bits != '8')
			bits = '8';
		if (flow == '\0')
			flow = 'r';
		sprintf(console_string, " console=ttyS0,%d%c%c%c", baud, parity, bits, flow);
		strcat(prom_getcmdline(), console_string);
		pr_info("Config serial console:%s\n", console_string);
	}
}
#endif

#ifdef CONFIG_KGDB

static int __init kgdb_parse_cmd(char *str)
{
	char *ptr = NULL;
	int line, speed;
	extern int (*generic_putDebugChar)(char);
	extern char (*generic_getDebugChar)(void);

	printk(KERN_INFO ": kgdb_parse_cmd()\n");
	if(!str)
		return 0;
	if ((ptr = strstr(str, "ttyS")) != NULL) {
		ptr += strlen("ttyS");
		if (*ptr != '0' && *ptr != '1')
			printk("KGDB: Unknown serial line /dev/ttyS%c, "
			       "falling back to /dev/ttyS1\n", *ptr);
		line = *ptr == '0' ? 0 : 1;
		printk("KGDB: Using serial line /dev/ttyS%d for session\n", line);

		speed = 0;
		if (*++ptr == ',')
		{
			int c;
			while ((c = *++ptr) && ('0' <= c && c <= '9'))
				speed = speed * 10 + c - '0';
		}
		printk(KERN_INFO ": kgdb_parse_cmd() a1\n");
#ifdef CONFIG_MIPS_ATLAS
		if (line == 1) {
			speed = saa9730_kgdb_hook(speed);
			generic_putDebugChar = saa9730_putDebugChar;
			generic_getDebugChar = saa9730_getDebugChar;
		}
		else
#endif

#ifdef CONFIG_LTQ_VBG400
			speed = wave400_kgdb_hook(speed);
			generic_putDebugChar = wave400_putDebugChar;
			generic_getDebugChar = wave400_getDebugChar;
#else
		{
			speed = rs_kgdb_hook(line, speed);
			generic_putDebugChar = rs_putDebugChar;
			generic_getDebugChar = rs_getDebugChar;
		}
#endif
		printk("KGDB: Using serial line /dev/ttyS%d at %d for "
		        "session, please connect your debugger\n",
		        line ? 1 : 0, speed);

		{
			char *s;
			for (s = "Please connect GDB to this port\r\n"; *s; )
				generic_putDebugChar(*s++);
		}

		/* Breakpoint is invoked after interrupts are initialised */
	}
	else {
		printk(KERN_INFO ": kgdb_config() kgdb=ttySx not support as parameter!!!\n");
		return 0;
	}
	kgdb_flag = 1;
	return 1;
}
__setup("kgdb", kgdb_parse_cmd);


void __init kgdb_config(void)
{
/*	extern int (*generic_putDebugChar)(char);
	extern char (*generic_getDebugChar)(void);
	char *argptr;
	int line, speed; */

	printk(KERN_INFO ": kgdb_config()\n");
/*	argptr = prom_getcmdline();
	if ((argptr = strstr(argptr, "kgdb=ttyS")) != NULL) {
		argptr += strlen("kgdb=ttyS");
		if (*argptr != '0' && *argptr != '1')
			printk("KGDB: Unknown serial line /dev/ttyS%c, "
			       "falling back to /dev/ttyS1\n", *argptr);
		line = *argptr == '0' ? 0 : 1;
		printk("KGDB: Using serial line /dev/ttyS%d for session\n", line);

		speed = 0;
		if (*++argptr == ',')
		{
			int c;
			while ((c = *++argptr) && ('0' <= c && c <= '9'))
				speed = speed * 10 + c - '0';
		}
#ifdef CONFIG_MIPS_ATLAS
		if (line == 1) {
			speed = saa9730_kgdb_hook(speed);
			generic_putDebugChar = saa9730_putDebugChar;
			generic_getDebugChar = saa9730_getDebugChar;
		}
		else
#endif
		{
			speed = rs_kgdb_hook(line, speed);
			generic_putDebugChar = rs_putDebugChar;
			generic_getDebugChar = rs_getDebugChar;
		}

		pr_info("KGDB: Using serial line /dev/ttyS%d at %d for "
		        "session, please connect your debugger\n",
		        line ? 1 : 0, speed);

		{
			char *s;
			for (s = "Please connect GDB to this port\r\n"; *s; )
				generic_putDebugChar(*s++);
		}

		// Breakpoint is invoked after interrupts are initialised 
	}
	else
		printk(KERN_INFO ": kgdb_config() kgdb=ttySx not support as parameter!!!\n");*/
}
#endif

void __init mips_nmi_setup(void)
{
	void *base;
	extern char except_vec_nmi;

	base = cpu_has_veic ?
		(void *)(CAC_BASE + 0xa80) :
		(void *)(CAC_BASE + 0x380);
	memcpy(base, &except_vec_nmi, 0x80);
	flush_icache_range((unsigned long)base, (unsigned long)base + 0x80);
}

void __init mips_ejtag_setup(void)
{
	void *base;
	extern char except_vec_ejtag_debug;

	base = cpu_has_veic ?
		(void *)(CAC_BASE + 0xa00) :
		(void *)(CAC_BASE + 0x300);
	memcpy(base, &except_vec_ejtag_debug, 0x80);
	flush_icache_range((unsigned long)base, (unsigned long)base + 0x80);
}

extern struct plat_smp_ops msmtc_smp_ops;

void __init prom_init(void)
{
	prom_argc = fw_arg0;
	_prom_argv = (int *) fw_arg1;
	_prom_envp = (int *) fw_arg2;

	mips_display_message("LINUX");


#ifdef CONFIG_MIPS_SEAD
	set_io_port_base(KSEG1);
#endif
	board_nmi_handler_setup = mips_nmi_setup;
	board_ejtag_handler_setup = mips_ejtag_setup;

	pr_info("\nLINUX started...\n");
	prom_init_cmdline();
#ifdef CONFIG_LTQ_VBG400
	// TBD WAVE400 init memory?
#else
	prom_meminit();
#endif
#ifdef CONFIG_SERIAL_8250_CONSOLE
	console_config();
#endif
#ifdef CONFIG_MIPS_MT_SMP
	register_smp_ops(&vsmp_smp_ops);
#endif
#ifdef CONFIG_MIPS_MT_SMTC
	register_smp_ops(&msmtc_smp_ops);
#endif
}
