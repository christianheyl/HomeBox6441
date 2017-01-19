/* ============================================================
Follow by
IEEE Standard for Information technologyiX
Telecommunications and information exchange between systemsiX
Local and metropolitan area networksiX
Specific requirements

Part 3: Carrier Sense Multiple Access with
Collision Detection (CSMA/CD) access method
and Physical Layer specifications

SECTION  FIVE:  This  section  includes  Clause 56  through  Clause 74  and  Annex  57A  through
Annex 74A. (Third printing: 22 June 2010.)

Annex 57A
Requirements for support of Slow Protocols


Programmer : Alvin Hsu, alvin_hsu@arcadyan.com.tw
=============================================================== */

#ifdef SUPERTASK
#include "oam.h"

#include "etcpip.h"
#include "if.h"

#include "ethernet.h"
#if(_SIP == 2)
#include "tel_mgr.h"
#endif

#define printk dprintf

#else // SUPERTASK
/*
 *  Common Head File
 */
#include <linux/autoconf.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/types.h>
#include <linux/ctype.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/miscdevice.h>
#include <linux/init.h>
#include <linux/etherdevice.h>  /*  eth_type_trans  */
#include <linux/ethtool.h>      /*  ethtool_cmd     */
#include <linux/if_ether.h>
#include <linux/if_arp.h>
#include <asm/uaccess.h>
#include <asm/unistd.h>
#include <asm/irq.h>
#include <asm/delay.h>
#include <asm/io.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <net/xfrm.h>
#include <linux/if_vlan.h>

#include <linux/kmod.h>
#include <linux/net.h>		/* struct socket, struct proto_ops */
#include <linux/socket.h>	/* SOL_SOCKET */
#include <linux/errno.h>	/* error codes */
#include <linux/capability.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/time.h>		/* struct timeval */
#include <linux/skbuff.h>
#include <linux/bitops.h>
#include <linux/init.h>
#include <net/sock.h>		/* struct sock */

#include <asm/uaccess.h>
#include <asm/atomic.h>
#include <asm/poll.h>

#include <linux/kmod.h>

#include "eoam.h"

int eoam_notifier_wrapper ();

#define BUFSZ1 ETHER_FRAME_MAX_LEN
#define GetBuffer(len) dev_alloc_skb(len)

#define dprintf printk
#define GetBufferLength(mp)         (((struct sk_buff *)mp)->len)
#define SetBufferLength(mp, length)    ((struct sk_buff *)mp)->len = length
#define BUF_BUFFER_PTR(mp)          (unsigned char *)(((struct sk_buff *)mp)->data)

unsigned long GetTime(void)
{
	return jiffies;
}

int EOAM_LOOPBACK_TASK(int s32Ifno, char *ps8PktBuff, int s32BuffLen);

#endif // SUPERTASK

#define __GCC_ATTR_DATA__
#define __GCC_ATTR_BSS__

char gEOAM_Ifname[EOAM_IFNAME_LENGTH] = ETH_EOAM_IFNAME;
#ifdef SUPERTASK
int gEOAM_Ifno = T_WAN_INT;
#endif

unsigned short gMTU = INFO_TLV_OAMPDU_DEFAULT;

unsigned char gEOAMEnableDebug = 0;

char gs8LocalEoamEnable = FALSE;
__GCC_ATTR_DATA__ char gs8LocalEoamBegin = TRUE;								/* A variable that resets the functions within OAM. */
__GCC_ATTR_DATA__ char gs8LocalEoamMode = EOAM_LAYER_DISABLE;		/* Used to configure the OAM sublayer entity in either Active or Passive mode. */

__GCC_ATTR_DATA__ char gs8DiscTimes =0;													/* Used to calc discovery process times */

__GCC_ATTR_DATA__ char gs8localEoamLoopbackITR = 0;							/*  */

__GCC_ATTR_DATA__ char gs8LocalLostLnkTimeDone = 0;							/* Timer flag */
__GCC_ATTR_DATA__ char gs8LoopBackSettingTimeDone = 0;					/* Timer flag */
__GCC_ATTR_DATA__ unsigned long gu64LocalLostLnkTimeSnap = 0;		/* Timer used to reset the Discovery state diagram */
__GCC_ATTR_DATA__ unsigned long gu64LoopbackLostTimeSnap = 0;		/* Timer used to reset the Loopback send-processing */


stEOAM_LAYER_CTL gstLocalEOamCtl ;
__GCC_ATTR_BSS__ stEOAM_LAYER_CTL gstTempLocalEOamCtl ;

/*
	[Support Events]

	0 Link fault
	0 Dying Gasp, Unrecoverable failure
	0 Critical Event, Unspecified critical link evt

	[Support func. for OAM configuration field]

	0 Unidirectional
	1 LoopBack
	0 Link Event
	0 VariRetrieval
*/
#if 1 // bitonic
// disable loopback function
static char gs8LocalEoamFunctionality[EOAM_SUPPORT_FUNC_NO] = {0, 0, 0, 0, 0, 0, 0};
#else
static char gs8LocalEoamFunctionality[EOAM_SUPPORT_FUNC_NO] = {0, 0, 0, 0, 1, 0, 0};
#endif
static char *gs8LocalEoamFunctionalityStr[EOAM_SUPPORT_FUNC_NO] = {FUNC_LINKFAULT_STR, FUNC_DYINGGASP_STR, FUNC_CRITICALEVENT_STR, FUNC_UNIDIRECTIONAL_STR, FUNC_LOOPBACK_STR, FUNC_LINKEVENT_STR, FUNC_VARIRETRIEVAL_STR, '\0'};

static const char SLOW_PTL_MULTIMAC[ETHER_MAC_LEN] = {0x01,0x80,0xC2,0x00,0x00,0x02};	/* Slow Protocol Multicast Addr */
#ifndef SUPERTASK
static const char MY_SLOW_PTL_MAC[ETHER_MAC_LEN] = {0X00};	/* Slow Protocol Multicast Addr */
#endif // SUPERTASK

static int gs32VoIPServiceStopUse=0;

extern void setDTLog(char * type, char * num, char *reason);
#ifdef _WAN_CLONE
extern int is_wan_uplink_enabled(void);
#endif
extern int get_wan_mode(void);
extern int switch_getPortLinkStatus(int port); // logical port
extern int vr9_GetPortLinkStatus(int port); // phy port

#if(_SIP == 2)
extern TEL_MGR_RC_T TEL_MGR_DoesAnyEmergencyCallExist();
#endif

extern int VoIPServiceStop();
extern int VoIPServiceRestart();

#ifndef SUPERTASK
typedef struct stGSETTING{
	char	s8LOCAL_EOAM_BEGIN;			/* A variable that resets the functions within OAM. */
	char 	s8LOCAL_EOAM_MODE;			/* Used  to  configure  the  OAM  sublayer  entity  in  either  Active  or  Passive  mode. */
} GSETTING;

typedef struct task_struct *OSHAL_PID;

GSETTING gSetting;
struct net_device *MY_SLOW_PTL_DEV = NULL;
OSHAL_PID MY_SLOW_PTL_THREAD_ID = NULL;

// Kernel Thread
char thread_s8EOamMode; // thread input data must be global variable.

void KernelThreadDealy(int mseconds)
{
	set_current_state(TASK_INTERRUPTIBLE);
	//schedule_timeout(jiffies);
	schedule_timeout (mseconds*HZ/1000);
}

/*******************************************************************************
Description: OS Wrapper API to Spawn a Task
Arguments:
Return:  Pointer to a Task Structure
*******************************************************************************/
OSHAL_PID OSHAL_Spawn_Thread(int (*threadfn) (void *ptr_data), void *ptr_data,char *taskName, unsigned long unused_flags){
    OSHAL_PID thread_id;
    /* Kick Start the Thread >>>>>>>>> */
    // flags|=(CLONE_FS | CLONE_FILES | CLONE_SIGHAND | SIGCHLD);
    // thread_id=kernel_thread(threadfn, ptr_data,flags);
    thread_id = kthread_run(threadfn, ptr_data, taskName);
    /* <<<<<<<<<<*/
    return 	thread_id;
}

/*******************************************************************************
Description: OS Wrapper API to Delete a Task
Arguments: None
Return:  0
*******************************************************************************/
void OSHAL_Kill_Thread(OSHAL_PID pTID){
    kthread_stop(pTID);
}

OSHAL_PID OSHAL_Current_Thread (void)
{
    return current;
}
// end of Kernel Thread

void GetEOAMInterfaceMacAddress()
{
	struct net_device *dev = NULL;

	// Get EOAM interface mac address
	if ((dev = __dev_get_by_name(&init_net, gEOAM_Ifname)) == NULL)
	{
		printk("[GetEOAMInterfaceMacAddress] can not get %s netdev\n", gEOAM_Ifname);
		return EOAM_FAIL;
	}
	if(MY_SLOW_PTL_DEV == NULL)
		MY_SLOW_PTL_DEV = dev;
	memcpy(MY_SLOW_PTL_MAC, dev->dev_addr, ETHER_MAC_LEN);
	dprintf("%s[GetEOAMInterfaceMacAddress] MAC=%02X:%02X:%02X:%02X:%02X:%02X\n%s\n",\
					 CLR1_33_YELLOW, dev->dev_addr[0],dev->dev_addr[1],dev->dev_addr[2],\
					 dev->dev_addr[3],dev->dev_addr[4],dev->dev_addr[5],CLR0_RESET);
}
#endif // SUPERTASK

#ifdef SUPERTASK
void SetEOAMIfnoAndName()
{
	int bEtherUplink = 0;

	#ifdef _WAN_CLONE
	if(is_wan_uplink_enabled())
	{
		bEtherUplink = 1;
	}
	#endif

	if(bEtherUplink)
	{
		SetEOAMBaseIfName(ETH_EOAM_IFNAME);
	}
	else
	{
		SetEOAMBaseIfName(PTM_EOAM_IFNAME);
	}
}
#endif

void SetEOAMBaseIfName(char *pBaseIfName)
{
	if(pBaseIfName == NULL) return;

	strcpy(gEOAM_Ifname, pBaseIfName);
#ifdef SUPERTASK
	if(strcmp(gEOAM_Ifname, ETH_EOAM_IFNAME) == 0)
		gEOAM_Ifno = T_WAN_CLONE_INT;// Ether-Uplink mode
	else
		gEOAM_Ifno = T_WAN_INT;// VDSL mode
#endif
}

#ifdef SUPERTASK
typedef unsigned long u32;
#endif
static inline void eoam_print_data(unsigned char *pData, u32 len, char *title)
{
    int i;

	printk("%s\n", title);
    printk("  pdata = %08X, len = %d\n", (u32)pData, len);
    for ( i = 1; i <= len; i++ )
    {
        if ( i % 16 == 1 )
            printk("  %4d:", i - 1);
        printk(" %02X", (int)(*((char*)pData + i - 1) & 0xFF));
        if ( i % 16 == 0 )
            printk("\n");
    }
    if ( (i - 1) % 16 != 0 )
        printk("\n");
}

/*****************************************************************
*	Utility
*****************************************************************/
#if(EOAM_DEBUG_DUMP == TRUE)
int DumpTLV_RAW(char *ps8buf, char *ps8color)
{
		int s32TlvOffset=0;
		int i;

	if(gEOAMEnableDebug == 0) return EOAM_SUCCESS;

		//uart_tx_onoff(1);
		for (i=0;i<INFO_TLV_LEN;i++)
		{
				if(i%4==0)	dprintf(" ");
	      if(i%16==0) dprintf("\n%04x: ", i);
	      dprintf("%s%02x %s", ps8color,ps8buf[i],CLR0_RESET);
	  }

		dprintf("\n\n");
		//uart_tx_onoff(0);

		return EOAM_SUCCESS;
}

int DumpTLV_Infomation(char *ps8buf, char *ps8color)
{
		int s32PduOffset=0;
		int i;

	if(gEOAMEnableDebug == 0) return EOAM_SUCCESS;

		//uart_tx_onoff(1);
		dprintf("\n");
		if(*(ps8buf+0) != INFO_TLV_LOCAL_INFO)
		{
				dprintf("Wrong Information Type=%02X! goto DUMP_RAW_DATA...\n",*(ps8buf));
				goto DUMP_RAW_DATA;
		}

		if(*(ps8buf+1) != INFO_TLV_LEN)
		{
				dprintf("Wrong Information Leng=%02X! goto DUMP_RAW_DATA...\n",*(ps8buf+1));
				goto DUMP_RAW_DATA;
		}

		if(*(ps8buf+2)&0x01 == 0)
		{
				dprintf("Wrong OAM Version=%02X! goto DUMP_RAW_DATA...\n",*(ps8buf+2));
				goto DUMP_RAW_DATA;
		}

		dprintf("DumpTLV_LocalInfo===============\n");
		while(*(ps8buf+s32PduOffset) != END_OF_TLV_MARKER)
		{
				dprintf("%d TLV : \n",(s32PduOffset/INFO_TLV_LEN)+1);
				dprintf("  Information Type=%02d(0x%02x)\n",*(ps8buf+s32PduOffset+0),*(ps8buf+s32PduOffset+0));
				dprintf("  Information Leng=%02d(0x%02x)\n",*(ps8buf+s32PduOffset+1),*(ps8buf+s32PduOffset+1));
				dprintf("  OAM Version=%02d\n",*(ps8buf+s32PduOffset+2));
				dprintf("  Revision[0]=0x%02x, [1]=0x%02x\n",*(ps8buf+s32PduOffset+3),*(ps8buf+s32PduOffset+4));
				dprintf("  OAM DTE State=%02d\n",*(ps8buf+s32PduOffset+5));
				dprintf("  OAM Configuration=%02d(0x%02x)\n",*(ps8buf+s32PduOffset+6),*(ps8buf+s32PduOffset+6));
		dprintf("  MAX OAMPDU Size[0]=%02d(0x%02x), [1]=%02d(0x%02X)\n",*((unsigned char *)(ps8buf+s32PduOffset+7)),*((unsigned char *)(ps8buf+s32PduOffset+7)),*((unsigned char *)(ps8buf+s32PduOffset+8)),*((unsigned char *)(ps8buf+s32PduOffset+8)));
		dprintf("  OUI[0]=0x%02x, [1]=0x%02x, [2]=0x%02x\n",*((unsigned char *)(ps8buf+s32PduOffset+9)),*((unsigned char *)(ps8buf+s32PduOffset+10)),*((unsigned char *)(ps8buf+s32PduOffset+11)));
				dprintf("  Vendor Specific Info[0]=%02d, [1]=%02d, [2]=%02d, [3]=%02d\n",*(ps8buf+s32PduOffset+12),*(ps8buf+s32PduOffset+13),*(ps8buf+s32PduOffset+14),*(ps8buf+s32PduOffset+15));
				s32PduOffset += INFO_TLV_LEN;
		}
		dprintf("DumpTLV_LocalInfo===============\n");
		//uart_tx_onoff(0);

		return EOAM_SUCCESS;

DUMP_RAW_DATA:
		for (i=0;i<INFO_TLV_LEN;i++)
		{
				if(i%4==0)	dprintf(" ");
			  if(i%16==0) dprintf("\n%04x: ", i);
			  //dprintf("%02x ", ps8buf[i]);
			  dprintf("%s%02x %s", ps8color,ps8buf[i],CLR0_RESET);
		}

		dprintf("\n\n");
		//uart_tx_onoff(0);

		return EOAM_FAIL;
}

int DumpTLV_LinkEvent(char *ps8buf)
{
		return EOAM_SUCCESS;
}

int DumpTLV_VariReq(char *ps8buf)
{
		return EOAM_SUCCESS;
}

int DumpTLV_VariResp(char *ps8buf)
{
		return EOAM_SUCCESS;
}

int DumpTLV_LoopBack(char *ps8buf, char *ps8color)
{
		int i;

	if(gEOAMEnableDebug == 0) return EOAM_SUCCESS;

		//uart_tx_onoff(1);
		dprintf("\n");
		if((*(ps8buf+0) != REMOTE_LB_ENABLE) || (*(ps8buf+0) != REMOTE_LB_DISABLE))
		{
				dprintf("Wrong LoopBack Ctrl Command(0x%X)! goto DUMP_RAW_DATA...\n",*(ps8buf+0));
				goto DUMP_RAW_DATA;
		}

		dprintf("DumpTLV_LoopBack===============\n");
		dprintf("LoopBack Remote LoopBack command = %d\n\n",*(ps8buf+0));
		for (i=0;i<LOOPBACK_TLV_LEN;i++)
		{
				if(i%4==0)	dprintf(" ");
			  if(i%16==0) dprintf("\n%04x: ", i);
			  //dprintf("%02x ", ps8buf[i]);
			  dprintf("%s%02x %s", ps8color,ps8buf[i],CLR0_RESET);
		}
		dprintf("DumpTLV_LoopBack===============\n");
		//uart_tx_onoff(0);

		return EOAM_SUCCESS;

DUMP_RAW_DATA:
		for (i=0;i<LOOPBACK_TLV_LEN;i++)
		{
				if(i%4==0)	dprintf(" ");
			  if(i%16==0) dprintf("\n%04x: ", i);
			  //dprintf("%02x ", ps8buf[i]);
			  dprintf("%s%02x %s", ps8color,ps8buf[i],CLR0_RESET);
		}

		dprintf("\n\n");
		//uart_tx_onoff(0);

		return EOAM_FAIL;
}

int DumpTLV_OrganSpecific(char *ps8buf)
{
		return EOAM_SUCCESS;
}


void DumpBuffer_RAW(char *ps8buf, int s32len, char *ps8color)
{
    int i;

	if(gEOAMEnableDebug == 0) return;

		//uart_tx_onoff(1);
		if(s32len==0)	dprintf("len=0!!ERROR!!\n");

    dprintf("%s[%s]%s>TO:[",ps8color,__FUNCTION__,CLR0_RESET);
    for(i=0; i<6; i++)
        dprintf("%s%02x %s",ps8color,ps8buf[i],CLR0_RESET);
    dprintf("]FM:[");
    for(i=6; i<12; i++)
        dprintf("%s%02x %s",ps8color,ps8buf[i],CLR0_RESET);
    dprintf("] TYPE:%s%02x%02x (%04x)%s\n",ps8color,ps8buf[12],ps8buf[13],*(short *)(ps8buf+12),CLR0_RESET);

    //if((*(short *)(ps8buf+12)&0xFFFF) == ET_SLOW_PTL)
    //{
		for(i=0;i<s32len;i++)
		{
				if(i%4==0)	dprintf(" ");
        if(i%16==0) dprintf("\n%04x: ", i);
        dprintf("%s%02x %s", ps8color,ps8buf[i],CLR0_RESET);
    }

    dprintf("\n\n");

    //}else
    	//dprintf("%sNot slow protocol packet!\n%s",CLR1_31_RED,CLR0_RESET);

		//uart_tx_onoff(0);

		return;
}

int DumpBuffer_EOAM(char *ps8buf, int s32len, char *ps8color)
{
    int i;

	if(gEOAMEnableDebug == 0) return EOAM_SUCCESS;

		//uart_tx_onoff(1);
		if(s32len==0)	dprintf("len=0!!ERROR!!\n");
		if((*(short *)(ps8buf+12)&0xFFFF) != ET_SLOW_PTL)
		{
				dprintf("%sNot slow protocol packet! TYPE:%02x%02x\n%s",CLR1_31_RED,ps8buf[12],ps8buf[13],CLR0_RESET);
				//uart_tx_onoff(0);
				return EOAM_FAIL;
		}

    dprintf("%s[%s]%s>TO:[",ps8color,__FUNCTION__,CLR0_RESET);
    for(i=0; i<6; i++)
        dprintf("%s%02x %s", ps8color,ps8buf[i],CLR0_RESET);
    dprintf("]FM:[");
    for(i=6; i<12; i++)
        dprintf("%s%02x %s", ps8color,ps8buf[i],CLR0_RESET);
    dprintf("] TYPE:%s%02x%02x %s\n",ps8color,ps8buf[12],ps8buf[13],CLR0_RESET);

    if((*(short *)(ps8buf+12)&0xFFFF) == ET_SLOW_PTL)
    {
				dprintf("Subtype    = %s%02d%s\n",ps8color,*(ps8buf+14),CLR0_RESET);
				dprintf("Flags[0][1]= %s%02x, %02x%s\n",ps8color,*(ps8buf+15),*(ps8buf+16),CLR0_RESET);
				dprintf("Code       = %s%02d%s\n",ps8color,*(ps8buf+17),CLR0_RESET);

				if(((gstLocalEOamCtl.s8DiscStatus == EOAM_STATUS_DISC_ACTIVE_SEND_LOCAL) && (gstLocalEOamCtl.s8RemoteStateValid == 1)) ||\
					((gstLocalEOamCtl.s8DiscStatus == EOAM_STATUS_DISC_PASSIVE_WAIT) && (gstLocalEOamCtl.s8RemoteStateValid == 1)) ||\
					(gstLocalEOamCtl.s8DiscStatus == EOAM_STATUS_DISC_SEND_LOCAL_REMOTE) ||\
					(gstLocalEOamCtl.s8DiscStatus == EOAM_STATUS_DISC_SEND_LOCAL_REMOTE_SUCCESS) ||\
					(gstLocalEOamCtl.s8DiscStatus == EOAM_STATUS_DISC_SEND_ANY))
				{
						if(*(ps8buf+17) == CODE_INFO)
						{
								DumpTLV_Infomation((ps8buf+18),ps8color);
						}
						else if(*(ps8buf+17) == CODE_EVENT)
						{
								DumpTLV_LinkEvent((ps8buf+18));
						}
						else if(*(ps8buf+17) == CODE_VARREQ)
						{
								DumpTLV_VariReq((ps8buf+18));
						}
						else if(*(ps8buf+17) == CODE_VARRESP)
						{
								DumpTLV_VariResp((ps8buf+18));
						}
						else if(*(ps8buf+17) == CODE_LOOPBACK_CTL)
						{
								//dprintf("Remote LoopBack CMD=%02d\n",*(ps8buf+18));
								DumpTLV_LoopBack((ps8buf+18),ps8color);
						}
						else if(*(ps8buf+17) == CODE_ORGSPEC)
						{
								DumpTLV_OrganSpecific((ps8buf+18));
						}
						else
						{
								dprintf("Wrong Code=%02d\n",*(ps8buf+17));

								dprintf("Dump RAW data ...");
								for(i=0;i<s32len;i++)
								{
										if(i%4==0)	dprintf(" ");
							      if(i%16==0) dprintf("\n%04x: ", i);

										dprintf("%s%02x %s", ps8color,ps8buf[i],CLR0_RESET);
							  }

								dprintf("\n\n");
								//uart_tx_onoff(0);

								return EOAM_FAIL;
						}
				}

				#if 1
				dprintf("Dump RAW data ...");
				for(i=0;i<s32len;i++)
				{
						if(i%4==0)	dprintf(" ");
			      if(i%16==0) dprintf("\n%04x: ", i);
						dprintf("%s%02x %s", ps8color,ps8buf[i],CLR0_RESET);
			  }
				#endif

				dprintf("\n\n");
    }

		//uart_tx_onoff(0);

		return EOAM_SUCCESS;
}

int DumpEOam_CtlInfo(void)
{
	if(gEOAMEnableDebug == 0) return EOAM_SUCCESS;

		//uart_tx_onoff(1);
		dprintf("[%s]Dump gstLocalEOamCtl structure!============\n",__FUNCTION__);
		dprintf("s8LocalLinkStatus=%d\n",gstLocalEOamCtl.s8LocalLinkStatus);
		dprintf("s8LocalDyingGasp=%d\n",gstLocalEOamCtl.s8LocalDyingGasp);
		dprintf("s8LocalVariRetrieval=%d\n",gstLocalEOamCtl.s8LocalVariRetrieval);
		dprintf("s8LocalCriticalEvent=%d\n",gstLocalEOamCtl.s8LocalCriticalEvent);

		dprintf("s8LocalEOamLoopBack=%d\n",gstLocalEOamCtl.s8LocalEOamLoopBack);
		dprintf("s8LocalUnidirectional=%d\n",gstLocalEOamCtl.s8LocalUnidirectional);

		dprintf("s8DiscStatus=%d, => ",gstLocalEOamCtl.s8DiscStatus);
		if(gstLocalEOamCtl.s8DiscStatus == EOAM_STATUS_DISC_FAULT)
			dprintf("EOAM_STATUS_DISC_FAULT\n");
		else if(gstLocalEOamCtl.s8DiscStatus == EOAM_STATUS_DISC_PASSIVE_WAIT)
			dprintf("EOAM_STATUS_DISC_PASSIVE_WAIT\n");
		else if(gstLocalEOamCtl.s8DiscStatus == EOAM_STATUS_DISC_ACTIVE_SEND_LOCAL)
			dprintf("EOAM_STATUS_DISC_ACTIVE_SEND_LOCAL\n");
		else if(gstLocalEOamCtl.s8DiscStatus == EOAM_STATUS_DISC_SEND_LOCAL_REMOTE)
			dprintf("EOAM_STATUS_DISC_SEND_LOCAL_REMOTE\n");
		else if(gstLocalEOamCtl.s8DiscStatus == EOAM_STATUS_DISC_SEND_LOCAL_REMOTE_SUCCESS)
			dprintf("EOAM_STATUS_DISC_SEND_LOCAL_REMOTE_SUCCESS\n");
		else if(gstLocalEOamCtl.s8DiscStatus == EOAM_STATUS_DISC_SEND_ANY)
			dprintf("EOAM_STATUS_DISC_SEND_ANY\n");
		else
			dprintf("\n");

		dprintf("s8LocalPdu=%d, => ",gstLocalEOamCtl.s8LocalPdu);
		if(gstLocalEOamCtl.s8LocalPdu == LOCAL_PDU_LF_INFO)
			dprintf("LOCAL_PDU_LF_INFO\n");
		else if(gstLocalEOamCtl.s8LocalPdu == LOCAL_PDU_RX_INFO)
			dprintf("LOCAL_PDU_RX_INFO\n");
		else if(gstLocalEOamCtl.s8LocalPdu == LOCAL_PDU_INFO)
			dprintf("LOCAL_PDU_INFO\n");
		else if(gstLocalEOamCtl.s8LocalPdu == LOCAL_PDU_ANY)
			dprintf("LOCAL_PDU_ANY\n");
		else
			dprintf("\n");

		dprintf("s8LocalStable=%d\n",gstLocalEOamCtl.s8LocalStable);
		dprintf("s8RemoteStable=%d\n",gstLocalEOamCtl.s8RemoteStable);
		dprintf("s8LocalSatisfied=%d\n",gstLocalEOamCtl.s8LocalSatisfied);
		dprintf("s8RemoteStateValid=%d\n",gstLocalEOamCtl.s8RemoteStateValid);


		dprintf("s8LocalEOamInited=%d\n",gstLocalEOamCtl.s8LocalEOamInited);
		dprintf("s8LocalEOamMode=%d, => ",gstLocalEOamCtl.s8LocalEOamMode);
		if(gstLocalEOamCtl.s8LocalEOamMode == EOAM_DISABLE_MODE)
			dprintf("EOAM_DISABLE_MODE\n");
		else if(gstLocalEOamCtl.s8LocalEOamMode == EOAM_PASSIVE_MODE)
			dprintf("EOAM_PASSIVE_MODE\n");
		else if(gstLocalEOamCtl.s8LocalEOamMode == EOAM_ACTIVE_MODE)
			dprintf("EOAM_ACTIVE_MODE\n");
		else
			dprintf("\n");

		dprintf("s8SrcOUI=%02X:%02X:%02X\n",gstLocalEOamCtl.s8LocalDTEOUI[0],gstLocalEOamCtl.s8LocalDTEOUI[1],gstLocalEOamCtl.s8LocalDTEOUI[2]);
		dprintf("[%s]Dump gstLocalEOamCtl structure!============\n",__FUNCTION__);
		//uart_tx_onoff(0);

		return EOAM_SUCCESS;
}
#endif

