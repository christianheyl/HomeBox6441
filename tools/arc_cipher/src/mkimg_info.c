#include "mkimg_head.h"

static table_entry_t uimage_arch[] = {
	{	IH_ARCH_INVALID,	NULL,		"Invalid ARCH",	},
	{	IH_ARCH_ALPHA,		"alpha",	"Alpha",	},
	{	IH_ARCH_ARM,		"arm",		"ARM",		},
	{	IH_ARCH_I386,		"x86",		"Intel x86",	},
	{	IH_ARCH_IA64,		"ia64",		"IA64",		},
	{	IH_ARCH_M68K,		"m68k",		"M68K",		},
	{	IH_ARCH_MICROBLAZE,	"microblaze",	"MicroBlaze",	},
	{	IH_ARCH_MIPS,		"mips",		"MIPS",		},
	{	IH_ARCH_MIPS64,		"mips64",	"MIPS 64 Bit",	},
	{	IH_ARCH_NIOS2,		"nios2",	"NIOS II",	},
	{	IH_ARCH_PPC,		"powerpc",	"PowerPC",	},
	{	IH_ARCH_PPC,		"ppc",		"PowerPC",	},
	{	IH_ARCH_S390,		"s390",		"IBM S390",	},
	{	IH_ARCH_SH,			"sh",		"SuperH",	},
	{	IH_ARCH_SPARC,		"sparc",	"SPARC",	},
	{	IH_ARCH_SPARC64,	"sparc64",	"SPARC 64 Bit",	},
	{	IH_ARCH_BLACKFIN,	"blackfin",	"Blackfin",	},
	{	IH_ARCH_AVR32,		"avr32",	"AVR32",	},
	{	-1,			"",		"",		},
};

static table_entry_t uimage_os[] = {
	{	IH_OS_INVALID,	NULL,		"Invalid OS",		},
	{	IH_OS_LINUX,	"linux",	"Linux",		},
#if defined(CONFIG_LYNXKDI) || defined(USE_HOSTCC)
	{	IH_OS_LYNXOS,	"lynxos",	"LynxOS",		},
#endif
	{	IH_OS_NETBSD,	"netbsd",	"NetBSD",		},
	{	IH_OS_RTEMS,	"rtems",	"RTEMS",		},
	{	IH_OS_U_BOOT,	"u-boot",	"U-Boot",		},
#if defined(CONFIG_CMD_ELF) || defined(USE_HOSTCC)
	{	IH_OS_QNX,	"qnx",		"QNX",			},
	{	IH_OS_VXWORKS,	"vxworks",	"VxWorks",		},
#endif
#if defined(CONFIG_INTEGRITY) || defined(USE_HOSTCC)
	{	IH_OS_INTEGRITY,"integrity",	"INTEGRITY",		},
#endif
#ifdef USE_HOSTCC
	{	IH_OS_4_4BSD,	"4_4bsd",	"4_4BSD",		},
	{	IH_OS_DELL,		"dell",		"Dell",			},
	{	IH_OS_ESIX,		"esix",		"Esix",			},
	{	IH_OS_FREEBSD,	"freebsd",	"FreeBSD",		},
	{	IH_OS_IRIX,		"irix",		"Irix",			},
	{	IH_OS_NCR,		"ncr",		"NCR",			},
	{	IH_OS_OPENBSD,	"openbsd",	"OpenBSD",		},
	{	IH_OS_PSOS,		"psos",		"pSOS",			},
	{	IH_OS_SCO,		"sco",		"SCO",			},
	{	IH_OS_SOLARIS,	"solaris",	"Solaris",		},
	{	IH_OS_SVR4,		"svr4",		"SVR4",			},
#endif
	{	-1,		"",		"",			},
};

