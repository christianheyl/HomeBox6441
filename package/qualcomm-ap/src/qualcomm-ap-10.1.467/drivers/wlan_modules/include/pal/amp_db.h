/*
 * Copyright (c) 2011 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 */
#ifndef __AMP_H__
#define __AMP_H__

#include "pal_osapi.h"
#include "pal_hci.h"
#include "tque.h"
#include "pal_cfg.h"
#include <sys/queue.h>

#define AMP_ASSOC_LEN           672       /* Max AMP assoc fragment length  */
#define NUM_OF_AMP_ASSOC_INFO   5         /* Max AMP assoc support */
#define DEFAULT_LSTO            32000     /* in slots */
#define LSTO_DIVIDER            3         /* How many link supervision req we want to send within LSTO */
#define HEAD_ROOM               0         
#define MAX_DATA_BUF_SZ         (Max80211_PAL_PDU_Size+HCI_ACL_HDR_SZ)      /* PAL MAX payload size */
#define IEEE80211_ADDR_LEN      6
#define MAX_POSH_CMD_SIZE       128
#define FLUSH_COM_EVENT_TIMEOUT 1000     /* in ms */

//We need to distinguish whether the packet send completed from MP
//is regular ACL data or specical packets
//For special packets, we do not send numberofcompleteblock event
//back to host
//for M4, the state machine has a special need to know if it is
//being sent out or not

#define NON_ACL_MASK                 0x80000000 //for logic link handler

//MASK for non ACL data, Reserve the lower 16 bits to real logic link ID
//Logic Link ID = (FlowID - upper 8 bits) | (PhyHdl - lower 8 bits)
#define MASK_FOR_LINK_SUPERVSION     ((NON_ACL_MASK) | 0x00010000)
#define MASK_FOR_ACTIVITY_REPORT     ((NON_ACL_MASK) | 0x00020000)
#define MASK_FOR_M1                  ((NON_ACL_MASK) | 0x00040000)
#define MASK_FOR_M2                  ((NON_ACL_MASK) | 0x00080000)
#define MASK_FOR_M3                  ((NON_ACL_MASK) | 0x00100000)
#define MASK_FOR_M4                  ((NON_ACL_MASK) | 0x00200000)

#define MAX_CHANNEL_COUNT                  64
#define MAX_11G_CHANNEL                    14
#define DEF_11G_REG_CLASS_ID               254
#define DEF_FCC_11G_REG_CLASS_ID           12
#define DEF_ETSI_11G_REG_CLASS_ID          4
#define DEF_MKK_11G_REG_CLASS_ID           30
#define DEF_FCC_11A_START_REG_CLASS_ID     1
#define DEF_FCC_11A_END_REG_CLASS_ID       5
#define DEF_ETSI_11A_START_REG_CLASS_ID    1
#define DEF_ETSI_11A_END_REG_CLASS_ID      3
#define IEEE80211_REG_EXT_ID               201
#define DEF_11G_MAX_FREQ                   2484
#define DEF_CH_11_FREQ                     2462
#define DEF_11G_CHANNEL                    11
#define AMP_COUNTRY_LENGTH                 3
#define INVALID_AMP_CHANNEL_NUM            0
#define INVALID_AMP_CHANNEL_FRQ            0
#define INVALID_AMP_CHANNEL_REGID          0
#define AMP_PREF_CHAN_LIST_TAG_LEN         1
#define AMP_PREF_CHAN_LIST_LENGTH_LEN      2

enum {					/* conformance test limits */
    CTL_FCC  = 0x10,
    CTL_MKK	 = 0x40,
    CTL_ETSI = 0x30,
    NOCTL    = 0xff,
};

#define MAX_CHANNEL_MAP_NUM          13
#define CHANNEL_MAP(_r, _c) channel_##_r##_c

static A_UINT8 channel_NOCTL254[11] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};