/*****************************************************************
*
*   PROCEDURE NAME:
*   EOAMTLV_InfoLocal_Request
*
*   DESCRIPTION:
*		Generate Local Information TLV.
*
*   NOTE:
*
*****************************************************************/
#ifdef SUPERTASK
static char* EOAMTLV_InfoLocal_Request(char s8TlvType, char *ps8PktBuff)
#else
static char* EOAMTLV_InfoLocal_Request(char s8TlvType, struct sk_buff *ps8PktBuff)
#endif // SUPERTASK
{
	static short s16Revision = 0;
	unsigned short nRemoteMTU, nLocalMTU;
	stLOCAL_INFO_TLV *pstLocalInfoTlv,*pstLocalInfoRMTTlv;

	EOAM_MSG(dprintf("[%s] ps8PktBuff=0x%x, s8TlvType=%d, ",__FUNCTION__,ps8PktBuff,s8TlvType));

		if(ps8PktBuff == NULL)
		{
				if((ps8PktBuff=GetBuffer(BUFSZ1))==NULL)
				{
						EOAM_MSG(dprintf("%s[%s]Get Buffer[%d] Fail!\n%s",CLR1_31_RED,__FUNCTION__,BUFSZ1,CLR0_RESET));
						return NULL;
				}
		}

		SetBufferLength(ps8PktBuff, ETHER_FRAME_MIN_LEN);
		memset(BUF_BUFFER_PTR(ps8PktBuff),0,ETHER_FRAME_MIN_LEN);
		//dprintf("GetBufferLength=%d\n",GetBufferLength(ps8PktBuff));

		/* Local Information TLV */
		if((s8TlvType&BIT00) == INFO_TLV_LOCAL_INFO)
		{
				EOAM_MSG(dprintf("Send Local Info TLV "));

				pstLocalInfoTlv = (stLOCAL_INFO_TLV *)(BUF_BUFFER_PTR(ps8PktBuff)+ETHER_HDR_LEN+SLOW_HDR_LEN);

				pstLocalInfoTlv->s8InfoType 	= INFO_TLV_LOCAL_INFO;
				pstLocalInfoTlv->s8InfoLeng 	= INFO_TLV_LEN;
				pstLocalInfoTlv->s8OAMVer	  	= (INFO_TLV_OAM_VERSION | 0x01);	/* Match IEEE802.3AH clause 57 */
				pstLocalInfoTlv->s8Rev[0]	  	= ((s16Revision)>>8)&0xFF;
				pstLocalInfoTlv->s8Rev[1]	  	= s16Revision&0xFF;
				if( (gstLocalEOamCtl.s8LocalStable == 0) || (gstLocalEOamCtl.s8RemoteStable == 0) )
					s16Revision++;
				pstLocalInfoTlv->s8InfoState 	= gstLocalEOamCtl.s8LocalParAction+(gstLocalEOamCtl.s8LocalMuxAction<<BIT01);
				//dprintf("gstLocalEOamCtl.s8LocalParAction=%d\n",gstLocalEOamCtl.s8LocalParAction);
				//dprintf("gstLocalEOamCtl.s8LocalMuxAction=%d\n",gstLocalEOamCtl.s8LocalMuxAction<<BIT01);

				if(gstLocalEOamCtl.s8LocalEOamMode == EOAM_ACTIVE_MODE)
						pstLocalInfoTlv->s8InfoOamCfg |= INFO_TLV_CONF_ACTIVE;
				else
						pstLocalInfoTlv->s8InfoOamCfg &= ~INFO_TLV_CONF_ACTIVE;

				/* DTE is capable of sending OAMPDUs when the receive path is non-operational or not*/
				if(gstLocalEOamCtl.s8LocalUnidirectional)
						pstLocalInfoTlv->s8InfoOamCfg |= INFO_TLV_CONF_UNIDIRC;

				/* DTE is capable of OAM remote loopback mode or not*/
				if(gstLocalEOamCtl.s8LocalEOamLoopBack)
						pstLocalInfoTlv->s8InfoOamCfg |= INFO_TLV_CONF_OAMRMLB;

				/* DTE supports interpreting Link Events or not*/
				if(gstLocalEOamCtl.s8LocalEoamLinkEvent)
						pstLocalInfoTlv->s8InfoOamCfg |= INFO_TLV_CONF_LINK;

				/* DTE supports sending Variable Response OAMPDUs or not*/
				if(gstLocalEOamCtl.s8LocalVariRetrieval)
						pstLocalInfoTlv->s8InfoOamCfg |= INFO_TLV_CONF_VARRESP;

				/* This value is compared to the remote's Maximum OAMPDU Size and the smaller of the two is used.*/
				if(gstLocalEOamCtl.s8RemoteStateValid)
				{
					//dprintf("[EOAMTLV_InfoLocal_Request] RMT MAX OAMPDU Size[0]=0x%02x,[1]=0x%02X\n",gstLocalEOamCtl.stRMTLocalInfoTlv.s8InfoOampduCfg[0],gstLocalEOamCtl.stRMTLocalInfoTlv.s8InfoOampduCfg[1]);
					//dprintf("[EOAMTLV_InfoLocal_Request] LOCAL MAX OAMPDU Size[0]=0x%02x,[1]=0x%02X\n",gstLocalEOamCtl.stLocalInfoTlv.s8InfoOampduCfg[0],gstLocalEOamCtl.stLocalInfoTlv.s8InfoOampduCfg[1]);
					nRemoteMTU = ((gstLocalEOamCtl.stRMTLocalInfoTlv.s8InfoOampduCfg[0]<<8)|(gstLocalEOamCtl.stRMTLocalInfoTlv.s8InfoOampduCfg[1]));
					nLocalMTU = ((gstLocalEOamCtl.stLocalInfoTlv.s8InfoOampduCfg[0]<<8)|(gstLocalEOamCtl.stLocalInfoTlv.s8InfoOampduCfg[1]));
					if(nRemoteMTU > INFO_TLV_OAMPDU_MAXSIZE)
					{
						pstLocalInfoTlv->s8InfoOampduCfg[0] = (gMTU & 0xFF00)>>8;
						pstLocalInfoTlv->s8InfoOampduCfg[1] = (gMTU & 0x00FF);
					}

					/* This value is compared to the remote's Maximum OAMPDU Size and the smaller of the two is used. */
					if(nRemoteMTU > nLocalMTU)
					{
						if(nLocalMTU == 0x0)
						{
							pstLocalInfoTlv->s8InfoOampduCfg[0] = (gMTU&0xFF00)>>8;
							pstLocalInfoTlv->s8InfoOampduCfg[1] = (gMTU&0x00FF);
						}
						else
						{
							pstLocalInfoTlv->s8InfoOampduCfg[0] = gstLocalEOamCtl.stLocalInfoTlv.s8InfoOampduCfg[0];
							pstLocalInfoTlv->s8InfoOampduCfg[1] = gstLocalEOamCtl.stLocalInfoTlv.s8InfoOampduCfg[1];
						}
					}
					else
					{
						/* Prevent 0 */
						if(nRemoteMTU == 0x0)
						{
							pstLocalInfoTlv->s8InfoOampduCfg[0] = (gMTU&0xFF00)>>8;
							pstLocalInfoTlv->s8InfoOampduCfg[1] = (gMTU&0x00FF);
						}
						else
						{
							pstLocalInfoTlv->s8InfoOampduCfg[0] = gstLocalEOamCtl.stRMTLocalInfoTlv.s8InfoOampduCfg[0];
							pstLocalInfoTlv->s8InfoOampduCfg[1] = gstLocalEOamCtl.stRMTLocalInfoTlv.s8InfoOampduCfg[1];
						}
					}
				}
				else
				{
					pstLocalInfoTlv->s8InfoOampduCfg[0] = (gMTU&0xFF00)>>8;
					pstLocalInfoTlv->s8InfoOampduCfg[1] = (gMTU&0x00FF);
				}

				//dprintf("s8InfoOampduCfg[0]=0x%02x,[1]=0x%02X\n",pstLocalInfoTlv->s8InfoOampduCfg[0],pstLocalInfoTlv->s8InfoOampduCfg[1]);

				pstLocalInfoTlv->s8LocalDTEOUI[0]	= gstLocalEOamCtl.s8LocalDTEOUI[0];
				pstLocalInfoTlv->s8LocalDTEOUI[1]	= gstLocalEOamCtl.s8LocalDTEOUI[1];
				pstLocalInfoTlv->s8LocalDTEOUI[2]	= gstLocalEOamCtl.s8LocalDTEOUI[2];

				pstLocalInfoTlv->stInfoTlvVenderInfo.s16VendorDTEType 	= 0;
				pstLocalInfoTlv->stInfoTlvVenderInfo.s16VendorSWRev 		= 0x01;

				memcpy((stLOCAL_INFO_TLV *)&gstLocalEOamCtl.stLocalInfoTlv,pstLocalInfoTlv,sizeof(stLOCAL_INFO_TLV));

				//DumpTLV_Infomation((char *)&gstLocalEOamCtl.stLocalInfoTlv,CLR1_37_WHITE);
		}

		/* Remote Information TLV */
		if((s8TlvType&BIT01) == INFO_TLV_REMOTE_INFO)
		{
				EOAM_MSG(dprintf("+ Remote Info TLV"));

				pstLocalInfoRMTTlv = (stLOCAL_INFO_TLV *)(BUF_BUFFER_PTR(ps8PktBuff)+ETHER_HDR_LEN+SLOW_HDR_LEN+INFO_TLV_LEN);

				memcpy(pstLocalInfoRMTTlv,(stLOCAL_INFO_TLV *)&gstLocalEOamCtl.stRMTLocalInfoTlv,sizeof(stLOCAL_INFO_TLV));
				pstLocalInfoRMTTlv->s8InfoType = INFO_TLV_REMOTE_INFO;
		}

		/* Organization Specific Information TLV */
		if((s8TlvType&BIT01) == INFO_TLV_SPEC_INFO)
		{
				EOAM_MSG(dprintf("+ Spec Info TLV",__FUNCTION__));
		}

		EOAM_MSG(dprintf("\n"));

		DumpTLV_Infomation(BUF_BUFFER_PTR(ps8PktBuff)+ETHER_HDR_LEN+SLOW_HDR_LEN,CLR1_37_WHITE);

		return ps8PktBuff;
}

/*****************************************************************
*
*   PROCEDURE NAME:
*   EOAMTLV_LoopBackCtl_Request
*
*   DESCRIPTION:
*	Generate Loopback CMD TLV.
*
*   NOTE:
*
*****************************************************************/
#ifdef SUPERTASK
static char* EOAMTLV_LoopBackCtl_Request(char s8EnableLB, char *ps8PktBuff)
#else
static char* EOAMTLV_LoopBackCtl_Request(char s8EnableLB, struct sk_buff *ps8PktBuff)
#endif // SUPERTASK
{
		char *pstLoopBackCMDTlv;

		EOAM_MSG(dprintf("[%s]s8EnableLB=%d\n",__FUNCTION__,s8EnableLB));

		if(ps8PktBuff == NULL)
		{
				if((ps8PktBuff=GetBuffer(BUFSZ1))==NULL)
				{
						dprintf("%s[%s]Get Buffer[%d] Fail!\n%s",CLR1_31_RED,__FUNCTION__,BUFSZ1,CLR0_RESET);
						return NULL;
				}

			//dprintf("%s[%s]got buffer id(%d),sz(%d),addr(%p)\n%s",CLR1_31_RED,__FUNCTION__,B1_ID,BUFSZ1,ps8PktBuff,CLR0_RESET);
			//show_buffer_count();
		}

		SetBufferLength(ps8PktBuff, ETHER_FRAME_MIN_LEN);
		memset(BUF_BUFFER_PTR(ps8PktBuff),0,ETHER_FRAME_MIN_LEN);
		//dprintf("GetBufferLength=%d\n",GetBufferLength(ps8PktBuff));

		pstLoopBackCMDTlv = (char *)(BUF_BUFFER_PTR(ps8PktBuff)+ETHER_HDR_LEN+SLOW_HDR_LEN);

		*(pstLoopBackCMDTlv+0) = s8EnableLB;

		return ps8PktBuff;

}

/*****************************************************************
*
*   PROCEDURE NAME:
*		EOAMPDU Request Command
*
*   DESCRIPTION:
*		Generated by the OAM client entity whenever an OAMPDU is to be
*		transferred to a peer entity.
*
*	NOTE:
*
*****************************************************************/
#ifdef SUPERTASK
static char* EOAMPDU_Request (char s8SubType, short s16Flags, char s8Code, char *ps8PktBuff)
#else
static char* EOAMPDU_Request (char s8SubType, short s16Flags, char s8Code, struct sk_buff *ps8PktBuff)
#endif // SUPERTASK
{
		stSLOW_PTL_HDR *pstSlowHdr;

		EOAM_MSG(dprintf("[%s]s16Flags=%X, s8Code=%X\n",__FUNCTION__,s16Flags,s8Code));

		if(ps8PktBuff == NULL)
		{
				if((ps8PktBuff=GetBuffer(BUFSZ1))==NULL)
				{
						EOAM_MSG(dprintf("%s[%s]Get Buffer[%d] Fail!\n%s",CLR1_31_RED,__FUNCTION__,BUFSZ1,CLR0_RESET));
						return NULL;
				}

				//dprintf("%s[%s]got buffer id(%d),sz(%d),addr(%p)\n%s",CLR1_31_RED,__FUNCTION__,B1_ID,BUFSZ1,ps8PktBuff,CLR0_RESET);
			//show_buffer_count();

			SetBufferLength(ps8PktBuff, ETHER_FRAME_MIN_LEN);
			memset(BUF_BUFFER_PTR(ps8PktBuff),0,ETHER_FRAME_MIN_LEN);
		}

		//EOAM_MSG(dprintf("GetBufferLength=%d\n",GetBufferLength(ps8PktBuff)));

		/* Setting Parameters */
		pstSlowHdr = (stSLOW_PTL_HDR *)(BUF_BUFFER_PTR(ps8PktBuff)+ETHER_HDR_LEN);

		pstSlowHdr->s8SubType 	= s8SubType;

		pstSlowHdr->s8Flags[0]	= (s16Flags&0xFF00);
		pstSlowHdr->s8Flags[1]	= (s16Flags&0x007F);

		if((pstSlowHdr->s8Code=s8Code) == CODE_INFO)
		{
			//if(*(BUF_BUFFER_PTR(ps8PktBuff)+ETHER_HDR_LEN+SLOW_HDR_LEN+1) != INFO_TLV_LOCAL_INFO)
				//return NULL;
		}
		else if((pstSlowHdr->s8Code=s8Code) == CODE_EVENT)
		{}
		else if((pstSlowHdr->s8Code=s8Code) == CODE_VARREQ)
		{}
		else if((pstSlowHdr->s8Code=s8Code) == CODE_VARRESP)
		{}
		else if((pstSlowHdr->s8Code=s8Code) == CODE_LOOPBACK_CTL)
		{}
		else if((pstSlowHdr->s8Code=s8Code) == CODE_ORGSPEC)
		{}
		else
		{
				return NULL;
		}

		return ps8PktBuff;
}

/*****************************************************************
*
*   PROCEDURE NAME:
*		EOAMI Request Command
*
*   DESCRIPTION:
*		This primitive is generated by the Parser function whenever a
*		frame is intended to be looped back to the remote DTE via the
*		Multiplexer function.
*
*		This primitive is generated by the Control function whenever
*		an OAMPDU is to be conveyed to the peer OAM entity via the
*		Multiplexer function, internal to the OAM sublayer.
*
*   NOTE:
*
*****************************************************************/
#ifdef SUPERTASK
static char* EOAMI_Request(char *ps8DestAddr, char *ps8SrcAddr, int s32FramChkSeq, char *ps8PktBuff)
#else
static char* EOAMI_Request(char *ps8DestAddr, char *ps8SrcAddr, int s32FramChkSeq, struct sk_buff *ps8PktBuff)
#endif // SUPERTASK
{
		stETHER_PTL_HDR *pstEtherHdr;

		EOAM_MSG(dprintf("[%s]ps8DestAddr=%02X:%02X:%02X:%02X:%02X:%02X, ",__FUNCTION__,\
				ps8DestAddr[0],ps8DestAddr[1],ps8DestAddr[2],ps8DestAddr[3],ps8DestAddr[4],ps8DestAddr[5]));

		EOAM_MSG(dprintf("ps8SrcAddr=%02X:%02X:%02X:%02X:%02X:%02X\n",\
				ps8SrcAddr[0],ps8SrcAddr[1],ps8SrcAddr[2],ps8SrcAddr[3],ps8SrcAddr[4],ps8SrcAddr[5]));


		if(ps8PktBuff == NULL)
		{
				if((ps8PktBuff=GetBuffer(BUFSZ1))==NULL)
				{
						EOAM_MSG(dprintf("%s[%s]Get Buffer[%d] Fail!\n%s",CLR1_31_RED,__FUNCTION__,BUFSZ1,CLR0_RESET));
						return NULL;
				}

				//dprintf("%s[%s]got buffer id(%d),sz(%d),addr(%p)\n%s",CLR1_31_RED,__FUNCTION__,B1_ID,BUFSZ1,ps8PktBuff,CLR0_RESET);
			//show_buffer_count();

			SetBufferLength(ps8PktBuff, ETHER_FRAME_MIN_LEN);
			memset(BUF_BUFFER_PTR(ps8PktBuff),0,ETHER_FRAME_MIN_LEN);
		}

		//EOAM_MSG(dprintf("GetBufferLength=%d\n",GetBufferLength(ps8PktBuff)));

		/* Setting Parameters */
		pstEtherHdr = (stETHER_PTL_HDR *)BUF_BUFFER_PTR(ps8PktBuff);

		memcpy(pstEtherHdr->s8DestAddr, ps8DestAddr, ETHER_MAC_LEN);
		memcpy(pstEtherHdr->s8SrcAddr, ps8SrcAddr, ETHER_MAC_LEN);
		pstEtherHdr->s16LengType = SLOW_PTL_TYPE;/*s16LengType;*/

		return ps8PktBuff;
}

/*****************************************************************
*
*   PROCEDURE NAME:
*   EOAM_PARSER
*
*   DESCRIPTION:
*
*   NOTE:
*
*****************************************************************/
#ifdef SUPERTASK
int EOAM_PARSE(int s32Ifno, char *ps8PktBuff, int s32BuffLen)
#else
int EOAM_PARSE(int s32Ifno, struct sk_buff *ps8PktBuff, int s32BuffLen)
#endif // SUPERTASK
{
		stETHER_PTL_HDR *pstEtherHdr;
		stSLOW_PTL_HDR *pstSlowHdr;

		EOAM_MSG(dprintf("%s[%s]s32Ifno=%d,s32BuffLen=%ld(%ld),  %s",CLR1_33_YELLOW,__FUNCTION__,s32Ifno,s32BuffLen,GetBufferLength(ps8PktBuff),CLR0_RESET));

		pstEtherHdr = (stETHER_PTL_HDR *)BUF_BUFFER_PTR(ps8PktBuff);
		pstSlowHdr = (stSLOW_PTL_HDR *)(BUF_BUFFER_PTR(ps8PktBuff)+ETHER_HDR_LEN);

		/*
		if(gs8LocalEoamEnable == TRUE)
		{
				if(TEL_MGR_DoesAnyEmergencyCallExist() == 0) // deny, there is emergency call
				{
						EOAM_MSG(dprintf("Emergency call, reset!\n"));

						if(klltsk(cur_task)<0)
						{
								EOAM_MSG(dprintf("%s[%s]Kill EOAM_DISCOVERY_TASK failed!\n%s",CLR1_31_RED,__FUNCTION__,CLR0_RESET));
						}
						else
						{
								EOAM_MSG(dprintf("%s[%s]Kill EOAM_DISCOVERY_TASK slot=%d\n%s",CLR1_32_GREEN,__FUNCTION__,cur_task,CLR0_RESET));
						}

						gs8LocalEoamEnable = FALSE;
						gs8LocalEoamBegin  = TRUE;
						gs8LocalEoamMode   = EOAM_LAYER_DISABLE;

						gs8DiscTimes = 0;

						#ifdef _WAN_CLONE
						if(is_wan_uplink_enabled())
						{
								if_table[T_WAN_CLONE_INT].flag &= ~IFF_EOAM_UP;
						}
						else
						#endif
						{
								if_table[T_WAN_INT].flag &= ~IFF_EOAM_UP;
						}

						EOAM_MSG(dprintf("[%s]EOAM ENABLE=%d,BEGIN=%d,MODE=%d\n",__FUNCTION__,gs8LocalEoamEnable,gs8LocalEoamBegin,gs8LocalEoamMode));
						setDTLog("EOAM-state leaving", NULL, NULL);
						VoIPServiceRestart();

						return EOAM_ERROR_LOOPBACK_TERMINATE;
				}
		}
		*/

		/* Parser passes received non-OAMPDUs to superior sublayer. */
		if(gstLocalEOamCtl.s8LocalParAction == LOCAL_PARSER_FWD)
		{
				EOAM_MSG(dprintf("Parser passes received non-OAMPDUs to superior sublayer.\n"));
				//EOAM_MSG(dprintf("%s[%s]got packet,len=%d,s8LocalParAction=%d\n%s",CLR1_35_MAGENTA,__FUNCTION__,s32BuffLen,gstLocalEOamCtl.s8LocalParAction,CLR0_RESET));
				return EOAM_SUCCESS;
		}
		/* Parser discards received non-OAMPDUs. */
		else if(gstLocalEOamCtl.s8LocalParAction == LOCAL_PARSER_DISCARD)
		{
				if(gs8LocalEoamMode == EOAM_LAYER_LOOPBACK_DTE)
				{
						EOAM_MSG(dprintf("Parser pass received packets.\n"));
						return EOAM_SUCCESS;
				}
				else
				{
						/* Discards received non-OAMPDUs. */
						EOAM_MSG(dprintf("Parser discards received packets.\n"));
						//EOAM_MSG(dprintf("Discards received non-OAMPDUs,s8LocalParAction=%d,type=%02X ....\n",gstLocalEOamCtl.s8LocalParAction,pstEtherHdr->s16LengType));
						return EOAM_ERROR_DISCARD_PACKET;
				}
		}
		/* Parser passes received non-OAMPDUs to Multiplexer during remote loopback test. */
		else if(gstLocalEOamCtl.s8LocalParAction == LOCAL_PARSER_LB)
		{
				if(gs8LocalEoamMode == EOAM_LAYER_LOOPBACK_DTE)
				{
						#if 0
						// if emergency call,
						if(gs32VoIPServiceStopUse == 1)
						{
								VoIPServiceRestart();
								gs32VoIPServiceStopUse = 0;

								if(TEL_MGR_DoesAnyEmergencyCallExist() == 0) // deny, there is emergency call
								{
										dprintf("Emergency call, reset!\n");
										if(gstTempLocalEOamCtl.s8LocalEOamMode == EOAM_ACTIVE_MODE)
										{
												EOAM_INIT(EOAM_LAYER_ACTIVE_DTE);
												gs8LocalEoamMode = EOAM_LAYER_ACTIVE_DTE;
										}
										else
										{
												EOAM_INIT(EOAM_LAYER_PASSIVE_DTE);
												gs8LocalEoamMode = EOAM_LAYER_PASSIVE_DTE;
										}

										return EOAM_ERROR_LOOPBACK_TERMINATE;
								}
								else
								{
										VoIPServiceStop();
										gs32VoIPServiceStopUse = 1;
								}
						}
						#endif

						#if 0
						if(TEL_MGR_DoesAnyEmergencyCallExist() == 0) // deny, there is emergency call
						{
								EOAM_MSG(dprintf("Emergency call, reset!\n"));
								if(gstTempLocalEOamCtl.s8LocalEOamMode == EOAM_ACTIVE_MODE)
								{
										EOAM_INIT(EOAM_LAYER_ACTIVE_DTE);
										gs8LocalEoamMode = EOAM_LAYER_ACTIVE_DTE;
								}
								else
								{
										EOAM_INIT(EOAM_LAYER_PASSIVE_DTE);
										gs8LocalEoamMode = EOAM_LAYER_PASSIVE_DTE;
								}

								return EOAM_ERROR_LOOPBACK_TERMINATE;
						}
						#endif


#ifdef SUPERTASK
						if(s32Ifno == T_LAN_INT)
						{
								EOAM_MSG(dprintf("Parser discards packets from other ifno(%d) in LoopBack.\n", s32Ifno));
								return EOAM_ERROR_DISCARD_PACKET;
						}
#endif // SUPERTASK

						//EOAM_MSG(dprintf("\n"));
						//DumpBuffer_RAW(BUF_BUFFER_PTR(ps8PktBuff), 32/*s32BuffLen*/, CLR1_37_WHITE);

						/* LoopBack Test */
						if(gstLocalEOamCtl.s8LocalEOamLBInited == EOAM_LB_START)
						{
								if((pstEtherHdr->s16LengType == (short)ET_SLOW_PTL) && (pstSlowHdr->s8SubType == SUBTYPE_EOAM))
								{
										EOAM_MSG(dprintf("Parser pass received OAMPDUs to the remote EOAM client in LoopBack.\n"));
										//EOAM_MSG(dprintf("%s[%s]got oam packet, len=%d,s8LocalParAction=%d\n%s",CLR1_35_MAGENTA,__FUNCTION__,s32BuffLen,gstLocalEOamCtl.s8LocalParAction,CLR0_RESET));
										//DumpBuffer_EOAM(BUF_BUFFER_PTR(buf), len, CLR1_37_WHITE);

										return EOAM_SUCCESS;
								}
								else
								{
										/* LoopBack received non-OAMPDUs. */
										EOAM_MSG(dprintf("Parser loopback received non-OAMPDUs in (%d) mode\n",gs8LocalEoamMode));
										//gs8localEoamLoopbackITR = 1;
										//EOAM_MSG(dprintf("gs8localEoamLoopbackITR=%d\n",gs8localEoamLoopbackITR));

										EOAM_LOOPBACK_TASK(s32Ifno,ps8PktBuff,s32BuffLen);

										return EOAM_ERROR_LOOPBACK_PACKET;
								}
						}
						else
						{
								EOAM_MSG(dprintf("Parser pass received packets.\n"));
								return EOAM_SUCCESS;
						}
				}
				else
				{
						/* LoopBack received non-OAMPDUs. */
						EOAM_MSG(dprintf("Parser loopback received non-OAMPDUs in wrong mode (%d)\n",gs8LocalEoamMode));


						return EOAM_ERROR_LOOPBACK_PACKET;
				}

		}

		return EOAM_SUCCESS;
}

