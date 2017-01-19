#ifdef __KERNEL__
#include <linux/autoconf.h>
#include <linux/kernel.h>       //for printk()
#include <linux/vmalloc.h>
#include <asm/uaccess.h>
#include <linux/types.h>
#include <linux/proc_fs.h>
#include <linux/file.h>
#include <linux/slab.h>

/*
 *  Chip Specific Header Files
 */
#include <asm/ifx/ifx_types.h>
#include <asm/ifx/ifx_regs.h>
#include <asm/ifx/common_routines.h>
#include <asm/ifx/ifx_clk.h>



#else
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

typedef unsigned short  __u16;
typedef signed short    __s16;
typedef unsigned int    __u32;
typedef signed int      __s32;
#endif
/* 32-bit ELF base types. */
typedef __u32   Elf32_Addr;
typedef __u16   Elf32_Half;
typedef __u32   Elf32_Off;
typedef __s32   Elf32_Sword;
typedef __u32   Elf32_Word;

typedef struct {
  Elf32_Word    sh_name;        /* Section name, index in string tbl */
  Elf32_Word    sh_type;        /* Type of section */
  Elf32_Word    sh_flags;       /* Miscellaneous section attributes */
  Elf32_Addr    sh_addr;        /* Section virtual addr at execution */
  Elf32_Off     sh_offset;      /* Section file offset */
  Elf32_Word    sh_size;        /* Size of section in bytes */
  Elf32_Word    sh_link;        /* Index of another section */
  Elf32_Word    sh_info;        /* Additional section information */
  Elf32_Word    sh_addralign;   /* Section alignment */
  Elf32_Word    sh_entsize;     /* Entry size if section holds table */
} Elf32_Shdr;

typedef struct elf32_sym{
  Elf32_Word    st_name;        /* Symbol name, index in string tbl */
  Elf32_Addr    st_value;       /* Value of the symbol */
  Elf32_Word    st_size;        /* Associated symbol size */
  unsigned char st_info;        /* Type and binding attributes */
  unsigned char st_other;       /* No defined meaning, 0 */
  Elf32_Half    st_shndx;       /* Associated section index */
} Elf32_Sym;

typedef struct elf32_hdr{
  unsigned char e_ident[16];    /* ELF "magic number" */
  Elf32_Half    e_type;
  Elf32_Half    e_machine;
  Elf32_Word    e_version;
  Elf32_Addr    e_entry;        /* Entry point virtual address */
  Elf32_Off     e_phoff;        /* Program header table file offset */
  Elf32_Off     e_shoff;        /* Section header table file offset */
  Elf32_Word    e_flags;
  Elf32_Half    e_ehsize;
  Elf32_Half    e_phentsize;
  Elf32_Half    e_phnum;
  Elf32_Half    e_shentsize;
  Elf32_Half    e_shnum;
  Elf32_Half    e_shstrndx;
} Elf32_Ehdr;

#define Elf_Shdr    Elf32_Shdr
#define Elf_Sym     Elf32_Sym
#define Elf_Ehdr    Elf32_Ehdr
#define Elf_Addr    Elf32_Addr


#define ELFMAG      "\177ELF"

// sh_types
#define SHT_NULL    0
#define SHT_PROGBITS    1
#define SHT_SYMTAB  2
#define SHT_STRTAB  3
#define SHT_RELA    4
#define SHT_HASH    5
#define SHT_DYNAMIC 6
#define SHT_NOTE    7
#define SHT_NOBITS  8
#define SHT_REL     9
#define SHT_SHLIB   10
#define SHT_DYNSYM  11
#define SHT_NUM     12
#define SHT_LOPROC  0x70000000
#define SHT_HIPROC  0x7fffffff
#define SHT_LOUSER  0x80000000
#define SHT_HIUSER  0xffffffff

// sh_flags
#define SHF_WRITE   0x1
#define SHF_ALLOC   0x2
#define SHF_EXECINSTR   0x4
#define SHF_MASKPROC    0xf0000000

