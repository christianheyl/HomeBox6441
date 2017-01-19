/******************************************************************************

                              Copyright (c) 2011
                            Lantiq Deutschland GmbH
                     Am Campeon 3; 85579 Neubiberg, Germany

******************************************************************************/
#include <linux/autoconf.h>
#include <linux/module.h>

#include <linux/kernel.h>   /* printk() */
#include <linux/slab.h>
#include <linux/fs.h>       /* everything... */
#include <linux/errno.h>    /* error codes */
#include <linux/types.h>    /* size_t */
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/poll.h>
#include <linux/ioport.h>
#include <linux/vmalloc.h>
#include <linux/delay.h>
#include <linux/sched.h>
#include <linux/param.h>
#include <linux/timer.h>
#include <linux/ctype.h>
#include <linux/interrupt.h>
#include <asm/uaccess.h>

#include <asm/io.h>
#include "cosic_drv.h"	
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,32)
#include <asm/semaphore.h>
#endif

#include <asm/irq.h>

#include <linux/unistd.h>
#ifdef CONFIG_LTT
#include <linux/marker.h>
#endif

#ifdef CONFIG_AR9
#include <asm-mips/ifx/ar9/ar9.h>
#include <asm-mips/ifx/ar9/irq.h>
#include <asm-mips/ifx/ifx_gpio.h>
static const int dect_gpio_module_id = IFX_GPIO_MODULE_DECT;
#endif	

#ifdef DECT_USE_USIF
#include <asm/ifx/ifx_usif_spi.h>
#else
#include <asm/ifx/ifx_ssc.h>
#endif
/*Global Data*/
extern e_CosicDrv_Mode eDriverMode;
extern x_CosicDrv_Transaction vxTransaction; 
extern wait_queue_head_t Dect_WakeupList;

extern x_CosicDrv_Buffer xRxBuffer[MAX_READ_BUFFERS];
extern unsigned int RxOverflowFlag;
extern unsigned short RxBufHead;
extern unsigned short RxBufTail;

extern x_CosicDrv_Buffer xTxBuffer[MAX_WRITE_BUFFERS];
extern unsigned int TxOverflowFlag;
extern unsigned short TxBufHead;
extern unsigned short TxBufTail;

extern int viSPILocked;
extern int Dect_excpFlag;
extern int	dect_gpio_intnum;				/* interrupt number */
extern unsigned long iDummyDevId;
#ifdef CVOIP_ONBOARD
extern int ssc_dect_hardware_reset(int on);
#endif
extern volatile unsigned int IntEdgFlg;
extern IFX_SSC_HANDLE *spi_dev_handler;

void tasklet_FallingEdge(unsigned long iDummy);
void tasklet_RisingEdge(unsigned long iDummy);
//int IsLastPktRcvdDummy;


/*Global Data Ends*/
#ifdef CONFIG_AR9
#define ACKNOWLEDGE_IRQ  gp_onu_gpio_write_with_bit_mask((unsigned long *)IFX_ICU_EIU_INC, (8),(8))
#define CONFIGURE_FALLING_EDGE_INT  \
				*(unsigned long *) IFX_ICU_EIU_EXIN_C &= 0xFFFF0FFF; \
				*(unsigned long *) IFX_ICU_EIU_EXIN_C |= 0x2000
#define CONFIGURE_RISING_EDGE_INT  \
				*(unsigned long *) IFX_ICU_EIU_EXIN_C &= 0xFFFF0FFF; \
				*(unsigned long *) IFX_ICU_EIU_EXIN_C |= 0x1000

#endif

#define SUB_PKT_LEN 16

struct tasklet_struct x_tasklet_HandleRisingEdge;
struct tasklet_struct x_tasklet_HandleFallingEdge;
IFX_SSC_HANDLE *SPI_CB_Dummy_handler=NULL;
char vacTxBuf[SUB_PKT_LEN];
char vacRxBuf[SUB_PKT_LEN];
char vcSeqNumOfLastTxPkt;
char vcDummy;
#ifdef HDX_CONFIG
char vcSendDummy;
int iTxRxFlag;//if set do rx else tx
#endif

void gp_onu_enable_external_interrupt(void);
void gp_onu_disable_external_interrupt(void);

#define H0_PAYLEN_13 0xD0
#define H0_PAYLEN_0  0x00
#define H0_ACK  0x00
#define H0_NACK 0X08

#define H0_FIRST_PACK 0x04
#define H0_MORE_PACK 	0X04
#define H0_LAST_PACK 	0X00

#define H0_DUMMY_ACK  H0_PAYLEN_0 | H0_ACK | H0_LAST_PACK;
#define HO_DUMMY_NACK HO_PAYLEN_0 | HO_NACK | HO_LAST_PACK;

#define HO_IsACK(pcBuffer) (!(pcBuffer & (0x08)))

#define HO_IsPayLoadPresent(pcBuffer) ( ( ((pcBuffer) & 0xF0)?1:0 ) )
//#define HO_IsPayLoadPresent(pcBuffer) ( ( ((pcBuffer) && 0xF0)?1:0 ) )

#define H0_GetPacketLen(pcBuffer) (((pcBuffer) & 0xF0) >> 4)

#define H1_GetNACKIndex(pcBuffer) ((pcBuffer & 0x1F))

#define H0H1_GetTxIndex(pcBuffer) ((((pcBuffer[0] & 0x03 )<< 3) | ((pcBuffer[1] & 0xE0) >> 5)) & 0x1f)

#define H1_GetPacketIndex(pcBuffer) (((pcBuffer) & 0xF0) >> 4)

#define H0_IsFirstPacket(pcBuffer) 0

#define H0_IsLastPacket(pcBuffer) ( !((pcBuffer)&0x04) )

#define IN
#define OUT
#ifdef UNITTEST
	extern char vcCorruption;
	extern char vcMode;
	extern char vcRxData;
	static int iteration = 2;
#endif

void compute_crc(IN char *pcPayLoad,
                 IN char PayLoadlen,
                 OUT char *crc_generated);

int IsCrcValid(IN char *pcPayLoad, 
               IN char payLoadLen, 
               IN char crc_received);

#ifndef CONFIG_DANUBE
/******************************************************************
 *  Function Name  : gp_onu_gpio_write_with_bit_mask 
 *  Description    : This function OR's the GPIO register with
 *                   updated value after masking with bitmask
 *  Input Values   : addr - Register address
 *                   value - Value to be ORed ro existing content
 *                   bit_mask - Bit mask to be applied on given register
 *  Output Values  : None
 *  Return Value   : None
 *  Notes          : 
 * ****************************************************************/