/*****************************************************************
*
*   PROCEDURE NAME:
*   EOAM_HANDLER
*
*   DESCRIPTION:
*
*   NOTE:
*
*****************************************************************/
#ifdef SUPERTASK
int EOAM_HANDLER(int s32Ifno, char *ps8PktBuff, int s32BuffLen)
#else
int EOAM_HANDLER(struct sk_buff *ps8PktBuff, int s32BuffLen)
#endif // SUPERTASK
{
	stETHER_PTL_HDR *pstEtherPtlHdr;
	stSLOW_PTL_HDR *pstSlowPtlHdr;
	char *ps8TlvBuff;

	short s16Flags;

	short s16TlvOffset=0;
	short s16SeqNumber;

	stLOCAL_INFO_TLV *pstRMTInfoTlv, *pstRMTRmtInfoTlv;

	char i;

#ifdef SUPERTASK
	EOAM_MSG(dprintf("[EOAM_HANDLER] s32Ifno: %d\n", s32Ifno));
#else
	EOAM_MSG(dprintf("[EOAM_HANDLER] gEOAM_Ifname=%s\n",gEOAM_Ifname));
#endif
	if(gs8LocalEoamEnable == false)
	{
		EOAM_MSG(dprintf("[EOAM_HANDLER] EOAM function disable\n"));
		return EOAM_FAIL;
	}

#ifdef SUPERTASK
#ifdef _WAN_CLONE
	if(is_wan_uplink_enabled())
	{
		if( (s32Ifno != gEOAM_Ifno) && (s32Ifno != T_WAN_THS_INT) )
		{
			EOAM_MSG(dprintf("[EOAM_HANDLER] s32Ifno(%d) != gEOAM_Ifno(%d) and s32Ifno(%d) != T_WAN_THS_INT(%d), discard\n", s32Ifno, gEOAM_Ifno, s32Ifno, T_WAN_THS_INT));
			return EOAM_FAIL;
		}
	}
	else
#endif
	{
		if(s32Ifno != gEOAM_Ifno)
		{
			EOAM_MSG(dprintf("[EOAM_HANDLER] s32Ifno(%d) != gEOAM_Ifno(%d), discard\n", s32Ifno, gEOAM_Ifno));
			return EOAM_FAIL;
		}
	}
#endif


#ifdef SUPERTASK
	EOAM_MSG(dprintf("%s[%s]s32Ifno=%d,s32BuffLen=%ld(%ld),%s",CLR1_33_YELLOW,__FUNCTION__,s32Ifno,s32BuffLen,GetBufferLength(ps8PktBuff),CLR0_RESET));
	EOAM_MSG(dprintf("%sMAC=%02X:%02X:%02X:%02X:%02X:%02X\n%s",\
						 CLR1_33_YELLOW,if_table[s32Ifno].macaddr[0],if_table[s32Ifno].macaddr[1],if_table[s32Ifno].macaddr[2],\
						 if_table[s32Ifno].macaddr[3],if_table[s32Ifno].macaddr[4],if_table[s32Ifno].macaddr[5],CLR0_RESET));
#else
	EOAM_MSG(dprintf("%s [%s] s32BuffLen=%ld(%ld),%s\n",CLR1_33_YELLOW,__FUNCTION__,s32BuffLen,GetBufferLength(ps8PktBuff),CLR0_RESET));
	EOAM_MSG(dprintf("%s [EOAM_HANDLER] MAC=%02X:%02X:%02X:%02X:%02X:%02X\n%s\n",\
						 CLR1_33_YELLOW, ps8PktBuff->dev->dev_addr[0],ps8PktBuff->dev->dev_addr[1],ps8PktBuff->dev->dev_addr[2],\
						 ps8PktBuff->dev->dev_addr[3],ps8PktBuff->dev->dev_addr[4],ps8PktBuff->dev->dev_addr[5],CLR0_RESET));
#endif // SUPERTASK


	if((ps8PktBuff == NULL) || (s32BuffLen<0))
	{
		EOAM_MSG(dprintf("[EOAM_HANDLER] Got ps8PktBuff error ....s32BuffLen=%d\n",s32BuffLen));
		//DumpBuffer_RAW(BUF_BUFFER_PTR(ps8PktBuff), s32BuffLen);
		return EOAM_ERROR_GET_BUFFER;
	}

	//if(gs8LocalEoamMode == EOAM_LAYER_LOOPBACK_DTE)
	//{
				//DumpBuffer_RAW(BUF_BUFFER_PTR(ps8PktBuff),s32BuffLen,CLR1_37_WHITE);
	//}

	/* 2) Passes the received OAMPDU to the OAM client */
	pstEtherPtlHdr = (stETHER_PTL_HDR *)BUF_BUFFER_PTR(ps8PktBuff);
	pstSlowPtlHdr = (stSLOW_PTL_HDR *)(BUF_BUFFER_PTR(ps8PktBuff)+ETHER_HDR_LEN);
	ps8TlvBuff = (char *)(BUF_BUFFER_PTR(ps8PktBuff)+ETHER_HDR_LEN+SLOW_HDR_LEN);

	/* 2-1) Check EOAMPDU field */
	if(memcmp(pstEtherPtlHdr->s8DestAddr, (char*)&SLOW_PTL_MULTIMAC[0], ETHER_MAC_LEN) != 0)
	{
		EOAM_MSG(dprintf("[EOAM_HANDLER] Wrong MAC address, %02X:%02X:%02X:%02X:%02X:%02X\n",\
						pstEtherPtlHdr->s8DestAddr[0],pstEtherPtlHdr->s8DestAddr[1],pstEtherPtlHdr->s8DestAddr[2],
						pstEtherPtlHdr->s8DestAddr[3],pstEtherPtlHdr->s8DestAddr[4],pstEtherPtlHdr->s8DestAddr[5]));

		DumpBuffer_RAW(BUF_BUFFER_PTR(ps8PktBuff),s32BuffLen,CLR1_37_WHITE);

		return EOAM_ERROR_MACADDR;
	}

	// SUBTYPE_EOAM, Operations, Administration, and Maintenance (OAM)
	if(pstSlowPtlHdr->s8SubType != SUBTYPE_EOAM)
	{
		EOAM_MSG(dprintf("[EOAM_HANDLER] Wrong Subtype=%d\n",pstSlowPtlHdr->s8SubType));
		return EOAM_ERROR_SUBTYPE;
	}

	/* Check packets from the same remote DTE */
	if((gs8LocalLostLnkTimeDone == 1) || (gs8LoopBackSettingTimeDone == 1))
	{
		if(memcmp(pstEtherPtlHdr->s8SrcAddr,(char*)&gstLocalEOamCtl.s8RMTDTEAddr[0], ETHER_MAC_LEN) != 0)
		{
			dprintf("[EOAM_HANDLER] Different DTE MAC address, src=%02X:%02X:%02X:%02X:%02X:%02X, ",\
								pstEtherPtlHdr->s8SrcAddr[0],pstEtherPtlHdr->s8SrcAddr[1],pstEtherPtlHdr->s8SrcAddr[2],
								pstEtherPtlHdr->s8SrcAddr[3],pstEtherPtlHdr->s8SrcAddr[4],pstEtherPtlHdr->s8SrcAddr[5]);

			dprintf("[EOAM_HANDLER] RMTDTEAddr=%02X:%02X:%02X:%02X:%02X:%02X\n",\
								gstLocalEOamCtl.s8RMTDTEAddr[0],gstLocalEOamCtl.s8RMTDTEAddr[1],gstLocalEOamCtl.s8RMTDTEAddr[2],
								gstLocalEOamCtl.s8RMTDTEAddr[3],gstLocalEOamCtl.s8RMTDTEAddr[4],gstLocalEOamCtl.s8RMTDTEAddr[5]);

			return EOAM_ERROR_MACADDR;
		}
	}

#if 0
		/* Update Vendor OUI */
#ifdef SUPERTASK
	gstLocalEOamCtl.s8LocalDTEOUI[0]			= if_table[s32Ifno].macaddr[0];
	gstLocalEOamCtl.s8LocalDTEOUI[1]			= if_table[s32Ifno].macaddr[1];
	gstLocalEOamCtl.s8LocalDTEOUI[2]			= if_table[s32Ifno].macaddr[2];
#else
	gstLocalEOamCtl.s8LocalDTEOUI[0]			= ps8PktBuff->dev->dev_addr[0];
	gstLocalEOamCtl.s8LocalDTEOUI[1]			= ps8PktBuff->dev->dev_addr[1];
	gstLocalEOamCtl.s8LocalDTEOUI[2]			= ps8PktBuff->dev->dev_addr[2];
#endif // SUPERTASK
#endif

	/* No LoopBack Function if Discovery process isn't finished! */
	if(gstLocalEOamCtl.s8RemoteDTEDiscovered == 0)
	{
		if(pstSlowPtlHdr->s8Code == CODE_LOOPBACK_CTL)
		{
			EOAM_MSG(dprintf("[EOAM_HANDLER] No LoopBack Function if Discovery process isn't finished!\n"));
			return EOAM_ERROR_IGNORED_PACKET;
		}
	}

	gstLocalEOamCtl.s16RMTFlags = ((pstSlowPtlHdr->s8Flags[0]<<8) | pstSlowPtlHdr->s8Flags[1]);
	//EOAM_MSG(dprintf("s16RMTFlags=0x%04x\n",gstLocalEOamCtl.s16RMTFlags));

	/*
	gstLocalEOamCtl.s8LocalLinkStatus duo to local device's phyical layer error,
	also gstLocalEOamCtl.s8LocalDyingGasp is caused from local unrecoverable failure,
	gstLocalEOamCtl.s8LocalCriticalEvent is meaned local DTE event, too.
	SHOULD not get from remote DTE!
	*/
	#if 0
	if(gstLocalEOamCtl.s16RMTFlags&EOAMPDU_FLAGS_LINKFAULT == EOAMPDU_FLAGS_LINKFAULT)
	{
		gstLocalEOamCtl.s8LocalLinkStatus = 1;
	}
	else
	{
		gstLocalEOamCtl.s8LocalLinkStatus = 0;
	}


	if(gstLocalEOamCtl.s16RMTFlags&EOAMPDU_FLAGS_DYINGASP == EOAMPDU_FLAGS_DYINGASP)
	{
		gstLocalEOamCtl.s8LocalDyingGasp = 1;
	}


	if(gstLocalEOamCtl.s16RMTFlags&EOAMPDU_FLAGS_CRITICEVT == EOAMPDU_FLAGS_CRITICEVT)
	{
		gstLocalEOamCtl.s8LocalCriticalEvent = 1;
	}
	#endif

	/*
	Check local status of remote side in receive packet.
	If remote sdie completes discovery function, we set remote side to stable status.
	*/
	if((((gstLocalEOamCtl.s16RMTFlags & EOAMPSU_FLAGS_LOCALSTATUS) >> 0x3) & 0x3) == LOCAL_DTE_DISC_COMPLETED)
	{
		gstLocalEOamCtl.s8RemoteStable = 1;
		//EOAM_MSG(dprintf("s8RemoteStable=%d\n",gstLocalEOamCtl.s8RemoteStable));
	}
	else if(((((gstLocalEOamCtl.s16RMTFlags & EOAMPSU_FLAGS_LOCALSTATUS) >> 0x3) & 0x3) == LOCAL_DTE_UNSATISFIED) ||\
			((((gstLocalEOamCtl.s16RMTFlags & EOAMPSU_FLAGS_LOCALSTATUS) >> 0x3) & 0x3) == LOCAL_DTE_DISC_UNCOMPLETED))
	{
		gstLocalEOamCtl.s8RemoteStable = 0;
		//EOAM_MSG(dprintf("s8RemoteStable=%d\n",gstLocalEOamCtl.s8RemoteStable));
	}
	else
	{
		/*
			This value shall not be sent. If the value 0x3 is received,
			it should be ignored and not change the last received value.
		*/
		if((((gstLocalEOamCtl.s16RMTFlags & EOAMPSU_FLAGS_LOCALSTATUS) >> 0x3) & 0x3) == LOCAL_DTE_RFU)
		{
			EOAM_MSG(dprintf("[%s]RFU EOAMPDU Flags=%02d!\n",__FUNCTION__,gstLocalEOamCtl.s16RMTFlags));
		}
		else
		{
			EOAM_MSG(dprintf("[%s]Wrong EOAMPDU Flags=%02d!\n",__FUNCTION__,gstLocalEOamCtl.s16RMTFlags));
			return EOAM_FAIL;
		}
	}

		/* 2-2) Process OAMPSUs and Updated parameter */
	while(1)
	{
		switch(pstSlowPtlHdr->s8Code)
		{
			case CODE_INFO:
				EOAM_MSG(dprintf("Information OAMPDU, Codes=%d, s16TlvOffset=%d, line %d\n",pstSlowPtlHdr->s8Code,s16TlvOffset, __LINE__));
				pstRMTInfoTlv = (stLOCAL_INFO_TLV *)(BUF_BUFFER_PTR(ps8PktBuff)+ETHER_HDR_LEN+SLOW_HDR_LEN+s16TlvOffset);
				pstRMTRmtInfoTlv = (stLOCAL_INFO_TLV *)(BUF_BUFFER_PTR(ps8PktBuff)+ETHER_HDR_LEN+SLOW_HDR_LEN+INFO_TLV_LEN);

				/* 2-2-1) Check Remote Info Tlv */
				if(s16TlvOffset<INFO_TLV_LEN)
				{
					if(pstRMTInfoTlv->s8InfoType == INFO_TLV_LOCAL_INFO)
					{
						if(pstRMTInfoTlv->s8InfoLeng != INFO_TLV_LEN)
						{
							gstLocalEOamCtl.s8RemoteStateValid = 0;

							EOAM_MSG(dprintf("pstRMTInfoTlv->s8InfoLeng!=%02X\n",pstRMTInfoTlv->s8InfoLeng));
							return EOAM_ERROR_INVAILD_PACKET;
						}

						/* LoopBack process */
						if(gs8LocalEoamMode == EOAM_LAYER_LOOPBACK_DTE)
						{
							EOAM_MSG(dprintf("s8LocalEOamLBInited=%d, ",gstLocalEOamCtl.s8LocalEOamLBInited));
							EOAM_MSG(dprintf("s8RMTParseAction=%d, ",pstRMTInfoTlv->s8InfoState&INFO_TLV_STATE_PARSER));
							EOAM_MSG(dprintf("s8RMTMuxAction=%d\n",pstRMTInfoTlv->s8InfoState&INFO_TLV_STATE_MUX));

							if(gstLocalEOamCtl.s8LocalEOamLBInited == EOAM_LB_START)
							{
								if(gstLocalEOamCtl.s8LocalEOamMode == EOAM_PASSIVE_MODE)
								{
									if(((pstRMTInfoTlv->s8InfoState&INFO_TLV_STATE_PARSER) == LOCAL_PARSER_DISCARD) &&\
										 ((pstRMTInfoTlv->s8InfoState&INFO_TLV_STATE_MUX) == LOCAL_MUX_FWD) &&\
										 (gstLocalEOamCtl.s8LocalParAction == REMOTE_PARSER_LB) &&\
										 (gstLocalEOamCtl.s8LocalMuxAction == REMOTE_MUX_DISCARD))

									{
										EOAM_MSG(dprintf("step 8) Remote DTE in Passive mode, check OAMPDUs keep loopback alive!\n"));
									}
									else
									{
										EOAM_MSG(dprintf("step 8) Remote DTE in Passive mode, no more correctly OAMPDUs, reset!\n"));
										if(gstTempLocalEOamCtl.s8LocalEOamMode == EOAM_ACTIVE_MODE)
										{
											EOAM_INIT(EOAM_LAYER_ACTIVE_DTE);
											gs8LocalEoamMode = EOAM_LAYER_ACTIVE_DTE;
										}
										else
										{
											EOAM_INIT(EOAM_LAYER_PASSIVE_DTE);
											gs8LocalEoamMode = EOAM_LAYER_PASSIVE_DTE;
										}

										return EOAM_ERROR_LOOPBACK_TERMINATE;
									}
								}
								else if(gstLocalEOamCtl.s8LocalEOamMode == EOAM_ACTIVE_MODE)
								{
									if(((pstRMTInfoTlv->s8InfoState&INFO_TLV_STATE_PARSER) == REMOTE_PARSER_LB) &&\
										((pstRMTInfoTlv->s8InfoState&INFO_TLV_STATE_MUX) == INFO_TLV_STATE_MUX/*REMOTE_MUX_DISCARD*/) &&\
										 (gstLocalEOamCtl.s8LocalParAction == LOCAL_PARSER_DISCARD) &&\
										 (gstLocalEOamCtl.s8LocalMuxAction == LOCAL_MUX_FWD))
									{
										EOAM_MSG(dprintf("setp 7) Local DTE in Active mode, check OAMPDUs keep loopback alive!\n"));
									}
									else
									{
										EOAM_MSG(dprintf("setp 7) Local DTE in Active mode, no more correctly OAMPDUs, reset!\n"));
										EOAM_INIT(EOAM_LAYER_ACTIVE_DTE);
										gs8LocalEoamMode = EOAM_LAYER_ACTIVE_DTE;

										return EOAM_ERROR_LOOPBACK_TERMINATE;
									}
								}

								gs8LocalLostLnkTimeDone = 0;
							}

							if(gstLocalEOamCtl.s8LocalEOamLBInited == EOAM_LB_START_INPROGRESS)
							{
								if(gstLocalEOamCtl.s8LocalEOamMode == EOAM_PASSIVE_MODE)
								{
									if(((pstRMTInfoTlv->s8InfoState&INFO_TLV_STATE_PARSER) == LOCAL_PARSER_DISCARD) &&\
										((pstRMTInfoTlv->s8InfoState&INFO_TLV_STATE_MUX) == LOCAL_MUX_FWD))
									{
										gstLocalEOamCtl.s8LocalEOamLBInited = EOAM_LB_START;
										EOAM_MSG(dprintf("step 6) Remote DTE in Passive mode, start LoopBack mode!\n"));
									}
								}
								else if(gstLocalEOamCtl.s8LocalEOamMode == EOAM_ACTIVE_MODE)
								{
									if(((pstRMTInfoTlv->s8InfoState&INFO_TLV_STATE_PARSER) == REMOTE_PARSER_LB) &&\
										((pstRMTInfoTlv->s8InfoState&INFO_TLV_STATE_MUX) == INFO_TLV_STATE_MUX/*REMOTE_MUX_DISCARD*/))
									{
										gstLocalEOamCtl.s8LocalMuxAction = LOCAL_MUX_FWD;
										gstLocalEOamCtl.s8LocalEOamLBInited = EOAM_LB_START;
										EOAM_MSG(dprintf("setp 5a) Local DTE in Active mode, start LoopBack mode!\n"));
									}
								}
							}

							if(gstLocalEOamCtl.s8LocalEOamLBInited == EOAM_LB_DISABLE)
							{
								if(gstLocalEOamCtl.s8LocalEOamMode == EOAM_PASSIVE_MODE)
								{
									if(((pstRMTInfoTlv->s8InfoState&INFO_TLV_STATE_PARSER) == LOCAL_PARSER_FWD) &&\
										 ((pstRMTInfoTlv->s8InfoState&INFO_TLV_STATE_MUX) == LOCAL_MUX_FWD) &&\
										 (gstLocalEOamCtl.s8LocalParAction == REMOTE_PARSER_FWD) &&\
										 (gstLocalEOamCtl.s8LocalMuxAction == REMOTE_MUX_FWD))
									{
										EOAM_MSG(dprintf("step h) Remote DTE in Passive mode, check OAMPDUs keep loopback left!\n"));

										/* Restore */
										gstLocalEOamCtl = gstTempLocalEOamCtl;
										gs8LocalEoamMode = EOAM_LAYER_PASSIVE_DTE;

										if(gs32VoIPServiceStopUse == 1)
										{
#ifdef SUPERTASK
											VoIPServiceRestart();
#endif
											gs32VoIPServiceStopUse = 0;
										}

										//DumpEOam_CtlInfo();

									}
									else
									{
										EOAM_MSG(dprintf("step h) Remote DTE in Passive mode, no more correctly OAMPDUs, reset!\n"));
										EOAM_MSG(dprintf("s8LocalParAction=%d, ",gstLocalEOamCtl.s8LocalParAction));
										EOAM_MSG(dprintf("s8LocalMuxAction=%d\n",gstLocalEOamCtl.s8LocalMuxAction));
										EOAM_MSG(dprintf("s8RMTParseAction=%d, ",pstRMTInfoTlv->s8InfoState&INFO_TLV_STATE_PARSER));
										EOAM_MSG(dprintf("s8RMTMuxAction=%d\n",pstRMTInfoTlv->s8InfoState&INFO_TLV_STATE_MUX));

										EOAM_INIT(EOAM_LAYER_PASSIVE_DTE);
										gs8LocalEoamMode = EOAM_LAYER_PASSIVE_DTE;

										return EOAM_ERROR_LOOPBACK_TERMINATE;
									}
								}
								else if(gstLocalEOamCtl.s8LocalEOamMode == EOAM_ACTIVE_MODE)
								{
									if(((pstRMTInfoTlv->s8InfoState&INFO_TLV_STATE_PARSER) == REMOTE_PARSER_FWD) &&\
										 ((pstRMTInfoTlv->s8InfoState&INFO_TLV_STATE_MUX) == REMOTE_MUX_FWD) &&\
										 (gstLocalEOamCtl.s8LocalParAction == LOCAL_PARSER_FWD) &&\
										 (gstLocalEOamCtl.s8LocalMuxAction == LOCAL_MUX_FWD))
									{
										EOAM_MSG(dprintf("setp g1) Local DTE in Active mode, check OAMPDUs keep loopback left!\n"));

										/* Restore */
										gstLocalEOamCtl = gstTempLocalEOamCtl;
										gs8LocalEoamMode = EOAM_LAYER_ACTIVE_DTE;

										if(gs32VoIPServiceStopUse == 1)
										{
#ifdef SUPERTASK
												VoIPServiceRestart();
#endif
												gs32VoIPServiceStopUse = 0;
										}

									}
									else
									{
										EOAM_MSG(dprintf("setp g1) Local DTE in Active mode, no more correctly OAMPDUs, reset!\n"));
										EOAM_MSG(dprintf("s8LocalParAction=%d, ",gstLocalEOamCtl.s8LocalParAction));
										EOAM_MSG(dprintf("s8LocalMuxAction=%d\n",gstLocalEOamCtl.s8LocalMuxAction));
										EOAM_MSG(dprintf("s8RMTParseAction=%d, ",pstRMTInfoTlv->s8InfoState&INFO_TLV_STATE_PARSER));
										EOAM_MSG(dprintf("s8RMTMuxAction=%d\n",pstRMTInfoTlv->s8InfoState&INFO_TLV_STATE_MUX));

										EOAM_INIT(EOAM_LAYER_ACTIVE_DTE);
										gs8LocalEoamMode = EOAM_LAYER_ACTIVE_DTE;

										return EOAM_ERROR_LOOPBACK_TERMINATE;
									}
								}

								gs8LocalLostLnkTimeDone = 0;
							}

							if(gstLocalEOamCtl.s8LocalEOamLBInited == EOAM_LB_EXIT_INPROGRESS)
							{
								if(gstLocalEOamCtl.s8LocalEOamMode == EOAM_PASSIVE_MODE)
								{
									if(((pstRMTInfoTlv->s8InfoState&INFO_TLV_STATE_PARSER) == LOCAL_PARSER_FWD) &&\
										 ((pstRMTInfoTlv->s8InfoState&INFO_TLV_STATE_MUX) == LOCAL_MUX_FWD))
									{
										gstLocalEOamCtl.s8LocalEOamLBInited = EOAM_LB_DISABLE;
										EOAM_MSG(dprintf("step f1) Remote DTE in Passive mode, exit LoopBack mode!\n"));
									}
								}
								else if(gstLocalEOamCtl.s8LocalEOamMode == EOAM_ACTIVE_MODE)
								{
									if(((pstRMTInfoTlv->s8InfoState&INFO_TLV_STATE_PARSER) == REMOTE_PARSER_FWD) &&\
										((pstRMTInfoTlv->s8InfoState&INFO_TLV_STATE_MUX) == REMOTE_MUX_FWD))
									{
										gstLocalEOamCtl.s8LocalParAction = LOCAL_PARSER_FWD;
										gstLocalEOamCtl.s8LocalMuxAction = LOCAL_MUX_FWD;

										EOAM_MSG(dprintf("setp e1) Local DTE in Active mode, exit LoopBack mode!\n"));
									}
								}
							}

							memcpy((stLOCAL_INFO_TLV *)&gstLocalEOamCtl.stRMTLocalInfoTlv,pstRMTInfoTlv,sizeof(stLOCAL_INFO_TLV));
							memcpy((stLOCAL_INFO_TLV *)&gstLocalEOamCtl.stRMTRemoteInfoTlv,pstRMTRmtInfoTlv,sizeof(stLOCAL_INFO_TLV));
						}
						else /* Normal mode */
						{
							EOAM_MSG(dprintf("[EOAM_HANDLER] s8RemoteStateValid=%d, line %d\n",gstLocalEOamCtl.s8RemoteStateValid, __LINE__));
							if(gstLocalEOamCtl.s8RemoteStateValid == 1)
							{
								EOAM_MSG(dprintf("[EOAM_HANDLER] stLocalInfoTlv.s8InfoOamCfg=0x%x, line %d\n",gstLocalEOamCtl.stLocalInfoTlv.s8InfoOamCfg, __LINE__));
								EOAM_MSG(dprintf("[EOAM_HANDLER] pstRMTInfoTlv->s8InfoOamCfg=0x%x, line %d\n",pstRMTInfoTlv->s8InfoOamCfg, __LINE__));
#if 0 // bitonic test
								if((gstLocalEOamCtl.stLocalInfoTlv.s8InfoOamCfg&0x1E) ==\
									(pstRMTInfoTlv->s8InfoOamCfg&0x1E))
#else
								// If Fuctions supported by client side also are supported by server side, we can say local satisfied.
								if( ((gstLocalEOamCtl.stLocalInfoTlv.s8InfoOamCfg & 0x1E) & (pstRMTInfoTlv->s8InfoOamCfg & 0x1E))
									== (gstLocalEOamCtl.stLocalInfoTlv.s8InfoOamCfg & 0x1E) )
#endif
								{
									gstLocalEOamCtl.s8LocalSatisfied = 1;
								}
								else
								{
									gstLocalEOamCtl.s8LocalSatisfied = 0;
								}

								EOAM_MSG(dprintf("[EOAM_HANDLER] gstLocalEOamCtl.s8LocalSatisfied=%d, line %d\n",gstLocalEOamCtl.s8LocalSatisfied, __LINE__));
							}

							/*
								if the Discovery state diagram variable remote_state_valid is TRUE,
								the Data field shall also contain the Remote Information TLV
							*/
							if(s16TlvOffset == 0)	// got from first time
							{
								if(gstLocalEOamCtl.s8DiscStatus == EOAM_STATUS_DISC_PASSIVE_WAIT)
								{
									if(pstRMTInfoTlv->s8InfoType == INFO_TLV_LOCAL_INFO)
									{
										gstLocalEOamCtl.s8RemoteStateValid = 1;
										memcpy((stLOCAL_INFO_TLV *)&gstLocalEOamCtl.stRMTLocalInfoTlv,pstRMTInfoTlv,sizeof(stLOCAL_INFO_TLV));
									}
									else
									{
										gstLocalEOamCtl.s8RemoteStateValid = 0;
										memset((stLOCAL_INFO_TLV *)&gstLocalEOamCtl.stRMTLocalInfoTlv,0,sizeof(stLOCAL_INFO_TLV));
									}

									memset((stLOCAL_INFO_TLV *)&gstLocalEOamCtl.stRMTRemoteInfoTlv,0,sizeof(stLOCAL_INFO_TLV));

								}
								else if((gstLocalEOamCtl.s8DiscStatus == EOAM_STATUS_DISC_ACTIVE_SEND_LOCAL) || \
												(gstLocalEOamCtl.s8DiscStatus == EOAM_STATUS_DISC_SEND_LOCAL_REMOTE) || \
												(gstLocalEOamCtl.s8DiscStatus == EOAM_STATUS_DISC_SEND_LOCAL_REMOTE_SUCCESS) || \
												(gstLocalEOamCtl.s8DiscStatus == EOAM_STATUS_DISC_SEND_ANY))
								{
									if((pstRMTRmtInfoTlv->s8InfoType == INFO_TLV_REMOTE_INFO) && (pstRMTInfoTlv->s8InfoType == INFO_TLV_LOCAL_INFO))
									{
										gstLocalEOamCtl.s8RemoteStateValid = 1;
										memcpy((stLOCAL_INFO_TLV *)&gstLocalEOamCtl.stRMTLocalInfoTlv,pstRMTInfoTlv,sizeof(stLOCAL_INFO_TLV));
										memcpy((stLOCAL_INFO_TLV *)&gstLocalEOamCtl.stRMTRemoteInfoTlv,pstRMTRmtInfoTlv,sizeof(stLOCAL_INFO_TLV));
									}
									else
									{
										gstLocalEOamCtl.s8RemoteStateValid = 0;
										memset((stLOCAL_INFO_TLV *)&gstLocalEOamCtl.stRMTLocalInfoTlv,0,sizeof(stLOCAL_INFO_TLV));
										memset((stLOCAL_INFO_TLV *)&gstLocalEOamCtl.stRMTRemoteInfoTlv,0,sizeof(stLOCAL_INFO_TLV));
									}
								}
								else
								{
									gstLocalEOamCtl.s8RemoteStateValid = 0;
								}
							}

							if(gstLocalEOamCtl.s8RemoteStateValid == 1)
							{
								gs8LocalLostLnkTimeDone = 0;
							}

							//EOAM_MSG(dprintf("s8RemoteStateValid=%d\n",gstLocalEOamCtl.s8RemoteStateValid));
							//DumpTLV_Infomation((char *)&gstLocalEOamCtl.stRMTLocalInfoTlv,CLR1_37_WHITE);
						}
					}
					else
					{
						gstLocalEOamCtl.s8RemoteStateValid = 0;

						EOAM_MSG(dprintf("[EOAM_HANDLER] Invaild Information TLV!\n"));
						return EOAM_ERROR_INVAILD_PACKET;
					}
				}
				else
				{
					if(pstRMTInfoTlv->s8InfoType == END_OF_TLV_MARKER)
					{
						/* Reset local_lost_link_timer */
						if(!gs8LocalLostLnkTimeDone)
						{
							gu64LocalLostLnkTimeSnap = GetTime();
							gs8LocalLostLnkTimeDone  = 1; /* Got first packet */
							memcpy((char *)&gstLocalEOamCtl.s8RMTDTEAddr[0],pstEtherPtlHdr->s8SrcAddr,ETHER_MAC_LEN);
						}

						EOAM_MSG(dprintf("[EOAM_HANDLER] No more TLV! s16TlvOffset=%ld,gs8LocalLostLnkTimeDone=%d,gu64LocalLostLnkTimeSnap=%ld, line %d\n",s16TlvOffset,gs8LocalLostLnkTimeDone,gu64LocalLostLnkTimeSnap, __LINE__));

						return EOAM_ERROR_END_TLV;

					}

					//DumpTLV_RAW((char *)pstRMTInfoTlv,CLR1_37_WHITE);
				}

				s16TlvOffset += INFO_TLV_LEN;

				break;

			case CODE_EVENT:
				EOAM_MSG(dprintf("[EOAM_HANDLER] Event Notification OAMPDU, Codes=%d!\n",pstSlowPtlHdr->s8Code));
				return EOAM_FAIL;


				s16TlvOffset += LINK_EVENT_TLV_LEN;
				break;

			case CODE_VARREQ:
				EOAM_MSG(dprintf("[EOAM_HANDLER] Variable Request OAMPDU, Codes=%d!\n",pstSlowPtlHdr->s8Code));
				return EOAM_FAIL;

				s16TlvOffset += VARI_REQ_TLV_LEN;
				break;

			case CODE_VARRESP:
				EOAM_MSG(dprintf("[EOAM_HANDLER] Variable Response OAMPDU, Codes=%d!\n",pstSlowPtlHdr->s8Code));
				return EOAM_FAIL;

				s16TlvOffset += VARI_RESP_TLV_LEN;
				break;

			case CODE_LOOPBACK_CTL:
				EOAM_MSG(dprintf("[EOAM_HANDLER] Loopback Control OAMPDU, Codes=%d, LB Cmd=%d\n", pstSlowPtlHdr->s8Code, *(ps8TlvBuff+0)));
				EOAM_MSG(dprintf("[EOAM_HANDLER] s8LocalEOamLBInited=%d, s8LocalEOamMode=%d\n", gstLocalEOamCtl.s8LocalEOamLBInited, gstLocalEOamCtl.s8LocalEOamMode));

				//DumpTLV_RAW(BUF_BUFFER_PTR(ps8PktBuff)+ETHER_HDR_LEN+SLOW_HDR_LEN,CLR1_37_WHITE);

				if(*(ps8TlvBuff+0) == REMOTE_LB_ENABLE)
				{
					/* Got Loopback Enable CTL, In EOAM_LB_DISABLE(init) */
					if(gstLocalEOamCtl.s8LocalEOamLBInited == EOAM_LB_DISABLE)
					{
						/* step 3) Remote DTE in Passive mode, received LoopBack control command OAMPDUs */
						if(gstLocalEOamCtl.s8LocalEOamMode == EOAM_PASSIVE_MODE)
						{
#ifndef SUPERTASK
							if(0)
#else
							if(TEL_MGR_DoesAnyEmergencyCallExist() == 0) // deny, there is emergency call
#endif
							{
								return EOAM_ERROR_IGNORED_PACKET;
							}
							else
							{
								EOAM_MSG(dprintf("step 3) Remote DTE in Passive mode, received LoopBack ENABLE control command OAMPDU!\n"));
								gstTempLocalEOamCtl = gstLocalEOamCtl;

								gs8LocalEoamMode = EOAM_LAYER_LOOPBACK_DTE;

								gstLocalEOamCtl.s8LocalParAction = REMOTE_PARSER_LB;
								gstLocalEOamCtl.s8LocalMuxAction = REMOTE_MUX_DISCARD;

#ifdef SUPERTASK
								setDTLog("Remote DTE in Passive mode, received LoopBack ENABLE control command OAMPDUs", NULL, NULL);

								VoIPServiceStop();
#endif
								gs32VoIPServiceStopUse = 1;
							}
						}

						if(gstLocalEOamCtl.s8LocalEOamMode == EOAM_ACTIVE_MODE)
						{
							if((gstLocalEOamCtl.stRMTLocalInfoTlv.s8InfoOamCfg&BIT00) == INFO_TLV_CONF_ACTIVE)
							{
								for(i=0;i<ETHER_MAC_LEN;i++)
								{
#ifdef SUPERTASK
									if(gstLocalEOamCtl.s8RMTDTEAddr[i]<if_table[gEOAM_Ifno].macaddr[i]) /* Lower */
#else
									if(gstLocalEOamCtl.s8RMTDTEAddr[i]<ps8PktBuff->dev->dev_addr[i]) /* Lower */
#endif // SUPERTASK
									{
#ifndef SUPERTASK
										if(0)
#else
										if(TEL_MGR_DoesAnyEmergencyCallExist() == 0) // deny, there is emergency call
#endif
										{
											return EOAM_ERROR_IGNORED_PACKET;
										}
										else
										{
											EOAM_MSG(dprintf("step 3) Remote DTE in Active mode, change Active to Passive mode!\n"));
											gstTempLocalEOamCtl = gstLocalEOamCtl;

											gs8LocalEoamMode = EOAM_LAYER_LOOPBACK_DTE;

											gstLocalEOamCtl.s8LocalEOamMode = EOAM_PASSIVE_MODE;

											gstLocalEOamCtl.s8LocalParAction = LOCAL_PARSER_LB;
											gstLocalEOamCtl.s8LocalMuxAction = LOCAL_MUX_DISCARD;

#ifdef SUPERTASK
											setDTLog("Remote DTE in Active mode, change Active to Passive mode!", NULL, NULL);

											VoIPServiceStop();
#endif
											gs32VoIPServiceStopUse = 1;
											break;
										}
									}
									else /* Higher */
									{
										return EOAM_ERROR_IGNORED_PACKET;
									}
								}
							}
						}

						gstLocalEOamCtl.s8LocalEOamLBInited = EOAM_LB_START_INPROGRESS;

						EOAM_MSG(dprintf("[EOAM_HANDLER] s8LocalParAction=%d,s8LocalMuxAction=%d\n",gstLocalEOamCtl.s8LocalParAction,gstLocalEOamCtl.s8LocalMuxAction));
					}
					else
					{
						EOAM_MSG(dprintf("[EOAM_HANDLER] Dismatch LoopBack state! s8LocalEOamLBInited=%d\n",gstLocalEOamCtl.s8LocalEOamLBInited));
					}
				}
				else if(*(ps8TlvBuff+0) == REMOTE_LB_DISABLE)
				{
					/* Got Loopback Disable CTL, In EOAM_LB_DISABLE(init) */
					if(gstLocalEOamCtl.s8LocalEOamLBInited == EOAM_LB_START)
					{
						EOAM_MSG(dprintf("step c) Remote DTE in Passive mode, received LoopBack DISABLE control command OAMPDU!\n"));

						gstLocalEOamCtl.s8LocalParAction = REMOTE_PARSER_FWD;
						gstLocalEOamCtl.s8LocalMuxAction = REMOTE_MUX_FWD;

						gstLocalEOamCtl.s8LocalEOamLBInited = EOAM_LB_EXIT_INPROGRESS;
					}
					else
					{
						EOAM_MSG(dprintf("[EOAM_HANDLER] Dismatch LoopBack state! s8LocalEOamLBInited=%d\n",gstLocalEOamCtl.s8LocalEOamLBInited));
					}
				}
				else
				{
					/* Error Handling */
					EOAM_MSG(dprintf("[EOAM_HANDLER]Wrong LoopBack control OAMPDU!\n"));
					//DumpTLV_LoopBack(ps8TlvBuff, CLR1_37_WHITE);
					return EOAM_ERROR_INVAILD_PACKET;
				}


				/* Reset loopback_setting_timer */
				if(!gs8LoopBackSettingTimeDone)
				{
					gu64LoopbackLostTimeSnap = GetTime();
					gs8LoopBackSettingTimeDone = 1; /* Got first packet */
					memcpy((char *)&gstLocalEOamCtl.s8RMTDTEAddr[0],pstEtherPtlHdr->s8SrcAddr,ETHER_MAC_LEN);
					EOAM_MSG(dprintf("[EOAM_HANDLER] gs8LoopBackSettingTimeDone=%d, gu64LoopbackLostTimeSnap=%ld\n",gs8LoopBackSettingTimeDone,gu64LoopbackLostTimeSnap));
				}

				return EOAM_ERROR_END_TLV;
				break;

			case CODE_RFU:
				EOAM_MSG(dprintf("[EOAM_HANDLER] Reserved, Codes=%d!\n",pstSlowPtlHdr->s8Code));

				/* Should be ignored on reception by OAM client */
				return EOAM_ERROR_IGNORED_PACKET;

				break;

			case CODE_ORGSPEC:
				EOAM_MSG(dprintf("[EOAM_HANDLER] Organization Specific OAMPDU, Codes=%d!\n",pstSlowPtlHdr->s8Code));
				return EOAM_FAIL;

				break;

			default:
				EOAM_MSG(dprintf("[%s]Wrong EOAMPDU Code!\n",__FUNCTION__));
				return EOAM_FAIL;

				break;
		}
	}

	return EOAM_SUCCESS;
}