#define SHT_MIPS_REGINFO    0x70000006

// These constants define the different elf file types
#define ET_NONE   0
#define ET_REL    1
#define ET_EXEC   2
#define ET_DYN    3
#define ET_CORE   4
#define ET_LOPROC 0xff00
#define ET_HIPROC 0xffff

#define WRITE_PHYSICAL_MEM_32b(data,addr)       IFX_REG_W32((data), (volatile unsigned  int *)addr)
#define READ_PHYSICAL_MEM_32b(addr)             IFX_REG_R32((volatile unsigned  int *)addr)
#define WRITE_PHYSICAL_MEM_8b(data,addr)        IFX_REG_W8((data), (volatile unsigned  int *)addr)
#define READ_PHYSICAL_MEM_8b(addr)              IFX_REG_R8((volatile unsigned  int *)addr)

#define DANUBE_PPE_SB_RAM0_ADDR(x)              ((volatile unsigned int *)(0xBE200000 + (((x) + 0x8000) << 2)))
//#define BULK_MEM_BASE_ADDR                      (0xBF280000)
//#define BULK_MEM_BASE_ADDR                      (DANUBE_PPE_SB_RAM0_ADDR(0))
#define BULK_MEM_BASE_ADDR                      (0xA1F10000)    // last1MB of the 32MB of external DDR memory