static void 
gp_onu_gpio_write_with_bit_mask(volatile unsigned long *addr, 
                                volatile unsigned long value,
                                volatile unsigned long bit_mask)
{
  volatile unsigned long reg_value;
  reg_value = *addr;
  reg_value &= (~bit_mask);
  reg_value |= value;
  *addr = reg_value;
  return;
}
#endif
/******************************************************************
 *  Function Name  : gp_onu_gpio_write_with_bit_mask 
 *  Description    : This function OR's the GPIO register with
 *                   updated value after masking with bitmask
 *  Input Values   : addr - Register address
 *                   value - Value to be ORed ro existing content
 *                   bit_mask - Bit mask to be applied on given register
 *  Output Values  : None
 *  Return Value   : None
 *  Notes          : 
 * ****************************************************************/
static void 
dump_pkt_data(char *pcBuff)
{
  int i;
  DBGH("##############Packet############\n");
  DBGH("Len = [%u] \n Ack/Nack = [%u] \n Last/more = [%u] \n TxSeq = [%u] \n AckSeq = [%u] \n",
          ((pcBuff[0]>>4)&0x0F),((pcBuff[0]>>3) & 0x01),((pcBuff[0]>>2) & 0x01),(((pcBuff[0]&0x03)<<3)|(pcBuff[1]>>5)),(pcBuff[1]&0x1F));
  DBGH("Payload:\t");
  for(i=2;i<15;i++){
    DBGH("[%x][%c]\t",pcBuff[i],pcBuff[i]);
  }
  DBGH("\n");
  DBGH("CRC = [%d]\n",pcBuff[15]);
  DBGH("#########End of Packet##########\n");
  return;
}

/******************************************************************
 *  Function Name  : prepare_tx_buffer
 *  Description    : This function prepares the transmit buffer.
    Transmit bufffer has two parts - Header and Payload.
    vxTransaction.TxHdr[0]
      H0.7-H0.4 - Packet Length - to be filled as per number of Payload packets
      H0.3      - ACK Bit 0=ACK / 1= NACK 
      H0.2 - 0 - Last Packet  (Based on Payload)
	     1 - More Packet to come
      H0.1 - H0.0  First 2 bits of transmitted sequence number
    vxTransaction.TxHdr[1]
      H1.7-H1.5 Last 3 bits of transmitted sequence number
      H1.4-H1.0 Sequence number of ACK or NACK packet (Based on Rcvd data)
    vxTransaction.TxHdr[2-14] - 13 bytes of payload
    vxTransaction.TxHdr[15] - CRC byte
 *  Input Values   : 
 *  Output Values  :
 *  Return Value   : 
 *  Notes          : 
 * ****************************************************************/