//FCC
static A_UINT8 channel_FCC1[4] = {36, 40, 44, 48};
static A_UINT8 channel_FCC2[4] = {52, 56, 60, 64};
static A_UINT8 channel_FCC3[4] = {149, 153, 157, 161};
static A_UINT8 channel_FCC4[11] = {100, 104, 108, 112,116, 120, 124, 128, 132, 136, 140};
static A_UINT8 channel_FCC5[5] = {165};
static A_UINT8 channel_FCC6[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
static A_UINT8 channel_FCC7[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
static A_UINT8 channel_FCC8[5] = {11, 12, 15, 17, 19};
static A_UINT8 channel_FCC9[5] = {11, 12, 15, 17, 19};
static A_UINT8 channel_FCC10[2] = {21, 25};
static A_UINT8 channel_FCC11[2] = {21, 25};
static A_UINT8 channel_FCC12[11] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};

//ETSI
static A_UINT8 channel_ETSI1[4] = {36, 40, 44, 48};
static A_UINT8 channel_ETSI2[4] = {52, 56, 60, 64};
static A_UINT8 channel_ETSI3[11] = {100, 104, 108, 112,116, 120, 124, 128, 132, 136, 140};
static A_UINT8 channel_ETSI4[13] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13};

//MKK
static A_UINT8 channel_MKK1[4] = {36, 40, 44, 48};
static A_UINT8 channel_MKK2[3] = {8, 12, 16};
static A_UINT8 channel_MKK3[3] = {8, 12, 16};
static A_UINT8 channel_MKK4[3] = {8, 12, 16};
static A_UINT8 channel_MKK5[3] = {8, 12, 16};
static A_UINT8 channel_MKK6[3] = {8, 12, 16};
static A_UINT8 channel_MKK7[4] = {184, 188, 192, 196};
static A_UINT8 channel_MKK8[4] = {184, 188, 192, 196};
static A_UINT8 channel_MKK9[4] = {184, 188, 192, 196};
static A_UINT8 channel_MKK10[4] = {184, 188, 192, 196};
static A_UINT8 channel_MKK11[4] = {184, 188, 192, 196};
static A_UINT8 channel_MKK12[4] = {7, 8, 9, 11};
static A_UINT8 channel_MKK13[4] = {7, 8, 9, 11};
static A_UINT8 channel_MKK14[4] = {7, 8, 9, 11};
static A_UINT8 channel_MKK15[4] = {7, 8, 9, 11};
static A_UINT8 channel_MKK16[6] = {183, 184, 185, 187, 188, 189};
static A_UINT8 channel_MKK17[6] = {183, 184, 185, 187, 188, 189};
static A_UINT8 channel_MKK18[6] = {183, 184, 185, 187, 188, 189};
static A_UINT8 channel_MKK19[6] = {183, 184, 185, 187, 188, 189};
static A_UINT8 channel_MKK20[6] = {183, 184, 185, 187, 188, 189};
static A_UINT8 channel_MKK21[6] = {6, 7, 8, 9, 10, 11};
static A_UINT8 channel_MKK22[6] = {6, 7, 8, 9, 10, 11};
static A_UINT8 channel_MKK23[6] = {6, 7, 8, 9, 10, 11};
static A_UINT8 channel_MKK24[6] = {6, 7, 8, 9, 10, 11};
static A_UINT8 channel_MKK25[8] = {182, 183, 184, 185, 186, 187, 188, 189};
static A_UINT8 channel_MKK26[8] = {182, 183, 184, 185, 186, 187, 188, 189};
static A_UINT8 channel_MKK27[8] = {182, 183, 184, 185, 186, 187, 188, 189};
static A_UINT8 channel_MKK28[8] = {182, 183, 184, 185, 186, 187, 188, 189};
static A_UINT8 channel_MKK29[8] = {182, 183, 184, 185, 186, 187, 188, 189};
static A_UINT8 channel_MKK30[13] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13};
static A_UINT8 channel_MKK31[1] = {14};
static A_UINT8 channel_MKK32[4] = {52,  56,  60,  64};
static A_UINT8 channel_MKK34[11] = {100, 104, 108, 112,116, 120, 124, 128, 132, 136, 140};