/*****************************************************************
*
*   PROCEDURE NAME:
*   EOAM_INIT
*
*   DESCRIPTION:
*		EOAM Sublayer Initialization
*
*   NOTE:
*
*****************************************************************/
int EOAM_INIT(char s8EOamMode)
{
		char i;

#ifdef SUPERTASK
		EOAM_MSG(dprintf("%s[%s]Ifno=%d, s8EOamMode=%d\n%s",CLR1_33_YELLOW, __FUNCTION__,T_LAN_INT,s8EOamMode,CLR0_RESET));
#else
		EOAM_MSG(dprintf("%s[%s] s8EOamMode=%d\n%s",CLR1_33_YELLOW, __FUNCTION__,s8EOamMode,CLR0_RESET));
		GetEOAMInterfaceMacAddress();
#endif // SUPERTASK

		/* OAM Event Flags */
		#if 0
		if((if_table[T_LAN_INT].flag & (IFF_MEDIA_UP|IFF_DRV_UP)) == (IFF_MEDIA_UP|IFF_DRV_UP))
			gstLocalEOamCtl.s8LocalLinkStatus	 	= 0;	// driver & media are ready
		else
			gstLocalEOamCtl.s8LocalLinkStatus			= 1;	/* Link fault */
		#endif

		#if 0
		gstLocalEOamCtl.s8LocalLinkStatus	 			= 0;			/* Link fault */
		gstLocalEOamCtl.s8LocalDyingGasp	 			= 0;			/* Unrecoverable failure */
		gstLocalEOamCtl.s8LocalCriticalEvent 		= 0;			/* Unspecified critical link evt */

		/* OAM Functions for OAM configuration field */
		gstLocalEOamCtl.s8LocalUnidirectional 	= 0;			/* Support Unidirectional */
		gstLocalEOamCtl.s8LocalEOamLoopBack			= 1;			/* Support LoopBack function */
		gstLocalEOamCtl.s8LocalEoamLinkEvent		= 1;			/* Support Link events */
		gstLocalEOamCtl.s8LocalVariRetrieval		= 0;			/* Support Sending variable response */
		#else
		gstLocalEOamCtl.s8LocalLinkStatus	 			= gs8LocalEoamFunctionality[0];			/* Link fault */
		gstLocalEOamCtl.s8LocalDyingGasp	 			= gs8LocalEoamFunctionality[1];			/* Unrecoverable failure */
		gstLocalEOamCtl.s8LocalCriticalEvent 		= gs8LocalEoamFunctionality[2];			/* Unspecified critical link evt */

		/* OAM Functions for OAM configuration field */
		gstLocalEOamCtl.s8LocalUnidirectional 	= gs8LocalEoamFunctionality[3];			/* Support Unidirectional */
		gstLocalEOamCtl.s8LocalEOamLoopBack			= gs8LocalEoamFunctionality[4];			/* Support LoopBack function */
		gstLocalEOamCtl.s8LocalEoamLinkEvent		= gs8LocalEoamFunctionality[5];			/* Support Link events */
		gstLocalEOamCtl.s8LocalVariRetrieval		= gs8LocalEoamFunctionality[6];			/* Support Sending variable response */
		#endif

		/* OAM Discovery Status */
		gstLocalEOamCtl.s8DiscStatus 						= EOAM_STATUS_DISC_FAULT;
		gstLocalEOamCtl.s8LocalPdu							= LOCAL_PDU_LF_INFO;
		gstLocalEOamCtl.s8LocalStable						= 0;
		gstLocalEOamCtl.s8LocalSatisfied		 		= 0;
		gstLocalEOamCtl.s8RemoteStable					= 0;
		gstLocalEOamCtl.s8RemoteStateValid			= 0;
		gstLocalEOamCtl.s8RemoteDTEDiscovered		= 0;

		/* OAM System Parameters */
		gstLocalEOamCtl.s8LocalParAction				= LOCAL_PARSER_FWD;
		gstLocalEOamCtl.s8LocalMuxAction				= LOCAL_MUX_FWD;

		if(s8EOamMode == EOAM_LAYER_PASSIVE_DTE)
		{
				gstLocalEOamCtl.s8LocalEOamMode			= EOAM_PASSIVE_MODE;
		}
		else if(s8EOamMode == EOAM_LAYER_ACTIVE_DTE)
		{
				gstLocalEOamCtl.s8LocalEOamMode			= EOAM_ACTIVE_MODE;
		}
		else
		{
#ifdef SUPERTASK
				EOAM_MSG(dprintf("%s[%s] EOAM ifno=%d Disable!\n%s",CLR1_31_RED,__FUNCTION__,gEOAM_Ifno,CLR0_RESET));
#else
				EOAM_MSG(dprintf("%s[%s] EOAM Disable!\n%s",CLR1_31_RED,__FUNCTION__,CLR0_RESET));
#endif // SUPERTASK
				memset(&gstLocalEOamCtl,0,sizeof(gstLocalEOamCtl));

				gs8LocalEoamEnable 	= FALSE;
				gs8LocalEoamBegin 	= TRUE;
				gs8LocalEoamMode 		= EOAM_LAYER_DISABLE;

#ifdef SUPERTASK
				if_table[gEOAM_Ifno].flag &= ~IFF_EOAM_UP;
#endif
				return EOAM_FAIL;
		}

		gstLocalEOamCtl.s8LocalEOamLBInited		= EOAM_LB_DISABLE;

		/* OAM Frame/PDU Parameters */
		gstLocalEOamCtl.s16LocalFlags					= 0;
		gstLocalEOamCtl.s8LocalCode						= 0;
		for(i=0;i<ETHER_MAC_LEN;i++)
			gstLocalEOamCtl.s8RMTDTEAddr[i]			= 0;
		gstLocalEOamCtl.s16RMTFlags						= 0;

		//EOAM_MSG(dprintf("T_WAN_CLONE_INT=%d, ", T_WAN_CLONE_INT));
		//EOAM_MSG(dprintf("T_WAN_INT=%d\n", T_WAN_INT));
#ifdef SUPERTASK
		gstLocalEOamCtl.s8LocalDTEOUI[0]			= if_table[gEOAM_Ifno].macaddr[0];
		gstLocalEOamCtl.s8LocalDTEOUI[1]			= if_table[gEOAM_Ifno].macaddr[1];
		gstLocalEOamCtl.s8LocalDTEOUI[2]			= if_table[gEOAM_Ifno].macaddr[2];
#else
				gstLocalEOamCtl.s8LocalDTEOUI[0]			= MY_SLOW_PTL_MAC[0];
				gstLocalEOamCtl.s8LocalDTEOUI[1]			= MY_SLOW_PTL_MAC[1];
				gstLocalEOamCtl.s8LocalDTEOUI[2]			= MY_SLOW_PTL_MAC[2];
#endif // SUPERTASK

		memset(&gstLocalEOamCtl.stLocalInfoTlv,0,sizeof(stLOCAL_INFO_TLV));
		memset(&gstLocalEOamCtl.stRMTLocalInfoTlv,0,sizeof(stLOCAL_INFO_TLV));
		memset(&gstLocalEOamCtl.stRMTRemoteInfoTlv,0,sizeof(stLOCAL_INFO_TLV));
		memset(&gstLocalEOamCtl.stRMTSpecInfoTlv,0,sizeof(stLOCAL_INFO_TLV));

		gstLocalEOamCtl.ps8LoopbackBuff				= NULL;

#ifdef SUPERTASK
		if_table[gEOAM_Ifno].flag |= IFF_EOAM_UP;
#endif

		gstLocalEOamCtl.s8LocalEOamInited 		= 1;

		//DumpEOam_CtlInfo();
#ifdef SUPERTASK
	EOAM_MSG(dprintf("%s[%s]EOAM init ifno=%d & mode=%d Succcess!\n%s",CLR1_33_YELLOW,__FUNCTION__,gEOAM_Ifno,s8EOamMode,CLR0_RESET));
#else
	EOAM_MSG(dprintf("%s[%s] EOAM init mode=%d gstLocalEOamCtl.s8LocalEOamInited=%d Succcess!\n%s",CLR1_33_YELLOW,__FUNCTION__,s8EOamMode,gstLocalEOamCtl.s8LocalEOamInited,CLR0_RESET));
#endif // SUPERTASK

		return gstLocalEOamCtl.s8LocalEOamInited;
}

/*****************************************************************
*
*   PROCEDURE NAME:
*   EOAM_LOOPBACK_TASK
*
*   DESCRIPTION:
*
*   NOTE:
*
*****************************************************************/
int EOAM_LOOPBACK_TASK(int s32Ifno, char *ps8PktBuff, int s32BuffLen)
{
		//stETHER_PTL_HDR *pstEtherHdr;
		//char *ps8LoopbackBuff;

		if((gstLocalEOamCtl.ps8LoopbackBuff=GetBuffer(BUFSZ1)) == NULL)
		{
				EOAM_MSG(dprintf("%s[%s]get buffer(%d) fail!\n%s",CLR1_31_RED,__FUNCTION__,BUFSZ1,CLR0_RESET));
				gstLocalEOamCtl.ps8LoopbackBuff = NULL;

				return EOAM_ERROR_GET_BUFFER;
		}

		SetBufferLength(gstLocalEOamCtl.ps8LoopbackBuff, s32BuffLen);
		memset(BUF_BUFFER_PTR(gstLocalEOamCtl.ps8LoopbackBuff),0,s32BuffLen);

		memcpy(BUF_BUFFER_PTR(gstLocalEOamCtl.ps8LoopbackBuff),BUF_BUFFER_PTR(ps8PktBuff),GetBufferLength(gstLocalEOamCtl.ps8LoopbackBuff));

		//memcpy(BUF_BUFFER_PTR(gstLocalEOamCtl.ps8LoopbackBuff),BUF_BUFFER_PTR(ps8PktBuff),GetBufferLength(gstLocalEOamCtl.ps8LoopbackBuff));

		//pstEtherHdr = (stETHER_PTL_HDR *)BUF_BUFFER_PTR(ps8LoopbackBuff);

		//memcpy(pstEtherHdr->s8DestAddr, pstEtherHdr->s8SrcAddr, ETHER_MAC_LEN);

		{
				//memcpy(pstEtherHdr->s8SrcAddr,(char*)&if_table[T_WAN_INT].macaddr,ETHER_MAC_LEN);
#ifdef SUPERTASK
			//memcpy(pstEtherHdr->s8SrcAddr,(char*)&if_table[T_WAN_INT].macaddr,ETHER_MAC_LEN);
			if_table[gEOAM_Ifno].driverp->send(gEOAM_Ifno,gstLocalEOamCtl.ps8LoopbackBuff,GetBufferLength(gstLocalEOamCtl.ps8LoopbackBuff));
#else
			if(MY_SLOW_PTL_DEV==NULL)
			{
				printk("MY_SLOW_PTL_DEV == NULL, line %d", __LINE__);
			}
			MY_SLOW_PTL_DEV->netdev_ops->ndo_start_xmit(gstLocalEOamCtl.ps8LoopbackBuff, MY_SLOW_PTL_DEV);
#endif // SUPERTASK
		}

		//if(RelBuffer(gstLocalEOamCtl.ps8LoopbackBuff) != 0/*SUCCESS*/)
		//{
				//EOAM_MSG(dprintf("%s[%s]Release Buffer Fail!\n%s",CLR1_31_RED,__FUNCTION__,CLR0_RESET));
				//return EOAM_FAIL;
		//}

		//gstLocalEOamCtl.ps8LoopbackBuff = NULL;

		return EOAM_SUCCESS;
}