unsigned long loadELF(char * fileName, unsigned long * _sp1, unsigned long * _gp1, unsigned long * _free_memory1)
{
    int i,j;
    Elf32_Ehdr *hdr;
    Elf32_Shdr *sechdrs;
    int fSize, size;
    __u32 * memPtr;
    char *secstrings, *strtab = NULL;
    unsigned int symindex = 0, strindex = 0;
    unsigned int load_addr = 0x00000000;
    char * sourcePtr;
    unsigned long __start = 0x00000000;
    *_sp1 = 0x00000000;
    *_gp1 = 0x00000000;
    *_free_memory1 = 0x00000000;

    #ifdef __KERNEL__
    struct file *filp;
    mm_segment_t    oldfs;
    unsigned char * destAddr;
    unsigned char value;

    printk("\n");

    // Write and read back from physical memory
//    for (i=0; i<20; i++)
//    {
//        WRITE_PHYSICAL_MEM_32b(i, BULK_MEM_BASE_ADDR+i);
//    }
//    for (i=0; i<20; i++)
//    {
//        printk("Address= 0x%8x value=0x%8x\n", BULK_MEM_BASE_ADDR+i, READ_PHYSICAL_MEM_32b(BULK_MEM_BASE_ADDR+i));
//    }
//    for (i=0; i<32; i++)
//    {
//        unsigned char * destAddr;
//        unsigned char value;
//
//        value    = (unsigned char)(i);
//        destAddr = (unsigned char *)(BULK_MEM_BASE_ADDR+i);
//        WRITE_PHYSICAL_MEM_8b(value, destAddr);
//    }
//    for (i=0; i<32; i++)
//    {
//        unsigned char * srcAddr;
//        unsigned char value;
//
//        srcAddr = (unsigned char *)(BULK_MEM_BASE_ADDR+i);
//        value    = (unsigned char)(READ_PHYSICAL_MEM_8b(srcAddr));
//        printk("Address= 0x%8x value=0x%x\n", (unsigned int)srcAddr, value);
//    }

    filp = filp_open(fileName,00,O_RDONLY);
    if (IS_ERR(filp)||(filp==NULL))
    {
        printk("%s: Cannot open %s.\n", __func__, fileName);
        return 0;
    }
    if (filp->f_op->read==NULL)
    {
        printk("%s: File(system) doesn't allow reads.\n", __func__);
        return 0;
    }

    oldfs = get_fs();
    set_fs(KERNEL_DS);

    // Get file size
    filp->f_pos = 0;
    fSize = filp->f_op->llseek(filp, 0, SEEK_END);
    printk("%s: File %s size=%d bytes\n", __func__, fileName, fSize);

    // Allocate memory and copy the whole file in the memory
    memPtr = vmalloc(fSize);

    if(memPtr == NULL)
    {
        printk("%s: No memory available?\n", __func__);
        fput(filp);
        set_fs(oldfs);
        return 0;
    }

    filp->f_pos = 0;
    size = filp->f_op->read(filp, (void *)memPtr, fSize, &filp->f_pos);

    if (size != fSize)
    {
        printk("%s: Read error\n", __func__);
        printk("%s: Read %d bytes\n", __func__, size);

        vfree(memPtr);
        return 0;
    }
    else
    {
        printk("%s: Read %d bytes\n", __func__, size);
    }

    set_fs(oldfs);
    // Close the file
    fput(filp);

    #else
    int fd;

    if ((fd = open(fileName, O_RDWR)) == -1)
    {
        printf("Cannot open %s.\n", fileName);
        return 0;
    }

    // Get file size
    lseek(fd, 0, SEEK_SET);
    fSize = lseek(fd, 0, SEEK_END);
    printf("%s: File %s size=%d bytes\n", __func__, fileName, fSize);

    // Allocate memory and copy the whole file in the memory
    memPtr = malloc(fSize);

    if(memPtr == NULL)
    {
        printf("%s: No memory available?\n", __func__);
        close(fd);
        return 0;
    }

    lseek(fd, 0, SEEK_SET);
    size = read(fd, (void *)memPtr, fSize);

    if (size != fSize)
    {
        printf("%s: Read error\n", __func__);
        printf("%s: Read %d bytes\n", __func__, size);

        free(memPtr);
        return 0;
    }
    else
    {
        printf("%s: Read %d bytes\n", __func__, size);
    }

    // Close the file
    close(fd);
    #endif

    hdr = (Elf_Ehdr *) memPtr;

    // Sanity check against non-elf binaries
    if (memcmp(hdr->e_ident, ELFMAG, 4) != 0)
    {
        #ifdef __KERNEL__
        printk("%s: Not ELF format?\n", __func__);
        vfree(memPtr);
        #else
        printf("%s: Not ELF format?\n", __func__);
        free(memPtr);
        #endif
        return 0;
    }

    if (hdr->e_type == ET_REL)
    {
        #ifdef __KERNEL__
        printk("%s: Sorry, don't know how to handle relocatable format\n", __func__);
        vfree(memPtr);
        #else
        printf("%s: Sorry, don't know how to handle relocatable format\n", __func__);
        free(memPtr);
        #endif
        return 0;
    }

    // Convenience variables
    sechdrs = (void *)hdr + hdr->e_shoff;
    secstrings = (void *)hdr + sechdrs[hdr->e_shstrndx].sh_offset;
    sechdrs[0].sh_addr = 0;

    for (i = 0; i < hdr->e_shnum; i++)
    {
        // Internal symbols and strings.
        if (sechdrs[i].sh_type == SHT_SYMTAB)
        {
            symindex = i;
            strindex = sechdrs[i].sh_link;
            strtab = (char *)hdr + sechdrs[strindex].sh_offset;

            // Mark the symtab's address for when we try to find the magic symbols
            sechdrs[i].sh_addr = (size_t) hdr + sechdrs[i].sh_offset;
        }

        // Filter sections we dont want in the final image
        if (!(sechdrs[i].sh_flags & SHF_ALLOC) || (sechdrs[i].sh_type == SHT_MIPS_REGINFO))
        {
//            printk("%s: ignore section, name %s type %x address 0x%x \n",
//                    __func__,
//                    secstrings + sechdrs[i].sh_name,
//                    sechdrs[i].sh_type,
//                    sechdrs[i].sh_addr);
            continue;
        }

        if (sechdrs[i].sh_addr < load_addr)
        {
            #ifdef __KERNEL__
            printk("%s: fully linked image has invalid section: \n\tname %s \n\ttype %x \n\taddress 0x%x, before load address of 0x%x\n",
                    __func__,
                    secstrings + sechdrs[i].sh_name,
                    sechdrs[i].sh_type,
                    sechdrs[i].sh_addr,
                    (unsigned int)load_addr);
            vfree(memPtr);
            #else
            printf("%s: fully linked image has invalid section: \n\tname %s \n\ttype %x \n\taddress 0x%x, before load address of 0x%x\n",
                    __func__,
                    secstrings + sechdrs[i].sh_name,
                    sechdrs[i].sh_type,
                    sechdrs[i].sh_addr,
                    (unsigned int)load_addr);
            free(memPtr);
            #endif
            return 0;
        }

        #ifdef __KERNEL__
        printk("%s: section: \n\tname %s, \n\taddr 0x%08x \n\tsize 0x%08x\n",// \n\tfrom location %p\n",
               __func__,
               secstrings + sechdrs[i].sh_name,
               sechdrs[i].sh_addr,
               sechdrs[i].sh_size);//,
               //hdr + sechdrs[i].sh_offset);

        sourcePtr = (char *)hdr + sechdrs[i].sh_offset;

        printk("\tcontent");
        //for (j=0; j<sechdrs[i].sh_size; j++)
        for (j=0; j<64; j++)
        {
            if ((j & 0x1F)==0)      printk("\n");
            printk("%2x ", (unsigned char)*(sourcePtr+j));
        }
        printk("......\n");

        if (sechdrs[i].sh_type != SHT_NOBITS)
        {
            printk("\tcopy section\n");
            //memcpy((void *)sechdrs[i].sh_addr, (char *)hdr + sechdrs[i].sh_offset, sechdrs[i].sh_size);

            sourcePtr = (char *)hdr + sechdrs[i].sh_offset;
            for (j=0; j<sechdrs[i].sh_size; j++)
            {
                value    = (unsigned char)*(sourcePtr+j);
                destAddr = (unsigned char *)(sechdrs[i].sh_addr + j);
                WRITE_PHYSICAL_MEM_8b(value, destAddr);
                //printk("\n destAddr=%8x value=%2x ", (unsigned int)destAddr, value);
            }
        }
        else
        {
            printk("\tinitialize section with 0x00\n");
            //memset((void *)sechdrs[i].sh_addr, 0, sechdrs[i].sh_size);
            for (j=0; j<sechdrs[i].sh_size; j++)
            {
                value    = 0x00;
                destAddr = (unsigned char *)(sechdrs[i].sh_addr + j);
                WRITE_PHYSICAL_MEM_8b(value, destAddr);
                //printk("\n destAddr=%8x value=%2x ", (unsigned int)destAddr, value);
            }
        }
        #else
        printf("%s: section: \n\tname %s, \n\taddr 0x%08x \n\tsize 0x%08x\n",// \n\tfrom location %p\n",
               __func__,
               secstrings + sechdrs[i].sh_name,
               sechdrs[i].sh_addr,
               sechdrs[i].sh_size);//,
               //hdr + sechdrs[i].sh_offset);

        sourcePtr = (char *)hdr + sechdrs[i].sh_offset;

        printf("\tcontent");
        //for (j=0; j<sechdrs[i].sh_size; j++)
        for (j=0; j<32; j++)
        {
            //if ((j & 0x1F)==0)      printf("\n");
            printf("%2x ", (unsigned char)*(sourcePtr+j));
        }
        printf("......\n");

        if (sechdrs[i].sh_type != SHT_NOBITS)
        {
            printf("\tcopy section\n");
            //memcpy((void *)sechdrs[i].sh_addr, (char *)hdr + sechdrs[i].sh_offset, sechdrs[i].sh_size);
        }
        else
        {
            printf("\tinitialize section with 0x00\n");
            //memset((void *)sechdrs[i].sh_addr, 0, sechdrs[i].sh_size);
        }
        #endif

    }

    // Be sure that the section copied are not left in cache
    // flush_icache_range((unsigned long)v->load_addr, (unsigned long)v->load_addr + v->len);

    // Find the location of the __start address, if any is available
    {
        Elf_Sym *sym = (void *)sechdrs[symindex].sh_addr;
        unsigned int i, n = sechdrs[symindex].sh_size / sizeof(Elf_Sym);

        for (i = 1; i < n; i++)
        {
            if (strcmp(strtab + sym[i].st_name, "__start") == 0)
            {
                __start = sym[i].st_value;
                #ifdef __KERNEL__
                printk("%s: __start addr=0x%08x\n", __func__, (unsigned int)__start);
                #else
                printf("%s: __start addr=0x%08x\n", __func__, (unsigned int)__start);
                #endif
            }
        }

        if (__start == 0)
        {
            #ifdef __KERNEL__
            printk("%s: Warning '__start' symbol is missing!", __func__);
            #else
            printf("%s: Warning '__start' symbol is missing!", __func__);
            #endif
        }
    }

    // Find the location of the _sp1 address, if any is available
    {
        Elf_Sym *sym = (void *)sechdrs[symindex].sh_addr;
        unsigned int i, n = sechdrs[symindex].sh_size / sizeof(Elf_Sym);

        for (i = 1; i < n; i++)
        {
            if (strcmp(strtab + sym[i].st_name, "_sp1") == 0)
            {
                *_sp1 = sym[i].st_value;
                #ifdef __KERNEL__
                printk("%s: _sp1 addr=0x%08x\n", __func__, (unsigned int)*_sp1);
                #else
                printf("%s: _sp1 addr=0x%08x\n", __func__, (unsigned int)*_sp1);
                #endif
            }
        }

        if (*_sp1 == 0)
        {
            #ifdef __KERNEL__
            printk("%s: Warning '_sp1' symbol is missing!", __func__);
            #else
            printf("%s: Warning '_sp1' symbol is missing!", __func__);
            #endif
        }
    }

    // Find the location of the _free_memory1 address, if any is available
    {
        Elf_Sym *sym = (void *)sechdrs[symindex].sh_addr;
        unsigned int i, n = sechdrs[symindex].sh_size / sizeof(Elf_Sym);

        for (i = 1; i < n; i++)
        {
            if (strcmp(strtab + sym[i].st_name, "_free_memory1") == 0)
            {
                *_free_memory1 = sym[i].st_value;
                #ifdef __KERNEL__
                printk("%s: _free_memory1 addr=0x%08x\n", __func__, (unsigned int)*_free_memory1);
                #else
                printf("%s: _free_memory1 addr=0x%08x\n", __func__, (unsigned int)*_free_memory1);
                #endif
            }
        }

        if (*_free_memory1 == 0)
        {
            #ifdef __KERNEL__
            printk("%s: Warning '_free_memory1' symbol is missing!", __func__);
            #else
            printf("%s: Warning '_free_memory1' symbol is missing!", __func__);
            #endif
        }
    }

    // Find the location of the _gp1 address, if any is available
    {
        Elf_Sym *sym = (void *)sechdrs[symindex].sh_addr;
        unsigned int i, n = sechdrs[symindex].sh_size / sizeof(Elf_Sym);

        for (i = 1; i < n; i++)
        {
            if (strcmp(strtab + sym[i].st_name, "_gp1") == 0)
            {
                *_gp1 = sym[i].st_value;
                #ifdef __KERNEL__
                printk("%s: _gp1 addr=0x%08x\n", __func__, (unsigned int)*_gp1);
                #else
                printf("%s: _gp1 addr=0x%08x\n", __func__, (unsigned int)*_gp1);
                #endif
            }
        }

        if (*_gp1 == 0)
        {
            #ifdef __KERNEL__
            printk("%s: Warning '_gp1' symbol is missing!", __func__);
            #else
            printf("%s: Warning '_gp1' symbol is missing!", __func__);
            #endif
        }
    }


    #ifdef __KERNEL__
    vfree(memPtr);
    #else
    free(memPtr);
    #endif

    return __start;
}
