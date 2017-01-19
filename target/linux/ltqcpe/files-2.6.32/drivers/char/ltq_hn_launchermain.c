//#include <linux/autoconf.h>

#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/types.h>
#include <linux/proc_fs.h>
#include <asm/uaccess.h>
#include <linux/cdev.h>

#ifndef MB_SIMULATION
#include "startMT.h"
#endif
#ifndef PARSE_ELF_AND_LOAD
static unsigned long * spPtr;
static unsigned long * freeMemoryPtr;
#endif

extern unsigned long loadELF(char * fileName, unsigned long * _sp1, unsigned long * _gp1, unsigned long * _free_memory1);
/************************************************************
 * proc files 
 ************************************************************/
static int RegisterProcfs(void);
static void DeRegisterProcfs(void);

/************************************************************
 * device operations
 ************************************************************/
static int RegisterDevice(void);
static int LAUNCHER_Init(void);

static int launcher_major = 0;        /* Major Device Number */
static int launcher_minor = 0;

typedef struct 
{
  struct cdev cdev;
  struct semaphore procSem;
}launcher_t;

static launcher_t* launcher = NULL;
static struct proc_dir_entry *LauncherDbgMemRead = NULL;
static struct proc_dir_entry *LauncherDbgMemWrite = NULL;

#define VPE1_BIN_NAME_LEN 128
#define VPE1_BIN_DEFAULT "hn_vpe1.bin"
static char *vpe1_bin = VPE1_BIN_DEFAULT;

static char fname[VPE1_BIN_NAME_LEN];
module_param(vpe1_bin, charp, 0000);
MODULE_PARM_DESC(vpe1_bin, "VPE1 binary file name");

#define VPE1_IMAGE_IN_ELF_FORMAT_DEFAULT "N"
static char *elf = VPE1_IMAGE_IN_ELF_FORMAT_DEFAULT;
module_param(elf, charp, 0000);
MODULE_PARM_DESC(elf, "VPE1 image in ELF format [Y/N]");

#define VPE1_START_DEFAULT "Y"
static char *start_fw = VPE1_START_DEFAULT;
module_param(start_fw, charp, 0000);
MODULE_PARM_DESC(start_fw, "start VPE1 after loading");

#define LAUNCHER_DBG_MEM_READ "vpe1_dbg_MRD"
#define LAUNCHER_DBG_MEM_WRITE "vpe1_dbg_MWD"
#define LAUNCHER_MOD_NAME  "vpe_launcher"

static struct file_operations launcher_fops = {
  .owner = THIS_MODULE,
};

unsigned long _start1;
unsigned long _sp1;
unsigned long _gp1;
unsigned long _free_memory1;

unsigned long loadLtqBin(char *ltq_bin_file);

#ifdef __KERNEL__
#ifndef DEBUG_VPE_LAUNCHER