typedef struct {
    UCHAR        isoName[3];
} COUNTRY_ISO_NAME;

#define VALID_COUNTRY_NUMBER    28

static COUNTRY_ISO_NAME ValidCountry[VALID_COUNTRY_NUMBER] = 
{ "US", "JP", "AT", "BE", "BG", "CY", "CZ", "DK", "EE", "FI", "FR", "DE", "GR", "HU",
  "IE", "IT", "LV", "LT", "MT", "NL", "PL", "PT", "RO", "SK", "SI", "ES", "SE", "GB" };

/*Channel loading information*/
typedef struct _AMP_CH
{
    A_UINT16              APcount;
    A_UINT16              freq;
    A_UINT8               ieee;        /* IEEE channel number */
    A_UINT8               regClassId;
} AMP_CH, *PAMP_CH;

#if (defined(_WIN32) || defined(_WIN64))
#include <pshpack1.h>
#endif

typedef struct data_buf_t {
    char       buf[MAX_DATA_BUF_SZ+HEAD_ROOM];
    short      sz;
    A_UINT8    type;
} DATA_BUF;

typedef struct data_node_t {
    NODE        node;
    DATA_BUF    u;
} HCI_DATA_NODE;

typedef struct tx_cb_t {
    A_UINT16    id;                     /* tx frames id   */
} TX_CB;


typedef struct active_report{
    A_UINT8             dst[MAC_ADDR_LEN];
    A_UINT8             src[MAC_ADDR_LEN];
    ACTIVITY_RPT_HDR    rpt;
} ACTIVE_REPORT;

typedef struct {
    A_UINT8     size;        /* len of struc */
    A_UINT32    tbttcount;   /* in beacon interval */
    A_UINT32    period;      /* in us */
    A_UINT32    duration;    /* in TUs */
    A_UINT32    offset;      /* in us */
    UCHAR       enable;      /* turn on/off the quite timer */
} BT30_ACTIVE_REPORT_PARAMS;

// for Extended Flow Spec., used as indices into EFS2UPtable
#define BE             0
#define MAX_BW         1    // GU_BW
#define MIN_LATENCY    2    //GU_LATENCY

enum ieee8021d_UserPriority {
    IEEE8021dUP_BE     = 0,
    IEEE8021dUP_BK     = 1, 
    IEEE8021dUP_EE     = 3, 
    IEEE8021dUP_CL     = 4,
    IEEE8021dUP_VI     = 5, 
    IEEE8021dUP_VO     = 6, 
    IEEE8021dUP_NC     = 7, 
};

static A_UINT8 EFS2UPtable[] = { IEEE8021dUP_BE, IEEE8021dUP_VI, IEEE8021dUP_VO};/*{BE, MAX_BW, MIN_LATENCY}*/

//-------------------------------------------------------------
// Supporting of PAL internal timer engine
// A simple duble loopback link list that allows the PAL
// timer to be inserted (SET) and removed (CANCEL) as needed
// PAL timers are statically allocated as part of PAL AMP_ASSOC
// per physical link architecture.
//-------------------------------------------------------------

typedef void (*TimerCallBackFunc)(PVOID Context);

typedef struct  _ListEntry_t{
    struct _ListEntry_t *Flink;
    struct _ListEntry_t *Blink;
} ListEntry;

typedef enum {
    PHYSICAL_LINK_TIMEOUT = 0,
    SECURITY_FRAME_TIMEOUT,
    LINK_SUPERVISION_TIMEOUT,
    EVENT_FLUSH_CMP_TIMEOUT
} eTIMEOUT_TYPE;