void prepare_tx_buffer(char *pcBuffer)
{
  int len =   xTxBuffer[TxBufHead].len;
	DBGN("prepare_tx_buffer, Entry\n");
  DBGH("<prepare_tx_buffer>Entry, TxH0 is %x, TxH1is %x\n",vxTransaction.TxHdr[0],vxTransaction.TxHdr[1]);	
  DBGH("<prepare_tx_buffer>Entry, RxH0 is %x, RxH1is %x\n",vxTransaction.RxHdr[0],vxTransaction.RxHdr[1]);	
#ifdef HDX_CONFIG
  if(eDriverMode == COSIC_DRV_MODE_WR && IsTxStateSet(IN_PROGRESS) && len>0){
#else
  if(IsTxStateSet(IN_PROGRESS) && len>0){
#endif
    /*Payload to send*/
    /* Index points to the location where the payload starts */
    int index = xTxBuffer[TxBufHead].index;
    //int len =   xTxBuffer[TxBufHead].len;
    char pack_num;
    char PayLoadSize = 0;
    pack_num = (index / 13) + 1;
    memset(pcBuffer,0,SUB_PKT_LEN);
#if 0
    /* Either last or more not first pkt */
    if(1 == pack_num){
      pcBuffer[0] |= H0_FIRST_PACK;
    }
#endif
    /*Last Packet ?? */
    if((index + 13) >=  len && !IsTxStateSet(LAST_PACKET)){
      memcpy(&pcBuffer[2],&xTxBuffer[TxBufHead].acCommand[index],len-index);
      /* Fill payload size */
      PayLoadSize = (len-index);
			DBGH("payload size is %d\n",PayLoadSize);
      pcBuffer[0] = (PayLoadSize << 4)|H0_LAST_PACK;
      SET_TXSTATE(LAST_PACKET);
    }else{
      memcpy(&pcBuffer[2],&xTxBuffer[TxBufHead].acCommand[index],13);
      /* Fix first 4 bytes to len 13 */
      PayLoadSize = 13;
      pcBuffer[0] = (H0_PAYLEN_13 | H0_MORE_PACK);
			xTxBuffer[TxBufHead].index += 13;
    }
    /* Set Packet number is 5 bits (2bits in byte 1 and 3 in byte 2) */
    pcBuffer[0] |= (pack_num >> 3) & 0x03;
    pcBuffer[1] = (pack_num << 5) & 0xE0; 
		vxTransaction.TxHdr[2] = pack_num;//squence no of pkt(gw->cv) for which ack has to come
    /* H0 and H1 are partially ready - How about computing some CRC ?*/
    //compute_crc(pcBuffer,len,&pcBuffer[15]);
  }else if ( (!HO_IsPayLoadPresent(vxTransaction.RxHdr[0])) && (IsTxStateSet(LAST_PACKET) || IsTxStateSet(TRANS_OVER) || !H0H1_GetTxIndex(vxTransaction.RxHdr)) && HO_IsACK(vxTransaction.TxHdr[0])){
  	/* Store previous Tx Header */
    memset(pcBuffer,0,SUB_PKT_LEN);
  	vxTransaction.TxHdr[0] = pcBuffer[0]; 
 		vxTransaction.TxHdr[1] = pcBuffer[1];
		DBGH("Nothing to send from gw side, sending dummmy\n");
		vcDummy = 1;
 		return;
	}else if (len == 0 || IsTxStateSet(LAST_PACKET)){
		DBGH("len==0 case: H0:%x, H1:%x\n",pcBuffer[0],pcBuffer[1]);
		if (IsRxStateSet(TRANS_OVER) || IsRxStateSet(LAST_PACKET)){
			SET_TXSTATE(TRANS_OVER);
			DBGN("Tx state set to trans over......\n");
		}
	}else{
		DBGE("Inside else part, payload present in Rx:%d\n",HO_IsPayLoadPresent(vacRxBuf[0]));
	}
	//pcBuffer[1] |= (((vxTransaction.RxHdr[0] & 0x03 )<< 3) | ((vxTransaction.RxHdr[1] & 0xE0) >> 5));
	pcBuffer[1] |= H0H1_GetTxIndex(vxTransaction.RxHdr);
	//printk("Ackseq set to %d\n",(pcBuffer[1] & 0x1f));
  /*No Payload to transmit OR After partial construction of H0 /H1 */
  if((vxTransaction.TxHdr[0] & H0_NACK) && (pcBuffer[1] & 0x1f)){ 
    /*If NACK flag set - by HandleRxData */
		//printk("setting nack in tx pkt...\n");
    pcBuffer[0] |= H0_NACK;
  }
  else{ /*Check If ACK is being transmitted for last Rcvd pack*/
    if((IsRxStateSet(LAST_PACKET))){
			//printk("Rx Transaction over\n");
      SET_RXSTATE(TRANS_OVER);
    }
  }
  /* H0 and H1 are ready - Compute CRC */
	compute_crc(pcBuffer,strlen(pcBuffer),&pcBuffer[15]);
  //pcBuffer[1] |= (vxTransaction.RxHdr[1] & 0x1F); //Index of ACK/NACK
  /* Store previous Tx Header */
  vxTransaction.TxHdr[0] = pcBuffer[0]; 
  vxTransaction.TxHdr[1] = pcBuffer[1];
	if (IsTxStateSet(LAST_PACKET)){
		vcSeqNumOfLastTxPkt = (((vxTransaction.TxHdr[0] & 0x03)<<3) | ((vxTransaction.TxHdr[1] >> 5)&0x07));
	}
  /*Can we move to driver to UNLOCK_SPI? */
/*
  if((IsTxStateSet(TRANS_OVER) || IsTxStateSet(IDLE_STATE)) &&
     (IsRxStateSet(TRANS_OVER) || IsRxStateSet(IDLE_STATE))){
    //eDriverMode = COSIC_DRV_MODE_WAIT_UNLOCK_SPI;
  	printk("<prepare_tx_buffer> Set mode to COSIC_DRV_MODE_WAIT_UNLOCK_SPI\n");	
  } 
*/
	DBGN("prepare_tx_buffer, Exit\n");
  return;
}
/******************************************************************
 *  Function Name  : HandleRxData
 *  Description    : This function handles the received data.
      Received buffer has two parts.
        Headers and Payload
          Headers - ACK / NACK - for TxData
          Len  - No of Payload packets
          Index - Index of Payload packets
          Last Packet flag
          Payload - With CRC
 *  Input Values   : 
 *  Output Values  :
 *  Return Value   : 
 *  Notes          : 
 * ****************************************************************/
int HandleRxData(char *pcBuffer)
{
  int ACK_RCVD = 0, len =0, index =0, first_packet = 0, last_packet = 0,
      invalid_crc=0, payload_present =0, nack_index=0,SEND_NACK=0;
	DBGN("HandleRxData, Entry\n");
  DBGH("<HandleRxData>Entry, TxH0 is %x, TxH1:%x\n",vxTransaction.TxHdr[0],vxTransaction.TxHdr[1]);	
	if (!IsCrcValid(pcBuffer,15,pcBuffer[15])){
		DBGE("Invalid CRC, send Nack.., index is %d, TxBufHead:%d\n",xTxBuffer[TxBufHead].index,TxBufHead);
    if(IsTxStateSet(LAST_PACKET)){
    	SET_TXSTATE(IN_PROGRESS);
    }
		DBGH("HandleRxData, TxHdr-2 is %x\n",vxTransaction.TxHdr[2]);// gw is waiting for ack of this pkt
		if ((vxTransaction.TxHdr[2]-1) && H0H1_GetTxIndex(vxTransaction.TxHdr)){
    	xTxBuffer[TxBufHead].index = (vxTransaction.TxHdr[2]-2)*13;
			vxTransaction.TxHdr[2] = 0;
		}

    //vxTransaction.TxHdr[0] = H0_NACK;
		//if ((!vxTransaction.RxHdr[0] && !vxTransaction.RxHdr[1]) || !H0_IsLastPacket(vxTransaction.RxHdr[0])){ //if prev recvd pkt is not last OR first pkt from cv gets corrupted, inrement TxSeq to send nack for next pkt
		if (!H0_IsLastPacket(vxTransaction.RxHdr[0])){ //if prev recvd pkt is not last,inrement TxSeq to send nack for next pkt
    	//index = (((vxTransaction.RxHdr[0]&0x03)<<3)|(vxTransaction.RxHdr[1]>>5));
    	index = H0H1_GetTxIndex(vxTransaction.RxHdr);
			//printk("index is %d\n",index);
    	vxTransaction.RxHdr[2] = ++index;//pkt has to be sent again from CV
    	vxTransaction.RxHdr[0] = 0;
    	vxTransaction.RxHdr[1] = 0; 
    	//vxTransaction.RxHdr[0] |= (index >> 3) & 0x03;
    	//vxTransaction.RxHdr[1] = (index << 5) & 0xE0; 
    	//vxTransaction.TxHdr[0] = H0_NACK;
			
		}
		//if (H0H1_GetTxIndex(vxTransaction.TxHdr) == 1)
			//return 0; //Just txed first pkt, no ack from cv is missed due to corruption

  	return 0;
	}
#if DBGH == printk
	DBGH("<HandleRxData> Rx Buffer --------\n");
  dump_pkt_data(pcBuffer);
#endif
  ACK_RCVD = HO_IsACK(pcBuffer[0]);
  nack_index= H1_GetNACKIndex(pcBuffer[1]);
  payload_present = HO_IsPayLoadPresent(pcBuffer[0]);
  DBGH("Ack recd %d nack index %d payload_present %d\n",ACK_RCVD,nack_index,payload_present);
  if(payload_present){
    len = H0_GetPacketLen(pcBuffer[0]);
    invalid_crc = !(IsCrcValid(pcBuffer,len,pcBuffer[15]));
    first_packet = H0_IsFirstPacket(pcBuffer[0]);
    last_packet = H0_IsLastPacket(pcBuffer[0]);
    index = (((pcBuffer[0]&0x03)<<3)|(pcBuffer[1]>>5));
    DBGH("Len=%d inv CRC=%d first=%d last=%d index=%d\n",len,invalid_crc,first_packet,last_packet,index);
  }else if (!nack_index){
		DBGN("Dummy pkt recvd......\n");
		vxTransaction.TxHdr[0] &= 0xf7; //clear ack/nack bit
		vxTransaction.RxHdr[0] &= 0xfc;//clear nack index
		vxTransaction.RxHdr[1] &= 0x1f;//clear nack index
		if (vcDummy){
			DBGE("it's a dummy transaction, ending it..\n");
			SET_TXSTATE(TRANS_OVER);
			SET_RXSTATE(TRANS_OVER);
		}
  	return 0;
	}
	if (index == vxTransaction.RxHdr[2])
		vxTransaction.RxHdr[2] = 0;//pkt is recvd successfully after retransmit from cv 
  /*Act on ACK/NACK only if TxTrans in progress / last packet sent */
  if((IsTxStateSet(IN_PROGRESS)) || (IsTxStateSet(LAST_PACKET))){
    /*Do not handle Consecutive NACKs*/	
    if(!ACK_RCVD && HO_IsACK(vxTransaction.RxHdr[0])){ 
      //Retransmit - previous packet 
      DBGE("Retransmit prev pkt\n");
      xTxBuffer[TxBufHead].index = (nack_index-1)*13;
      if(IsTxStateSet(LAST_PACKET)){
        SET_TXSTATE(IN_PROGRESS);
      }
    }
    else{
      /*Either ACK Rcvd of consecutive NACKs - can be Ignored*/
      ACK_RCVD = 1;// Will be used to set RxHdr[0] 
      if(IsTxStateSet(LAST_PACKET) && nack_index && vcSeqNumOfLastTxPkt == nack_index){
        DBGN("Tx Transaction over\n");
        SET_TXSTATE(TRANS_OVER);
      }/*
      else{
        printk("Add 13 to index\n");
    		//vxTransaction.TxHdr[0] |= ((index << 3) >> 6);
    		//vxTransaction.TxHdr[1] |= (index << 5);
        //xTxBuffer[TxBufHead].index += 13;
      }*/
    }
  }
  if(payload_present && first_packet){
    if(!(IsRxStateSet(IDLE_STATE))){
      DBGE("Sending NACK\n");
      SEND_NACK =1; //TxHdr[0];
    }
  }
/*
  else if(payload_present && invalid_crc){
    if(1 == index){ // NACK for first packet
      printk("RX state is idle\n");
      SET_RXSTATE(IDLE_STATE);
    }	
    printk("Sending NACK-1\n");
    SEND_NACK=1;
  }
*/
  if(!SEND_NACK && payload_present){/*Valid Packet - Copy Payload*/
    int index = xRxBuffer[RxBufTail].index;
    memcpy(&xRxBuffer[RxBufTail].acCommand[index],pcBuffer+2,len);
    xRxBuffer[RxBufTail].index += len;
    xRxBuffer[RxBufTail].len += len;
    if(last_packet && !vxTransaction.RxHdr[2]){//last pkt received from cv and no pkt has to be retranmitted from cv
      DBGN("Set rx state to last pkt\n");
      SET_RXSTATE(LAST_PACKET);//Last packet with valid CRC - Send ACK in next Cycle
    }
  }
  /*Update what is received */
  vxTransaction.RxHdr[0] = pcBuffer[0];
  vxTransaction.RxHdr[1] = pcBuffer[1];
  /*Update ACK Flag*/
  if(ACK_RCVD){
    vxTransaction.RxHdr[0] = (pcBuffer[0] & 0xF7);
  }
  /*Update what to transmit next*/
  if(!SEND_NACK){
    vxTransaction.TxHdr[0] = H0_ACK;
  }
  else{
    vxTransaction.TxHdr[0] = H0_NACK;
  }
	/* if ack is recvd for last sent pkt, nothing is expected from cv, move cv to transaction over state*/
  //printk("<HandleRxData> vcSeqNumOfLastTxPkt:%x, sendnack:%d\n",vcSeqNumOfLastTxPkt,ACK_RCVD);	
	if (ACK_RCVD && vcSeqNumOfLastTxPkt == nack_index && !payload_present){
  	//printk("<HandleRxData> Set Rx state to trans over\n");	
		SET_RXSTATE(TRANS_OVER);
	}
  /*Can we move to driver to UNLOCK_SPI? */
/*
  if(!payload_present){
    if((IsTxStateSet(TRANS_OVER) || IsTxStateSet(IDLE_STATE))&&
       (IsRxStateSet(TRANS_OVER) || IsRxStateSet(IDLE_STATE))){
      //eDriverMode = COSIC_DRV_MODE_WAIT_UNLOCK_SPI;
  		printk("<HandleRxData> Set mode to COSIC_DRV_MODE_WAIT_UNLOCK_SPI\n");	
    }
  }
*/
  //printk("<HandleRxData>Exit, TxH0 is %x, TxH1:%x\n",vxTransaction.TxHdr[0],vxTransaction.TxHdr[1]);	
  //printk("<HandleRxData>Exit, RxH0 is %x, RxH1:%x\n",vxTransaction.RxHdr[0],vxTransaction.RxHdr[1]);	
  DBGN("<HandleRxData>Exit\n");	
  return 0;
}
/******************************************************************
 *  Function Name  : ifx_cosic_AsyncTxCB
 *  Description    : Callback invoked on successfully locking the 
 *                   Tx
 *  Input Values   : None
 *  Output Values  : None
 *  Return Value   : None
 *  Notes          : 
 * ****************************************************************/
void
ifx_cosic_AsyncTxRxCB(int iHandle, int iRetVal)
{
#if 0
  unsigned long flags;
#endif
  DBGN("<ifx_cosic_AsyncTxRxCB>Entry\n");
#ifdef UNITTEST
  static int i=0;
  static int a=0;
  static char acTemp[SUB_PKT_LEN];
	switch (vcMode){
	case 0: //GW -->> CVoIP
  if(i==0){
    memcpy(acTemp,vacTxBuf,SUB_PKT_LEN);
    memset(vacRxBuf,0,SUB_PKT_LEN);
    i=1;
  }
  else{
    int iTxSeq=0, iAckSeq=0;
    memcpy(vacRxBuf,acTemp,2);
    /* Swap the seq No */
    iTxSeq = ((acTemp[0] & 0x03)<<3) | ((acTemp[1] >> 5)&0x07);
    iAckSeq = acTemp[1] & 0x1F;
    vacRxBuf[0] &= 0x0F;  // no data from cv side
    vacRxBuf[1] &= ~(0x1F);  
    vacRxBuf[1] |= iTxSeq;  //aarif
    memcpy(acTemp,vacTxBuf,SUB_PKT_LEN);
		if (a == 0 && vcCorruption && vcCorruption == ((vacRxBuf[1] & 0x1F))){
			printk("Packet with ack number %d is corrupted\n",vcCorruption);
			vacRxBuf[0] |= 0x08;  // to recv Nack for the pkt
			i = 0; //next pkt has to be dummy from CV side
			a++; //corrupt only once
		}
		compute_crc(vacRxBuf,strlen(vacRxBuf),&vacRxBuf[15]);
  }
	break;
	case 1: //CVoIP -->> GW
	printk("RxData lentgh is %d\n",vcRxData);
  if(i==0){
    memcpy(acTemp,vacTxBuf,SUB_PKT_LEN);
    memset(vacRxBuf,0,SUB_PKT_LEN);
    i=1;
		if (vcRxData > 13){
			vacRxBuf[0] = 0xD4; //pktlen =13, more data bit set
			memset(&vacRxBuf[2],'A',SUB_PKT_LEN-2);
			vacRxBuf[1] |= 0x20;//TxSeq=1
			vcRxData -= 13;
		}else{ //last pkt
			vacRxBuf[0] = (vcRxData << 4) |H0_LAST_PACK;
			vacRxBuf[1] |= 0x20;//TxSeq=1
			printk("vacRxbuf-0 is %x\n",vacRxBuf[0]);
			memset(&vacRxBuf[2],'A',(int)vcRxData);
			vcRxData = 0;
		}
	compute_crc(vacRxBuf,strlen(vacRxBuf),&vacRxBuf[15]);
  }
  else{
    int iTxSeq=0, iAckSeq=0;
		i++;
    memcpy(vacRxBuf,acTemp,2);
    /* Swap the seq No */
    iTxSeq = ((acTemp[0] & 0x03)<<3) | ((acTemp[1] >> 5)&0x07);
    iAckSeq = acTemp[1] & 0x1F;
		if (vcRxData && vcRxData <= 13){//last pkt
			vacRxBuf[0] = (vcRxData << 4) |H0_LAST_PACK;
			vacRxBuf[1] |= (i<<5);
			memset(&vacRxBuf[2],'A',(int)vcRxData);
			vcRxData = 0;
		}else if (vcRxData > 13){
			vacRxBuf[0] = 0xD4; //pktlen =13, more data bit set
			memset(&vacRxBuf[2],'A',SUB_PKT_LEN-2);
			//vacRxBuf[1] |= 0x20;//TxSeq=1
			vacRxBuf[1] |= (i<<5);
			vcRxData -= 13;
		}
    vacRxBuf[1] &= ~(0x1F);  
    vacRxBuf[1] |= iTxSeq;  //aarif
    memcpy(acTemp,vacTxBuf,SUB_PKT_LEN);
		compute_crc(vacRxBuf,strlen(vacRxBuf),&vacRxBuf[15]);
		printk("Sequence number of pkt recvd from cv %d\n",((vacRxBuf[1] & 0xe0)>>5));
		if (a == 0 && vcCorruption && vcCorruption == ((vacRxBuf[1] & 0xe0)>>5)){
			printk("Packet with ack number %d is corrupted\n",vcCorruption);
			vacRxBuf[0] |= 0x08;  // to recv Nack for the pkt
			i = 0; //next pkt has to be dummy from CV side
			a++; //corrupt only once
		}
  }
	//compute_crc(vacRxBuf,strlen(vacRxBuf),&vacRxBuf[15]);
	break;
	case 2: //full duplex
  if(i==0){
    memcpy(acTemp,vacTxBuf,SUB_PKT_LEN);
    memset(vacRxBuf,0,SUB_PKT_LEN);
    i=1;
		memset(vacRxBuf,1,SUB_PKT_LEN);
		vacRxBuf[0] = 0xD0;
		vacRxBuf[1] = 0x20;
  }
  else{
    int iTxSeq=0, iAckSeq=0;
    memcpy(vacRxBuf,acTemp,2);
    /* Swap the seq No */
    iTxSeq = ((acTemp[0] & 0x03)<<3) | ((acTemp[1] >> 5)&0x07);
    iAckSeq = acTemp[1] & 0x1F;
		vacRxBuf[0] = 0x00;//just ack for prev pkt
    vacRxBuf[1] = 0x00;  
    vacRxBuf[1] |= iTxSeq;  //aarif
    memcpy(acTemp,vacTxBuf,SUB_PKT_LEN);
  }
	compute_crc(vacRxBuf,strlen(vacRxBuf),&vacRxBuf[15]);
	break;
	}//switch
#endif
/*
	printk("sent data is\n");
	for(i=2;i<15;i++){
    printk("%c",vacTxBuf[i]);
  }
	printk("\nrecvd data is\n");
	for(i=2;i<15;i++){
    printk("%c",vacRxBuf[i]);
  }
	printk("\n");
*/
  /* Handle the received data */
  //dump_pkt_data(vacRxBuf);
#ifdef HDX_CONFIG
	DBGH("ifx_cosic_AsyncTxRxCB RxH0:%x, RxH1:%x\n",vacRxBuf[0],vacRxBuf[1]);
	if (H0H1_GetTxIndex(vacRxBuf) || H1_GetNACKIndex(vacRxBuf[1]))
#endif
  HandleRxData(vacRxBuf);
#ifdef UNITTEST
  tasklet_RisingEdge(0);
#if 0
	if(HO_IsPayLoadPresent(vacRxBuf[0])){
		IsLastPktRcvdDummy = 0;
	}else{
		printk("Dummy pkt is recvd\n");
		IsLastPktRcvdDummy = 1;
	}
#endif
  HandleRxData(vacRxBuf);//aarif
	if ( IsTxStateSet(TRANS_OVER) && 
			(IsRxStateSet(TRANS_OVER)/* || IsRxStateSet(LAST_PACKET)*/) && 
			viSPILocked){
		// pull CS high
		printk("Transaction over, Releasing lock on SPI\n");
    //ifx_sscCosicUnLock();
		iteration = 2;
		i=0;
		eDriverMode = COSIC_DRV_MODE_IDLE;
		SET_TXSTATE(IDLE_STATE); 
		SET_RXSTATE(IDLE_STATE); 
		ifx_sscAsyncUnLock(spi_dev_handler);
    memset(acTemp,0,SUB_PKT_LEN);
	}else{
		printk("\n +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\nStarting Iteration = %d, TxState:%d, RxState:%d, Calling FallingEdge\n",iteration++,vxTransaction.TxHdr[3],vxTransaction.RxHdr[3]);
  	tasklet_FallingEdge(0);
	}
#endif
#if 0
  /* Do some error handling */
  atomic_set(&vuiIgnoreint,0);
  local_irq_save(flags);
  /* If rising edge is received already */
  if(IntEdgFlg == 0){
    ifx_sscCosicUnLock();
    //gw_stats.uiUnLockAtTxCB++;
  }
  local_irq_restore(flags);
  atomic_set(&vuiIgnoreint,0);
#endif
  DBGN("<ifx_cosic_AsyncTxRxCB>Exit\n");
  return;
}

/******************************************************************
 *  Function Name  : send_ssc_txrx_buffer
 *  Description    : The function sends and receives data from
 		     Cosic VoIP over SPI bus based on driver mode.
         COSIC_DRV_WR_MODE
         COSIC_DRV_RD_MODE
           Check if there is any valid data to send before preparing write buffer
         COSIC_DRV_RD_WR_MODE
         COSIC_DRV_LAST_PACKET
 *  Input Values   : void
 *  Output Values  : void
 *  Return Value   : void
 *  Notes          : 
 * ****************************************************************/
void
send_ssc_txrx_buffer(char *pcTxBuf,
                     char *pcRxBuf)
{
  IFX_SSC_ASYNC_CALLBACK_t xSscTaskletcb;
  DBGN("<send_ssc_txrx_buffer>Entry\n");
#if DBGH == printk
#ifdef HDX_CONFIG
	if (!iTxRxFlag){
		DBGH("<send_ssc_txrx_buffer> Tx Buffer +++++++\n");
  	dump_pkt_data(pcTxBuf);
	}
#endif
#endif
  memset(vacRxBuf,0,SUB_PKT_LEN);
  xSscTaskletcb.pFunction = ifx_cosic_AsyncTxRxCB;
  xSscTaskletcb.functionHandle = (int)spi_dev_handler;
#ifdef HDX_CONFIG
	if (iTxRxFlag){
  	DBGH("<send_ssc_txrx_buffer> Doing Rx --------\n");
  	if(ifx_sscAsyncRx(spi_dev_handler, &xSscTaskletcb, pcRxBuf, SUB_PKT_LEN) < 0){ 
    	DBGE("<send_ssc_txrx_buffer>AsyncTxRx Failed\n");
  	}
		iTxRxFlag = 0;//Next falling edge for Tx
	}else{
  	DBGH("<send_ssc_txrx_buffer> Doing Tx ++++++++\n");
  	if(ifx_sscAsyncTx(spi_dev_handler, &xSscTaskletcb, pcTxBuf, SUB_PKT_LEN) < 0){ 
    	DBGE("<send_ssc_txrx_buffer>AsyncTxRx Failed\n");
  	}
		iTxRxFlag = 1;//Next falling edge for Rx
	}
#else
  if(ifx_sscAsyncTxRx(spi_dev_handler, &xSscTaskletcb, pcTxBuf, SUB_PKT_LEN, 
     pcRxBuf, SUB_PKT_LEN) < 0){ 
    DBGE("<send_ssc_txrx_buffer>AsyncTxRx Failed\n");
  }
#endif
  DBGN("<send_ssc_txrx_buffer>Exit\n");
  return;
}
/******************************************************************
 *  Function Name  : CosicDrv_TxRxData
 *  Description    : The function sends and receives data from
 		     Cosic VoIP over SPI bus based on driver mode.
         COSIC_DRV_WR_MODE
         COSIC_DRV_RD_MODE
           Check if there is any valid data to send before preparing write buffer
         COSIC_DRV_RD_WR_MODE
         COSIC_DRV_LAST_PACKET
 *  Input Values   : void
 *  Output Values  : void
 *  Return Value   : void
 *  Notes          : 
 * ****************************************************************/
void CosicDrv_TxRxData(void)
{
  DBGN("<CosicDrv_TxRxData>Entry");
  switch(eDriverMode){
    case COSIC_DRV_MODE_RD: 
    {
      /* prevents sending another command */
#ifndef HDX_CONFIG
      if((IsTxStateSet(IDLE_STATE)) && !(IsTxBufEmpty)){
        SET_TXSTATE(IN_PROGRESS);
        eDriverMode = COSIC_DRV_MODE_RDWR;
      }
#endif
    }
    break; 
    case COSIC_DRV_MODE_WR: 
    case COSIC_DRV_MODE_RDWR:
    break;
    default:
      DBGE("<CosicDrv_TxRxData> State %d not handled\n",eDriverMode);
      return;
  }
  /* Prepare the data to be sent */
	//printk("current state is %d, RxHdr0 is %x\n",vxTransaction.TxHdr[3], vacRxBuf[0]);
	memset(vacTxBuf,0,SUB_PKT_LEN);
	memset(vacRxBuf,0,SUB_PKT_LEN);
#ifdef HDX_CONFIG
	// if SendDummy flag is set, don't call prepare_tx_buffer
	if (!vcSendDummy && !iTxRxFlag){
  	prepare_tx_buffer(vacTxBuf);
	}else if (vcSendDummy){
		vcSendDummy = 0;
	}
#else
  prepare_tx_buffer(vacTxBuf);
#endif
  /* Send or recive the data over the SPI bus */
#ifdef UNITTEST
  printk("<CosicDrv_TxRxData> Dump TxBuff data\n");
  dump_pkt_data(vacTxBuf);
	ifx_cosic_AsyncTxRxCB(0,0);
#else
  send_ssc_txrx_buffer(vacTxBuf,vacRxBuf);
#endif
  DBGN("<CosicDrv_TxRxData>Exit\n");
  return;
}
/******************************************************************
 *  Function Name  : CosicDrv_InitDataTransfer
 *  Description    : The second call back function invoked when SPI
    bus is locked. This callback can be invoked from two places.
    cosic_drv_write - When Driver is Idle mode and user
		      space app initiates command transfer. At this point
		      of time SPI is locked - Chip selected is pulled down.
		      Nothing to be done at this point of time.
    tasklet_FallingEdge - When interrupt comes and driver
			is COSIC_DRV_MODE_IDLE - the driver state is moved to
			COSIC_DRV_MODE_RX and then SPI Bus is locked.
			Once SPI is locked in recevie mode - driver can begin
			data transfer.
 *  Input Values   : 
 *  Output Values  :
 *  Return Value   : 
 *  Notes          : 
 * ****************************************************************/
void CosicDrv_InitDataTransfer (int iHandle, int iRetVal)
{
	DBGN("CosicDrv_InitDataTransfer, Entry\n");
  if(COSIC_DRV_MODE_WR == eDriverMode) {
    DBGN("<InitDataTransfer> In Tx Mode\n");
    SET_TXSTATE(IN_PROGRESS);
#ifdef UNITTEST
    //tasklet_FallingEdge(0);
#endif	
  }
  else if(COSIC_DRV_MODE_RD == eDriverMode){
    DBGN("<InitDataTransfer> In Rx Mode\n");
    SET_RXSTATE(IN_PROGRESS);
    if(IsRxBufFull){
      DBGE("<InitDataTransfer>Rx Buffer Full !!!\n");
      vxTransaction.TxHdr[0] |= H0_NACK;
    }
  }
  else{
    DBGE("<InitDataTransfer>Drv State %d not handled\n",eDriverMode);
  }
/*
	if ( IsTxStateSet(TRANS_OVER) &&
       IsRxStateSet(TRANS_OVER) &&
       viSPILocked){
    printk("Transaction over, Releasing lock on SPI\n");
    eDriverMode = COSIC_DRV_MODE_IDLE;
    SET_TXSTATE(IDLE_STATE);
    SET_RXSTATE(IDLE_STATE);
    ifx_sscAsyncUnLock(spi_dev_handler);
  }
*/
	DBGN("CosicDrv_InitDataTransfer, Exit\n");
  return;
}
/******************************************************************
 *  Function Name  : tasklet_FallingEdge
 *  Description    : 
 *  Input Values   : 
 *  Output Values  :
 *  Return Value   : 
 *  Notes          : 
 * ****************************************************************/
void tasklet_FallingEdge(unsigned long iDummy)
{		
	IFX_SSC_ASYNC_CALLBACK_t xSscTaskletcb;
	static int Iteration =1;
	int i;
	//int Diff =0;
#ifdef HDX_CONFIG
	e_CosicDrv_Mode eDriverModeLocal;
	if (COSIC_DRV_MODE_RD == eDriverMode && Iteration == 1){
		DBGH("<tasklet_FallingEdge> Setting vcSendDummy flag\n");
		vcSendDummy = 1;
	}
#endif
  DBGH("<tasklet_FallingEdge>Entry, \n==============================Iterartion:%d====\n",Iteration++);
  if(COSIC_DRV_MODE_IDLE == eDriverMode){
#ifdef AARIF
    IFX_SSC_ASYNC_CALLBACK_t xSscTaskletcb;
    xSscTaskletcb.pFunction = CosicDrv_InitDataTransfer;
    xSscTaskletcb.functionHandle = (int)SPI_CB_Dummy_handler;
    eDriverMode = COSIC_DRV_MODE_RD;
    printk("<tasklet_FallinEdge> Int in Idle-Mode\n");
    if(ifx_sscAsyncLock(spi_dev_handler,&xSscTaskletcb) < 0){
      printk("<tasklet_FallinEdge>SPI Lock failed\n");
      return;
    }	
#else
  DBGN("<tasklet_FallingEdge>Exit\n");
  return;
#endif
  }
  else 
	if((COSIC_DRV_MODE_WAIT_UNLOCK_SPI != eDriverMode) ||
	  (COSIC_DRV_MODE_MAX != eDriverMode)) { //aarif, review the condition
					vcDummy = 0;//clear the flag
			    CosicDrv_TxRxData();
  }
  else{
    DBGE("<tasklet_FallingEdge>Int in Invalid state\n");
  }

	if ( IsTxStateSet(TRANS_OVER) && 
		 	 IsRxStateSet(TRANS_OVER) && 
			 viSPILocked){
		DBGH("Transaction over, Releasing lock on SPI\n");
		Iteration = 1;
#ifdef HDX_CONFIG
		iTxRxFlag = 0;;//if set do rx else tx
		eDriverModeLocal = eDriverMode;
		DBGH("<tasklet_FallingEdge> eDriverModeLocal is %d\n",eDriverModeLocal);
#endif
		eDriverMode = COSIC_DRV_MODE_IDLE;
		SET_TXSTATE(IDLE_STATE); 
		SET_RXSTATE(IDLE_STATE); 
		ifx_sscAsyncUnLock(spi_dev_handler);
		//delay
		//udelay(300000);
#ifdef HDX_CONFIG
//#if 0
    if ((!vcDummy) && (eDriverModeLocal == COSIC_DRV_MODE_RD) && xRxBuffer[RxBufTail].len){//aarif
#else
    if ((!vcDummy) && xRxBuffer[RxBufTail].len){//aarif
#endif
      DBGH("data is available in RxBuff to be read by app...\n");
      DBGH("Now RxBufTail is %d, RxBufHead is %d\n",RxBufTail,RxBufHead);
			printk("Data just Recvd in RxBufTail %d is \n=====[",RxBufTail);
			for (i=0; i<xRxBuffer[RxBufTail].len; i++){
				printk("%c",xRxBuffer[RxBufTail].acCommand[i]);
			}
			printk("]=====\n");
      INC_RXBUF_TAIL;
      //DGBH("Seting Dect_excpFlag  and waking up wait queue.....\n");
      Dect_excpFlag = 1;
      wake_up_interruptible(&Dect_WakeupList);
    }
#ifdef HDX_CONFIG
    if ((eDriverModeLocal == COSIC_DRV_MODE_WR) && xTxBuffer[TxBufHead].len){//aarif
#else
    if (xTxBuffer[TxBufHead].len){//aarif
#endif
      DBGH("data is successfull sent to CVOIP written by app...\n");
			printk("Data just sent from TxBufHead %d is \n=====[",TxBufHead);
			for (i=0; i<xTxBuffer[TxBufHead].len; i++){
				printk("%c",xTxBuffer[TxBufHead].acCommand[i]);
			}
			printk("]=====\n");
			memset(&xTxBuffer[TxBufHead],0,sizeof(x_CosicDrv_Buffer));
      INC_TXBUF_HEAD;
      DBGH("Now tx head is %d, tail is %d\n",TxBufHead, TxBufTail);
		}
/*
		Diff = TxBufTail-TxBufHead;
		if (Diff<0)
			Diff += 12;
		if (((Diff>=0) && xTxBuffer[TxBufHead].len)){
*/
		if (xTxBuffer[TxBufHead].len){
			//udelay(100);
			DBGN("<We have more data to send\n");
    	xSscTaskletcb.pFunction = CosicDrv_InitDataTransfer;
    	xSscTaskletcb.functionHandle = (int)SPI_CB_Dummy_handler;
    	eDriverMode = COSIC_DRV_MODE_WR;
    	DBGH("Calling Async Lock\n");
    	if(ifx_sscAsyncLock(spi_dev_handler,&xSscTaskletcb) < 0) {
      	DBGE("<cosic_drv_write>SPI Lock Failed\n");
    		eDriverMode = COSIC_DRV_MODE_IDLE;
      	return;
    	}
		}
	}

  DBGN("<tasklet_FallingEdge>Exit\n");
  return;
}

/******************************************************************
 *  Function Name  : tasklet_RisingEdge
 *  Description    : Unlock SPI bus - check if there more data
 										 Reset - 
										  transaction related info
											driver state
										 If there is more data to send - pull down
										 CS.
 *  Input Values   : 
 *  Output Values  :
 *  Return Value   : 
 *  Notes          : 
 * ****************************************************************/
void tasklet_RisingEdge(unsigned long iDummy)
{
  if(COSIC_DRV_MODE_IDLE == eDriverMode){
    IFX_SSC_ASYNC_CALLBACK_t xSscTaskletcb;
    xSscTaskletcb.pFunction = CosicDrv_InitDataTransfer;
    xSscTaskletcb.functionHandle = (int)SPI_CB_Dummy_handler;
    eDriverMode = COSIC_DRV_MODE_RD;
    DBGN("<tasklet_FallinEdge> Int in Idle-Mode\n");
    if(ifx_sscAsyncLock(spi_dev_handler,&xSscTaskletcb) < 0){
      DBGE("<tasklet_FallinEdge>SPI Lock failed\n");
      return;
    }
	}
/*
	if ( IsTxStateSet(TRANS_OVER) && 
		 	 IsRxStateSet(TRANS_OVER) && 
			 viSPILocked){
		printk("Transaction over, Releasing lock on SPI\n");
		eDriverMode = COSIC_DRV_MODE_IDLE;
		SET_TXSTATE(IDLE_STATE); 
		SET_RXSTATE(IDLE_STATE); 
		ifx_sscAsyncUnLock(spi_dev_handler);
	}
*/
	//if (IsTxStateSet(TRANS_OVER))
  	//eDriverMode = COSIC_DRV_MODE_IDLE;
}

/******************************************************************
 *  Function Name  : CosicDrv_ISR
 *  Description    : Cosic driver is always interested in the first falling
                     edge interrupt and last rising edge interrupt.
                     First falling edge interrupt marks the beginning of
		     the transaction. A tasklet should be scheduled - to 
                     start the SPI data transfer.
                     When data transfer is complete - Transaction over
                     flag should be set. Only when this flag is set -
                     Rising edge interrupt should be handled and SPI lock
                     should be released.
 *  Input Values   : 
 *                   dev_id device id
 *  Output Values  : None
 *  Return Value   : Status of IRQ handling
 * ****************************************************************/
irqreturn_t CosicDrv_ISR(int irq, void *dev_id) 
{ 
  /* Acknowledge the interrupt  */
  ACKNOWLEDGE_IRQ;
  DBGN("I\n");
  //TODO gw_stats.uiInt++;/*Total number of interrupts got from cosic modem*/
  if(IntEdgFlg){
    DBGN("R\n");
    /*Rising edge so next should be for falling edge*/
    CONFIGURE_FALLING_EDGE_INT;
    IntEdgFlg =0;
    if(COSIC_DRV_MODE_WAIT_UNLOCK_SPI == eDriverMode || COSIC_DRV_MODE_IDLE == eDriverMode){
        tasklet_hi_schedule(&x_tasklet_HandleRisingEdge);
    }
  }
  else{
    DBGN("F\n");
    /*falling edge so next should be for rising edge*/
    CONFIGURE_RISING_EDGE_INT;
    IntEdgFlg =1;
    //TODO gw_stats.uiTmpInt++;
    /* schedule data transfer tasklet */
  	if(COSIC_DRV_MODE_IDLE != eDriverMode){
    	tasklet_hi_schedule(&x_tasklet_HandleFallingEdge);/*schedule tasklet for falling edge unlock*/
		}
  }
  return IRQ_HANDLED;
} 
/******************************************************************
 *  Function Name  : CosicDrv_GPIOInit
 *  Description    : GPIO interrupt setting for LMAC
 *  Input Values   : None
 *  Output Values  : None
 *  Return Value   : None
 *  Notes          : 
 * ****************************************************************/
void CosicDrv_GPIOInit(void)
{
  DBGN("<CosicDrv_GPIOInit>Entry\n");
#ifdef CONFIG_AR9
    ifx_gpio_pin_reserve(GPIO_INT,IFX_GPIO_MODULE_DECT);//39
		ifx_gpio_dir_in_set(GPIO_INT,IFX_GPIO_MODULE_DECT);
		ifx_gpio_altsel0_clear(GPIO_INT,IFX_GPIO_MODULE_DECT);
		ifx_gpio_altsel1_set(GPIO_INT,IFX_GPIO_MODULE_DECT);
		ifx_gpio_pudsel_set(GPIO_INT,IFX_GPIO_MODULE_DECT);
		ifx_gpio_puden_set(GPIO_INT,IFX_GPIO_MODULE_DECT);

    ifx_gpio_pin_reserve(GPIO_CS,IFX_GPIO_MODULE_DECT);//13
		ifx_gpio_dir_out_set(GPIO_CS,IFX_GPIO_MODULE_DECT);
		ifx_gpio_altsel0_clear(GPIO_CS,IFX_GPIO_MODULE_DECT);
#ifdef TEMP
		ifx_gpio_altsel1_set(GPIO_CS,IFX_GPIO_MODULE_DECT);
#else
		ifx_gpio_altsel1_clear(GPIO_CS,IFX_GPIO_MODULE_DECT);
#endif
		ifx_gpio_open_drain_set(GPIO_CS,IFX_GPIO_MODULE_DECT);

#ifdef CVOIP_ONBOARD
    ifx_gpio_pin_reserve(GPIO_RESET,IFX_GPIO_MODULE_DECT);//22
		ifx_gpio_dir_out_set(GPIO_RESET,IFX_GPIO_MODULE_DECT);
		ifx_gpio_altsel0_clear(GPIO_RESET,IFX_GPIO_MODULE_DECT);
		ifx_gpio_altsel1_clear(GPIO_RESET,IFX_GPIO_MODULE_DECT);
		ifx_gpio_open_drain_set(GPIO_RESET,IFX_GPIO_MODULE_DECT);

    //udelay(300000);
    /* reset pin setting low */
		//ssc_dect_hardware_reset(0);

    /* delay 10ms */
    //udelay(10000);

    /* reset pin setting high */
		//ssc_dect_hardware_reset(1);
#endif

#else
		ifx_gpio_register(IFX_GPIO_MODULE_DECT);
#endif	
  DBGN("<CosicDrv_GPIOInit>Exit\n");
}

/******************************************************************
 *  Function Name  : gp_onu_enable_external_interrupt 
 *  Description    : This function enables the GPIO interrupt
 *  Input Values   : None
 *  Output Values  : None
 *  Return Value   : None
 *  Notes          : 
 * ****************************************************************/
void gp_onu_enable_external_interrupt(void)
{
#ifdef CONFIG_AR9
  /* setting falling edge setting */
  gp_onu_gpio_write_with_bit_mask((unsigned long*)IFX_ICU_EIU_EXIN_C, (0x02000),(0x07000));
	IntEdgFlg = 0; /*Indicates falling edge*/
  /* external 5 interrupt enable */
  gp_onu_gpio_write_with_bit_mask((unsigned long*)IFX_ICU_EIU_INEN, (8),(8));
#endif
  tasklet_init(&x_tasklet_HandleRisingEdge,(void*)&tasklet_RisingEdge,
	       (unsigned long)&iDummyDevId);
  tasklet_init(&x_tasklet_HandleFallingEdge,(void*)&tasklet_FallingEdge,
	       (unsigned long)&iDummyDevId);
}
/******************************************************************
 *  Function Name  : gp_onu_disable_external_interrupt 
 *  Description    : This function disables the GPIO interrupt
 *  Input Values   : None
 *  Output Values  : None
 *  Return Value   : None
 *  Notes          : 
 * ****************************************************************/
void gp_onu_disable_external_interrupt(void)
{
  /* Disable interrupt */
#ifdef CONFIG_AR9
  gp_onu_gpio_write_with_bit_mask((unsigned long*)IFX_ICU_EIU_INEN, (0),(8));
#endif
}
/******************************************************************
 *  Function Name  : compute_crc
 *  Description    : This function computes the CRC for Tx Buffer
 *  Input Values   : Payload, Length
 *  Output Values  : Generated CRC
 *  Return Value   : None
 *  Notes          : 
 * ****************************************************************/
void compute_crc(IN char *pcPayLoad,
		 IN char PayLoadlen,
		 OUT char *crc_generated)

{
  int i;
  char cCrc=0;
  DBGN("<compute_crc>Entry\n");
  for(i=0;i<(SUB_PKT_LEN-1);i++){
    cCrc += pcPayLoad[i];
  }
  *crc_generated = -cCrc;
  DBGN("<compute_crc>Exit\n");
  return;
}
/******************************************************************
 *  Function Name  : IsCrcValid 
 *  Description    : This function Validates the CRC received
 *  Input Values   : Payload, Length, Received CRC
 *  Output Values  : None
 *  Return Value   : None
 *  Notes          : 
 * ****************************************************************/
int 
IsCrcValid(IN char *pcPayLoad, 
           IN char payLoadLen, 
           IN char crc_received)
{
  int i;
  char cCrc=0;
  DBGN("<IsCrcValid>Entry\n");
  for(i=0;i<SUB_PKT_LEN;i++){
    cCrc += pcPayLoad[i];
  }
  if(cCrc == 0){
    DBGH("<IsCrcValid>CRC is valid\n");
    return 1;
  }
  DBGE("<IsCrcValid>CRC is Invalid\n");
  return 0;
}