static table_entry_t uimage_type[] = {
	{	IH_TYPE_INVALID,    NULL,	  "Invalid Image",	},
	{	IH_TYPE_FILESYSTEM, "filesystem", "Filesystem Image",	},
	{	IH_TYPE_FIRMWARE,   "firmware",	  "Firmware",		},
	{	IH_TYPE_KERNEL,	    "kernel",	  "Kernel Image",	},
	{	IH_TYPE_MULTI,	    "multi",	  "Multi-File Image",	},
	{	IH_TYPE_RAMDISK,    "ramdisk",	  "RAMDisk Image",	},
	{	IH_TYPE_SCRIPT,     "script",	  "Script",		},
	{	IH_TYPE_STANDALONE, "standalone", "Standalone Program", },
	{	IH_TYPE_FLATDT,     "flat_dt",    "Flat Device Tree",	},
	{	IH_TYPE_KWBIMAGE,   "kwbimage",   "Kirkwood Boot Image",},
	{	IH_TYPE_IMXIMAGE,   "imximage",   "Freescale i.MX Boot Image",},
	{	-1,		    "",		  "",			},
};

static table_entry_t uimage_comp[] = {
	{	IH_COMP_NONE,	"none",		"uncompressed",		},
	{	IH_COMP_BZIP2,	"bzip2",	"bzip2 compressed",	},
	{	IH_COMP_GZIP,	"gzip",		"gzip compressed",	},
	{	IH_COMP_LZMA,	"lzma",		"lzma compressed",	},
	{	IH_COMP_LZO,	"lzo",		"lzo compressed",	},
	{	-1,		"",		"",			},
};

char *get_table_entry_name (table_entry_t *table, char *msg, int id)
{
	for (; table->id >= 0; ++table) {
		if (table->id == id)
			return table->lname;
	}
	return (msg);
}

const char *genimg_get_os_name (uint8_t os)
{
	return (get_table_entry_name (uimage_os, "Unknown OS", os));
}

const char *genimg_get_arch_name (uint8_t arch)
{
	return (get_table_entry_name (uimage_arch, "Unknown Architecture", arch));
}

const char *genimg_get_type_name (uint8_t type)
{
	return (get_table_entry_name (uimage_type, "Unknown Image", type));
}

const char *genimg_get_comp_name (uint8_t comp)
{
	return (get_table_entry_name (uimage_comp, "Unknown Compression", comp));
}

static void image_print_type (const image_header_t *hdr)
{
	const char *os, *arch, *type, *comp;

	
	os = genimg_get_os_name (hdr->ih_os);
	arch = genimg_get_arch_name (hdr->ih_arch);
	type = genimg_get_type_name (hdr->ih_type);
	comp = genimg_get_comp_name (hdr->ih_comp);

	printf ("%s %s %s (%s)\n", arch, os, type, comp);
}

void genimg_print_size (uint32_t size)
{
	printf ("%d Bytes = %.2f kB = %.2f MB\n",
			size, (double)size / 1.024e3,
			(double)size / 1.048576e6);
}

int image_print_contents (const void *ptr)
{
	const image_header_t *hdr = (const image_header_t *)ptr;
	const char *p;
	p = "   ";
	unsigned int time = endian_swap(hdr->ih_time);

	printf ("%sImage Name:   %.*s\n", p, IH_NMLEN, image_get_name (hdr));
	printf ("%sCreated:      ", p);
	printf ("%s", ctime((const time_t *)&time));
	printf ("%sImage Type:   ", p);
	image_print_type (hdr);
	printf ("%sData Size:    ", p);
	genimg_print_size (image_get_data_size (hdr));
	printf ("%sLoad Address: %08x\n", p, hdr->ih_load);
	printf ("%sEntry Point:  %08x\n", p, hdr->ih_ep);

	if(	!(image_check_arch(hdr, IH_ARCH_MIPS)) || !(image_check_os(hdr, IH_OS_LINUX))	||
		!(image_check_type(hdr, IH_TYPE_KERNEL))||!(image_check_magic(hdr))
	)	return -1;
	else
		return 0;
}