/*****************************************************************
*
*   PROCEDURE NAME:
*   EOAM_TX_TASK
*
*   DESCRIPTION:
*
*   NOTE:
*
*****************************************************************/
int EOAM_TX_TASK(void)
{
	char *ps8PktBuff;

	char s8SubType=0;
	//short s16Flags=0;
	//char s8Code=0;
	int s32FramChkSeq=0;

	char s8EOamPduCnt;

	unsigned long u64SpentTime,u64LastTimeSnap;
	unsigned char bShouldStop = 0;

	EOAM_MSG(dprintf("%s[%s] s8DiscStatus=%d,s8LocalEOamLBInited=%d\n%s",CLR1_33_YELLOW,__FUNCTION__,gstLocalEOamCtl.s8DiscStatus,gstLocalEOamCtl.s8LocalEOamLBInited,CLR0_RESET));
	//EOAM_MSG(dprintf("gs8LocalEoamMode=%d\n",gs8LocalEoamMode));

	/* Transmit Normal mode */
TX_STATUS_RESET:
	s8EOamPduCnt = EOAMPDU_COUNT_PER_SECOND;

	u64LastTimeSnap = GetTime();
	//EOAM_MSG(dprintf("%s[%s]u64LastTimeSnap=%ld\n%s",CLR1_33_YELLOW,__FUNCTION__,u64LastTimeSnap,CLR0_RESET));

TX_STATUS_WAIT_TX:
	do
	{
#ifdef SUPERTASK
		dlytsk(cur_task, DLY_TICKS, MSECTOTICKS(100));
#else
		KernelThreadDealy(100);
		if(kthread_should_stop())
		{
			bShouldStop = 1;
			printk("[EOAM_TX_TASK] kthread_should_stop\n");
			return EOAM_SUCCESS;
		}
#endif // SUPERTASK

		/* b) Valid request to send an OAMPDU present */
		if(s8EOamPduCnt == 0)
		{
			EOAM_MSG(dprintf("[EOAM_TX_TASK] s8EOamPduCnt=%d, run out of pdu quota! ...\n",s8EOamPduCnt));
			return EOAM_SUCCESS;
		}

		if(gs8LocalEoamMode == EOAM_LAYER_LOOPBACK_DTE)
		{
			if((gstLocalEOamCtl.s8LocalEOamLBInited == EOAM_LB_DISABLE) || \
				(gstLocalEOamCtl.s8LocalEOamLBInited == EOAM_LB_EXIT))
			{
				if(gstLocalEOamCtl.s8LocalEOamMode == EOAM_ACTIVE_MODE)
				{
					goto TX_STATUS_OAMPDU;
				}
			}
			else if(gstLocalEOamCtl.s8LocalEOamLBInited == EOAM_LB_START_INPROGRESS)
			{
				if(gstLocalEOamCtl.s8LocalEOamMode == EOAM_PASSIVE_MODE)
				{
					if((gstLocalEOamCtl.s8LocalParAction == REMOTE_PARSER_LB) &&\
						 (gstLocalEOamCtl.s8LocalMuxAction == REMOTE_MUX_DISCARD))
					{
						goto TX_STATUS_OAMPDU;
					}
				}
			}
			else if(gstLocalEOamCtl.s8LocalEOamLBInited == EOAM_LB_EXIT_INPROGRESS)
			{
				if(gstLocalEOamCtl.s8LocalEOamMode == EOAM_PASSIVE_MODE)
				{
					if((gstLocalEOamCtl.s8LocalParAction == REMOTE_PARSER_FWD) &&\
						 (gstLocalEOamCtl.s8LocalMuxAction == REMOTE_MUX_FWD))
					{
						goto TX_STATUS_OAMPDU;
					}
				}
			}
			else if(gstLocalEOamCtl.s8LocalEOamLBInited == EOAM_LB_START)
			{
				#if 0
				if(gs8localEoamLoopbackITR == 1)
				{
					EOAM_MSG(dprintf("Parser loopback received non-OAMPDUs, gs8localEoamLoopbackITR=%d\n",gs8localEoamLoopbackITR));

					/* Send LoopBack non-OAMPDUs Packets */
					if_table[gEOAM_Ifno].driverp->send(gEOAM_Ifno,gstLocalEOamCtl.ps8LoopbackBuff,GetBufferLength(gstLocalEOamCtl.ps8LoopbackBuff));

					#if 0
					//uart_tx_onoff(1);
					{
					    int i;
							dprintf("\n[%s]DUMP LB BUFFER.....Len=%ld",__FUNCTION__,GetBufferLength(gstLocalEOamCtl.ps8LoopbackBuff));
					    for(i=0;i<GetBufferLength(gstLocalEOamCtl.ps8LoopbackBuff);i++)
						{
									if(i%4==0)	dprintf(" ");
					        if(i%16==0) dprintf("\n%04x: ", i);
					        dprintf("%02x ", *(BUF_BUFFER_PTR(gstLocalEOamCtl.ps8LoopbackBuff)+i));
					    }
					    dprintf("\n\n");
					}
					//uart_tx_onoff(0);
					#endif

					if(RelBuffer(gstLocalEOamCtl.ps8LoopbackBuff) != EOAM_SUCCESS)
					{
						EOAM_MSG(dprintf("%s[%s]Release Buffer Fail!\n%s",CLR1_31_RED,__FUNCTION__,CLR0_RESET));
						return EOAM_FAIL;
					}

					gstLocalEOamCtl.ps8LoopbackBuff		= NULL;
					gs8localEoamLoopbackITR = 0;
				}
				#endif
			}
		}

		/* valid pdu req 1 */
/*	bitonic
		In EOAM_DISCOVERY_TASK(), we will check gstLocalEOamCtl.s8DiscStatus value.
		If (gstLocalEOamCtl.s8DiscStatus == EOAM_STATUS_DISC_SEND_LOCAL_REMOTE_SUCCESS),
			and both gstLocalEOamCtl.s8LocalSatisfied and gstLocalEOamCtl.s8RemoteStable are equal to 1,
		we will set gstLocalEOamCtl.s8DiscStatus = EOAM_STATUS_DISC_SEND_ANY and
			gstLocalEOamCtl.s8LocalPdu = LOCAL_PDU_ANY;
		So, the followin case will not be touched.
		Otherwise, IAD will send EOAM packet every 200 mili-second.
*/
		if(((gstLocalEOamCtl.s8LocalLinkStatus == 0)&& \
			(gstLocalEOamCtl.s8LocalDyingGasp == 0) && \
			(gstLocalEOamCtl.s8LocalCriticalEvent == 0) && \
			(gstLocalEOamCtl.s8LocalPdu == LOCAL_PDU_INFO)))
		{
			s8EOamPduCnt -= 1;
			EOAM_MSG(dprintf("[EOAM_TX_TASK] s8EOamPduCnt=%d, goto TX_STATUS_OAMPDU ...\n",s8EOamPduCnt));
			goto TX_STATUS_OAMPDU;
		}

		/* valid pdu req 2 */
		if(gstLocalEOamCtl.s8LocalPdu == LOCAL_PDU_ANY)
		{
			if((gstLocalEOamCtl.s8LocalLinkStatus == 1)|| \
			    (gstLocalEOamCtl.s8LocalDyingGasp == 1)|| \
				(gstLocalEOamCtl.s8LocalCriticalEvent == 1))
			{
				s8EOamPduCnt -= 1;
				EOAM_MSG(dprintf("[EOAM_TX_TASK] s8EOamPduCnt=%d, goto TX_STATUS_OAMPDU ...\n",s8EOamPduCnt));

				goto TX_STATUS_OAMPDU;
			}
		}

		u64SpentTime = GetTime() - u64LastTimeSnap;
		//EOAM_MSG(dprintf("%su64SpentTime=%ld\n%s",CLR1_33_YELLOW,u64SpentTime,CLR0_RESET));

	}while(u64SpentTime < ONE_SECOND_JIFFIES);
	EOAM_MSG(dprintf("%s[%s]u64SpentTime=%ld\n%s",CLR1_33_YELLOW,__FUNCTION__,u64SpentTime,CLR0_RESET));

	if(s8EOamPduCnt<EOAMPDU_COUNT_PER_SECOND)
	{
		/* need to reset 1's second timer */
		EOAM_MSG(dprintf("s8EOamPduCnt=%d, need to reset ...\n",s8EOamPduCnt));
		goto TX_STATUS_RESET;//return EOAM_SUCCESS;
	}
	else if(s8EOamPduCnt==EOAMPDU_COUNT_PER_SECOND)
	{
		/* This prevents the Discovery process from restarting. */
	}

	/* a) Expiration of pdu_timer */
	if(gstLocalEOamCtl.s8LocalPdu == LOCAL_PDU_RX_INFO)
	{
		if(gstLocalEOamCtl.s8RemoteStateValid == 0)
		{
			/* need to reset 1's second timer */
			EOAM_MSG(dprintf("s8LocalPdu=%d, can't send any OAMPDU ...\n",gstLocalEOamCtl.s8LocalPdu));
			goto TX_STATUS_RESET;//return EOAM_SUCCESS;
		}
	}

	#if 0 // not sure
	/* Don't support unidirectional */
	if((pstEOamLayerCtl->s8LocalUnidirectional == 0) && (pstEOamLayerCtl->s8LocalPdu == LOCAL_PDU_LF_INFO))
	{
		/* need to reset 1's second timer */
		EOAM_MSG(dprintf("s8LocalUnidirectional=%d & s8LocalPdu=%d, need to reset 1's second timer ...\n",pstEOamLayerCtl->s8LocalUnidirectional,pstEOamLayerCtl->s8LocalPdu));
		goto RELEASE_BUFFER;
	}
	#endif

	//EOAM_MSG(dprintf("After WAIT_TX....\n"));
	//EOAM_MSG(dprintf("gs8LocalEoamMode=%d\n",gs8LocalEoamMode));
	//EOAM_MSG(dprintf("gstLocalEOamCtl.s8LocalEOamLBInited=%d\n",gstLocalEOamCtl.s8LocalEOamLBInited));

	/* Multiplexer */
TX_STATUS_OAMPDU :

	s8SubType = SUBTYPE_EOAM;

	/* LoopBack mode, OAMPDUs  */
	if(gs8LocalEoamMode == EOAM_LAYER_LOOPBACK_DTE)
	{
			if((gstLocalEOamCtl.s8LocalEOamLBInited == EOAM_LB_DISABLE) ||	\
				 (gstLocalEOamCtl.s8LocalEOamLBInited == EOAM_LB_EXIT))
			{
					if(gstLocalEOamCtl.s8LocalEOamMode == EOAM_PASSIVE_MODE)
					{
							/* Info OAMPDUs are continually sent to keep the OAM Discovery process from restarting.*/
							gstLocalEOamCtl.s8LocalCode = CODE_INFO;
					}
					else if(gstLocalEOamCtl.s8LocalEOamMode == EOAM_ACTIVE_MODE)
					{
							if((gstLocalEOamCtl.s8LocalParAction == REMOTE_PARSER_DISCARD) &&\
								 (gstLocalEOamCtl.s8LocalMuxAction == REMOTE_MUX_DISCARD))
							{
									/* Active DTE will send Loopback CMD when begin LoopBack mode */
									gstLocalEOamCtl.s8LocalCode = CODE_LOOPBACK_CTL;
							}
							else if((gstLocalEOamCtl.s8LocalParAction == REMOTE_PARSER_FWD) &&\
								 			(gstLocalEOamCtl.s8LocalMuxAction == REMOTE_MUX_FWD))
							{
									gstLocalEOamCtl.s8LocalCode = CODE_INFO;
							}
					}
			}
			else if((gstLocalEOamCtl.s8LocalEOamLBInited == EOAM_LB_START_INPROGRESS)	||	\
						 (gstLocalEOamCtl.s8LocalEOamLBInited == EOAM_LB_EXIT_INPROGRESS))
			{
					/*
						Both remote DTE or local DTE myself and Passive or Active mode,
						it will send INFO OAMPDU back for updating state.
					*/
					gstLocalEOamCtl.s8LocalCode = CODE_INFO;
			}
			else if(gstLocalEOamCtl.s8LocalEOamLBInited == EOAM_LB_START)
			{
					gstLocalEOamCtl.s8LocalCode = CODE_INFO;
			}

			/* In LoopBack mode, it should be stable! */
			gstLocalEOamCtl.s16LocalFlags &= ~EOAMPDU_FLAGS_LOCALEVAL;
			gstLocalEOamCtl.s16LocalFlags |= EOAMPDU_FLAGS_LOCALSTABLE;

			gstLocalEOamCtl.s16LocalFlags &= ~EOAMPDU_FLAGS_REMOTEEVAL;
			gstLocalEOamCtl.s16LocalFlags |= EOAMPDU_FLAGS_REMOTESTABLE;

			//EOAM_MSG(dprintf("LB mode, gstLocalEOamCtl.s16LocalFlags=0x%04x, gstLocalEOamCtl.s8LocalCode=0x%02x\n",gstLocalEOamCtl.s16LocalFlags,gstLocalEOamCtl.s8LocalCode));

	}
	else /* Normal mode */
	{
			/* Prevent discovery process timeout */
			if(gs8LocalLostLnkTimeDone == 1)
			{
					if((gstLocalEOamCtl.s8DiscStatus == EOAM_STATUS_DISC_PASSIVE_WAIT) || (gstLocalEOamCtl.s8DiscStatus == EOAM_STATUS_DISC_ACTIVE_SEND_LOCAL))
					{
							if(gstLocalEOamCtl.s8RemoteStateValid == 1)
							{
									EOAM_MSG(dprintf("s8RemoteStateValid == 1, Prevent discovery process timeout, return EOAM_SUCCESS, line %d\n", __LINE__));
									return EOAM_SUCCESS;
							}
					}

					if(gstLocalEOamCtl.s8DiscStatus == EOAM_STATUS_DISC_SEND_LOCAL_REMOTE)
					{
							if(gstLocalEOamCtl.s8LocalSatisfied == 1)
							{
									EOAM_MSG(dprintf("s8LocalSatisfied == 1, Prevent discovery process timeout\n"));
									return EOAM_SUCCESS;
							}
					}
			}

			gstLocalEOamCtl.s8LocalCode = CODE_INFO;

			if(gstLocalEOamCtl.s8LocalLinkStatus == 1)
			{
					gstLocalEOamCtl.s16LocalFlags |= EOAMPDU_FLAGS_LINKFAULT;
			}
			else
			{
					gstLocalEOamCtl.s16LocalFlags &= ~EOAMPDU_FLAGS_LINKFAULT;
			}

			if(gstLocalEOamCtl.s8LocalDyingGasp == 1)
			{
					gstLocalEOamCtl.s16LocalFlags |= EOAMPDU_FLAGS_DYINGASP;
			}
			else
			{
					gstLocalEOamCtl.s16LocalFlags &= ~EOAMPDU_FLAGS_DYINGASP;
			}

			if(gstLocalEOamCtl.s8LocalCriticalEvent == 1)
			{
					gstLocalEOamCtl.s16LocalFlags |= EOAMPDU_FLAGS_CRITICEVT;
			}
			else
			{
					gstLocalEOamCtl.s16LocalFlags &= ~EOAMPDU_FLAGS_CRITICEVT;
			}

			//if((gstLocalEOamCtl.s8LocalSatisfied == 1) && (gstLocalEOamCtl.s8RemoteStable == 1))
			if(gstLocalEOamCtl.s8LocalStable == 1)
			{
					gstLocalEOamCtl.s16LocalFlags &= ~EOAMPDU_FLAGS_LOCALEVAL;
					gstLocalEOamCtl.s16LocalFlags |= EOAMPDU_FLAGS_LOCALSTABLE;
			}
			else
			{
					gstLocalEOamCtl.s16LocalFlags |= EOAMPDU_FLAGS_LOCALEVAL;
					gstLocalEOamCtl.s16LocalFlags &= ~EOAMPDU_FLAGS_LOCALSTABLE;
			}

			if(gstLocalEOamCtl.s8RemoteStateValid == 1)
			{
					//EOAM_MSG(dprintf("s16RMTFlags=0x%x\n",gstLocalEOamCtl.s16RMTFlags));
					if((gstLocalEOamCtl.s16RMTFlags&EOAMPDU_FLAGS_LOCALEVAL) == EOAMPDU_FLAGS_LOCALEVAL)
					{
				gstLocalEOamCtl.s16LocalFlags &= ~EOAMPDU_FLAGS_REMOTESTABLE;
							gstLocalEOamCtl.s16LocalFlags |= EOAMPDU_FLAGS_REMOTEEVAL;
					}

					if((gstLocalEOamCtl.s16RMTFlags&EOAMPDU_FLAGS_LOCALSTABLE) == EOAMPDU_FLAGS_LOCALSTABLE)
					{
				gstLocalEOamCtl.s16LocalFlags &= ~EOAMPDU_FLAGS_REMOTEEVAL;
							gstLocalEOamCtl.s16LocalFlags |= EOAMPDU_FLAGS_REMOTESTABLE;
					}
			}

			//EOAM_MSG(dprintf("Normal mode, gstLocalEOamCtl.s16LocalFlags=0x%04x, gstLocalEOamCtl.s8LocalCode=0x%x\n",gstLocalEOamCtl.s16LocalFlags,gstLocalEOamCtl.s8LocalCode));
	}


		/* TX_OAMPDU state */
		//if(ps8PktBuff == NULL)
		//{
		if((ps8PktBuff=GetBuffer(BUFSZ1)) == NULL)
		{
#ifdef SUPERTASK
			EOAM_MSG(dprintf("%s[%s]get buffer(%d),(%d) fail!\n%s",CLR1_31_RED,__FUNCTION__,B1_ID,BUFSZ1,CLR0_RESET));
#else
			EOAM_MSG(dprintf("%s[%s]get buffer szie (%d) fail!\n%s",CLR1_31_RED,__FUNCTION__,BUFSZ1,CLR0_RESET));
#endif // SUPERTASK
				return EOAM_ERROR_GET_BUFFER;
		}
		//}

		/* LoopBack mode */
		if(gs8LocalEoamMode == EOAM_LAYER_LOOPBACK_DTE)
		{
				if((gstLocalEOamCtl.s8LocalEOamLBInited == EOAM_LB_DISABLE)	||	\
					 (gstLocalEOamCtl.s8LocalEOamLBInited == EOAM_LB_EXIT))
				{
						if(gstLocalEOamCtl.s8LocalEOamMode == EOAM_PASSIVE_MODE)
						{
								EOAM_MSG(dprintf("setp f2) Remote DTE in Passive mode, send Info OAMPDU & prepare to exit LoopBack mode!\n"));

								if(EOAMTLV_InfoLocal_Request((INFO_TLV_LOCAL_INFO+INFO_TLV_REMOTE_INFO),ps8PktBuff) == NULL)
										return EOAM_ERROR_GET_BUFFER;
						}
						else if(gstLocalEOamCtl.s8LocalEOamMode == EOAM_ACTIVE_MODE)
						{
								if(gstLocalEOamCtl.s8LocalEOamLBInited == EOAM_LB_DISABLE)
								{
										if((gstLocalEOamCtl.s8LocalParAction == REMOTE_PARSER_DISCARD) &&\
									     (gstLocalEOamCtl.s8LocalMuxAction == REMOTE_MUX_DISCARD))
										{
												EOAM_MSG(dprintf("setp 2) Local DTE in Active mode, send LB control OAMPDU (REMOTE_LB_ENABLE)\n"));
												if(EOAMTLV_LoopBackCtl_Request(REMOTE_LB_ENABLE,ps8PktBuff) == NULL)
													return EOAM_ERROR_GET_BUFFER;

												gstLocalEOamCtl.s8LocalEOamLBInited = EOAM_LB_START_INPROGRESS;
										}
										else if((gstLocalEOamCtl.s8LocalParAction == REMOTE_PARSER_FWD) &&\
									          (gstLocalEOamCtl.s8LocalMuxAction == REMOTE_MUX_FWD))
										{
												EOAM_MSG(dprintf("setp g2) Local DTE in Active mode, send Info OAMPDU & prepare to exit LoopBack mode!\n"));

												if(EOAMTLV_InfoLocal_Request((INFO_TLV_LOCAL_INFO+INFO_TLV_REMOTE_INFO),ps8PktBuff) == NULL)
													return EOAM_ERROR_GET_BUFFER;
										}
								}
								else
								{
										EOAM_MSG(dprintf("setp b) Local DTE in Active mode, send LB control OAMPDU (REMOTE_LB_DISABLE)\n"));
										if(EOAMTLV_LoopBackCtl_Request(REMOTE_LB_DISABLE,ps8PktBuff) == NULL)
											return EOAM_ERROR_GET_BUFFER;

										gstLocalEOamCtl.s8LocalEOamLBInited = EOAM_LB_EXIT_INPROGRESS;
								}
						}
				}
				else if((gstLocalEOamCtl.s8LocalEOamLBInited == EOAM_LB_START_INPROGRESS) || \
								(gstLocalEOamCtl.s8LocalEOamLBInited == EOAM_LB_EXIT_INPROGRESS))
				{

						if(EOAMTLV_InfoLocal_Request((INFO_TLV_LOCAL_INFO+INFO_TLV_REMOTE_INFO),ps8PktBuff) == NULL)
							return EOAM_ERROR_GET_BUFFER;

						if(gstLocalEOamCtl.s8LocalEOamLBInited == EOAM_LB_START_INPROGRESS)
						{
								if(gstLocalEOamCtl.s8LocalEOamMode == EOAM_PASSIVE_MODE)
								{
										EOAM_MSG(dprintf("step 4) Remote DTE in Passive mode, send INFO OAMPDUs with PAR=LB(1), MUX=DISCARD(1)\n"));
								}
								else if(gstLocalEOamCtl.s8LocalEOamMode == EOAM_ACTIVE_MODE)
								{
										EOAM_MSG(dprintf("step 5b) Local DTE in Active mode, send INFO OAMPDUs with PAR=DISCARD(2), MUX=FWD(0)\n"));
								}
						}
						else
						{
								if(gstLocalEOamCtl.s8LocalEOamMode == EOAM_PASSIVE_MODE)
								{
										EOAM_MSG(dprintf("step d) Remote DTE in Passive mode, send INFO OAMPDUs with PAR=FWD(0), MUX=FWD(0)\n"));
								}
								else if(gstLocalEOamCtl.s8LocalEOamMode == EOAM_ACTIVE_MODE)
								{
										EOAM_MSG(dprintf("step e2) Local DTE in Active mode, send INFO OAMPDUs with PAR=FWD(0), MUX=FWD(0)\n"));
										gstLocalEOamCtl.s8LocalEOamLBInited = EOAM_LB_DISABLE;
								}
						}
				}
				else if(gstLocalEOamCtl.s8LocalEOamLBInited == EOAM_LB_START)
				{
						if(EOAMTLV_InfoLocal_Request((INFO_TLV_LOCAL_INFO+INFO_TLV_REMOTE_INFO),ps8PktBuff) == NULL)
							return EOAM_ERROR_GET_BUFFER;
				}
				else
				{

				}
		}
		else	/* Normal mode */
		{
				/* Sending Information OAMPDUs no Information TLVs in the Data field. */
				if(gstLocalEOamCtl.s8DiscStatus == EOAM_STATUS_DISC_FAULT)
				{
						SetBufferLength(ps8PktBuff, ETHER_FRAME_MIN_LEN);
						memset(BUF_BUFFER_PTR(ps8PktBuff),0,ETHER_FRAME_MIN_LEN);

						gstLocalEOamCtl.s16LocalFlags &= EOAMPDU_FLAGS_LINKFAULT;
				}
				/* Sending Information OAMPDUs that only contain the Local Information TLV */
				else if(gstLocalEOamCtl.s8DiscStatus == EOAM_STATUS_DISC_PASSIVE_WAIT)
				{
						if(gstLocalEOamCtl.s8RemoteStateValid == 1)
						{
								if(EOAMTLV_InfoLocal_Request(INFO_TLV_LOCAL_INFO,ps8PktBuff) == NULL)
									return EOAM_ERROR_GET_BUFFER;
						}
						else
						{
								EOAM_MSG(dprintf("s8DiscStatus=%d & s8RemoteStateValid=%d, so send nothing ...\n",gstLocalEOamCtl.s8DiscStatus,gstLocalEOamCtl.s8RemoteStateValid));
								return EOAM_SUCCESS;//goto RELEASE_BUFFER;
						}
				}
				else if(gstLocalEOamCtl.s8DiscStatus == EOAM_STATUS_DISC_ACTIVE_SEND_LOCAL)
				{
						if(EOAMTLV_InfoLocal_Request(INFO_TLV_LOCAL_INFO,ps8PktBuff) == NULL)
							return EOAM_ERROR_GET_BUFFER;
				}
				/* Sending Information OAMPDUs that contain both the Local and Remote Information TLVs. */
				else if((gstLocalEOamCtl.s8DiscStatus == EOAM_STATUS_DISC_SEND_LOCAL_REMOTE) ||\
						(gstLocalEOamCtl.s8DiscStatus == EOAM_STATUS_DISC_SEND_LOCAL_REMOTE_SUCCESS))
				{
						if(EOAMTLV_InfoLocal_Request((INFO_TLV_LOCAL_INFO+INFO_TLV_REMOTE_INFO),ps8PktBuff) == NULL)
							return EOAM_ERROR_GET_BUFFER;
				}
				else if(gstLocalEOamCtl.s8DiscStatus == EOAM_STATUS_DISC_SEND_ANY)
				{
						if(EOAMTLV_InfoLocal_Request((INFO_TLV_LOCAL_INFO+INFO_TLV_REMOTE_INFO),ps8PktBuff) == NULL)
							return EOAM_ERROR_GET_BUFFER;
				}
		}

		if(EOAMPDU_Request(s8SubType,gstLocalEOamCtl.s16LocalFlags,gstLocalEOamCtl.s8LocalCode,ps8PktBuff) == NULL)
			return EOAM_ERROR_GET_BUFFER;

		{
#ifdef SUPERTASK
				if(EOAMI_Request((char*)&SLOW_PTL_MULTIMAC[0],(char*)&if_table[gEOAM_Ifno].macaddr,s32FramChkSeq,ps8PktBuff) == NULL)
#else
				if(EOAMI_Request((char*)&SLOW_PTL_MULTIMAC[0],(char*)MY_SLOW_PTL_MAC,s32FramChkSeq,ps8PktBuff) == NULL)
#endif // SUPERTASK
					return EOAM_ERROR_GET_BUFFER;
		}

		//DumpBuffer_EOAM(BUF_BUFFER_PTR(ps8PktBuff),GetBufferLength(ps8PktBuff),CLR1_37_WHITE);

		// test dump.....
		//if(gstLocalEOamCtl.s8LocalEOamLBInited == EOAM_LB_INPROGRESS)
		//if(s8EOamPduCnt == 9)
		//if(gstLocalEOamCtl.s8DiscStatus == EOAM_STATUS_DISC_SEND_ANY)
		//{
				//DumpBuffer_RAW(BUF_BUFFER_PTR(ps8PktBuff), 64, CLR1_37_WHITE);
		//}

		/* Send Packet */
		{
#ifdef SUPERTASK
			if_table[gEOAM_Ifno].driverp->send(gEOAM_Ifno,ps8PktBuff,GetBufferLength(ps8PktBuff));
#else
			if(MY_SLOW_PTL_DEV==NULL)
			{
				printk("MY_SLOW_PTL_DEV == NULL, line %d", __LINE__);
			}
			MY_SLOW_PTL_DEV->netdev_ops->ndo_start_xmit(ps8PktBuff, MY_SLOW_PTL_DEV);
#endif // SUPERTASK
		}
		//ifp->driverp->send(pstEOamLayerCtl->s8LocalIfno,ps8PktBuff,GetBufferLength(ps8PktBuff));

		/* LoopBack timer */
		if(gs8LocalEoamMode == EOAM_LAYER_LOOPBACK_DTE)
		{
				if(gs8LoopBackSettingTimeDone == 1) /* Got LoopBack CMD packet, RemotDTE */
				{
						/* Send packet less than 1 sec */
						if((u64SpentTime=GetTime()-gu64LoopbackLostTimeSnap)<ONE_SECOND_JIFFIES)
						{
								gs8LoopBackSettingTimeDone = 0;
								EOAM_MSG(dprintf("%su64SpentTime=%ld<ONE_SECOND_JIFFIES, gs8LoopBackSettingTimeDone=%d\n%s",CLR1_33_YELLOW,u64SpentTime,gs8LoopBackSettingTimeDone,CLR0_RESET));

								//goto TX_STATUS_WAIT_TX;
								//dlytsk(cur_task, DLY_TICKS, MSECTOTICKS(1000-u64SpentTime));
						}
						else
						{
								EOAM_MSG(dprintf("%su64SpentTime=%ld>ONE_SECOND_JIFFIES, Resent LB OAMPDU\n%s",CLR1_33_YELLOW,u64SpentTime,gs8LoopBackSettingTimeDone,CLR0_RESET));
						}
				}
		}
		else
		{
				if((u64SpentTime=GetTime()-u64LastTimeSnap)<ONE_SECOND_JIFFIES)
				{
						EOAM_MSG(dprintf("u64SpentTime=%ld<ONE_SECOND_JIFFIES, go back to TX_STATUS_WAIT_TX...\n",u64SpentTime));
						goto TX_STATUS_WAIT_TX;
				}
		}

		//RELEASE_BUFFER:
		return EOAM_SUCCESS;
}

