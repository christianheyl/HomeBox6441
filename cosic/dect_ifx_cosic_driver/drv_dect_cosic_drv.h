/******************************************************************************

                              Copyright (c) 2009
                            Lantiq Deutschland GmbH
                     Am Campeon 3; 85579 Neubiberg, Germany

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/
#ifndef _DRVER_COSIC_DRV_H_
#define _DRVER_COSIC_DRV_H_


#define COSIC_DATA_HIGH_LENGTH	0
#define COSIC_DATA_LOW_LENGTH	1
#define COSIC_DATA_MSG_TYPE		2

#define COSIC_MAC_MSG			1

#define DECT_DEVICE_INTNUM		5		// TODO: I don't know interrupt number now ! ??
#define MAX_SPI_DATA_LEN	312

#define COSIC_VOICE_PACKET			0x17		/*  VOICE PACKET DATA  TO TAPI */
#define COSIC_MAC_PACKET			0x01		/*  MAC COMMAND PACKET  DATA TO HMAC/LMAC */
#define COSIC_DECT_SYNC_PACKET   0x02		/*  DECT synchronization Packet */
#define COSIC_DECT_SETUP_PACKET  0x03		/*  DECT Setup Packet */
#define COSIC_DECT_DEBUG_PACKET  0x04		/* DECT 	debug infomation packet */
#define COSIC_DECT_FU10_PACKET   0x05    /* DECT uplane data packet */

#define COSIC_DECT_START_SYNC_PACKET	0xAA	/*  DECT First rtx sync packet */
#define MAX_SPI_SYNC_PACKET_LEN		8



/* MAC COMMAND PACKET DATA */
typedef enum
{
      HMACCmd_DEBUGMSG_IN_LMAC         = 0x10,

      LMACCmd_BOOT_RQ_HMAC             = 0x20,
      HMACCmd_BOOT_IND_LMAC,
      LMACCmd_PARAMETER_PRELOAD_RQ_HMAC,

      LMACCmd_GFSK_VALUE_READ_RQ_HMAC  = 0x30,
      HMACCmd_GFSK_VALUE_READ_IND_LMAC,
      LMACCmd_GFSK_VALUE_WRITE_RQ_HMAC,
      LMACCmd_OSC_VALUE_READ_RQ_HMAC,
      HMACCmd_OSC_VALUE_READ_IND_LMAC,
      LMACCmd_OSC_VALUE_WRITE_RQ_HMAC,
      HMACCmd_SLOT_FRAME_IND_LMAC,

      LMACCmd_SEND_DUMMY_RQ_HMAC       = 0x40,
      LMACCmd_A44_SET_RQ_HMAC,
      LMACCmd_TBR6_TEST_MODE_RQ_HMAC,
      LMACCmd_Qt_MESSAGE_SET_RQ_HMAC,
      LMACCmd_SWITCH_HO_TO_TB_RQ_HMAC,
                
      LMACCmd_PAGE_RQ_HMAC             = 0x50,
      LMACCmd_PAGE_CANCEL_RQ_HMAC,
      HMACCmd_BS_INFO_SENT_LMAC,

      HMACCmd_ACCESS_RQ_LMAC           = 0x60,
      LMACCmd_ACCESS_CFM_HMAC,
      HMACCmd_ESTABLISHED_IN_LMAC,
      LMACCmd_RELEASE_TBC_RQ_HMAC,
      HMACCmd_RELEASE_TBC_IN_LMAC,
      HMACCmd_CO_DATA_DTR_LMAC,
      LMACCmd_CO_DATA_RQ_HMAC,
      HMACCmd_CO_DATA_IN_LMAC,

      LMACCmd_ENC_KEY_RQ_HMAC          = 0x70,
      HMACCmd_ENC_EKS_IND_LMAC,
      HMACCmd_ENC_EKS_FAIL_LMAC,

      LMACCmd_VOICE_EXTERNAL_HMAC      = 0x80,
      LMACCmd_VOICE_INTERNAL_HMAC,
      LMACCmd_VOICE_CONFERENCE_HMAC,

	   LMACCmd_SLOTTYPE_MOD_RQ_HMAC			= 0x90,
	   HMACCmd_SLOTTYPE_MOD_CFM_LMAC,
	   HMACCmd_SLOTTYPE_MOD_TRIGGERED_LMAC,

	   LMACCmd_MODULE_RESET_RQ_HMAC			= 0xA0,			// FOR MODEM  SOFT RESET
	   HMACCmd_DEBUG_MESSAGE_IND_LMAC,

      LMACCmd_NOEMO_RQ_HMAC            = 0xA8,  // FOR MODEM  SOFT RESET
      HMACCmd_NOEMO_IND_LMAC

	  
} MAC_MESSAGE_ID;


/* Cosic Thread status type */
typedef enum
{
	COSIC_THREAD_INIT_STATUS,
	COSIC_THREAD_SNYC_STATUS,
	COSIC_THREAD_TRANSFER_STATUS,
	COSIC_THREAD_RECEIVE_STATUS
} COSIC_THREAD_STATUS;