typedef struct _paltimer_t{
    ListEntry         node;      //To hook up with timer engine link list
    void*             context;
    TimerCallBackFunc cb;
    A_UINT32          duration;
    systime_t         start_tick;
    eTIMEOUT_TYPE     timer_type;
 //   A_UINT8           invalid;
} PAL_TIMER;

#if (defined(_WIN32) || defined(_WIN64))
#include <poppack.h>
#endif

typedef struct {
    ListEntry             PAL_timerlist;
    spinlock_t            PAL_timerLock;
    systime_t             PAL_SysTick;
    A_UINT8               PAL_timerInit;
} PAL_TIMER_LIST;

typedef enum {
    AMP_CONNECTION_TERMINATOR = 0,  /* peer responder */
    AMP_CONNECTION_ORIGINATOR = 1,  /* peer creater */
} AMP_CONNECTION_TYPE;

typedef struct  callback_param_t {
    struct  amp_dev_t     *dev;                /* PAL_DEV  */
    A_UINT8                buf[128];           /* parameters */
} CALLBACK_PARAM;

typedef struct llid_info_t {
    A_UINT16          logical_link_id;           /* logical link id */
    A_UINT32          be_flush_tout;             /* best effort flush time out */
    A_UINT16          failedContactCounter;      /* failed Contact Counter */
    FLOW_SPEC         tx_spec;                   /* tx enhanced flow spec id */
    FLOW_SPEC         rx_spec;                   /* rx enhanced flow spec id */
    HANDLE            flushtimer;                /* auto flush timer */
    CALLBACK_PARAM    flushtimer_param;          /* auto flush timer parameters */
} LLID_INFO;

typedef enum {
    DISCONNECTED_STATE = 0,         /* physical link state */
    STARTING_STATE,
    CONNECTING_STATE,
    AUTHENTICATING_STATE,
    CONNECTED_STATE,
    DISCONNECTING_STATE,
} PHY_LINK_STATE;

#define DEFINE_SUPPORT_CMD(c, p) {(c), (p)}
#define N(a) (sizeof(a) / sizeof(a[0]))

typedef struct support_cmd_t{
    A_UINT16   cmd;             /* support command id */
    A_UINT16   bit_pos;         /* high 8 bit: byte index low 8 bit: bit mask */
} SUPPORT_CMD;


#define FLAG_CompleteEvent          0x01
#define FLAG_DisconCompleteEvent    0x02
#define FLAG_LINK_SVTO_Event        0x04

#define MAX_FLOW_SPECS_PER_PHY_LINK 16
#define FRAME_TYPE_AUTH             0xB0
#define FRAME_TYPE_ASSOC            0x00
#define FRAME_TYPE_ASSOC_RESP       0x10
#define FRAME_TYPE_DATA             0x08
#define SRC_ADDR_OFFSET             0x0A
#define SNAP_TYPE_OFFSET            38 

/* private define type, used for identify data frame subtype */
#define FRAME_TYPE_M1               0xFC
#define FRAME_TYPE_M2               0xFD
#define FRAME_TYPE_M3               0xFE
#define FRAME_TYPE_M4               0xFF

#define IEEE_802_11A                0x00000001
#define IEEE_802_11B                0x00000002
#define IEEE_802_11G                0x00000004
#define IEEE_802_11N                0x00000008

#define PAL_POSH_API         ((POSH_OPS *)Dev->PPoshApi)

typedef struct {
    A_UINT32    OID;
    A_UINT8     PoshCmd[MAX_POSH_CMD_SIZE];
} POSH_CMD;

#define MAX_POSH_CONTEXT_SIZE        354

typedef struct {
    A_UINT8         EventId;
    A_UINT8         Status;
    A_UINT32        VapPort;
    A_UINT16        Reason;
    A_UINT8         PeerMac[6];
    A_UINT8         QosEnable;
    A_UINT16        Size;
    A_UINT8         Context[MAX_POSH_CONTEXT_SIZE];
} MP_EVENT_CONTEXT;