/*****************************************************************
*
*   PROCEDURE NAME:
*   EOAM_DISCOVERY_TASK
*
*   DESCRIPTION:
*		OAM provides a mechanism to detect the presence of an OAM sublayer
*		at the remote DTE.
*
*   NOTE:
*		s8EOamMode
*		1: EOAM_LAYER_PASSIVE_DTE
*		2: EOAM_LAYER_ACTIVE_DTE
*
*****************************************************************/
#ifdef SUPERTASK
void EOAM_DISCOVERY_TASK(char s8EOamMode)
#else
void EOAM_DISCOVERY_TASK(void *input_data)
#endif
{
#ifndef SUPERTASK
	char s8EOamMode;
#endif
	stEOAM_LAYER_CTL stTempLocalEOamCtl;

	unsigned long u64SpentTime;
	char s8InitTimes=0;

#ifndef SUPERTASK
	s8EOamMode = *((char *)input_data);
#endif
	printk("[EOAM_DISCOVERY_TASK] s8EOamMode: %d, gstLocalEOamCtl.s8LocalEOamInited: %d, line %d\n", s8EOamMode, gstLocalEOamCtl.s8LocalEOamInited, __LINE__);
	//uart_tx_onoff(1);
EOAM_RESET:
#ifdef SUPERTASK
	while((if_table[gEOAM_Ifno].flag & IFF_DRV_UP) != IFF_DRV_UP)
	{
		s8InitTimes++;

		EOAM_MSG(dprintf("if_table[%d].flag=0x%x, Driver not UP!, delay 10 secs and try %d times\n", gEOAM_Ifno, if_table[gEOAM_Ifno].flag, s8InitTimes));
		dlytsk(cur_task, DLY_SECS, 10);

		if(s8InitTimes == 3)
		{
			EOAM_MSG(dprintf("Init out of changes!! kill Task\n"));
			goto EOAM_DISCOVERY_FAIL;
		}
	}
#endif

	if(gstLocalEOamCtl.s8LocalEOamInited != 1)
	{
		EOAM_MSG(dprintf("%s[EOAM_DISCOVERY_TASK] step 1) Init EOAM_INIT\n%s",CLR1_32_GREEN,CLR0_RESET));
		if(EOAM_INIT(s8EOamMode))
		{
			gs8LocalEoamEnable = TRUE;
			gs8LocalEoamBegin  = FALSE;
			gs8LocalEoamMode   = s8EOamMode;

			EOAM_MSG(dprintf("[%s]EOAM ENABLE=%d,BEGIN=%d,MODE=%d\n",__FUNCTION__,gs8LocalEoamEnable,gs8LocalEoamBegin,gs8LocalEoamMode));
		}
		else
		{
			EOAM_MSG(dprintf("%sEOAM_INIT failed!\n%s",CLR1_31_RED,CLR0_RESET));
			goto EOAM_DISCOVERY_FAIL;
		}
	}
	else
	{
		gs8LocalEoamBegin  = FALSE;
		gs8LocalEoamMode   = s8EOamMode;
	}


	EOAM_MSG(dprintf("%s[EOAM_DISCOVERY_TASK] step 2) Discovery or LoopBack process\n%s",CLR1_32_GREEN,CLR0_RESET));
	EOAM_MSG(dprintf("[%s] EOAM_MODE=%d,DISCOVERY STATUS=%d\n",__FUNCTION__,gs8LocalEoamMode,gstLocalEOamCtl.s8DiscStatus));

	while(1)
	{
		//EOAM_MSG(dprintf("%svr9_GetPortLinkStatus(%d)=%d\n%s",CLR1_31_RED,ETHER_UPLINK_WAN_PORT,vr9_GetPortLinkStatus(ETHER_UPLINK_WAN_PORT),CLR0_RESET));
		//EOAM_MSG(dprintf("%sswitch_getPortLinkStatus(%d)=%d\n%s",CLR1_31_RED,ETHER_UPLINK_WAN_PORT,switch_getPortLinkStatus(ETHER_UPLINK_WAN_PORT),CLR0_RESET));
#ifndef SUPERTASK
		if(kthread_should_stop())
		{
			printk("[EOAM_DISCOVERY_TASK]  kthread_should_stop, line %d\n", __LINE__);
			break;
		}
#if 0
		// wait one second
		KernelThreadDealy(1000);
		if(kthread_should_stop())
		{
			printk("[EOAM_DISCOVERY_TASK]  kthread_should_stop, line %d\n", __LINE__);
			break;
		}
#endif
#endif

#ifdef SUPERTASKx
		if(switch_getPortLinkStatus(ETHER_UPLINK_WAN_PORT) == 0)
		{
			if(gs8LocalEoamMode == EOAM_LAYER_LOOPBACK_DTE)
			{
				if(gstTempLocalEOamCtl.s8LocalEOamMode == EOAM_ACTIVE_MODE)
				{
					EOAM_INIT(EOAM_LAYER_ACTIVE_DTE);
					gs8LocalEoamMode = EOAM_LAYER_ACTIVE_DTE;
				}
				else
				{
					EOAM_INIT(EOAM_LAYER_PASSIVE_DTE);
					gs8LocalEoamMode = EOAM_LAYER_PASSIVE_DTE;
				}
			}
			else
			{
				if(gstLocalEOamCtl.s8LocalEOamMode== EOAM_PASSIVE_MODE)
				{
					EOAM_INIT(EOAM_LAYER_PASSIVE_DTE);
					gs8LocalEoamMode = EOAM_LAYER_PASSIVE_DTE;

					//gstLocalEOamCtl.s8DiscStatus = EOAM_STATUS_DISC_PASSIVE_WAIT;
				}
				else if (gstLocalEOamCtl.s8LocalEOamMode== EOAM_ACTIVE_MODE)
				{
					EOAM_INIT(EOAM_LAYER_ACTIVE_DTE);
					gs8LocalEoamMode = EOAM_LAYER_ACTIVE_DTE;

					//gstLocalEOamCtl.s8DiscStatus = EOAM_STATUS_DISC_ACTIVE_SEND_LOCAL;
				}
			}
		}
#endif // SUPERTASK
		/* Testing whether Loopback mode can be operated */
		if(gs8LocalEoamMode == EOAM_LAYER_LOOPBACK_DTE)
		{
			EOAM_MSG(dprintf("%sstep 2-1) LoopBack process\n%s",CLR1_32_GREEN,CLR0_RESET));

			/* Reset when discovery isn't finished! */
			if(gstLocalEOamCtl.s8RemoteDTEDiscovered != 1)
			{
				if(gstLocalEOamCtl.s8LocalEOamMode== EOAM_PASSIVE_MODE)
				{
					gs8LocalEoamMode = EOAM_LAYER_PASSIVE_DTE;

					gstLocalEOamCtl.s8DiscStatus = EOAM_STATUS_DISC_PASSIVE_WAIT;
				}
				else if (gstLocalEOamCtl.s8LocalEOamMode== EOAM_ACTIVE_MODE)
				{
					gs8LocalEoamMode = EOAM_LAYER_ACTIVE_DTE;

					gstLocalEOamCtl.s8DiscStatus = EOAM_STATUS_DISC_ACTIVE_SEND_LOCAL;
				}

				EOAM_MSG(dprintf("%sReset when discovery isn't finished!\n%s",CLR1_32_GREEN,CLR0_RESET));
			}

			/* LoopBack Init-Routine */
			if(gstLocalEOamCtl.s8LocalEOamLBInited == EOAM_LB_DISABLE)
			{
				if(gstLocalEOamCtl.s8LocalEOamMode == EOAM_PASSIVE_MODE)
				{
					/* case 1. when is remote DTE myself ========================== */
					/* Setting in EOAM_HANDLER() */

					/* case 2. when is local DTE myself =========================== */
					/* No Permitted to send Loopback Control OAMPDUs because PASSIVE */
				}
				else if(gstLocalEOamCtl.s8LocalEOamMode == EOAM_ACTIVE_MODE)
				{
					/* Into LoopBack mode by using EOAM_LOOPBACK_ENABLE() */
					/* setp 1) Local DTE in Active mode, set PAR&MUX=DISCARD */

					/* Backup EOAM Status */
					gstTempLocalEOamCtl = gstLocalEOamCtl;

					/* LoopBack step 1) */
					gstLocalEOamCtl.s8LocalParAction = LOCAL_PARSER_DISCARD;
					gstLocalEOamCtl.s8LocalMuxAction = LOCAL_MUX_DISCARD;
					EOAM_MSG(dprintf("setp 1) Local DTE in Active mode, set PAR=DISCARD(2) & MUX=DISCARD(1)\n"));
				}
			}

			/* LoopBack Exit-routine */
			if(gstLocalEOamCtl.s8LocalEOamLBInited == EOAM_LB_EXIT)
			{
				if(gstLocalEOamCtl.s8LocalEOamMode == EOAM_PASSIVE_MODE)
				{
					/* Wrong s8LocalEOamMode !! */
				}
				else if(gstLocalEOamCtl.s8LocalEOamMode == EOAM_ACTIVE_MODE)
				{
					/* case 1. when is remote DTE myself ========================== */
					/* case 2. when is local DTE myself =========================== */
					/* Exit LoopBack mode by using EOAM_LOOPBACK_ENABLE() */

					gstLocalEOamCtl.s8LocalParAction = LOCAL_PARSER_DISCARD;
					gstLocalEOamCtl.s8LocalMuxAction = LOCAL_MUX_DISCARD;
					EOAM_MSG(dprintf("setp a) Local DTE in Active mode, set PAR=DISCARD(2) & MUX=DISCARD(1)\n"));
				}

			}

			EOAM_MSG(dprintf("s8LocalEOamMode=%d,s8LocalEOamLBInited=%d\n",gstLocalEOamCtl.s8LocalEOamMode,gstLocalEOamCtl.s8LocalEOamLBInited));
			EOAM_MSG(dprintf("s8LocalParAction=%d,s8LocalMuxAction=%d\n",gstLocalEOamCtl.s8LocalParAction,gstLocalEOamCtl.s8LocalMuxAction));

		}
		else	// Normal mode ================================================== //
		{
			/* 2) Discovery process */
			EOAM_MSG(dprintf("%s[EOAM_DISCOVERY_TASK] step 2-1) Discovery process, change discovery state\n%s",CLR1_32_GREEN,CLR0_RESET));
			EOAM_MSG(dprintf("gstLocalEOamCtl.s8DiscStatus=%d\n",gstLocalEOamCtl.s8DiscStatus));
			EOAM_MSG(dprintf("gstLocalEOamCtl.s8LocalPdu=%d\n",gstLocalEOamCtl.s8LocalPdu));
			EOAM_MSG(dprintf("gstLocalEOamCtl.s8LocalStable=%d\n",gstLocalEOamCtl.s8LocalStable));
			EOAM_MSG(dprintf("gstLocalEOamCtl.s8LocalSatisfied=%d\n",gstLocalEOamCtl.s8LocalSatisfied));
			EOAM_MSG(dprintf("After...\n"));

			switch(gstLocalEOamCtl.s8DiscStatus)
			{
				case EOAM_STATUS_DISC_FAULT:	//1
					EOAM_MSG(dprintf("[EOAM_DISCOVERY_TASK] gstLocalEOamCtl.s8LocalLinkStatus=%d\n",gstLocalEOamCtl.s8LocalLinkStatus));
					if(gstLocalEOamCtl.s8LocalLinkStatus == 1)
					{
						gstLocalEOamCtl.s8DiscStatus 		= EOAM_STATUS_DISC_FAULT;
						gstLocalEOamCtl.s8LocalPdu 			= LOCAL_PDU_LF_INFO;
					}
					else
					{
						if(gstLocalEOamCtl.s8LocalEOamMode == EOAM_PASSIVE_MODE)
						{
							gstLocalEOamCtl.s8DiscStatus 	= EOAM_STATUS_DISC_PASSIVE_WAIT;
							gstLocalEOamCtl.s8LocalPdu 		= LOCAL_PDU_RX_INFO;
						}
						else if(gstLocalEOamCtl.s8LocalEOamMode == EOAM_ACTIVE_MODE)
						{
							gstLocalEOamCtl.s8DiscStatus 	= EOAM_STATUS_DISC_ACTIVE_SEND_LOCAL;
							gstLocalEOamCtl.s8LocalPdu 		= LOCAL_PDU_INFO;
						}
					}

					gstLocalEOamCtl.s8LocalStable 			= 0;
					gstLocalEOamCtl.s8LocalSatisfied 		= 0;	/* Discovery not completed! */
					gstLocalEOamCtl.s8RemoteStable 			= 0;
					gstLocalEOamCtl.s8RemoteStateValid 	= 0;

					gs8LocalLostLnkTimeDone = 0;							/* Stop local_lost_link_timer */

					if(gs32VoIPServiceStopUse == 1)
					{
#ifdef SUPERTASK
						setDTLog("Leave EOAM-state", NULL, NULL);
						VoIPServiceRestart();
#endif
						gs32VoIPServiceStopUse = 0;
					}

				break;

				case EOAM_STATUS_DISC_PASSIVE_WAIT:
				case EOAM_STATUS_DISC_ACTIVE_SEND_LOCAL:
					EOAM_MSG(dprintf("[EOAM_DISCOVERY_TASK] gstLocalEOamCtl.s8RemoteStateValid=%d\n",gstLocalEOamCtl.s8RemoteStateValid));
					if(gstLocalEOamCtl.s8RemoteStateValid == 1)
					{
						gstLocalEOamCtl.s8DiscStatus 		= EOAM_STATUS_DISC_SEND_LOCAL_REMOTE;
						gstLocalEOamCtl.s8LocalPdu 			= LOCAL_PDU_INFO;

						gstLocalEOamCtl.s8LocalStable			= 0;
						gstLocalEOamCtl.s8LocalSatisfied 	= 0;
					}

				break;

				case EOAM_STATUS_DISC_SEND_LOCAL_REMOTE:
					EOAM_MSG(dprintf("[EOAM_DISCOVERY_TASK] gstLocalEOamCtl.s8LocalSatisfied=%d\n",gstLocalEOamCtl.s8LocalSatisfied));
					if(gstLocalEOamCtl.s8LocalSatisfied == 1)
					{
						gstLocalEOamCtl.s8DiscStatus 		= EOAM_STATUS_DISC_SEND_LOCAL_REMOTE_SUCCESS;
						gstLocalEOamCtl.s8LocalPdu 			= LOCAL_PDU_INFO;
						gstLocalEOamCtl.s8LocalStable 	= 1;
						//pstEOamLayerCtl->s8LocalSatisfied 	= 0;
					}

				break;

				case EOAM_STATUS_DISC_SEND_LOCAL_REMOTE_SUCCESS:	//5
					EOAM_MSG(dprintf("[EOAM_DISCOVERY_TASK] gstLocalEOamCtl.s8LocalSatisfied=%d\n",gstLocalEOamCtl.s8LocalSatisfied));
					EOAM_MSG(dprintf("gstLocalEOamCtl.s8RemoteStable=%d\n",gstLocalEOamCtl.s8RemoteStable));
					if((gstLocalEOamCtl.s8LocalSatisfied == 1) && (gstLocalEOamCtl.s8RemoteStable == 1))
					{
						gstLocalEOamCtl.s8DiscStatus 		= EOAM_STATUS_DISC_SEND_ANY;
						gstLocalEOamCtl.s8LocalPdu 			= LOCAL_PDU_ANY;
					}

					if(gstLocalEOamCtl.s8LocalSatisfied == 0)
					{
						gstLocalEOamCtl.s8DiscStatus 		= EOAM_STATUS_DISC_SEND_LOCAL_REMOTE;
						gstLocalEOamCtl.s8LocalPdu 			= LOCAL_PDU_INFO;
						gstLocalEOamCtl.s8LocalStable 	= 0;
					}

				break;

				case EOAM_STATUS_DISC_SEND_ANY:		//6
					if((gstLocalEOamCtl.s8LocalSatisfied == 1) && (gstLocalEOamCtl.s8RemoteStable == 0))
					{
						gstLocalEOamCtl.s8DiscStatus 		= EOAM_STATUS_DISC_SEND_LOCAL_REMOTE_SUCCESS;
						gstLocalEOamCtl.s8LocalPdu 			= LOCAL_PDU_INFO;
						gstLocalEOamCtl.s8LocalStable 	= 1;
					}

					if(gstLocalEOamCtl.s8LocalSatisfied == 0)
					{
						gstLocalEOamCtl.s8DiscStatus 		= EOAM_STATUS_DISC_SEND_LOCAL_REMOTE;
						gstLocalEOamCtl.s8LocalPdu 			= LOCAL_PDU_INFO;
						gstLocalEOamCtl.s8LocalStable 	= 0;
					}

					if(gstLocalEOamCtl.s8RemoteDTEDiscovered == 0)
					{
						gstLocalEOamCtl.s8RemoteDTEDiscovered = 1;	/* Found Remote DTE */
						EOAM_MSG(dprintf("s8RemoteDTEDiscovered=%d, Found Remote DTE!\n",gstLocalEOamCtl.s8RemoteDTEDiscovered));

#ifdef SUPERTASK
						setDTLog("Entry EOAM-state", NULL, NULL);
#endif
						//VoIPServiceStop();
						//gs32VoIPServiceStopUse = 1;
					}

				break;

				default:
				break;
			}

			EOAM_MSG(dprintf("gstLocalEOamCtl.s8DiscStatus=%d\n",gstLocalEOamCtl.s8DiscStatus));
			EOAM_MSG(dprintf("gstLocalEOamCtl.s8LocalPdu=%d\n",gstLocalEOamCtl.s8LocalPdu));
			EOAM_MSG(dprintf("gstLocalEOamCtl.s8LocalStable=%d\n",gstLocalEOamCtl.s8LocalStable));
			EOAM_MSG(dprintf("gstLocalEOamCtl.s8LocalSatisfied=%d\n",gstLocalEOamCtl.s8LocalSatisfied));
		}

		/* 3) In each state, the OAM sublayer sends specified OAMPDUs once a second. */
		EOAM_MSG(dprintf("%s[EOAM_DISCOVERY_TASK] step 3) Send specified OAMPDUs once a second\n%s",CLR1_32_GREEN,CLR0_RESET));
		EOAM_TX_TASK();
			//goto EOAM_DISCOVERY_FAIL;
		/* 4) If OAM is
			  reset,
			  disabled,
			  the local_lost_link_timer expires
			  the local_link_status equals FAIL,
			  the Discovery process returns to the FAULT state.
		*/
		if(gs8LocalEoamBegin == TRUE)
		{
			gs8LocalEoamMode = gstLocalEOamCtl.s8LocalEOamMode;
			EOAM_MSG(dprintf("gs8LocalEoamBegin=%d,gs8LocalEoamMode=%d, Reset EOAM!\n",gs8LocalEoamBegin,gs8LocalEoamMode));
			goto EOAM_RESET;
		}

		if(gs8LocalEoamEnable == FALSE)
		{
			EOAM_MSG(dprintf("gs8LocalEoamEnable=%d,Kill Task!\n",gs8LocalEoamEnable));
			goto EOAM_DISCOVERY_FAIL;
		}

		if(gstLocalEOamCtl.s8LocalLinkStatus == 1)
		{
			EOAM_MSG(dprintf("%ss8LocalLinkStatus=%d, Reset Discovery state!\n%s",CLR1_33_YELLOW,gstLocalEOamCtl.s8LocalLinkStatus,CLR0_RESET));
			gstLocalEOamCtl.s8DiscStatus = EOAM_STATUS_DISC_FAULT;
		}

		/* Check whether Discovery process is finished */
		if(gstLocalEOamCtl.s8DiscStatus <= EOAM_STATUS_DISC_SEND_ANY)
		{
				EOAM_MSG(dprintf("gstLocalEOamCtl.s8DiscStatus=%d, gs8LocalLostLnkTimeDone=%d, gu64LocalLostLnkTimeSnap=%ld\n",gstLocalEOamCtl.s8DiscStatus,gs8LocalLostLnkTimeDone,gu64LocalLostLnkTimeSnap));
				if((u64SpentTime=GetTime()-gu64LocalLostLnkTimeSnap)>EOAM_LOCAL_LOST_LINK_TIME)
				{
						gstLocalEOamCtl.s8DiscStatus = EOAM_STATUS_DISC_FAULT;
						gs8DiscTimes++;
						EOAM_MSG(dprintf("%su64SpentTime=%ld>%ld, try %d times, Reset Discovery state to %d!\n%s",CLR1_33_YELLOW,u64SpentTime,EOAM_LOCAL_LOST_LINK_TIME,gs8DiscTimes,gstLocalEOamCtl.s8DiscStatus,CLR0_RESET));
				}
				else
				{
						gs8LocalLostLnkTimeDone = 0;
				}

				if(gstLocalEOamCtl.s8DiscStatus < EOAM_STATUS_DISC_SEND_ANY)
				{
						gstLocalEOamCtl.s8RemoteDTEDiscovered = 0;
#ifdef SUPERTASK
								setDTLog("EOAM Discovery continue....", NULL, NULL);
#endif
				}

		}

		if(gs8LocalEoamMode == EOAM_LAYER_LOOPBACK_DTE)
		{
				if(gstLocalEOamCtl.s8LocalEOamLBInited == EOAM_LB_START)
				{
						if((u64SpentTime=GetTime()-gu64LocalLostLnkTimeSnap)>EOAM_LOCAL_LOST_LINK_TIME)
						{
								EOAM_MSG(dprintf("step 9) LoopBack LinkLost time out!! reset!\n"));
								if(gstTempLocalEOamCtl.s8LocalEOamMode == EOAM_ACTIVE_MODE)
								{
										EOAM_INIT(EOAM_LAYER_ACTIVE_DTE);
										gs8LocalEoamMode = EOAM_LAYER_ACTIVE_DTE;
								}
								else
								{
										EOAM_INIT(EOAM_LAYER_PASSIVE_DTE);
										gs8LocalEoamMode = EOAM_LAYER_PASSIVE_DTE;
								}
						}
				}
		}

	}

	return;

EOAM_DISCOVERY_FAIL:
#ifdef SUPERTASK
	if(klltsk(cur_task)<0)
	{
			EOAM_MSG(dprintf("%s[%s]Kill EOAM_DISCOVERY_TASK failed!\n%s",CLR1_31_RED,__FUNCTION__,CLR0_RESET));
	}
	else
	{
			EOAM_MSG(dprintf("%s[%s]Kill EOAM_DISCOVERY_TASK slot=%d\n%s",CLR1_32_GREEN,__FUNCTION__,cur_task,CLR0_RESET));
	}
#endif

	gs8LocalEoamEnable = FALSE;
	gs8LocalEoamBegin  = TRUE;
	gs8LocalEoamMode   = EOAM_LAYER_DISABLE;

	gs8DiscTimes = 0;

#ifdef SUPERTASK
	if_table[gEOAM_Ifno].flag &= ~IFF_EOAM_UP;
#endif

	EOAM_MSG(dprintf("[%s]EOAM ENABLE=%d,BEGIN=%d,MODE=%d\n",__FUNCTION__,gs8LocalEoamEnable,gs8LocalEoamBegin,gs8LocalEoamMode));
#ifdef SUPERTASK
	setDTLog("Disable EOAM-state", NULL, NULL);
#endif

		return;
}

/*****************************************************************
*
*   PROCEDURE NAME:
*   EOAM_LOOPBACK_MODE_ENABLE
*
*   DESCRIPTION:
*		Enable EOAM LoopBack Function API only for ACTIVE mode both local
*		and remote DTE.
*
*   NOTE:
*		MUST had been called EOAM_ENABLE() and Enable EOAM successful!!
*
*		LoopBack Status
*		DISABLE --> START
*		EXIT --> DISABLE
*
*****************************************************************/
const int EOAMAPI_LOOPBACK_ENABLE(char s8Enable)
{
		if(gstLocalEOamCtl.s8RemoteDTEDiscovered == 0)
		{
				EOAM_MSG(dprintf("Discovery unfinished!s8DiscStatus=%d\n",gstLocalEOamCtl.s8DiscStatus));
				return EOAM_ERROR_DISCOVERY_UNFINISHED;
		}

		if(s8Enable == 1)
		{
				if(gstLocalEOamCtl.s8LocalEOamMode == EOAM_ACTIVE_MODE)
				{
						if(gstLocalEOamCtl.s8LocalEOamLoopBack == 1)
						{
								EOAM_MSG(dprintf("Support Loopback function!s8LocalEOamLoopBack=%d,gs8LocalEoamMode=%d\n",gstLocalEOamCtl.s8LocalEOamLoopBack,gs8LocalEoamMode));
								gs8LocalEoamMode = EOAM_LAYER_LOOPBACK_DTE;

								EOAM_MSG(dprintf("s8LocalEOamLBInited=%d,gs8LocalEoamMode=%d\n",gstLocalEOamCtl.s8LocalEOamLBInited,gs8LocalEoamMode));
						}
						else
						{
								EOAM_MSG(dprintf("No support Loopback function!s8LocalEOamLoopBack=%d,gs8LocalEoamMode=%d\n",gstLocalEOamCtl.s8LocalEOamLoopBack,gs8LocalEoamMode));
								return EOAM_ERROR_UNSUPPORT_LB;
						}
				}
				else
				{
						EOAM_MSG(dprintf("Wrong EOAM mode, s8LocalEOamMode=%d\n", gstLocalEOamCtl.s8LocalEOamMode));
				}

		}
		else
		{
				if(gs8LocalEoamMode != EOAM_LAYER_LOOPBACK_DTE)
				{
						EOAM_MSG(dprintf("EOAM isn't under LoopBack mode!\n"));
						return EOAM_ERROR_WRONG_MODE;
				}

				if(gstLocalEOamCtl.s8LocalEOamLBInited == EOAM_LB_START)
				{
						EOAM_MSG(dprintf("Disable LoopBack function!\n"));
						gstLocalEOamCtl.s8LocalEOamLBInited = EOAM_LB_EXIT;
				}
				else
				{
						EOAM_MSG(dprintf("Wrong LoopBack state! s8LocalEOamLBInited=%d\n", gstLocalEOamCtl.s8LocalEOamLBInited));
				}
		}

		return EOAM_SUCCESS;
}

/*****************************************************************
*
*   PROCEDURE NAME:
*   EOAM_ENABLE
*
*   DESCRIPTION:
*		ENABLE/DISABLE EOAM API
*
*   NOTE:
*		s8Enable
*		1: ENABLE, 0: DISABLE
*
*		s8EOamMode
*		1: EOAM_LAYER_PASSIVE_DTE
*		2: EOAM_LAYER_ACTIVE_DTE
*
*****************************************************************/
const int EOAMAPI_ENABLE(char s8Enable, char s8EOamMode)
{
	int s32Slot, s32Status;

	dprintf("%s[%s]%d s8Enable=%d,s8EOamMode=%d\n%s",CLR1_33_YELLOW,__FUNCTION__,HZ,s8Enable,s8EOamMode,CLR0_RESET);

	if(s8Enable == 1)
	{
		if(gs8LocalEoamEnable == TRUE)
		{
			if(gstLocalEOamCtl.s8LocalEOamInited == 1)
			{
				dprintf("[EOAMAPI_ENABLE] Already EOAM Enable!\n");
#ifdef SUPERTASK
				setDTLog("Enable EOAM", NULL, NULL);
#endif
				//VoIPServiceStop();
			}
			else
			{
				dprintf("[EOAMAPI_ENABLE] EOAM init fail!\n");
				//VoIPServiceRestart();
				//uart_tx_onoff(0);
				return EOAM_ERROR_INIT;
			}
		}
		else
		{
			/* Init EOAM */
			if(EOAM_INIT(s8EOamMode))
			{
				gs8LocalEoamEnable = TRUE;
				gs8LocalEoamBegin  = FALSE;
				gs8LocalEoamMode 	 = s8EOamMode;

				gSetting.s8LOCAL_EOAM_MODE	= s8EOamMode;
				gSetting.s8LOCAL_EOAM_BEGIN	= false;
			}
			else
			{
				return EOAM_ERROR_INIT;
			}

			/* RUN EOAM_DISCOVERY_TASK */
#ifdef SUPERTASK
			if((s32Slot=slttsk((void (fAR *)(void))EOAM_DISCOVERY_TASK))<0)
			{
				if((s32Status = runtsk(80, (void(*)(void))EOAM_DISCOVERY_TASK, 8000, s8EOamMode)) < SUCCESS)
#else
				thread_s8EOamMode = s8EOamMode;
				MY_SLOW_PTL_THREAD_ID = OSHAL_Spawn_Thread(EOAM_DISCOVERY_TASK,(void *)&thread_s8EOamMode,"EOAM",0);
				if(MY_SLOW_PTL_THREAD_ID == NULL)
#endif
				{
					dprintf("%s[EOAMAPI_ENABLE] RUNTASK EOAM_DISCOVERY_TASK fails\n%s",CLR1_31_RED,CLR0_RESET);
					gSetting.s8LOCAL_EOAM_MODE  = EOAM_LAYER_DISABLE;
					gSetting.s8LOCAL_EOAM_BEGIN = TRUE;

					memset(&gstLocalEOamCtl,0,sizeof(gstLocalEOamCtl));

					gs8LocalEoamEnable = FALSE;
					gs8LocalEoamBegin  = TRUE;
					gs8LocalEoamMode   = EOAM_LAYER_DISABLE;

					gs8DiscTimes = 0;

					return EOAM_ERROR_TASK_INIT;
				}
				else
				{
					dprintf("%s[EOAMAPI_ENABLE] RUNTASK EOAM_DISCOVERY_TASK ...\n%s",CLR1_32_GREEN,CLR0_RESET);
#ifdef SUPERTASK
					setDTLog("Enable EOAM-state", NULL, NULL);
#endif
					//VoIPServiceStop();
				}
#ifdef SUPERTASK
			}
			else
			{
				dprintf("[%s]EOAM ENABLE=%d,BEGIN=%d,MODE=%d\n",__FUNCTION__,gs8LocalEoamEnable,gs8LocalEoamBegin,gs8LocalEoamMode);
				//uart_tx_onoff(0);
				//VoIPServiceRestart();

				return EOAM_FAIL;
			}
#endif
		}
#ifndef SUPERTASK
		eoam_notifier_wrapper();
#endif
	}
	else
	{
		/* KILL EOAM_DISCOVERY_TASK */
		if(gs8LocalEoamEnable == TRUE)
		{
#ifdef SUPERTASK
			if((s32Slot=slttsk((void (fAR *)(void))EOAM_DISCOVERY_TASK))>0)
#else
			if(MY_SLOW_PTL_THREAD_ID != NULL)
#endif
			{
#ifdef SUPERTASK
				if(klltsk(s32Slot)<0)
				{
					dprintf("%s[%s]Kill EOAM_DISCOVERY_TASK failed!\n%s",CLR1_31_RED,__FUNCTION__,CLR0_RESET);
					//uart_tx_onoff(0);

					return EOAM_FAIL;
				}
#else
				OSHAL_Kill_Thread(MY_SLOW_PTL_THREAD_ID);
				MY_SLOW_PTL_THREAD_ID = NULL;
#endif

#ifdef SUPERTASK
				dprintf("%s[%s]Kill EOAM_DISCOVERY_TASK slot=%d\n%s",CLR1_32_GREEN,__FUNCTION__,s32Slot,CLR0_RESET);
#else
				dprintf("%s[%s]Kill EOAM_DISCOVERY_TASK\n%s",CLR1_32_GREEN,__FUNCTION__,CLR0_RESET);
#endif

				memset(&gstLocalEOamCtl,0,sizeof(gstLocalEOamCtl));

				gs8LocalEoamEnable = FALSE;
				gs8LocalEoamBegin  = TRUE;
				gs8LocalEoamMode   = EOAM_LAYER_DISABLE;

				gs8DiscTimes = 0;

#ifdef SUPERTASK
				if_table[gEOAM_Ifno].flag &= ~IFF_EOAM_UP;
#endif
			}

			dprintf("[%s]EOAM ENABLE=%d,BEGIN=%d,MODE=%d\n",__FUNCTION__,gs8LocalEoamEnable,gs8LocalEoamBegin,gs8LocalEoamMode);
			//setDTLog("Leave EOAM-state", NULL, NULL);
#ifdef SUPERTASK
			setDTLog("Disable EOAM", NULL, NULL);
#endif
			//VoIPServiceRestart();
		}
		else
		{
			dprintf("Already EOAM Disnable!\n");
#ifdef SUPERTASK
			setDTLog("Disable EOAM", NULL, NULL);
#endif
			//VoIPServiceRestart();
		}

	}

	return EOAM_SUCCESS;
}


