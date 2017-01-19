/* func.c contains the device functions */

#include <linux/mm.h>
#include <linux/fs.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <asm/uaccess.h>

#include "dk.h"
#include "dk_ioctl.h"
#include "client.h"



static int dk_open
(	
 	struct inode *inode, 
	struct file *file
)
{
		int minor;
		int major;
		int error;
#ifdef DK_DEBUG
		printk("DK:: dk_open \n");
#endif
		major = MAJOR(inode->i_rdev);
		minor = MINOR(inode->i_rdev);
		minor = minor & 0x0f;
		
#ifdef DK_DEBUG
		printk("DK:: dk_open:minor=%d\n", minor);
#endif
		/* 
		 * register client returns the cli id 
		 * or error value
		 */ 
		error = register_client(major, minor);
		if (error < 0) {
				return error;
		}

		/*
		 * Store the client id in the private data field
		 */
		 
		file->private_data = (void *)((unsigned long)error);

		return 0;
}

static int dk_release
(	
 	struct inode *inode, 
	struct file *file
)
{
		int cli_id;
#ifdef DK_DEBUG
		printk("DK:: dk_release \n");
#endif
		cli_id = (int) ((unsigned long)file->private_data);

		unregister_client(cli_id);
		
		return 0;
}

/*
 * Architectures vary in how they handle caching for addresses
 * outside of main memory.
 *
 */
static inline int uncached_access(struct file *file, unsigned long addr)
{
         /*
          * Accessing memory above the top the kernel knows about or through a file pointer
          * that was marked O_SYNC will be done non-cached.
          */
         if (file->f_flags & O_SYNC)
                 return 1;
         return addr >= __pa(high_memory);
}

static int dk_mmap
(
 	struct file *file,
	struct vm_area_struct *vma
)
{
#if defined(__HAVE_PHYS_MEM_ACCESS_PROT)
        unsigned long offset = vma->vm_pgoff << PAGE_SHIFT;

        vma->vm_page_prot = phys_mem_access_prot(file, offset,
                                                 vma->vm_end - vma->vm_start,
                                                 vma->vm_page_prot);
#elif defined(pgprot_noncached)
        unsigned long offset = vma->vm_pgoff << PAGE_SHIFT;
        int uncached;

        uncached = uncached_access(file, offset);
        if (uncached)
                vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
#endif

        /* Remap-pfn-range will mark the range VM_IO and VM_RESERVED */
        if (remap_pfn_range(vma,
                            vma->vm_start,
                            vma->vm_pgoff,
                            vma->vm_end-vma->vm_start,
                            vma->vm_page_prot))
                return -EAGAIN;

		return 0; 
}