#define MAX_PAL_EVENT_MSG_SZ  (sizeof(MP_EVENT_CONTEXT) + 12)

typedef enum {
    PAL_HCI_CMD_QUEUE,
    PAL_MP_EVENT_QUEUE,
    PAL_TEST_EVENT_QUEUE,
    PAL_TIMER_EVENT_QUEUE
} PAL_Queue_Type;

typedef struct pal_in_buf_t {
    char           buf[MAX_PAL_EVENT_MSG_SZ];
    A_UINT32       MpEventId;
    A_UINT16       sz;
    PAL_Queue_Type QueueType;
} PAL_IN_BUF;


typedef struct cmd_que_t {
    NODE       node;
    PAL_IN_BUF PalInEvent;
} PAL_EVENT_NODE;

//-------------------------------------------------------
//PAL could enter create AMP state anytime due to 
//the system reset
//-------------------------------------------------------
typedef enum {
    PAL_IDLE_STATE = 0,
    PAL_CREATE_AMP_MAC_STATE,
    PAL_QUERY_INITIAL_CHANNEL_INFO,
    PAL_READY_STATE,
    PAL_CREATE_AMP_VAC_FAILED_STATE,
    PAL_DELETE_AMP_VAC_FAILED_STATE
} E_PAL_DEVICE_STATE;

typedef enum {
    AMP_RF_POWER_DOWN = 0,
    AMP_FOR_BT_ONLY,
    AMP_NO_CAPACITY,
    AMP_LOW_CAPACITY,
    AMP_MEDIUM_CAPACITY,
    AMP_HIGH_CAPACITY,
    AMP_FULL_CAPACITY
} E_AMP_STATUS;

typedef struct t_SEC_RECORD_
{
    A_UINT8             link_key_len;                 /* physical link key length */
    A_UINT8             link_key_type;                /* physical link key type   */
    A_UINT8             link_key[LINK_KEY_LEN];       /* physical link key cache  */

    A_UINT8             replay_counter;
    A_UINT8             replay_init_count;
    A_UINT32            ptk_retry_timer;
    //4-Way handshaking, to avoid step upon by other physical link thread
    A_UINT8             ptk[64], g_pmk[LINK_KEY_LEN]; /* Pair Transition Key, and Pair Mast Key */
    A_UINT8             gtk[LINK_KEY_LEN];
    A_UINT8             a_nonce[LINK_KEY_LEN];        //A_Nonce 
    A_UINT8             s_nonce[LINK_KEY_LEN];        //S_Nonce used during 4-way handshaking

} t_SEC_RECORD;

typedef struct  remote_assoc_info_t{
    struct amp_dev_t    *amp_dev;                   /* point back to PAL_DEV */
    A_UINT8             phy_link_hdl;               /* physical link handle */
    A_UINT8             valid;                      /* tag                  */
    A_UINT8             connection_side;            /* role in physical link creation process */
    A_UINT8             status;                     /* should remove    */
    A_UINT8             amp_assoc[AMP_ASSOC_LEN];   /* assoc triplets   */
    A_UINT16            assoc_len;                  /* assoc triplets length */
    AMP_CH              channel_selected;           /* working channel */
    A_UINT8             phyType;                    /* wifi mac working mode 11g/a/n */
    A_UINT8             *hwaddr;                    /* point to remote mac addr in amp assoc */
    A_UINT8             phy_link_state;             /* physical link state(used in phy link state machine) */
    A_UINT8             phy_link_action_state;      /* physical link creation action state */
    A_UINT8             phylink_status;             /* physical link complete status */
    A_UINT8             ping_cnt;                   /* Outstanding pings */
    A_UINT8             Debug_cnt;
    A_UINT8             srm;                        /* Short Range Mode */
    A_UINT8             rts_on;                     /* PAL rts status rename later   */
    A_UINT32            flag;                       /* Misc Flags for state machine */
    A_UINT16            link_supervision_timeout;
    A_UINT16            last_assigned_llid;                         /* last asigned logical channel id */
    LLID_INFO           logical_link[MAX_FLOW_SPECS_PER_PHY_LINK];  /* logical link data structer */
    PAL_TIMER           phylinktimer;               /* Connect timer per physical link */
    PAL_TIMER           lstotimer;                  /* Link supervision timer */
    PAL_TIMER           retry_timer;                  
    A_UINT8             qos_support;
    A_UINT64            num_of_frames;              /* Keeps a count of num of frames recv'd */
    t_SEC_RECORD        sec_record;                 /* Security record, including link key and ptk, et al */
                                                    /*  There shall ne no new Async command from PAL->POSH->MP */
                                                    /*  Until the previous MP OID request are done */
    A_UINT8             CTL2G;
    A_UINT8             CTL5G;
} AMP_ASSOC_INFO;