/*****************************************************************
*
*   PROCEDURE NAME:
*   EOAMAPI_GET_EOAM_STATUS
*
*   DESCRIPTION:
*		Check EOAM enable/disable
*
*		RETURN:
*		0: Disable, 1: Enable
*
*
*   NOTE:
*
*****************************************************************/
const int EOAMAPI_GET_EOAM_STATUS(void)
{
		return gs8LocalEoamEnable;
}


/*****************************************************************
*
*   PROCEDURE NAME:
*   EOAMAPI_GET_EOAM_MODE
*
*   DESCRIPTION:
*		Get EOAM mode
*
*		RETURN:
*		0: EOAM_LAYER_DISABLE
*		1: EOAM_LAYER_PASSIVE_DTE
*		2: EOAM_LAYER_ACTIVE_DTE
*		3: EOAM_LAYER_LOOPBACK_DTE
*
*   NOTE:
*
*****************************************************************/
const int EOAMAPI_GET_EOAM_MODE(void)
{
		return gs8LocalEoamMode;
}

/*****************************************************************
*
*   PROCEDURE NAME:
*   EOAMAPI_GET_EOAM_MODE_INNER
*
*   DESCRIPTION:
*		Get EOAM mode inner
*
*		RETURN:
*		1: EOAM_PASSIVE_MODE
*		2: EOAM_ACTIVE_MODE
*
*   NOTE:
*
*****************************************************************/
const int EOAMAPI_GET_EOAM_MODE_INNER(void)
{
		if(gs8LocalEoamEnable == 0)
		{
				EOAM_MSG(dprintf("EOAM not Enable!\n"));
				return EOAMAPI_FAIL;
		}

		return gstTempLocalEOamCtl.s8LocalEOamMode;

}

/*****************************************************************
*
*   PROCEDURE NAME:
*   EOAMAPI_GET_FUNCTIONALITY_STRING
*
*   DESCRIPTION:
*
*		s32EoamFunc
*		EOAM_FUNC_LINKFAULT				0
*		EOAM_FUNC_DYINGGASP				1
*		EOAM_FUNC_CRITICALEVENT		2
*		EOAM_FUNC_UNIDIRECTIONAL	3
*		EOAM_FUNC_LOOPBACK				4
*		EOAM_FUNC_LINKEVENT				5
*		EOAM_FUNC_VARIRETRIEVAL		6
*
*		RETURN:
*		0 : NOT SUPPORT,  1 : SUPPORT, -1: ERROR
*
*   NOTE:
*
*****************************************************************/
const int EOAMAPI_GET_FUNCTIONALITY(int s32EoamFunc)
{
		if(s32EoamFunc >= EOAM_SUPPORT_FUNC_NO)
		{
				EOAM_MSG(dprintf("Wrong Function number!\n"));
				return -1;
		}

		return gs8LocalEoamFunctionality[s32EoamFunc];
}

/*****************************************************************
*
*   PROCEDURE NAME:
*   EOAMAPI_GET_EOAM_EVENT
*
*   DESCRIPTION:
*		Get EOAM events, Flags field
*
*		s8LocalRemoteDTE
*		1: EOAM_LOCAL_DTE
*		2: EOAM_REMOTE_DTE
*
*		RETURN:
*		0 : NOT SUPPORT,  1 : SUPPORT, -1: ERROR
*
*   NOTE:
*
*****************************************************************/
const short EOAMAPI_GET_EOAM_EVENT(char s8LocalRemoteDTE)
{
		if(s8LocalRemoteDTE == EOAM_LOCAL_DTE)
		{
				if(gs8LocalEoamEnable)
				{
						if(gstLocalEOamCtl.s8DiscStatus != EOAM_STATUS_DISC_PASSIVE_WAIT)
						{
								return gstLocalEOamCtl.s16LocalFlags;
						}
						else
						{
								return EOAMAPI_FAIL;
						}
				}
				else
				{
						return EOAMAPI_FAIL;
				}
		}
		else if(s8LocalRemoteDTE == EOAM_REMOTE_DTE)
		{
				if(gs8LocalEoamEnable)
				{
						return gstLocalEOamCtl.s16RMTFlags;
				}
				else
				{
						return EOAMAPI_FAIL;
				}
		}
		else
		{
				return EOAMAPI_FAIL;
		}

}

/*****************************************************************
*
*   PROCEDURE NAME:
*   EOAMAPI_GET_LB_CONNECT
*
*   DESCRIPTION:
*		Get EOAM LoopBack Connect/Disconnect
*
*		s8LocalRemoteDTE
*		1: EOAM_LOCAL_DTE
*		2: EOAM_REMOTE_DTE
*
*		RETURN:
*		0 : NOT SUPPORT,  1 : SUPPORT, -1: ERROR
*
*   NOTE:
*
*****************************************************************/
const char *EOAMAPI_GET_LB_CONNECT(void)
{
		if(gs8LocalEoamEnable == 0)
		{
				EOAM_MSG(dprintf("EOAM not Enable!\n"));
				return STRING_LB_DISCONNECTION;
		}

		if(gs8LocalEoamMode == EOAM_LAYER_LOOPBACK_DTE)
		{
				//if(gstLocalEOamCtl.s8LocalEOamMode == EOAM_PASSIVE_MODE)
				//{
				if((gstLocalEOamCtl.s8LocalEOamLBInited == EOAM_LB_START_INPROGRESS)||\
					(gstLocalEOamCtl.s8LocalEOamLBInited == EOAM_LB_EXIT_INPROGRESS))
				{
						return STRING_LB_INPROGRESS;
				}
				else if(gstLocalEOamCtl.s8LocalEOamLBInited == EOAM_LB_START)
				{
						return STRING_LB_CONNECTION;
				}
				else
				{
						return STRING_LB_DISCONNECTION;
				}
				//}
				//else if(gstLocalEOamCtl.s8LocalEOamMode == EOAM_ACTIVE_MODE)
				//{
				//}
				//else
				//{
				//		return STRING_LB_DISCONNECTION;
				//}
		}
		else
		{
				return STRING_LB_DISCONNECTION;
		}

}

/*****************************************************************
*
*   PROCEDURE NAME:
*   EOAMAPI_GET_OPERATIONAL_STATUS_STR
*
*   DESCRIPTION:
*		Get EOAM Discovery Status string
*
*		RETURN:
*		DISABLE
*		LINK_FAULT
*		PASSIVE_WAIT
*		ACTIVE_SEND_LOCAL
*		SEND_LOCAL_REMOTE
*		SEND_LOCAL_REMOTE_OK
*		OPERATIONAL
*		UNKNOWN_STATUS
*
*   NOTE:
*
*****************************************************************/
const char *EOAMAPI_GET_OPERATIONAL_STATUS_STR(void)
{
		if(gstLocalEOamCtl.s8LocalEOamInited == 0)
		{
				EOAM_MSG(dprintf("EOAM not INIT!\n"));

				return STRING_DISABLE;
		}


		switch(gstLocalEOamCtl.s8DiscStatus)
		{
				case EOAM_STATUS_DISC_FAULT:
							return STRING_LINKFAULT;
							break;

				case EOAM_STATUS_DISC_PASSIVE_WAIT:
							return STRING_PASSIVE_WAIT;
							break;

				case EOAM_STATUS_DISC_ACTIVE_SEND_LOCAL:
							return STRING_ACTIVE_SEND_LOCAL;
							break;

				case EOAM_STATUS_DISC_SEND_LOCAL_REMOTE:
							return STRING_SEND_LOCAL_REMOTE;
							break;

				case EOAM_STATUS_DISC_SEND_LOCAL_REMOTE_SUCCESS:
							return STRING_SEND_LOCAL_REMOT_OK;
							break;

				case EOAM_STATUS_DISC_SEND_ANY:
							return STRING_OPERATIONAL;
							break;

				default:
							return STRING_UNKNOW_STATUS;
							EOAM_MSG(dprintf("Unknown Status!\n"));
							break;

		}

		return STRING_UNKNOW_STATUS;
}

/*****************************************************************
*
*   PROCEDURE NAME:
*   EOAMAPI_GET_LINKFAULT_EVT
*
*   DESCRIPTION:
*		Get EOAM link fault event string
*
*		RETURN:
*		REMOTE_DTE
*		LOCAL_DTE
*		NONE
*
*   NOTE:
*
*****************************************************************/
const char *EOAMAPI_GET_LINKFAULT_STR(void)
{
		if(gstLocalEOamCtl.s8LocalEOamInited == 0)
		{
				EOAM_MSG(dprintf("EOAM not INIT!\n"));
				return LINK_FAULT_NONE_STR;
		}

		if(gstLocalEOamCtl.s8LocalLinkStatus == 1)
		{
				return LINK_FAULT_LOCAL_DTE_STR;
		}
		else
		{
				return LINK_FAULT_NONE_STR;
		}


		return LINK_FAULT_NONE_STR;
}

/*****************************************************************
*
*   PROCEDURE NAME:
*   EOAMAPI_GET_REMOTE_MODE_STR
*
*   DESCRIPTION:
*		Get Remote EOAM mode string
*
*		RETURN:
*		ACTIVE
*		PASSIVE
*		NONE
*
*   NOTE:
*
*****************************************************************/
const char *EOAMAPI_GET_REMOTE_MODE_STR(void)
{
		if(gs8LocalEoamEnable == 0)
		{
				EOAM_MSG(dprintf("EOAM not INIT!\n"));
				return STRING_NONE;
		}

		if(gstLocalEOamCtl.s8RemoteStateValid == 1)
		{
				if(gstLocalEOamCtl.stRMTLocalInfoTlv.s8InfoOamCfg&INFO_TLV_CONF_ACTIVE)
				{
						EOAM_MSG(dprintf("Remote EOAM DTE in Active mode!\n"));
						return STRING_ACTIVE_MODE;
				}
				else
				{
						EOAM_MSG(dprintf("Remote EOAM DTE in Passive mode!\n"));
						return STRING_ACTIVE_MODE;
				}
		}
		else
		{
				EOAM_MSG(dprintf("Remote EOAM DTE NOT avaliable!\n"));
				return STRING_NONE;
		}

		return STRING_NONE;
}

/*****************************************************************
*
*   PROCEDURE NAME:
*   EOAMAPI_GET_REMOTE_INFO
*
*   DESCRIPTION:
*		Get Remote EOAM mode string
*
*
*   NOTE:
*
*****************************************************************/
const int EOAMAPI_GET_REMOTE_INFO(int s32RemoteFunc)
{
	short nTmpShortVal = 0;

		if(gs8LocalEoamEnable == 0)
		{
				EOAM_MSG(dprintf("EOAM not INIT!\n"));
				if((s32RemoteFunc == EOAM_REMOTE_MAC) || (s32RemoteFunc == EOAM_REMOTE_OUI))
						return 0;
				else
						return EOAMAPI_FAIL;
		}

		if(gstLocalEOamCtl.s8RemoteStateValid == 1)
		{
				switch(s32RemoteFunc)
				{
						case EOAM_REMOTE_MODE:
									if(gstLocalEOamCtl.stRMTLocalInfoTlv.s8InfoOamCfg&INFO_TLV_CONF_ACTIVE)
									{
											EOAM_MSG(dprintf("Remote EOAM DTE in Active mode!\n"));
											return (int)INFO_TLV_CONF_ACTIVE;
									}
									else
									{
											EOAM_MSG(dprintf("Remote EOAM DTE in Passive mode!\n"));
											return 0;
									}

									break;

						case EOAM_REMOTE_MAC:
									return &gstLocalEOamCtl.s8RMTDTEAddr[0];
									break;

						case EOAM_REMOTE_OUI:
									return &gstLocalEOamCtl.stRMTLocalInfoTlv.s8LocalDTEOUI[0];
									break;

						case EOAM_REMOTE_VERSION:

								 	return (int)gstLocalEOamCtl.stRMTLocalInfoTlv.s8OAMVer;
									break;

						case EOAM_REMOTE_MAX_SIZE:
#if 1 // bitonic
									memcpy((unsigned char *)&nTmpShortVal, gstLocalEOamCtl.stRMTLocalInfoTlv.s8InfoOampduCfg, 2);
									return (int)ntohs(nTmpShortVal);
#else
									return (int)((gstLocalEOamCtl.stRMTLocalInfoTlv.s8InfoOampduCfg[0]<<0x8) |\
															 (gstLocalEOamCtl.stRMTLocalInfoTlv.s8InfoOampduCfg[1]));
#endif
									break;

						case EOAM_REMOTE_VENDOR:

									return (gstLocalEOamCtl.stRMTLocalInfoTlv.stInfoTlvVenderInfo.s16VendorDTEType<<0x10) ||\
												 (gstLocalEOamCtl.stRMTLocalInfoTlv.stInfoTlvVenderInfo.s16VendorSWRev);
									break;

						case EOAM_REMOTE_PAR_ACT:

									return (int)gstLocalEOamCtl.stRMTLocalInfoTlv.s8InfoState&INFO_TLV_STATE_PARSER;
									break;

						case EOAM_REMOTE_MUX_ACT:

									return (int)((gstLocalEOamCtl.stRMTLocalInfoTlv.s8InfoState&INFO_TLV_STATE_MUX)>>0x2);
									break;

						case EOAM_REMOTE_FUNC_UNIDIRECTIONAL:

									return (int)((gstLocalEOamCtl.stRMTLocalInfoTlv.s8InfoOamCfg&INFO_TLV_CONF_UNIDIRC)>>0x1);
									break;

						case EOAM_REMOTE_FUNC_LOOPBACK:

									return (int)((gstLocalEOamCtl.stRMTLocalInfoTlv.s8InfoOamCfg&INFO_TLV_CONF_OAMRMLB)>>0x2);
									break;

						case EOAM_REMOTE_FUNC_LINKEVENT:

									return (int)((gstLocalEOamCtl.stRMTLocalInfoTlv.s8InfoOamCfg&INFO_TLV_CONF_LINK)>>0x3);
									break;

						case EOAM_REMOTE_FUNC_VARIRETRIEVAL:

									return (int)((gstLocalEOamCtl.stRMTLocalInfoTlv.s8InfoOamCfg&INFO_TLV_CONF_VARRESP)>>0x4);
									break;

						default:

									EOAM_MSG(dprintf("Unknown Status!\n"));
									break;
				}

		}
		else
		{
				EOAM_MSG(dprintf("Remote EOAM DTE NOT avaliable!\n"));
		}

		if((s32RemoteFunc == EOAM_REMOTE_MAC) || (s32RemoteFunc == EOAM_REMOTE_OUI))
				return 0;
		else
				return EOAMAPI_FAIL;
}

/*****************************************************************
*
*   PROCEDURE NAME:
*   EOAMAPI_CONSOLE_EOAM_INFO
*
*   DESCRIPTION:
*		Show EOAM info for supertask! console debug
*
*		0: CONSOLE_DBG_MAIN_PAGE
*		1: CONSOLE_DBG_STATISTICS
*		2: CONSOLE_DBG_LOOPBACK
*
*
*   NOTE:
*
*****************************************************************/
const int EOAMAPI_CONSOLE_EOAM_INFO(char s8PageNo)
{
		int i=0, j;

		char s8LinkFault;
		char s8ParAction, s8MuxAction;
		short s16Flags;

		int s32RetVal;

		char *ps8RemoteMac, *ps8RemoteOUI;

		//uart_tx_onoff(1);
		switch(s8PageNo)
		{
				case CONSOLE_DBG_MAIN_PAGE:
						dprintf("EOAM Main Page ==============================\n");
						/* EOAM INTERFACE ============================================= */
						dprintf("%s[ EOAM INTERFACE ]\n%s",CLR1_33_YELLOW,CLR0_RESET);
						/* Functionality */
						dprintf("\n%s : \n", TITLE_FUNCTIONALITY_STR);
						for(i=0; i<EOAM_SUPPORT_FUNC_NO; i++)
						{
								dprintf("%d. %s  : ",i,gs8LocalEoamFunctionalityStr[i]);
								if(EOAMAPI_GET_FUNCTIONALITY(i) == 0)
								{
										dprintf("%s\n", STRING_DISABLE);
								}
								else
								{
										dprintf("%s\n", STRING_ENABLE);
								}
						}


						dprintf("\n");

						/* Mode */
						dprintf("%s : ", TITLE_LOCAL_MODE_STR);
						if(EOAMAPI_GET_EOAM_MODE() == EOAM_LAYER_DISABLE)
						{
								dprintf("%s\n", STRING_DISABLE);
						}
						else if(EOAMAPI_GET_EOAM_MODE() == EOAM_LAYER_PASSIVE_DTE)
						{
								dprintf("%s\n", STRING_PASSIVE_MODE);
						}
						else if(EOAMAPI_GET_EOAM_MODE() == EOAM_LAYER_ACTIVE_DTE)
						{
								dprintf("%s\n", STRING_ACTIVE_MODE);
						}
						else if(EOAMAPI_GET_EOAM_MODE() == EOAM_LAYER_LOOPBACK_DTE)
						{
								dprintf("%s\n", STRING_LOOPBACK_MODE);
						}
						else
						{
								dprintf("%s\n", STRING_DISABLE);
						}

						/* Status */
						dprintf("%s : ", TITLE_STATUS_STR);
						if(EOAMAPI_GET_EOAM_STATUS() == 0)
						{
								dprintf("%s\n", STRING_DISABLE);
						}
						else
						{
								dprintf("%s\n", STRING_ENABLE);
						}

						/* EOAM INFORMATION ============================================= */
						dprintf("%s\n[ EOAM INFORMATION ]\n%s",CLR1_33_YELLOW,CLR0_RESET);
						/* Local Information */
						dprintf("\n%s : \n", TITLE_LOCAL_INFORMATION_STR);

						/* Operational Status */
						dprintf("%s : ", TITLE_OPERATIONAL_STATUS_STR);
						if(EOAMAPI_GET_EOAM_STATUS() == 0)
						{
								dprintf("%s\n", STRING_DISABLE);
						}
						else
						{
								dprintf("%s\n",EOAMAPI_GET_OPERATIONAL_STATUS_STR());
						}

						/* Link Fault DTE */
						dprintf("%s : ", TITLE_LINK_FAULT_STR);
						if(EOAMAPI_GET_EOAM_STATUS() == 0)
						{
								dprintf("%s\n", LINK_FAULT_NONE_STR);
						}
						else
						{
								if((s16Flags = EOAMAPI_GET_EOAM_EVENT(EOAM_LOCAL_DTE)) != EOAMAPI_FAIL)
								{
										if(s16Flags&EOAMPDU_FLAGS_LINKFAULT)
										{
												s8LinkFault |= BIT00;
										}
										else
										{
												s8LinkFault &= ~BIT00;
										}
								}
								else
								{
										s8LinkFault &= ~BIT00;
								}

								if((s16Flags = EOAMAPI_GET_EOAM_EVENT(EOAM_REMOTE_DTE)) != EOAMAPI_FAIL)
								{
										if(s16Flags&EOAMPDU_FLAGS_LINKFAULT)
										{
												s8LinkFault |= BIT01;
										}
										else
										{
												s8LinkFault &= ~BIT01;
										}
								}
								else
								{
										s8LinkFault &= ~BIT01;
								}

								if(s8LinkFault == BIT00)
								{
										dprintf("%s\n", LINK_FAULT_LOCAL_DTE_STR);
								}
								else if(s8LinkFault == BIT01)
								{
										dprintf("%s\n", LINK_FAULT_REMOTE_DTE_STR);
								}
								else if(s8LinkFault == (BIT00+BIT01))
								{
										dprintf("%s\n", LINK_FAULT_LOCAL_REMOTE_STR);
								}
								else
								{
										dprintf("%s\n", LINK_FAULT_NONE_STR);
								}

						}

						/* Remote Information */
						dprintf("\n%s : \n", TITLE_REMOTE_INFORMATION_STR);
						/* Mode */
						dprintf("%s : %s\n", TITLE_RMT_MODE_STR, EOAMAPI_GET_REMOTE_MODE_STR());

						/* MAC Address */
						if((ps8RemoteMac = EOAMAPI_GET_REMOTE_INFO(EOAM_REMOTE_MAC)) != NULL)
						{
								dprintf("%s : %02X:%02X:%02X:%02X:%02X:%02X\n", TITLE_RMT_MAC_ADDR_STR, \
									ps8RemoteMac[0],ps8RemoteMac[1],ps8RemoteMac[2],ps8RemoteMac[3],ps8RemoteMac[4],ps8RemoteMac[5]);
						}
						else
						{
								dprintf("%s : %02X:%02X:%02X:%02X:%02X:%02X\n", TITLE_RMT_MAC_ADDR_STR, 0,0,0,0,0,0);
						}

						/* OUI */
						if((ps8RemoteOUI = EOAMAPI_GET_REMOTE_INFO(EOAM_REMOTE_OUI)) != NULL)
						{
								dprintf("%s : %02X:%02X:%02X\n", TITLE_RMT_OUI_STR, ps8RemoteOUI[0],ps8RemoteOUI[1],ps8RemoteOUI[2]);
						}
						else
						{
								dprintf("%s : %02X:%02X:%02X\n", TITLE_RMT_OUI_STR, 0,0,0);
						}

						/* Functionality */
						dprintf("%s : \n", TITLE_FUNCTIONALITY_STR);
						j=EOAM_REMOTE_FUNC_UNIDIRECTIONAL;
						for (i=0; i<4; i++)
						{
								if((s32RetVal = EOAMAPI_GET_REMOTE_INFO(j)) != EOAMAPI_FAIL)
								{
										if(s32RetVal)
										{
												dprintf("%d. %s  : %s\n",i,gs8LocalEoamFunctionalityStr[3+i], STRING_ENABLE);
										}
										else
										{
												dprintf("%d. %s  : %s\n",i,gs8LocalEoamFunctionalityStr[3+i], STRING_DISABLE);
										}
								}
								else
								{
										dprintf("%d. %s  : %s\n",i,gs8LocalEoamFunctionalityStr[3+i], STRING_DISABLE);
								}

								j++;

						}

						/* Version */
						dprintf("\n%s : 0x%X\n", TITLE_RMT_VERSION_STR, EOAMAPI_GET_REMOTE_INFO(EOAM_REMOTE_VERSION));
						/* Max OAMPDU Size */
						dprintf("%s : 0x%X\n", TITLE_RMT_MAX_OAMPDU_SIZE_STR, EOAMAPI_GET_REMOTE_INFO(EOAM_REMOTE_MAX_SIZE));
						/* Vendor Information */
						dprintf("%s : 0x%X\n", TITLE_RMT_VENDOR_SPEC_INFO_STR, EOAMAPI_GET_REMOTE_INFO(EOAM_REMOTE_VENDOR));
						/* Parser Action */
						dprintf("\n%s : ", TITLE_RMT_PAR_ACTION_STR);
						if((s8ParAction = EOAMAPI_GET_REMOTE_INFO(EOAM_REMOTE_PAR_ACT)) == REMOTE_PARSER_FWD)
						{
								dprintf("%s\n", STRING_PARSER_FWD);
						}
						else if((s8ParAction = EOAMAPI_GET_REMOTE_INFO(EOAM_REMOTE_PAR_ACT)) == REMOTE_PARSER_LB)
						{
								dprintf("%s\n", STRING_PARSER_LB);
						}
						else if((s8ParAction = EOAMAPI_GET_REMOTE_INFO(EOAM_REMOTE_PAR_ACT)) == REMOTE_PARSER_DISCARD)
						{
								dprintf("%s\n", STRING_PARSER_DISCARD);
						}
						else
						{
								dprintf("%s\n", STRING_PARSER_RESERVED);
						}

						/* Multiplexer Action */
						dprintf("%s : ", TITLE_RMT_MUX_ACTION_STR);
						if((s8ParAction = EOAMAPI_GET_REMOTE_INFO(EOAM_REMOTE_MUX_ACT)) == REMOTE_MUX_FWD)
						{
								dprintf("%s\n", STRING_MUX_FWD);
						}
						else if((s8ParAction = EOAMAPI_GET_REMOTE_INFO(EOAM_REMOTE_MUX_ACT)) == REMOTE_MUX_DISCARD)
						{
								dprintf("%s\n", STRING_MUX_DISCARD);
						}
						else
						{
								dprintf("%s\n", STRING_MUX_RESERVED);
						}

						dprintf("EOAM Main Page ==============================\n");
						break;

				case CONSOLE_DBG_STATISTICS:

						break;

				case CONSOLE_DBG_LOOPBACK:
						dprintf("EOAM LoopBack Page ==============================\n");
						/* EOAM LOOPBACK ============================================= */
						dprintf("%s\n[ EOAM LOOPBACK ]\n%s",CLR1_33_YELLOW,CLR0_RESET);
						/* LoopBack Connection */
						if(EOAMAPI_GET_EOAM_MODE_INNER() == EOAM_ACTIVE_MODE)
						{
								dprintf("\n%s : \n", TITLE_LOOOPBACK_START_STR);
								dprintf("\n%s : %s\n", TITLE_LOOOPBACK_CONNECTION_STR, EOAMAPI_GET_LB_CONNECT());
						}
						else
						{
								dprintf("\n%s : %s\n", TITLE_LOOOPBACK_CONNECTION_STR, EOAMAPI_GET_LB_CONNECT());
						}

						/* LoopBack Parameters */
						dprintf("\n%s : (Only for ACTIVE mode)\n", TITLE_LOOOPBACK_PARAMETER_STR);

						/* LoopBack Parameters */
						dprintf("\n%s : \n", TITLE_LOOOPBACK_INFORMATION_STR);
						dprintf("\n\n\n\n\n");

						dprintf("EOAM LoopBack Page ==============================\n");
						break;

				default:
						dprintf("Page NOT avaliable!\n");

						break;
		}



		//uart_tx_onoff(0);

		return EOAM_SUCCESS;
}