static int dk_ioctl
(
 	struct inode *inode, 
	struct file *file,
	unsigned int cmd,
	unsigned long arg
)
{
		INT32 ret=-1;
		INT32 data;
		struct cfg_op co;
		INT32 cli_id;
		INT32 i;
		struct client_info ci;
		struct event_op eo;
		event_handle evt_hnd;
		p_event_struct p_event;
		p_atheros_dev p_client;

		
#ifdef DK_DEBUG
		printk("DK::dk_ioctl \n");
#endif

		cli_id = (int) ((unsigned long)file->private_data);
		p_client = get_client(cli_id);
		if (p_client == NULL) {
				printk("DK:: Invalid client \n");
				return -1;
		}
		switch (cmd) {
			case DK_IOCTL_GET_VERSION:
#ifdef DK_DEBUG
				printk("DK:: DK_IOCTL_GET_VERISION \n");
#endif
				data = (DRV_MAJOR_VERSION << 16) | (DRV_MINOR_VERSION);
				ret = put_user(data, (INT32 *)arg);
				break;
			case DK_IOCTL_GET_CLIENT_INFO:
#ifdef DK_DEBUG
				printk("DK:: DK_IOCTL_GET_CLIENT_INFO \n");
#endif
				if (get_cli_info(cli_id,&ci) < 0) {
					printk("DK:: get_cli_info failed, cli_id : %d \n", cli_id);
					ret = -1;
				} else {
					ret = copy_to_user((void *)arg,(void *)&ci,sizeof(ci));
				}
				ret = 0;
				break;
			case DK_IOCTL_CFG_READ:
				if (copy_from_user((void *)&co,(void *)arg,sizeof(co))) {
						return -EFAULT;
				}
#ifdef DK_DEBUG
				printk("DK::Cfg read @ offset %x \n",co.offset);
#endif
#if defined(OWL_PB42) || defined(PYTHON_EMU)
#ifdef WASP_OSPREY
   if(cli_id!=0){ // For DBDC operation, Wasp radio's client ID is zero; 
#endif
				if (cli_cfg_read(cli_id,co.offset,co.size,&co.value) < 0) {
					ret = -1;
				} else {
					ret = copy_to_user((void *)arg,(void *)&co,sizeof(co));
				}
#ifdef WASP_OSPREY
  }
#endif
#endif
				break;
                        case DK_IOCTL_RTC_REG_READ:
                                if (copy_from_user((void *)&co,(void *)arg,sizeof(co))) {
                                                return -EFAULT;
                                }
#ifdef DK_DEBUG
                                printk("DK::Rtc reg read @ offset %x \n",co.offset);
#endif
#ifndef OCTEON
                                if (rtc_reg_read(cli_id,co.offset,&co.value) < 0) {
                                        ret = -1;
                                } else {
                                        ret = copy_to_user((void *)arg,(void *)&co,sizeof(co));
                                }
#endif
                                break;

			case DK_IOCTL_GET_CHIP_ID:
				if (copy_from_user((void *)&co,(void *)arg,sizeof(co))) {
						return -EFAULT;
				}
#ifdef DK_DEBUG
				printk("DK::Reading Chio ID @ offset %x \n",co.offset);
#endif
#ifndef OCTEON
				if (get_chip_id(cli_id,co.offset,co.size,&co.value) < 0) {
					ret = -1;
				} else {
					ret = copy_to_user((void *)arg,(void *)&co,sizeof(co));
				}
				break;
#endif
			case DK_IOCTL_CFG_WRITE:
				if (copy_from_user((void *)&co,(void *)arg,sizeof(co))) {
						return -EFAULT;
				}
#ifdef DK_DEBUG
				printk("DK::Cfg write @ offset %x : %x \n",co.offset,co.value);
#endif
#if defined(OWL_PB42) || defined(PYTHON_EMU)
#ifdef WASP_OSPREY
   if(cli_id!=0){ // For DBDC operation, Wasp radio's client ID is zero; 
#endif
				if (cli_cfg_write(cli_id,co.offset,co.size,co.value) < 0) {
					ret = -1;
				} else {
					ret = 0;
				}
#ifdef WASP_OSPREY
  }
#endif
#endif
				break;
                        case DK_IOCTL_FULL_ADDR_WRITE:
                                if (copy_from_user((void *)&co,(void *)arg,sizeof(co))) {
                                                return -EFAULT;
                                }
#ifdef DK_DEBUG
                                printk("DK::full addr write @ address %x : %x \n",co.offset,co.value);
#endif
#ifdef AP83
                                if (full_addr_write(cli_id,co.offset,co.value) < 0) {
                                        ret = -1;
                                } else {
                                        ret = 0;
                                }
#endif
                                break;
                        case DK_IOCTL_FULL_ADDR_READ:
                                if (copy_from_user((void *)&co,(void *)arg,sizeof(co))) {
                                                return -EFAULT;
                                }
#ifdef DK_DEBUG
                                printk("DK::Full add read @ address %x \n",co.offset);
#endif
#ifdef AP83
                                if (full_addr_read(cli_id,co.offset,&co.value) < 0) {
                                        ret = -1;
                                } else {
                                        ret = copy_to_user((void *)arg,(void *)&co,sizeof(co));
                                }
#endif
                                break;
                        case DK_IOCTL_RTC_REG_WRITE:
                                if (copy_from_user((void *)&co,(void *)arg,sizeof(co))) {
                                                return -EFAULT;
                                }
#ifdef DK_DEBUG
                                printk("DK::rtc write @ offset %x : %x \n",co.offset,co.value);
#endif
#ifdef AP83
#ifndef WASP
                                if (rtc_reg_write(cli_id,co.offset,co.value) < 0) {
                                        ret = -1;
                                } else {
                                        ret = 0;
                                }
#endif
#endif
                                break;

			case DK_IOCTL_CREATE_EVENT:
#ifdef DK_DEBUG
				printk("DK::Create event \n");
#endif
				if (copy_from_user((void *)&eo,(void *)arg,sizeof(eo))) {
						return -EFAULT;
				}
				ret = -1;
				if (eo.valid) {
			 		evt_hnd.eventID = eo.param[5] & 0xffff;
					evt_hnd.f2Handle = (eo.param[5] >> 16) & 0xffff;
					p_event = createEvent (eo.param[0], // type
					                       eo.param[1], // persistent
					                       eo.param[2], // param1
					                       eo.param[3], // param2
					                       eo.param[4], // param3
					                       evt_hnd);
					if (p_event != NULL) {
						// need to look at the event type to see which queue
						switch (p_event->type ) {
							case ISR_INTERRUPT:
								//if param1 is zero, we, by default
								// set the "ISR IMR" to pass everything
								if ( 0 == p_event->param1 ) {
									p_event->param1 = 0xffffffff;
								}
								if (pushEvent(p_event, &p_client->isr_event_q,
								               TRUE) ) {
										ret = 0;
								} else {
									printk("DK::Push Event Failed \n");
									kfree(p_event);
								} 
								break;
							default:
								printk("DK::Event Type %d not supported \n",p_event->type);
								kfree(p_event);
								break;
						}
					} 
				}
				break;
			case DK_IOCTL_GET_NEXT_EVENT:
#ifdef DK_DEBUG
				printk("DK::Get next event \n");
#endif
				ret = 0;
				eo.valid = 0;
				if (p_client->trigered_event_q.queueSize) {
					if (checkForEvents(&p_client->trigered_event_q,TRUE)){ 
						p_event = popEvent(&p_client->trigered_event_q,TRUE);
						eo.valid = 1;
						eo.param[0] = p_event->type;
						eo.param[1] = p_event->persistent;
						eo.param[2] = p_event->param1; 
						eo.param[3] = p_event->param2;
						eo.param[4] = p_event->param3;
						eo.param[5] = (p_event->eventHandle.f2Handle << 16) | 
						               p_event->eventHandle.eventID; 
						for (i=0;i<6;i++) { 
							eo.param[6+i] = p_event->result[i]; 
						} 
					#ifdef DK_DEBUG 
						printk("DK:: Pop event %x \n",(UINT32)p_event);
					#endif 
						kfree(p_event);
					} 
				}
				ret = copy_to_user((void *)arg,(void *)&eo,sizeof(eo));
				break;
            case DK_IOCTL_FLASH_READ:
                printk("DK:: Flash read is not supported any more from art driver\n");
                break;
            case DK_IOCTL_FLASH_WRITE:
                printk("DK:: Flash read is not supported any more from art driver\n");
                break; 
/*
#ifdef OWL_PB42
            case DK_IOCTL_MAC_WRITE:
#ifdef DK_DEBUG
                 printk("DK::Get DK_IOCTL_MAC_WRITE\n ");
#endif
                 if (copy_from_user((void *)&flashMac,(void *)arg,sizeof(flashMac))) {
                      printk("DK:: Copy_from_user failed 1\n");
                      return -EFAULT;
                 }
                 if (copy_from_user((void *)mac0Addr,(void *)flashMac.pAddr0, 6)){
                     printk("DK:: Copy_from_user failedi 2\n");
                     return -EFAULT;
                 }
                 if (copy_from_user((void *)mac1Addr,(void *)flashMac.pAddr1, 6)){
                     printk("DK:: Copy_from_user failed 3\n");
                     return -EFAULT;
                 }

#ifdef DK_DEBUG
                 printk("DK:: MAC Addr\n");
				 for(i=0; i<6; i++)
					printk("%x  ", mac0Addr[i]);
				 printk("\n");
				 for(i=0; i<6; i++)
					printk("%x  ", mac1Addr[i]);
				 printk("\n");
#endif

				memcpy(&hw_mac_cfg, 0xbf7f0000, 16);
				ar7100_spi_sector_erase(0x7f0000);
				// Copy mac address to ath_hw_cfg structure
				for(i=0; i<6; i++)
			        hw_mac_cfg.macAddr0[i] = mac0Addr[i];

				for(i=0; i<6; i++)
			        hw_mac_cfg.macAddr1[i] = mac1Addr[i];

				ar7100_spi_write_page(0x7f0000, &hw_mac_cfg, 256);				
				ret = 1;
                break; 
#endif
*/
			default:
				printk("DK::Unreconginzed ioctl command %d \n",cmd);
				break;
		}
		return ret;
}


static struct file_operations dk_fops = {
	owner:	THIS_MODULE,
	open:	dk_open,
	release: dk_release,
	mmap:	dk_mmap,
	ioctl  : dk_ioctl
};

INT32  dk_dev_init(void) {
		int status;
		status = register_chrdev(DK_MAJOR_NUMBER,"dk",&dk_fops);
		printk("dk_dev_init::status after register_chrdev(dk) = %d\n", status);

#ifdef DK_UART
	        status |= register_chrdev(DK_UART_MAJOR_NUMBER, "dk_uart", &dk_fops);
		printk("dk_dev_init::status after register_chrdev(dk_uart) = %d\n", status);
#endif
		return status;
}

void dk_dev_exit(void) {
		unregister_chrdev(DK_MAJOR_NUMBER,"dk");
//#ifdef DK_UART
		unregister_chrdev(DK_UART_MAJOR_NUMBER,"dk_uart");
//#endif
}