typedef struct  pal_cfg_t {
    A_UINT64                hci_event_mask[2];
    AMP_ASSOC_INFO*         local_ampassoc;                 /* local amp assoc cache    */
    LOCAL_AMP_INFO          local_ampinfo;                  /* local amp information cache */
    A_UINT16                conn_accept_timeout;            /* physical link connection timeout */      
    A_UINT16                logical_link_accept_timeout;    /* logical  link connection timeout */  
    A_UINT8                 flow_control_mode;              /* Flow Contorl Mode */
    LOCATION_DATA_CFG       loc;                            /* Location Configuration */
    A_UINT32                free_guaranteed_bandwidth;      /* unused guarantee bandwidth   */
} PAL_CFG;

typedef struct pal_tx_queue_data {
    struct  amp_dev_t    *amp_dev; 
    os_mesg_queue_t      queue;
    bool                 active;
    bool                 initialized;
} pal_tx_queue_data_t, *ppal_tx_queue_data_t;

#ifdef ATH_SUPPORT_HTC
#define AMP_MAX_DATA_QUEUE_SIZE   8

typedef struct _ACL_DATA_QUEUE 
{
    UCHAR*                     buffer;
    ULONG                      length;
} ACL_DATA_QUEUE, *PACL_DATA_QUEUE;
#endif

typedef struct _DEVICE_EXTENSION {
	PVOID               pDevice;
	PDRIVER_OBJECT      pDriver;
	ULONG               DeviceNumber;
	PFILE_OBJECT        BTFileObject;
	PFILE_OBJECT        FileObject;
	PDEVICE_OBJECT	    BTDeviceObject;
	PDEVICE_OBJECT	    TargetDeviceObject;
	NDIS_HANDLE         InterfaceNotificationHandle;
	NDIS_HANDLE         DeviceNotificationHandle;
	ULONG               KernelContext;
	BOOLEAN             BtKernFound;
	BOOLEAN             KrnlDown;
	mesg_lock_t          BtKernLock;                            /* Synchronize between BT Dev Deregistration and Event/Data Indication */
    UNICODE_STRING      SymbolicLink;
	BOOLEAN             SymbolicLinkAllocated;
	BOOLEAN             appExclusive;
	BOOLEAN             alreadyRemoved;
    WCHAR               deviceNameBufferVista[50];
    PVOID               pGUID;
} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