const int EOAMAPI_GET_EOAM_DEBUG(void)
{
		return gEOAMEnableDebug;
}
#if 0 // bitonic
/*****************************************************************
*
*   PROCEDURE NAME:
*   TEST_SEND_TLV_LOCALINFO
*
*   DESCRIPTION:
*
*
*   NOTE:
*
*****************************************************************/
void TEST_SEND_TLV_LOCALINFO(void)
{
	char *ps8PktBuff;
	stLOCAL_INFO_TLV *pstLocalInfoTlv;

	static short s16Revision=0;

	char s8SubType;
	short s16Flags=0;
	char s8Code;

	int s32FramChkSeq = 0xAAAAAAAA;	// default value


	EOAM_MSG(dprintf("%s[%s]\n%s",CLR1_33_YELLOW,__FUNCTION__,CLR0_RESET));

	while(1)
	{
		/* Generate datagram */
		if((ps8PktBuff=GetBuffer(BUFSZ1)) == NULL)
		{
			dprintf("get buffer[%d] fail!\n",BUFSZ1);
			return;
		}

		SetBufferLength(ps8PktBuff, ETHER_FRAME_MIN_LEN);
		memset(BUF_BUFFER_PTR(ps8PktBuff),0,ETHER_FRAME_MIN_LEN);

		pstLocalInfoTlv = (stLOCAL_INFO_TLV *)(BUF_BUFFER_PTR(ps8PktBuff)+ETHER_HDR_LEN+SLOW_HDR_LEN);

		/* stLocalInfoTlv ============================== */
		pstLocalInfoTlv->s8InfoType = INFO_TLV_LOCAL_INFO;
		pstLocalInfoTlv->s8InfoLeng = INFO_TLV_LEN;

		pstLocalInfoTlv->s8OAMVer  	= 0x01;

		pstLocalInfoTlv->s8Rev[0]	= ((s16Revision++)>>8)&0xFF;
		pstLocalInfoTlv->s8Rev[1]	= s16Revision&0xFF;
		//dprintf("s8Rev=%02x,%02x, s16Revision=0x%04x\n",pstLocalInfoTlv->s8Rev[0],pstLocalInfoTlv->s8Rev[1],s16Revision);

		pstLocalInfoTlv->s8InfoState = (INFO_TLV_PARSER_FWD+(INFO_TLV_MUX_FWD<<INFO_TLV_STATE_PARSER));
		//dprintf("s8InfoState=%02x\n",pstLocalInfoTlv->s8InfoState);

		pstLocalInfoTlv->s8InfoOamCfg = (INFO_TLV_CONF_ACTIVE+INFO_TLV_CONF_UNIDIRC+INFO_TLV_CONF_OAMRMLB);
		//dprintf("s8InfoOamCfg=%02x\n",pstLocalInfoTlv->s8InfoOamCfg);

		pstLocalInfoTlv->s8InfoOampduCfg[0] = 0x02;
		pstLocalInfoTlv->s8InfoOampduCfg[1] = 0x00;
		//dprintf("s8InfoOampduCfg=%02x,%02x\n",pstLocalInfoTlv->s8InfoOampduCfg[0],pstLocalInfoTlv->s8InfoOampduCfg[1]);

		pstLocalInfoTlv->s8LocalDTEOUI[0] = if_table[T_LAN_INT].macaddr[0];
		pstLocalInfoTlv->s8LocalDTEOUI[1] = if_table[T_LAN_INT].macaddr[1];
		pstLocalInfoTlv->s8LocalDTEOUI[2] = if_table[T_LAN_INT].macaddr[2];

		pstLocalInfoTlv->stInfoTlvVenderInfo.s16VendorDTEType  	= 0;
		pstLocalInfoTlv->stInfoTlvVenderInfo.s16VendorSWRev		= 0;
		//DumpTLV_Infomation(BUF_BUFFER_PTR(ps8PktBuff)+ETHER_HDR_LEN+SLOW_HDR_LEN);

		/* OAMPDU ============================== */
		s8SubType = SUBTYPE_EOAM;
		s16Flags |= EOAMPDU_FLAGS_LOCALSTABLE;
		s8Code = CODE_INFO;

		EOAMPDU_Request(s8SubType,s16Flags,s8Code,ps8PktBuff);
		EOAMI_Request((char*)&SLOW_PTL_MULTIMAC[0],if_table[T_LAN_INT].macaddr,s32FramChkSeq,ps8PktBuff);

		DumpBuffer_EOAM(BUF_BUFFER_PTR(ps8PktBuff),GetBufferLength(ps8PktBuff),CLR1_37_WHITE);

		/* Send Packet */
		if_table[T_LAN_INT].driverp->send(T_LAN_INT,ps8PktBuff,GetBufferLength(ps8PktBuff));

		dlytsk(cur_task, DLY_SECS, 10);
	}

	return;
}


/*****************************************************************
*
*   PROCEDURE NAME:
*   TEST_SEND_TLV_LOCALINFO_TASK
*
*   DESCRIPTION:
*
*
*   NOTE:
*
*****************************************************************/
int TEST_SEND_TLV_LOCALINFO_TASK(void)
{
	int s32Status;

	//uart_tx_onoff(1);
	EOAM_MSG(dprintf("%s[%s]\n%s",CLR1_32_GREEN,__FUNCTION__,CLR0_RESET));

	if ((s32Status = runtsk(80, (void(*)(void))TEST_SEND_TLV_LOCALINFO, 8000)) < SUCCESS)
	{
		dprintf("%s1) RUNTASK TEST_SEND_TLV_LOCALINFO TASK fails %d\n%s", CLR1_31_RED,s32Status,CLR0_RESET);
		return EOAM_FAIL;
	}else
		dprintf("%s1) RUNTASK id=%d TEST_SEND_TLV_LOCALINFO TASK...\n%s", CLR1_32_GREEN,s32Status,CLR0_RESET);
	//uart_tx_onoff(0);

	return EOAM_SUCCESS;
}
#endif

/*
typedef enum {
		EOAM_LAYER_DISABLE = 0,		//0
		EOAM_LAYER_PASSIVE_DTE,		//1
		EOAM_LAYER_ACTIVE_DTE,		//2
		EOAM_LAYER_LOOPBACK_DTE,	//3
}enEOAM_LAYER_MODE;
*/

/*****************************************************************
*
*   PROCEDURE NAME:
*   TEST_DISCOVERY_TASK
*
*   DESCRIPTION:
*	Test OAM Discovery Process in Transmitt Platform
*
*	s8EOamMode
*	1: EOAM_LAYER_PASSIVE_DTE
*	2: EOAM_LAYER_ACTIVE_DTE
*
*   NOTE:
*	1) SHOULD MASK EOAM_DISCOVERY_TASK in soho.c or use klltsk
*	2)
*
*****************************************************************/
int TEST_DISCOVERY_TASK(char s8EOamMode)
{
	int s32Slot, s32Status;

	EOAM_MSG(dprintf("%s[%s]....\n%s",CLR1_32_GREEN,__FUNCTION__,CLR0_RESET));

	/* 1) KILL TASK EOAM_DISCOVERY_TASK */
#ifdef SUPERTASK
	if((s32Slot=slttsk((void (fAR *)(void))EOAM_DISCOVERY_TASK))<0)	dprintf("%s[%s]Get EOAM_DISCOVERY_TASK slot(%d) failed!\n%s",CLR1_31_RED,__FUNCTION__,s32Slot,CLR0_RESET);
	else
	{
		dprintf("%s[%s]Got EOAM_DISCOVERY_TASK slot=%d\n%s",CLR1_32_GREEN,__FUNCTION__,s32Slot,CLR0_RESET);
		if(klltsk(s32Slot)<0)
		{
			dprintf("%s[%s]Kill EOAM_DISCOVERY_TASK failed!\n%s",CLR1_31_RED,__FUNCTION__,CLR0_RESET);
			return EOAM_FAIL;
		}else
			dprintf("%s[%s]Kill EOAM_DISCOVERY_TASK slot=%d\n%s",CLR1_32_GREEN,__FUNCTION__,s32Slot,CLR0_RESET);
	}
#else
	if(MY_SLOW_PTL_THREAD_ID != NULL)
	{
		OSHAL_Kill_Thread(MY_SLOW_PTL_THREAD_ID);
		MY_SLOW_PTL_THREAD_ID = NULL;
	}
#endif // SUPERTASK

	/* 2) RUN EOAM_DISCOVERY_TASK */
#ifdef SUPERTASK
	if ((s32Status = runtsk(80, (void(*)(void))EOAM_DISCOVERY_TASK, 8000, s8EOamMode)) < SUCCESS)
	{
		dprintf("%sRUNTASK EOAM_DISCOVERY_TASK fails %d\n%s", CLR1_31_RED,s32Status,CLR0_RESET);
		return EOAM_FAIL;
	}else
		dprintf("%sRUNTASK id=%d EOAM_DISCOVERY_TASK ...\n%s", CLR1_32_GREEN,s32Status,CLR0_RESET);
#else
	thread_s8EOamMode = s8EOamMode;
	MY_SLOW_PTL_THREAD_ID = OSHAL_Spawn_Thread(EOAM_DISCOVERY_TASK,&thread_s8EOamMode,"EOAM",0);
	if(MY_SLOW_PTL_THREAD_ID == NULL)
	{
		dprintf("%sRUNTASK EOAM_DISCOVERY_TASK fails\n%s", CLR1_31_RED,CLR0_RESET);
		return EOAM_FAIL;
	}
#endif // SUPERTASK

	return EOAM_SUCCESS;
}

/*****************************************************************
*
*   PROCEDURE NAME:
*   TEST_KILL_DISCOVERY_TASK
*
*   DESCRIPTION:
*	Kill OAM Discovery Process
*
*	s8EOamMode
*	1: EOAM_LAYER_PASSIVE_DTE
*	2: EOAM_LAYER_ACTIVE_DTE
*
*   NOTE:
*
*****************************************************************/
int TEST_KILL_DISCOVERY_TASK(void)
{
	int s32Slot;

	EOAM_MSG(dprintf("%s[%s]....\n%s",CLR1_32_GREEN,__FUNCTION__,CLR0_RESET));

	/* 1) KILL TASK EOAM_DISCOVERY_TASK */
#ifdef SUPERTASK
	if((s32Slot=slttsk((void (fAR *)(void))EOAM_DISCOVERY_TASK))<0)	dprintf("%s[%s]Get EOAM_DISCOVERY_TASK slot(%d) failed!\n%s",CLR1_31_RED,__FUNCTION__,s32Slot,CLR0_RESET);
	else
	{
		dprintf("%s[%s]Got EOAM_DISCOVERY_TASK slot=%d\n%s",CLR1_32_GREEN,__FUNCTION__,s32Slot,CLR0_RESET);
		if(klltsk(s32Slot)<0)
		{
			dprintf("%s[%s]Kill EOAM_DISCOVERY_TASK failed!\n%s",CLR1_31_RED,__FUNCTION__,CLR0_RESET);
			return EOAM_FAIL;
		}else
			dprintf("%s[%s]Kill EOAM_DISCOVERY_TASK slot=%d\n%s",CLR1_32_GREEN,__FUNCTION__,s32Slot,CLR0_RESET);
	}
#else
	if(MY_SLOW_PTL_THREAD_ID != NULL)
	{
		OSHAL_Kill_Thread(MY_SLOW_PTL_THREAD_ID);
		MY_SLOW_PTL_THREAD_ID = NULL;
	}
#endif // SUPERTASK

	return EOAM_SUCCESS;
}

#ifndef SUPERTASK
/*********************************************
 * usermode-helper API to arcusb_notifier.sh *
 *********************************************/
// usermode-helper API for calling arcusb_notifier.sh
static char  notifier_argv[5][40] = {"/usr/sbin/eoam_get_mtu.sh", "" /*arg 1*/, "" /*arg 2*/, "" /*arg 3*/, "" /*arg 4*/};
static char  notifier_envp[3][40] = {"HOME=/", "TERM=linux", "PATH=/sbin:/usr/sbin:/bin:/usr/bin"};
static char *notifier_argv_ptr[6] = {&notifier_argv[0][0], &notifier_argv[1][0], &notifier_argv[2][0], &notifier_argv[3][0], &notifier_argv[4][0], NULL};
static char *notifier_envp_ptr[4] = {&notifier_envp[0][0], &notifier_envp[1][0], &notifier_envp[2][0], NULL};

//int eoam_notifier_wrapper (char *arg1, char *arg2, char *arg3, char *arg4)
int eoam_notifier_wrapper ()
{
	int r;
	//strcpy (notifier_argv_ptr[1], arg1);
	//strcpy (notifier_argv_ptr[2], arg2);
	//strcpy (notifier_argv_ptr[3], arg3);
	//strcpy (notifier_argv_ptr[4], arg4);
	printk ("%s: before call call_usermodehelper()\n",__func__);
	printk ("notifier_argv_ptr[0]=%s\n",notifier_argv_ptr[0]);
	printk ("notifier_argv_ptr[1]=%s\n",notifier_argv_ptr[1]);
	printk ("notifier_argv_ptr[2]=%s\n",notifier_argv_ptr[2]);
	printk ("notifier_argv_ptr[3]=%s\n",notifier_argv_ptr[3]);
	printk ("notifier_argv_ptr[4]=%s\n",notifier_argv_ptr[4]);
	r = call_usermodehelper (notifier_argv_ptr[0], notifier_argv_ptr, notifier_envp_ptr, UMH_WAIT_PROC);
	printk ("%s: after call call_usermodehelper(), r=%d\n",__func__,r);
	return r;
}

static void proc_write_eoam_help( void )
{
	printk(	"EOAM help:\n"
			"help		- show this help message\n"
			"enable-debug		- turn on to dump debug message\n"
			"disable-debug		- turn off to dump debug message\n"
			"<enable/disable> <passive_mode/active_mode>		- enable/disable passive/active mode\n"
			"mtu <mtu value>	- set local mtu value\n"
		);
}

int proc_read_eoam(char *page, char **start, off_t off, int count, int *eof, void *data)
{
    int len = 0;
    int ival;
    int i;
    char *pStr = NULL;
	unsigned char *ps8RemoteMac, *ps8RemoteOUI;

	len += sprintf(page + len, "EOAM Status:\n");

	len += sprintf(page + len, "Transmit packets per second: %d\n", EOAMPDU_COUNT_PER_SECOND);

   	// Interface
	len += sprintf(page + len, "Interface Functionality:\n");

    // 0: disable, 1: enable
    ival = EOAMAPI_GET_FUNCTIONALITY(EOAM_FUNC_LINKFAULT);
	len += sprintf(page + len, "	Link Fault: %s\n", (ival == 1) ? "enable" : "disable");

    ival = EOAMAPI_GET_FUNCTIONALITY(EOAM_FUNC_DYINGGASP);
	len += sprintf(page + len, "	Dying Gasp: %s\n", (ival == 1) ? "enable" : "disable");

    ival = EOAMAPI_GET_FUNCTIONALITY(EOAM_FUNC_CRITICALEVENT);
	len += sprintf(page + len, "	Critical Event: %s\n", (ival == 1) ? "enable" : "disable");

    ival = EOAMAPI_GET_FUNCTIONALITY(EOAM_FUNC_UNIDIRECTIONAL);
	len += sprintf(page + len, "	Unidirectional: %s\n", (ival == 1) ? "enable" : "disable");

    ival = EOAMAPI_GET_FUNCTIONALITY(EOAM_FUNC_LOOPBACK);
	len += sprintf(page + len, "	LoopBack: %s\n", (ival == 1) ? "enable" : "disable");

    ival = EOAMAPI_GET_FUNCTIONALITY(EOAM_FUNC_LINKEVENT);
	len += sprintf(page + len, "	Link Event: %s\n", (ival == 1) ? "enable" : "disable");

    ival = EOAMAPI_GET_FUNCTIONALITY(EOAM_FUNC_VARIRETRIEVAL);
	len += sprintf(page + len, "	Vari Retrieval: %s\n", (ival == 1) ? "enable" : "disable");

    switch(gs8LocalEoamMode)
   	{
   		case EOAM_LAYER_DISABLE:
    		len += sprintf(page + len, "Mode: %s\n", "EOAM_LAYER_DISABLE");
   		break;

   		case EOAM_LAYER_PASSIVE_DTE:
    		len += sprintf(page + len, "Mode: %s\n", "EOAM_LAYER_PASSIVE_DTE");
   		break;

   		case EOAM_LAYER_ACTIVE_DTE:
    		len += sprintf(page + len, "Mode: %s\n", "EOAM_LAYER_ACTIVE_DTE");
   		break;

   		case EOAM_LAYER_LOOPBACK_DTE:
    		len += sprintf(page + len, "Mode: %s\n", "EOAM_LAYER_LOOPBACK_DTE");
   		break;
   	}

    len += sprintf(page + len, "	Status: %s\n", (gs8LocalEoamEnable == TRUE) ? "enable" : "disable");

    len += sprintf(page + len, "\n");

   	// DUT
    len += sprintf(page + len, "Local Information:\n");

    pStr = EOAMAPI_GET_OPERATIONAL_STATUS_STR();
    len += sprintf(page + len, "	Operational Status: %s\n", pStr);

    pStr = EOAMAPI_GET_LINKFAULT_STR();
    len += sprintf(page + len, "	Link Fault DTE: %s\n", pStr);

    len += sprintf(page + len, "	MTU: %d\n", gMTU);

    len += sprintf(page + len, "\n");

   	// Remote
    len += sprintf(page + len, "Remote Information:\n");
   	pStr = EOAMAPI_GET_REMOTE_MODE_STR();
    len += sprintf(page + len, "	Remote Mode: %s\n", pStr);

	ps8RemoteMac = (unsigned char *)EOAMAPI_GET_REMOTE_INFO(EOAM_REMOTE_MAC);
	if(ps8RemoteMac != NULL)
	{
    	len += sprintf(page + len, "	Remote MAC: %02X:%02X:%02X:%02X:%02X:%02X\n", ps8RemoteMac[0],ps8RemoteMac[1],ps8RemoteMac[2],ps8RemoteMac[3],ps8RemoteMac[4],ps8RemoteMac[5]);
	}
	else
	{
    	len += sprintf(page + len, "	Remote MAC: %02X:%02X:%02X:%02X:%02X:%02X\n", 0,0,0,0,0,0);
	}

	ps8RemoteOUI = (unsigned char *)EOAMAPI_GET_REMOTE_INFO(EOAM_REMOTE_OUI);
	if(ps8RemoteOUI != NULL)
	{
    	len += sprintf(page + len, "	Remote OUI: %02X:%02X:%02X\n", ps8RemoteOUI[0],ps8RemoteOUI[1],ps8RemoteOUI[2]);
	}
	else
	{
    	len += sprintf(page + len, "	Remote OUI: %02X:%02X:%02X\n", 0, 0, 0);
	}

    len += sprintf(page + len, "\n");
	len += sprintf(page + len, "Remote Functionality:\n");
    ival = EOAMAPI_GET_REMOTE_INFO(EOAM_REMOTE_FUNC_UNIDIRECTIONAL);
	len += sprintf(page + len, "	Unidirectional: %s\n", (ival == 1) ? "enable" : "disable");

    ival = EOAMAPI_GET_REMOTE_INFO(EOAM_REMOTE_FUNC_LOOPBACK);
	len += sprintf(page + len, "	LoopBack: %s\n", (ival == 1) ? "enable" : "disable");

    ival = EOAMAPI_GET_REMOTE_INFO(EOAM_REMOTE_FUNC_LINKEVENT);
	len += sprintf(page + len, "	Link Event: %s\n", (ival == 1) ? "enable" : "disable");

    ival = EOAMAPI_GET_REMOTE_INFO(EOAM_REMOTE_FUNC_VARIRETRIEVAL);
	len += sprintf(page + len, "	Vari Retrieval: %s\n", (ival == 1) ? "enable" : "disable");

	len += sprintf(page + len, "	Version: 0x%X\n", EOAMAPI_GET_REMOTE_INFO(EOAM_REMOTE_VERSION));

	len += sprintf(page + len, "	Max OAMPDU Size: 0x%X\n", EOAMAPI_GET_REMOTE_INFO(EOAM_REMOTE_MAX_SIZE));

	len += sprintf(page + len, "	Vendor Information: 0x%x\n", EOAMAPI_GET_REMOTE_INFO(EOAM_REMOTE_VENDOR));

   	// Loopback
   	pStr = EOAMAPI_GET_LB_CONNECT();
	len += sprintf(page + len, "	LoopBack Connection: %s\n", pStr);

    *eof = 1;

    return len;
}

#define atoi(str) simple_strtoul(((str != NULL) ? str : ""), NULL, 0)
int proc_write_eoam( struct file* file, const char* buffer, unsigned long count, void* data )
{
	char			*pPtr, *pTmpPtr;
	char			sBuf[128];
	unsigned char	bGetEnableValue = 0, bGetModeValue = 0;
	unsigned char	bEnable = 0, bEOAMMode = EOAM_LAYER_DISABLE;
	unsigned char	strEOAMMode[50];
	int len;

	printk("[proc_write_eoam] start\n");

	memset(sBuf, 0, sizeof(sBuf));

	/* trim the tailing space, tab and LF/CR*/
	if ( count > 0 )
	{
		if (count >= sizeof(sBuf))
			count = sizeof(sBuf) - 1;

		if (copy_from_user(sBuf, buffer, count))
			goto proc_write_eoam_exit;

		pPtr = (char*)sBuf + count - 1;
		len = count;
		for (; *pPtr==' ' || *pPtr=='\t' || *pPtr=='\n' || *pPtr=='\r'; pPtr++)
		{
			*pPtr = '\0';
		}
	}
	else
	{
		proc_write_eoam_help();
		goto proc_write_eoam_exit;
	}

	/* enable-debug */
	if ( strnicmp( sBuf, "enable-debug", strlen("enable-debug") ) == 0 )
	{
		printk("	enable-debug, len=%d\n", len);

		len -= strlen("enable-debug");
		for (pPtr=sBuf+strlen("enable-debug"); *pPtr==' ' || *pPtr=='\t'; pPtr++)
		{
			len--;
		}
		gEOAMEnableDebug = 1;
		goto proc_write_eoam_exit;
	}

	if ( strnicmp( sBuf, "disable-debug", strlen("disable-debug") ) == 0 )
	{
		printk("	disable-debug, len=%d\n", len);

		len -= strlen("disable-debug");
		for (pPtr=sBuf+strlen("disable-debug"); *pPtr==' ' || *pPtr=='\t'; pPtr++)
		{
			len--;
		}
		gEOAMEnableDebug = 0;
		goto proc_write_eoam_exit;
	}

	// set local mtu value
	if ( strnicmp( sBuf, "mtu", strlen("mtu") ) == 0 )
	{
		int nValue;

		printk("	mtu, len=%d\n", len);

		len -= strlen("mtu");
		for (pPtr=sBuf+strlen("mtu"); *pPtr==' ' || *pPtr=='\t'; pPtr++)
		{
			len--;
		}

		if(len <= 0)
		{
			proc_write_eoam_help();
			goto proc_write_eoam_exit;
		}

		// get value
		nValue = atoi(pPtr);
		gMTU = nValue;

		gstLocalEOamCtl.stLocalInfoTlv.s8InfoOampduCfg[0] = (gMTU & 0xFF00)>>8;
		gstLocalEOamCtl.stLocalInfoTlv.s8InfoOampduCfg[1] = (gMTU & 0x00FF);

		goto proc_write_eoam_exit;
	}

	// set EOAM for Ether-Uplink
	if ( strnicmp( sBuf, "EthIfName", strlen("EthIfName") ) == 0 )
	{
		int nValue;

		printk("	EthIfName, len=%d\n", len);

		len -= strlen("EthIfName");
		for (pPtr=sBuf+strlen("EthIfName"); *pPtr==' ' || *pPtr=='\t'; pPtr++)
		{
			len--;
		}

		if(len <= 0)
		{
			proc_write_eoam_help();
			goto proc_write_eoam_exit;
		}

		strcpy(gEOAM_Ifname, ETH_EOAM_IFNAME);

		goto proc_write_eoam_exit;
	}

	// set EOAM for Ether-Uplink
	if ( strnicmp( sBuf, "VDSL", strlen("VDSL") ) == 0 )
	{
		int nValue;

		printk("	VDSL, len=%d\n", len);

		len -= strlen("VDSL");
		for (pPtr=sBuf+strlen("VDSL"); *pPtr==' ' || *pPtr=='\t'; pPtr++)
		{
			len--;
		}

		if(len <= 0)
		{
			proc_write_eoam_help();
			goto proc_write_eoam_exit;
		}

		strcpy(gEOAM_Ifname, PTM_EOAM_IFNAME);

		goto proc_write_eoam_exit;
	}

	// set local mtu value
	if ( strnicmp( sBuf, "test", strlen("test") ) == 0 )
	{
		int nValue;

		printk("	test, len=%d\n", len);

		len -= strlen("test");
		for (pPtr=sBuf+strlen("test"); *pPtr==' ' || *pPtr=='\t'; pPtr++)
		{
			len--;
		}

		eoam_notifier_wrapper();

		goto proc_write_eoam_exit;
	}


	/* enable */
	if ( strnicmp( sBuf, "enable", strlen("enable") ) == 0 )
	{
		printk("	enable, len=%d\n", len);

		len -= strlen("enable");
		for (pPtr=sBuf+strlen("enable"); *pPtr==' ' || *pPtr=='\t'; pPtr++)
		{
			len--;
		}
		bGetEnableValue = 1;
		bEnable = 1;
		goto proc_write_eoam_process_mode;
	}

	/* disable */
	if ( strnicmp( sBuf, "disable", strlen("disable") ) == 0 )
	{
		printk("	disable, len=%d\n", len);

		len -= strlen("disable");
		for (pPtr=sBuf+strlen("disable"); *pPtr==' ' || *pPtr=='\t'; pPtr++)
		{
			len--;
		}
		bGetEnableValue = 1;
		bEnable = 0;
	}
	else
	{
		proc_write_eoam_help();
		goto proc_write_eoam_exit;
	}

proc_write_eoam_process_mode:
	/* passive mode */
	if ( strnicmp( pPtr, "passive_mode", strlen("passive_mode") ) == 0 )
	{
		printk("	passive_mode, len=%d\n", len);

		len -= strlen("passive_mode");
		bGetModeValue = 1;
		bEOAMMode = EOAM_LAYER_PASSIVE_DTE;
		goto proc_write_eoam_set_value;
	}

	/* active mode */
	if ( strnicmp( pPtr, "active_mode", strlen("active_mode") ) == 0 )
	{
		printk("	active_mode, len=%d\n", len);

		len -= strlen("active_mode");
		bGetModeValue = 1;
		bEOAMMode = EOAM_LAYER_ACTIVE_DTE;
	}

proc_write_eoam_set_value:
	//gs8LocalEoamEnable = bEnable;
	//gs8LocalEoamMode 	 = bEOAMMode;

	EOAMAPI_ENABLE(bEnable, bEOAMMode);

proc_write_eoam_exit:
	printk("[proc_write_eoam] end\n");
	return count;
}
#endif

void eoam_exit(void)
{
	if(gs8LocalEoamEnable == FALSE) return;
	EOAMAPI_ENABLE(0, EOAM_LAYER_PASSIVE_DTE);
#ifndef SUPERTASK
	MY_SLOW_PTL_DEV = NULL;
#endif
}

EXPORT_SYMBOL(EOAM_HANDLER);
EXPORT_SYMBOL(proc_write_eoam);
EXPORT_SYMBOL(proc_read_eoam);
EXPORT_SYMBOL(EOAMAPI_GET_EOAM_DEBUG);