typedef struct drv_uplane_data
{
      unsigned char PROCID;
      unsigned char MSG;
      unsigned char LengthHi;
      unsigned char LengthLo;
      unsigned char Status;
      unsigned char Parameter4;
      unsigned char Uplan[G_PTR_MAX_COUNT];
      unsigned char CurrentInc;
} UPLANE_PACKET;

/* Cosic data packet type */
typedef struct cosic_packet
{
	union
	{
		/* HMAC */
      unsigned char	temp_hmac[sizeof(HMAC_QUEUES)];
      HMAC_QUEUES   temp_hmac_struct;
		/* Dect synchronizatio packet */
		struct
		{
			unsigned char Sync_Packet_RxTxstatus;
			unsigned char Sync_Packet_TimeStamp;
			unsigned char Sync_Packet_Reserved;
		} SYNC_PACKET;

		/* DECT Setup Packet */
		struct
		{
			unsigned char Setup_Packet_SPIMode;
			unsigned char Setup_RXTX_status[13];
		} SETUP_PACKET;
		/* DECT Setup Packet */

#ifdef CATIQ_UPLANE
      UPLANE_PACKET DATA_PACKET;
#endif

		struct
		{
			  unsigned char debug_info_id;
			  unsigned char rw_indicator;		// read write indicator
			  unsigned char CurrentInc; 		
			  unsigned char G_PTR_buf[43/*G_PTR_MAX_DEBUG_COUNT*/];

		} DEBUG_PACKET;

	} uni;
} COSIC_PACKET;



#define MAX_TAPI_VOICE_BUFFER_SIZE			80


#define MAX_TAPI_BUFFER_CNT		200



/* TAPI packet defenition [[ */
#define TAPI_B_VOICE_PACKET					0x00
#define TAPI_B_DATA_PACKET					0x01
#define TAPI_MAC_COMMAND_PACKET				0x02
#define TAPI_SETUP_PACKET					0x03
#define TAPI_SYNC_PACKET					0x04



#define TAPI_VOICE_PACKET_TYPE_INDEX		0
#define TAPI_VOICE_PACKET_SLOT_INDEX		0
#define TAPI_VOICE_PACKET_TYPE(x)			(((x)&0xF0)>>4)
#define TAPI_VOICE_PACKET_SLOT(x)			((x)&0x0F)


#define TAPI_VOICE_PACKET_SUB_TYPE_INDEX	1
#define TAPI_VOICE_PACKET_SEQ_INDEX			1

#define TAPI_VOICE_PACKET_SUB_TYPE(x)		(((x)&0xF0)>>4)
#define TAPI_VOICE_PACKET_SEQ(x)			((x)&0x0F)

#define TAPI_VOICE_PACKET_LEN_HIGH			2
#define TAPI_VOICE_PACKET_LEN_LOW			3

#define TAPI_VOICE_PACKET_LEN(x,y)			((x)<<8 | (y))


#define TAPI_VOICE_PACKET_HEADER_LEN		8
/* TAPI packet defenition ]] */





/* tapi voice buffer struct */
typedef struct TAPI_voice_read_buffer
{
	unsigned char channel;
	unsigned char voice_info;
	unsigned char cyclic_index;
	unsigned char voice_len;
	//unsigned char voice_buffer[MAX_TAPI_VOICE_BUFFER_SIZE];
	unsigned char *p_voice_buffer;

} TAPI_VOICE_READ_BUFFER;




/* tapi voice buffer struct */
typedef struct TAPI_voice_write_buffer
{
	unsigned char type_slot;
	unsigned char info_cyclic_index;
	unsigned char voice_len_high;
	unsigned char voice_len_low;
	unsigned char burst_error_info[4];
	unsigned char voice_buffer[MAX_TAPI_VOICE_BUFFER_SIZE];
} TAPI_VOICE_WRITE_BUFFER;

#ifdef NOT_REQD
/* extern valialbe */
extern int	dect_gpio_intnum;		/*  valiable interrupt number */
#endif /* NOT_REQD */ /* Narendra - moved to drv_dect.c */

typedef struct semaphore*  COSIC_Driver_mutex_t;

extern wait_queue_head_t dect_interrupt_queue;	/*  process wait queue */

//extern kthread_t cosic_drv_kthread;
//extern kthread_t cosic_drv_TAPI_Read_kthread;					/* cosic kernel TAPI DATA read thread */
extern unsigned char Dect_mutex_free;
extern unsigned int Max_spi_data_len_val;

extern void start_tapiThread  ( void );
extern void stop_tapiThread   ( void );
extern void start_cosicThread ( void );
extern void stop_cosicThread  ( void );

/* for test */
void ifx_GPIO3_set(int on_off);

irqreturn_t Dect_GPIO_interrupt              ( int irq, void *dev_id );
void        gp_onu_enable_external_interrupt ( void );
void        Dect_int_GPIO_init               ( void );
void        trigger_cosic_driver             ( void );
void        gp_onu_disable_external_interrupt( void );



//void COSIC_drv_Kthread(kthread_t *pthread);
//void COSIC_drv_TAPI_Read_Kthread(kthread_t *pthread);

#endif