typedef struct  amp_dev_t {
    NDIS_HANDLE         MpHandle;                              /* Passed from during PAL_MAIN_ENTRY */
    osdev_t             osdev;
    E_PAL_DEVICE_STATE  pal_dev_state;                         /* PAL global device state */
    A_UINT8             hwaddr[MAC_ADDR_LEN];                  /* mac address of interface */
    A_UINT8             CountryCode[3];
    A_UINT8             srm;                                   /* short range mode status  */
    A_UINT8             HostPresent;                           /* Do not enable PAL SM if no host */
    A_UINT8             PAL_PoshDebug_level;
    A_UINT16            flush_llc;                             /* flush */
    A_BOOL              tx_pause;                              /* pause tx deque */
    A_UINT16            g_seq_num; 
    /*user space simulation var*/
    A_UINT32            wifi_caps;                             /* wifi card capability */
    A_UINT32            pal_80211_cap;                         /* PAL 80211 capability caps */
    A_UINT32            VapNumber;                             /* Vap port ID */
	A_UINT32            ampSelChannel;                         /* Fixed channel for testing and DEMO */
    A_BOOL              ampThruTest;                           /* Registry to disable PAL dump frame */
    A_UINT8             ampFilterName[40];                     /* Registry to record filter driver name */
    A_BOOL              ampGUID;                               /* Registry to enable Third Party GUID */
    A_UINT32            ampDbgWdmEnable;                       /* Mirror for registry debug */
    A_BOOL              PalDataInQueue;                        /* Flag so we won't send Link Supervision Req when we are good */
    A_BOOL              ampBeaconStarted;                      /* for testing, once it starts, we do not stop */
    A_BOOL              ResetInProcess;                        /* Indication that reset is in progress */
    A_BOOL              FailtoStopAMP;                         /* Record that we are failing to send STOP AMP */
    TQUE                PalEvents_q;                           /* hci cmd rx queue */
    TQUE                wr_q;                                  /* data tx queue */  
 
    PAL_TIMER_LIST      PalTimerList;                          /* Link list entry for PAL timer */      
    spinlock_t          PalEventQLock;                         /* hci cmd queue mutex */
    mesg_lock_t         PalDataQLock;                          /* Prevent re-entry to the 80211 construction code */
    PAL_PACKET_QUEUE    PacketQueue;                           /* Queue of Packets */
    pal_tx_queue_data_t pal_tx_queue;                          /* TX queue */
    AMP_ASSOC_INFO      rem_assoc_amp[NUM_OF_AMP_ASSOC_INFO];
    PAL_CFG             pal_cfg;                               /* PAL startup configuration parameters */
    PAL_TIMER           FlushCmpTimer;                         /* Send Enhanced Flush complete timer */
    DEVICE_EXTENSION    DevExt;                                /* For communication with BT device */

    struct ath_timer    *pPal_Timer;
    void                *PPoshApi;                             /* POSH API, needs to be initialized */
    void                *PoshObj;

#ifdef ATH_SUPPORT_HTC
    /* Data Thread related context */
    KEVENT              DataWaitEvent;
    BOOLEAN             DataThreadRun;
    NDIS_SPIN_LOCK      DataSpinLock;
    PETHREAD            pDataThreadObj;    
    
    /* Use for Data Thread */
    ACL_DATA_QUEUE      DataQueue[AMP_MAX_DATA_QUEUE_SIZE];
    UCHAR               DataQueueHead;
    UCHAR               DataQueueTail;
#endif
} AMP_DEV;

typedef struct  cmd_tbl_t {
    A_UINT16    cmdid;                                        /* HCI CMD ID */
    A_UINT32    (*fn)(AMP_DEV *, void* cmd_pkt, void* buf);    /* Dispatch Function Entry */
    A_UINT8     *strCmd;
} OCF_CMD_TBL;

/* PAL Capability, static definition here */
#define PAL_CAN_UTIL_RCV_ACT_REPORT               0x01
#define PAL_CAN_SCHL_RCV_ACT_REPORT               0x02

#define PHY_HDL_VALID(x)                          ((x)->valid)
#define AMP_ASSOC_VALID(x)                        ((x)->assoc_len != 0)
#define AMP_ASSOC_CHAN_SELECTED(x)                ((x)->channel_selected.ieee != 0)

#define PAL_CREATE_LLID_FROM_PHYHDL(x, y)         ((y) << 8 | (x))
#define PAL_GET_PHY_HDL_FROM_LLID(x)              ((x) & 0xFF)
#define PAL_GET_FLOW_ID_FROM_LLID(x)              (((x)>>8) & 0x0F)