#define printf(fmt, ...) \
	({ if (0) printk(KERN_DEBUG fmt, ##__VA_ARGS__) ; 0; })   

#define dprintk(fmt, ...) do {} while(0) 
#else
#define printf(fmt, ...) printk(KERN_DEBUG fmt, ##__VA_ARGS__)    
#define dprintk printk
#endif

#define WRITE_PHYSICAL_MEM_8b(data,addr)        IFX_REG_W8((data), (volatile unsigned  int *)addr)
#define READ_PHYSICAL_MEM_8b(addr)             IFX_REG_R8((volatile unsigned  int *)addr)
#endif

/************************************************************
 * Function defines 
 ************************************************************/
int ProcRead(char *buffer, 
	     char **buffer_location,
	     off_t offset, int buffer_length, int *eof, 
	     void *data)
{
  int len = 0;
  
  if(launcher)
  {
      len += sprintf(buffer+len, "vpe launcher module\n");
      len += sprintf(buffer+len, "VPE binary: %s\n", vpe1_bin);
  }

  return len;
}

int ProcWrite(struct file* file, 
	      const char *buffer,
	      unsigned long cnt,
	      void *data)
{
  if((NULL != buffer) && (0 != cnt) && (cnt < VPE1_BIN_NAME_LEN))
  {
    if (0 != copy_from_user(fname, buffer, cnt))
      return -EFAULT;
  }

  fname[cnt-1] = '\0';   //take away new line from the input 
  vpe1_bin = fname;
  
  return cnt;
}

struct {
    unsigned int vpe1_addr;
    unsigned int data;
} gt_Vpe1MemWriteRequest;

struct {
    unsigned int vpe1_addr;
    unsigned int rd_len;
} gt_Vpe1MemDumpRequest;

#define KSEG1_START     ((unsigned int)0xA0000000)
#define KSEG1_END       ((unsigned int)0xBFFFFFFF)

unsigned int READ_PHYSICAL_MEM_32b(unsigned int addr)
{
#if 0
    return( 
           ((0xFF & READ_PHYSICAL_MEM_8b(addr)) << 24) |
           ((0xFF & READ_PHYSICAL_MEM_8b(addr + 1)) << 16) |
           ((0xFF & READ_PHYSICAL_MEM_8b(addr + 2)) << 8) |
            (0xFF & READ_PHYSICAL_MEM_8b(addr + 3)));
#else
        return(*(unsigned int *)addr);
#endif

}


static int ShowMemDumpRequest(char *buffer, 
		    char **buffer_location,
		    off_t offset, int buffer_length, int *eof, 
		    void *data)
{
    int len = 0;
    unsigned int start, end;
    unsigned int temp;

    unsigned int line_len = strlen("0x12345678: 0xabcd1234\n");

    down(&(launcher->procSem));

    len += sprintf(buffer+len, "0x%08x %d\n",
        gt_Vpe1MemDumpRequest.vpe1_addr,
        gt_Vpe1MemDumpRequest.rd_len);


    start = gt_Vpe1MemDumpRequest.vpe1_addr;
    end = gt_Vpe1MemDumpRequest.vpe1_addr + gt_Vpe1MemDumpRequest.rd_len*sizeof(int);

    if ((start < KSEG1_START) || (start > KSEG1_END)) 
    {
        len += sprintf(buffer+len, "bad start addr 0x%08x\n", start);
        return len;
    }

    if (end > KSEG1_END + 1)
    {
        end = KSEG1_END + 1;
    }

    for (; (start < end) && ((len + line_len) < buffer_length); start += 4)
    {
        temp = READ_PHYSICAL_MEM_32b(start);
        len += sprintf(buffer+len, "0x%08x: 0x%08x\n", start, temp);
    }

    up(&(launcher->procSem));

    return len;
}

static int SetVpe1MemDump(struct file* file, 
	      const char *buffer,
	      unsigned int cnt,
	      void *data)
{
    int i, addr, len;

    dprintk(KERN_INFO "buffer_length %d\n", cnt);

    i = sscanf(buffer, "%x %d", &addr, &len);
    if (i == 2)
    {
        dprintk(KERN_INFO "addr = 0x%08x, len = 0x%08x\n", addr, len);
        gt_Vpe1MemDumpRequest.vpe1_addr = addr;
        gt_Vpe1MemDumpRequest.rd_len = len;
    }

    return cnt;

}


static int ShowMemWriteRequest(char *buffer, 
		    char **buffer_location,
		    off_t offset, int buffer_length, int *eof, 
		    void *data)
{
    int len = 0;


    len += sprintf(buffer+len, "0x%08x %d\n",
        gt_Vpe1MemWriteRequest.vpe1_addr,
        gt_Vpe1MemWriteRequest.data);

    return len;
}

static int Vpe1MemWrite(struct file* file, 
	      const char *buffer,
	      unsigned int cnt,
	      void *data)
{
    int i, addr, wr_data;


    i = sscanf(buffer, "%x %x", &addr, &wr_data);
    if (i == 2)
    {
        dprintk(KERN_INFO "addr = 0x%08x, data = 0x%08x\n", addr, wr_data);
        gt_Vpe1MemWriteRequest.vpe1_addr = addr;
        gt_Vpe1MemWriteRequest.data = wr_data;
    }


    if ((addr < KSEG1_START) || (addr + 4 > KSEG1_END)) 
    {
        dprintk(KERN_INFO "bad addr 0x%08x\n", addr);
        return cnt;
    }

    *((int *)addr) = wr_data;

    return cnt;

}

int RegisterProcfs()
{
  int status = 0;

  /* create a proc_fs for the drive*/
  LauncherDbgMemRead = create_proc_entry(LAUNCHER_DBG_MEM_READ, 
					0666, 0);

  if(NULL == LauncherDbgMemRead)
  {
      dprintk(KERN_INFO "Failed to create Launcher proc write file(%s)\n", LAUNCHER_DBG_MEM_READ);
      status = -EFAULT;
      return status;
  }
  else
  {

      LauncherDbgMemRead->read_proc = ShowMemDumpRequest;
      LauncherDbgMemRead->write_proc = SetVpe1MemDump;
  }


  /* create a proc_fs for the drive*/
  LauncherDbgMemWrite = create_proc_entry(LAUNCHER_DBG_MEM_WRITE,
					0666, 0);

  if(NULL == LauncherDbgMemWrite)
  {
      dprintk(KERN_INFO "Failed to create Launcher proc write file(%s)\n", LAUNCHER_DBG_MEM_WRITE);
      status = -EFAULT;
      return status;
  }
  else
  {

      LauncherDbgMemWrite->read_proc = ShowMemWriteRequest;
      LauncherDbgMemWrite->write_proc = Vpe1MemWrite;
  }

  dprintk(KERN_INFO "******************* proc entry created **************\n");
  return status;
}


void DeRegisterProcfs(void)
{

  if(NULL != LauncherDbgMemRead)
  {
      remove_proc_entry(LAUNCHER_DBG_MEM_READ, 0);  
  }

  if(NULL != LauncherDbgMemWrite)
  {
      remove_proc_entry(LAUNCHER_DBG_MEM_WRITE, 0);  
  }
}


int RegisterDevice(void)
{
  int status = 0;
  dev_t devNo;

  status = alloc_chrdev_region(&devNo, launcher_minor, 1, LAUNCHER_MOD_NAME);

  if(0 != status)
  {
    dprintk(KERN_INFO "%s Failed to register device.\n", 
	   __func__);  
  }
  else
  {
      launcher_major = MAJOR(devNo);
      
      /* connect the file operations with the cdev*/
      cdev_init(&(launcher->cdev), &launcher_fops);
      launcher->cdev.owner = THIS_MODULE;
      
      status = cdev_add(&(launcher->cdev), devNo, 1); 


      init_MUTEX(&(launcher->procSem));

      if (0 == status)
      {
	    dprintk(KERN_INFO "%s: Added device /dev/%s as %u:%u\n",
		 __func__, LAUNCHER_MOD_NAME, launcher_major, 0);
      }
      else
      {
	     dprintk(KERN_INFO "%s: Failed add device /dev/%s \n",
		 __func__, LAUNCHER_MOD_NAME);
      }
  }

  return status;
}


/**
 *  Initialize module
 **/
int __init ModuleInit (void)
{
  int retVal;
  
  if(NULL != launcher)
  {
      dprintk("freeing launcher object...\n");
      kfree(launcher);
  }
  else
  {
      dprintk("kmallocating launcher object...\n");
      launcher = kmalloc(sizeof(launcher_t), GFP_KERNEL); 
  }
  
  if(NULL == launcher)
  {
      dprintk(KERN_INFO "%s: NO memory\n", __func__);
      return -ENOMEM;
  }
  
  dprintk(KERN_INFO "VPE1 binary: %s\n", vpe1_bin);

  /* Register procfs */
  retVal = RegisterProcfs();
  if(0 == retVal)
  {
      /* Register device */
      retVal = RegisterDevice();
      if(0 == retVal)
	  {
	      /* invoke private init function */
	      retVal = LAUNCHER_Init();
	  }
  }

  if(0 == retVal)
  {
      dprintk(KERN_INFO "%s: module initialized\n", __func__);
  }
  else
  {
      dprintk(KERN_INFO "%s: module initialization failed\n", __func__);
  }

  return retVal;
}

/**
 *  exit module
 **/
void __exit ModuleExit (void)
{
  dprintk(KERN_INFO "%s \n", __func__);
#ifndef MB_SIMULATION
  dprintk("Stop multi VPE configuration!\n");
  stopMT();
#endif

#ifndef PARSE_ELF_AND_LOAD
  kfree(spPtr);
  kfree(freeMemoryPtr);
#endif

  /* Remove the cdev */
  cdev_del(&launcher->cdev);
  
  /* Release the major number */
  unregister_chrdev_region(MKDEV(launcher_major, launcher_minor), 1);
  
  /* Remove proc entry */
  DeRegisterProcfs();
  
  kfree(launcher);
  launcher = NULL;
  dprintk(KERN_INFO "%s: module is removed.\n", __func__);
}


/************************************************************
 * Static functions 
 ************************************************************/
int LAUNCHER_Init()
{
  dprintk(KERN_INFO "%s: Here we go ... \n", __func__);

#ifndef MB_SIMULATION
  // Set TCs in disabled state
  initMT();

#ifdef PARSE_ELF_AND_LOAD
  dprintk("\t*************************************\n");
  dprintk("\tvpe1 image file name: %s\n", vpe1_bin);
  dprintk("\tvpe1 image in ELF format   : %s\n", elf);
  dprintk("\t*************************************\n");
  
  if ((strcmp(elf, "Y") == 0) ||(strcmp(elf, "y") == 0))
  {
      dprintk("Loading ELF imgage file...\n");

      // Load the binary image that will be started on VPE1
      _start1 = loadELF(vpe1_bin, &_sp1, &_gp1, &_free_memory1);
  }
  else // must be BIN format
  {
      dprintk("Loading LTQ-BIN imgage file...\n");

      _start1 = loadLtqBin(vpe1_bin);

      _free_memory1 = (int)0xa1fa4000;
      _sp1 = (int)0xa1fa4000;
      _gp1 = (int)0xa1f64c80;
      dprintk("Detected 0x%08x as HN FW start address\n", (int)_start1);

  }

  if ((strcmp(start_fw, "y") != 0) && (strcmp(start_fw, "Y") != 0)) 
  {
      return(0);
  }

  if (_start1 != 0)
  {
      // Show the current state
      showMT();

      // The physical memory was initialized by the loadELF() function. The same function initialized
      //the _sp1, _gp1, _free_memory1 and _start1 variables
      dprintk("\n Start multi VPE configuration!\n");
      runMT(_sp1, _free_memory1, _gp1, _start1);
  }
#else
  spPtr = kmalloc(4096, GFP_KERNEL);
  if(NULL == spPtr)
  {
      dprintk(KERN_INFO "%s: No 4KB memory available for stack?\n", __func__);
      return -ENOMEM;
  }
  else
  {
      freeMemoryPtr = kmalloc(1024*100, GFP_KERNEL);
      if(NULL == freeMemoryPtr)
      {
          dprintk(KERN_INFO "%s: NO 100KB free memory available?\n", __func__);
          kfree(spPtr);
          return -ENOMEM;
      }
      else
      {
          memset(spPtr, 0, 4096);
          memset(freeMemoryPtr, 0, 1024*100);
          dprintk("\n spPtr=0x%8lx, freeMemoryPtr=0x%8lx", spPtr, freeMemoryPtr);

          dprintk("\n Start multi VPE configuration!\n");
          runMT(spPtr, freeMemoryPtr, spPtr, 0);
      }
  }
#endif

#endif  //MB_SIMULATION

  return 0;
}

#ifndef MB_SIMULATION
//
// start of loadLtqBin.c
//

#ifdef __KERNEL__
	#include <linux/autoconf.h>
	#include <linux/kernel.h>       //for dprintk()
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
    #include <stdlib.h>
    #include <stdio.h>
    #ifndef WIN32
    #include <netinet/in.h>
    #endif
#endif

#define BOOT_PAGE_MASK      0x80000000
#define START_PAGE_MASK     0x40000000
#define BUFFER_32KB         (32*1024)

#ifndef __KERNEL__
static unsigned int buffer[BUFFER_32KB/4];
static unsigned int tempbuf[BUFFER_32KB/4];
#else
static unsigned int *buffer;
static unsigned int *tempbuf;
#endif

typedef struct {
    unsigned int __gul_BinFileSizeInLongs;
    unsigned int __gul_CheckSum;
    unsigned int __gul_NumOfPages;
} BinFileHeader_t;

typedef struct {
    unsigned int __gul_FileOffsetOf_Page_n;
    unsigned int __gul_InternalAddr_Page_n;
    unsigned int __gul_SizeOf_Page_n;
} PageHeader_t;


#ifdef __KERNEL__
static int Fread(void* dest_buf, size_t obj_size, size_t nobj, struct file *stream)
#else
static int Fread(void* dest_buf, size_t obj_size, size_t nobj, FILE *stream)
#endif
{
    int status;

    status = 0;
    if (obj_size*nobj > 0)
    {
#ifdef __KERNEL__
        status = stream->f_op->read(stream, (void *)dest_buf, obj_size*nobj, &stream->f_pos);
        status = status/obj_size;

#else
        int i, *ptr = (int*)dest_buf;

        if ((status = fread((void*)tempbuf, obj_size, 1, stream)) == nobj)
        {
            for (i = 0; i < (obj_size*nobj)/4; i++)
            {
                ptr[i] = ntohl(tempbuf[i]);    
            }
        }
#endif
    }

    return(status);
}


#if 0
    // format of bin file for references.
.BIN_FILE_HEADER : 
{
    __gul_BeginOfHeaderTable = .;
    __gul_BinFileSizeInLongs = .;
        LONG(LOADADDR(.LAST_NBSS_PAGE) >> 2);                /* bin file size in long words */
        LONG(0);                /* checksum if any */
    __gul_NumOfPages = .;
        LONG(9);                
} > VPE1_BIN_IMAGE

.PAGE_0_HDR :
{

    __gul_FileOffsetOf_Page_0 = .;
        LONG(LOADADDR(.THRDX_KERNEL));             /* internal MIPS address - source */
    __gul_InternalAddr_Page_0 = .;
        LONG(ADDR(.THRDX_KERNEL));                 /* internal MIPS address - destination */
    __gul_SizeOf_Page_0 = .;
        LONG(0x80000000 | (((SIZEOF(.THRDX_KERNEL) + 7) >> 3) << 1));    /* size of page in LONG words */
} > VPE1_BIN_IMAGE
#endif

unsigned long loadLtqBin(char *ltqbin_fname)
{
    int fSize;
    int page_num;

    unsigned long __start = 0;
    BinFileHeader_t g_BinFileHeader;
    PageHeader_t g_PageHeader;

#ifndef __KERNEL__
    FILE *binfile;
    fpos_t fpos_cur_header;

    if ((binfile = fopen(ltqbin_fname, "rb")) == NULL)
    {
        printf("Failed to read %s\n", ltqbin_fname);
        return(0);
    }

    
    fseek(binfile, 0, SEEK_END);
    fSize = ftell(binfile);
    rewind(binfile);
    fseek(binfile, 0, SEEK_SET);
#else
    struct file *binfile, tempfile;
    mm_segment_t    oldfs;

    buffer = NULL;
    tempbuf = NULL;


    binfile = filp_open(ltqbin_fname, 00, O_RDONLY);

    if (IS_ERR(binfile) || (binfile == NULL))
    {
        dprintk("%s: Cannot open %s.\n", __func__, ltqbin_fname);
        return 0;
    }
    if (binfile->f_op->read==NULL)
    {
        dprintk("%s: File(system) doesn't allow reads.\n", __func__);
        return 0;
    }

    oldfs = get_fs();
    set_fs(KERNEL_DS);

    // Get file size
    binfile->f_pos = 0;
    fSize = binfile->f_op->llseek(binfile, 0, SEEK_END);

    // allocate 64KB buffer
    //buffer = vmalloc(BUFFER_32KB);
    buffer = kmalloc(BUFFER_32KB, GFP_KERNEL);

    if(buffer == NULL)
    {
        dprintk("%s: No memory available?\n", __func__);
        goto _launcher_exit;
    }

#if 0
    // allocate 64KB buffer
    //tempbuf = vmalloc(BUFFER_32KB);
    tempbuf = kmalloc(BUFFER_32KB, GFP_KERNEL);

    if(tempbuf == NULL)
    {
        dprintk("%s: No memory available?\n", __func__);
        goto _launcher_exit;
    }
#endif

    binfile->f_pos = 0;
#endif


    printf("\nbin file disk size = %d bytes\n", fSize);

    // read in file header - 4 long words
    if (Fread((void*)&g_BinFileHeader, sizeof(BinFileHeader_t), 1, binfile) != 1)
    {
        printf("Failed to read Bin File Header\n");
        goto _launcher_exit;
    }

    printf("bin file embedded image size = %d bytes (0x%08x long words)\n\n", 
           g_BinFileHeader.__gul_BinFileSizeInLongs*4,
           g_BinFileHeader.__gul_BinFileSizeInLongs);

    printf("total num of pages: %d\n\n", g_BinFileHeader.__gul_NumOfPages);

    for (page_num = 0; page_num < g_BinFileHeader.__gul_NumOfPages; page_num++)
    {
        unsigned int SizeOfPage = g_PageHeader.__gul_SizeOf_Page_n & ~BOOT_PAGE_MASK;
        SizeOfPage = g_PageHeader.__gul_SizeOf_Page_n & ~START_PAGE_MASK;

        // read page header
        if (Fread((void*)&g_PageHeader, sizeof(PageHeader_t), 1, binfile) != 1)
        {
            printf("Failed to read Page Header for page %d\n", page_num);
            __start = 0;
            goto _launcher_exit;
        }

#ifdef __KERNEL__
        tempfile.f_pos = binfile->f_pos;
#else
        fgetpos(binfile, &fpos_cur_header);
#endif

        SizeOfPage = g_PageHeader.__gul_SizeOf_Page_n & ~BOOT_PAGE_MASK;
        SizeOfPage = SizeOfPage & ~START_PAGE_MASK;

        printf("Page %d:\n", page_num);
        printf("\tSource Address: 0x%08x\n", g_PageHeader.__gul_FileOffsetOf_Page_n);
        printf("\tDestination Address: 0x%08x\n", g_PageHeader.__gul_InternalAddr_Page_n);

        if (((g_PageHeader.__gul_SizeOf_Page_n & START_PAGE_MASK) == START_PAGE_MASK) && (SizeOfPage != 0))
        {
            __start = g_PageHeader.__gul_InternalAddr_Page_n;
            printf("\t*** Found vpe1 start address __start = 0x%08x ***\n", (unsigned int)__start);
        }

        if (((g_PageHeader.__gul_SizeOf_Page_n & BOOT_PAGE_MASK) == BOOT_PAGE_MASK) && (SizeOfPage != 0))
        {
            printf("\n\tLoading %d long words (0x%08x bytes)...\n\n", SizeOfPage, SizeOfPage*4);

#ifdef __KERNEL__
            binfile->f_op->llseek(binfile, g_PageHeader.__gul_FileOffsetOf_Page_n, SEEK_SET);
#else
            fseek(binfile, g_PageHeader.__gul_FileOffsetOf_Page_n, SEEK_SET);
#endif

            {
                unsigned int loaded_size = SizeOfPage*4;
                unsigned int cur_size, j;
                unsigned char *p_cur_Dest = (unsigned char *)g_PageHeader.__gul_InternalAddr_Page_n;
                unsigned char *p_cur_Source = (unsigned char *)buffer;

                while(loaded_size > 0)
                {
                    if (loaded_size > BUFFER_32KB)
                    {
                        cur_size = BUFFER_32KB;
                    }
                    else
                    {
                        cur_size = loaded_size;
                    }
                    loaded_size -= cur_size;

                    printf("\t\tread from bin file and write block of %d bytes to vpe1 target...\n", cur_size);

                    if (Fread((void*)buffer, 1, cur_size, binfile) != cur_size)
                    {
                        printf("\t\t!!! Failed to read %s after %d bytes !!!\n", vpe1_bin, (int)binfile->f_pos);
                        __start = 0;
                        goto _launcher_exit;
                    }
#ifdef __KERNEL__
                    else 
                    {
                        // write to target memory
                        p_cur_Source = (unsigned char *)buffer;

                        for (j = 0; j < cur_size; j++)
                        {
#if 1
                            WRITE_PHYSICAL_MEM_8b((unsigned char)p_cur_Source[j], p_cur_Dest++);
                            //WRITE_PHYSICAL_MEM_8b((char)0xa0, p_cur_Dest++);
#else
                            printf("0x%02x = 0x%08x >>\n",(char)p_cur_Source[j], (int)p_cur_Dest++);
#endif
                        }

                    }
#endif
                }

                printf("\t\tLast location written = 0x%08x\n", (int)p_cur_Dest - 1);
                
            }
#ifdef __KERNEL__
            binfile->f_pos = tempfile.f_pos;
#else
            fsetpos(binfile, &fpos_cur_header);
#endif

        }
        else
        {
            printf("\n\tSkip loading: non-boot page or empty page. Size = %d bytes...\n\n", SizeOfPage*4);
        }
        
    }

_launcher_exit:
#ifdef __KERNEL__
    if(buffer != NULL)
    {
        //vfree(buffer);
        kfree(buffer);
    }

    if(tempbuf != NULL)
    {
        //vfree(tempbuf);
        kfree(tempbuf);
    }

    set_fs(oldfs);
    // Close the file
    fput(binfile);
#else
    fclose(binfile);

#endif
    return(__start);

}


#ifndef __KERNEL__
int main(int argc, char **argv)
{
    if (argc != 2)
    {
        printf("Wrong arguments\n"
               "Usage: LtqBinReader.exe bin_file_name\n");
        return(1);
    }

    loadLtqBin(argv[1]);

    return(0);
}
#endif
#endif  //MB_SIMULATION
module_init (ModuleInit);
module_exit (ModuleExit);
MODULE_LICENSE ("GPL");