#define GET_LLID_FROM_PKT_HDL(x)                  ((x) & 0xFFF)

#define PAL_ENABLE                                6
#define PAL_MINI_LATENCY                          100
#define PAL_CONTORLLER_TYPE                       1
#define PAL_CAPABILITY                            1
#define PAL_MAX_FLUSH_TIMEOUT                     100
#define PAL_BE_FLUSH_TIMEOUT                      100
#define PAL_MAX_BANDWIDTH                         30000
#define PAL_MAX_GUARANTEED_BANDWIDTH              20000
#define HCI_PAL_VERSION                           0x1
#define HCI_PAL_SUB_VERISON                       0x1
#define HCI_ATHORS_NAME                           0x45
#define PAL_VERSION                               0x1
#define PAL_SUB_VERSION                           0x1
#define PAL_SIG_NAME                              0x55FF
#define HCI_VERSION                               0x5
#define HCI_REVISION                              0x1
#define AMP_ACL_PACKET_FLAG                       0x3000

void
amp_db_init(AMP_DEV *Dev);

void
alloc_rem_assoc_amp(AMP_DEV* dev, A_UINT8 phy_link_hdl, AMP_ASSOC_INFO **amp);

void
free_rem_assoc_amp( AMP_ASSOC_INFO *amp);

A_UINT8 
get_total_remote_links(AMP_DEV* dev, A_UINT16 phy_link_hdl);

void
get_remote_assoc_amp(AMP_DEV *Dev, A_UINT16 phy_link_hdl, AMP_ASSOC_INFO **amp);

void
parse_amp_assoc(A_UINT8 *buf, A_UINT8 tag, A_UINT8 **info , A_UINT16 TotalLen);

void
perform_chan_select(AMP_ASSOC_INFO *l_amp, AMP_ASSOC_INFO *r_amp, AMP_CH *SelChannel);

A_UINT8 *
append_tlv(A_UINT8 *buf, A_UINT8 tag, A_UINT16 len, A_UINT8 * val);

A_UINT16
generate_logical_link(AMP_ASSOC_INFO *amp, FLOW_SPEC *tx_spec, FLOW_SPEC *rx_spec);

AMP_ASSOC_INFO 
*get_remote_amp_assoc_via_addr(AMP_DEV* dev, A_UINT8 *addr);

AMP_ASSOC_INFO  
*get_remote_amp_assoc_via_state(AMP_DEV* dev, A_UINT32 Event);

void
find_logical_link_info(AMP_ASSOC_INFO *amp, A_UINT16 id, LLID_INFO **info);

A_BOOL
free_logical_link(AMP_ASSOC_INFO *amp, A_UINT16 id);

A_BOOL
get_phy_hdl(AMP_DEV *Dev, A_UINT8 *addr, A_UINT8 *phy_hdl);

A_UINT8 
PAL_post_event_q(AMP_DEV* Dev, PAL_Queue_Type QueueType, A_UINT32 EventId, A_UINT8 *buf, A_UINT16 sz);

A_UINT8     
pal_init_idle_state(AMP_DEV *Dev, VOID *pDriverObject);

A_UINT8 
pal_init_ready_state(AMP_DEV *Dev);

A_UINT8     
pal_init_get_channel_info_state(AMP_DEV *Dev);

A_UINT32    
pal_process_hci_cmd(AMP_DEV *Dev, A_UINT8 *buf, A_UINT16 sz);

A_INT8 
mapEFS2UserPriority(FLOW_SPEC efs);

void 
Get_Channel_MAP(A_UINT8* des, A_UINT8 CTL, A_UINT8 checkId );

A_INT8 
IsValidCTL(A_UINT8 CTL, A_UINT8* Country);

#endif /* __AMP_H__ */
