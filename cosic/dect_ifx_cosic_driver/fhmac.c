/******************************************************************************

                              Copyright (c) 2009
                            Lantiq Deutschland GmbH
                     Am Campeon 3; 85579 Neubiberg, Germany

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/
/*****************************************************************************
*                                                                           *
*     Workfile   :  FMAC.PSL                                                *
*     Date       :  18 Nov, 2005                                       *
*     Contents   :  MAC.PSL - MEDIUM ACCESS LAYER FT SIDE                   *
*     Hardware   :  IFX 87xx                                               *
*                                                                           *
*****************************************************************************
*/
/* ========                                                             */
/* Includes                                                             */
/* ========                                                             */
#include <linux/string.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <asm/cpu-info.h>
#include "DECT.H"

#include "drv_dect.h"
#include "fhmac.h"
#include "fmbc.h"
#include "FMAC_DEF.H"
#include "FDEF.H"


/* ===============                                                      */
/* Local variables                                                      */
/* ===============                                                      */
/* tapi channel array */  
unsigned char Tapi_Channel_array[MAX_MCEI];
extern x_IFX_Mcei_Buffer xMceiBuffer[MAX_MCEI];
extern unsigned char KNL_STATE_OF_STACK[MAX_LINK];		/* check for mac layer     hiryu_20070911 */
extern unsigned int Max_spi_data_len_val;
extern int valid_voice_pkt[MAX_MCEI];
extern int iDriverStatus;
#if 1  // LOW_DUTY_SUPPORT
unsigned char     LowDuty_Support = FALSE;
#endif
/* ============================                                         */
/* Local Function Declarations                                          */
/* ============================                                         */
/* =====================                                                */
/* Function Definitions                                                 */
/* =====================                                                */

void HMAC_INIT(void)
{
  unsigned char i;
  Init_MBC();
  Init_BsCh_Queue();

  /* mcei array initial */
#if 0
  for(i=0;i<MAX_MCEI;i++)
	  Tapi_Channel_array[i] = 0xff;
#else
  for(i=0;i<MAX_MCEI;i++)
    xMceiBuffer[i].iKpiChan = 0xff;
#endif
}




/*
**************************************************************************
*                                                                        *
*     Function :  MBC_Release_TBC                                        *
*                                                                        *
**************************************************************************
*                                                                        *
*     Purpose  :  Routine performs all necessary actions which have to   *
*                 be done when a TB of an MBC has been released.         *
*     Parms    :  mcei: identifies the MBC of the released TB            *
*                 lbn: identifies the TB which has been released         *
*     Returns  :  none                                                   *
*     Remarks  :                                                         *
*                                                                        *
**************************************************************************
*/
static void MBC_Release_TBC (BYTE mcei, BYTE lbn)
{
  BYTE other_lbn;
  HMAC_QUEUES buffer;
  memset(&buffer, 0x00, sizeof(buffer));

  /* The actions to be performed      */
  /* depend on the MBC state.         */
  switch (Mcei_Table[mcei].Mbc_State)
  {
    /* The link was not established.    */
    case MBC_ST_CON_RQ:
      Init_Mcei_Table_Element(mcei);
      break;

    /* The link was established. Inform */
    /* the DLC.                         */
    #ifdef DECT_NG
    case MBC_ST_SM_INVOKING:
    #endif
    case MBC_ST_EST:
      Init_Mcei_Table_Element(mcei);
      {
        buffer.PROCID = LC;
        buffer.MSG = LC_DIS_IN_MAC;
        buffer.CurrentInc= mcei;
        Dect_SendtoStack(&buffer);
      }
      break;

    /* HO: one of the two TB's has been */
    /* released.                        */
    #ifdef DECT_NG
    case MBC_ST_CM_HO:
    {
    /* Confirm Slottype modification  */
    /* ---------------------------------*/
    /* P1: Logical Connection No (LCN)  */
    /* P2: MCEI                    */
    /* P3: Slot_type  ( 0x03: MT_ATTRIBUTES_SLOT_TYPE_LONG, 0x00:MT_ATTRIBUTES_SLOT_TYPE_FULL)                   */
    /* P4: success(1)/failure(0)             */
    // TODO: change plz....
#if 0
    Send_Message_To_APP( FP_SLOTTYPE_MOD_IN_MAC, NULL, mcei , lbn, 0, Mcei_Table[mcei].wbs_req, TRUE  );
#endif

			/* it is  special interface */
			/* hmac --> app    derect send command */
			buffer.PROCID = PROCMAX;
			buffer.MSG = FP_SLOTTYPE_MOD_IN_MAC_TO_APP;
			buffer.Parameter1 = mcei;
			buffer.Parameter3 = Mcei_Table[mcei].wbs_req;	// slot type

			buffer.Parameter4 = TRUE;	// success / fail
	
			buffer.CurrentInc= mcei;
			Dect_SendtoStack(&buffer);


    /* break; */
    }
    #endif
    case MBC_ST_HO:
      Detach_TBC_from_MBC(mcei, lbn);
      Mcei_Table[mcei].Mbc_State = MBC_ST_EST;
      /* Note: The remaining TB's type is */
      /* changed to TRAFFIC_BEARER and    */
      /* Ref_Lbn is cleared.              */
      /* This has to be protected since   */
      /* on ISR level a TB can be         */
      /* converted to a DB anytime.       */
      other_lbn = Get_Lbn_of_only_TB(mcei);

      buffer.PROCID = LMAC;
      buffer.MSG = MAC_SWITCH_HO_TO_TB_RQ_HMAC;
      buffer.Parameter1 = other_lbn;
      buffer.Parameter2 = mcei;
      Dect_SendtoLMAC(&buffer);
      break;

    /* Local Release Request            */
    case MBC_ST_RELEASE_RQ:
      Detach_TBC_from_MBC (mcei, lbn);
      /* if the MBC has no LBN any more   */
      /* the MBC's release is complete    */
      if ( (Mcei_Table[mcei].lbn_1 == NO_LBN) && (Mcei_Table[mcei].lbn_2 == NO_LBN) )
      {
        Init_Mcei_Table_Element(mcei);
        {
          buffer.PROCID = LC;
          buffer.MSG = LC_DIS_CFM_MAC;
          buffer.CurrentInc= mcei;
          Dect_SendtoStack(&buffer);
        }
      }
      else
      {
        /* just wait for the other TB to be released */
      }
      break;

    default:
      break;
  }
}



void DECODE_HMAC(HMAC_QUEUES *value)
{
  HMAC_QUEUES buffer;
  memset(&buffer, 0x00, sizeof(buffer));

  DECT_DEBUG_MED("\nPROCID=%d MSG=%d Para1=%d Para2=%d Para3=%d Para4=%d\n", value->PROCID, value->MSG, 
  	value->Parameter1, value->Parameter2, value->Parameter3, value->Parameter4);

 
  switch (value->MSG) 
  {
#ifdef CATIQ_UPLANE
    case MAC_FU10_DATA_IN:
      // We received a packet and need to hand it upwards
      //printk("\xd\xa***Post FU10 2DectStack\xd\xa");
#ifdef DEBUG_GPIO
  		   ssc_dect_haredware_reset(0);
#endif
#ifndef LTQ_RAW_DPSU
      {
         int  i; 
         unsigned char CRC = 0;

         for( i = 0;  i < 64;  i++ )
         {
            CRC += value->G_PTR_buf[i];
         }

         if( value->Parameter1 != CRC )
         {
            value->Parameter3 &= ~0x10;      // Clear IP Data OK Flag for discarding PDU Data in FU10 Layer
            value->G_PTR_buf[0] = 0x00;      // Clear 1st 3 Bytes in PDU for discarding PDU Data in FU10 Layer
            value->G_PTR_buf[1] = 0x00;
            value->G_PTR_buf[2] = 0x00;
         }

         value->Parameter1 = 0;
         Dect_SendtoStack(value);
	   }
#else
      printk("MAC_FU10_DATA_IN\n");
      Dect_SendtoStack(value);
#endif
#ifdef DEBUG_GPIO
  		   ssc_dect_haredware_reset(1);
#endif
         //printk("\xd\xa***Post OK\xd\xa");
         break;

    case MAC_FU10_DATA_RQ:
         // TODO send buffer via SPI
         // ???: Kann das gehen? Antwort: JA!
         //printk("\xd\xa***Post FU10 2 LMAC\xd\xa");
#ifdef DEBUG_GPIO
  		   ssc_dect_haredware_reset(0);
#endif
         //printk("MAC_FU10_DATA_RQ\n");
         Dect_SendtoLMAC(value);
#ifdef DEBUG_GPIO
  		   ssc_dect_haredware_reset(1);
#endif
         //printk("\xd\xa***Post OK\xd\xa");
         break;
#endif
	case MAC_DEBUGMSG_IN_LMAC:
#if 1
       DECT_DEBUG_HIGH("DBG=%02x:%02x%02x%02x%02x%02x %02x%02x%02x%02x%02x %02x%02x%02x\r\n",
       value->Parameter1, value->Parameter2, value->Parameter3, value->Parameter4,
       value->G_PTR_buf[0], value->G_PTR_buf[1], value->G_PTR_buf[2], value->G_PTR_buf[3],
       value->G_PTR_buf[4], value->G_PTR_buf[5], value->G_PTR_buf[6], value->G_PTR_buf[7],
       value->G_PTR_buf[8], value->G_PTR_buf[9] );
#else
       DECT_DEBUG_HIGH("DBG=%02x:%02x%02x %02x%02x[%02d%02d%02d%02d]\n",
       value->Parameter1, value->Parameter2, value->Parameter3, value->G_PTR_buf[4], value->Parameter4,
       value->G_PTR_buf[0], value->G_PTR_buf[1], value->G_PTR_buf[2], value->G_PTR_buf[3] );
#endif
         break;
#if 0
       DECT_DEBUG_HIGH("DBG=%02x:%02x%02x %02x%02x[%02d%02d%02d%02d]\n",
       value->Parameter1, value->Parameter2, value->Parameter3, value->G_PTR_buf[4], value->Parameter4,
       value->G_PTR_buf[0], value->G_PTR_buf[1], value->G_PTR_buf[2], value->G_PTR_buf[3] );
       break;
#endif

    case MAC_SEND_DUMMY_RQ_ME:  /*  IN LINE CODE T0011    */
      /* TRANSITION:      T0011                                                */
      /* EVENT:           MAC_SEND_DUMMY_RQ_ME                                 */
      /* DESCRIPTION:     create new dummy bearer                              */
      /* STARTING STATE:  AI_ATAD_AT                                           */
      /* END STATE:       AI_ATAD_AT                                           */
      /* ----------------------------------------------------------------------*/
      buffer.PROCID = LMAC;
      buffer.MSG = MAC_SEND_DUMMY_RQ_HMAC;
      buffer.Parameter1 = value->Parameter1;
      buffer.Parameter2 = value->Parameter2;
      buffer.Parameter3 = value->Parameter3;
      buffer.Parameter4 = value->Parameter4;
      memcpy(buffer.G_PTR_buf, value->G_PTR_buf, G_PTR_MAX_COUNT);
      Dect_SendtoLMAC(&buffer);


	  DECT_DEBUG_HIGH("MAC_SEND_DUMMY_RQ_ME  MSG=%d  G_PTR[4]=%d\n",buffer.MSG, buffer.G_PTR_buf[4]);
       //printk("\n MAC_SEND_DUMMY_RQ_ME:\n");
      break;

    case MAC_MCEI_REQUEST_LMAC:  /*  IN LINE CODE T0100    */
      {
        /* TRANSITION:      T100                                                 */
        /* EVENT:           MAC_MCEI_REQUEST_LMAC                                 */
        /* DESCRIPTION:     A portable has made either a 'basic access rq' or a  */
        /*                  'basic handover rq'. The process level must process  */
        /*                  this request.                                        */
        /* STARTING STATE:  AI_ATAD_AT                                           */
        /* END STATE:       AI_ATAD_AT                                           */
        /* REMARK:          meanwhile on ISR level 'wait messages' are exchanged.*/
        /* ----------------------------------------------------------------------*/
        BYTE lbn, b_type,  temp_lbn=NO_LBN;
        BYTE mcei;
        BYTE i;

        /* Traffic Bearer Part              */
        /* ================================ */
        lbn = value->Parameter1;
        b_type = value->Parameter2;

        if( b_type == TRAFFIC_BEARER)
        {
          /* it is a basic access request;    */
          /* a new MBC is needed              */
          /* -------------------------------- */
          mcei = New_Mcei();
          if (mcei != NO_MCEI)
          {
            Mcei_Table[mcei].Mbc_State = MBC_ST_CON_RQ;
            Attach_TBC_to_MBC (mcei, lbn);
            /* copy PMID into MBC data element  */
            for (i = 0; i < 3; i++)
              Mcei_Table[mcei].pmid[i] = value->G_PTR_buf[i];

#if 0
             printk("SET PMID Mcei_Table[%d].pmid[%d %d %d]\n", mcei, Mcei_Table[mcei].pmid[0], Mcei_Table[mcei].pmid[1], Mcei_Table[mcei].pmid[2]);


            for (i = 0; i < 6; i++)
             printk("SET PMID Mcei_Table[%d].pmid[%d %d %d]\n", i, Mcei_Table[i].pmid[0], Mcei_Table[i].pmid[1], Mcei_Table[i].pmid[2]);
#endif
			
          }
        }
        else if ( b_type == HANDOVER_BEARER )
        {
          /* it is a handover request; get    */
          /* the MBC of the other TB          */
          /* ----------------------------------- */
          mcei = Get_Mcei(lbn, value->G_PTR_buf);
          /* proceed only if a MBC is found   */
          /* and the MBC is in the right state*/
          if ( mcei != NO_MCEI )
          {
#ifdef DECT_NG
              if(( Mcei_Table[mcei].Mbc_State == MBC_ST_EST ) || ( Mcei_Table[mcei].Mbc_State == MBC_ST_SM_INVOKING ))
              {
                  if( Mcei_Table[mcei].Mbc_State == MBC_ST_SM_INVOKING )
                      Mcei_Table[mcei].Mbc_State = MBC_ST_CM_HO;
                  else if( Get_no_of_mcei_assigned( ) )
//                  else if( (Get_CC_State( mcei ) == 0) || Get_no_of_mcei_assigned( ) )
                  {
					 buffer.PROCID = LMAC;
					 buffer.MSG = MAC_MCEI_CONFIRM_HMAC;
					 buffer.Parameter1 = lbn;
					 buffer.Parameter2 = NO_MCEI;
					 buffer.Parameter3 = 0;
					 buffer.Parameter4 = 0;
					 Dect_SendtoLMAC(&buffer);
                     return;
                  }
				  else
                      Mcei_Table[mcei].Mbc_State = MBC_ST_HO;
#else
            if( Mcei_Table[mcei].Mbc_State == MBC_ST_EST )
            {
              Mcei_Table[mcei].Mbc_State = MBC_ST_HO;
#endif
              temp_lbn = Get_Lbn_of_only_TB (mcei);

              Attach_TBC_to_MBC (mcei, lbn);
            }
            else
            {
              mcei = NO_MCEI;
            }
          }
        }
        else  // No bearer type -- For safety!!!
        {
          mcei = NO_MCEI;
        }
        buffer.PROCID = LMAC;
        buffer.MSG = MAC_MCEI_CONFIRM_HMAC;
        buffer.Parameter1 = lbn;
        buffer.Parameter2 = mcei;
        buffer.Parameter3 = 0;
        buffer.Parameter4 = temp_lbn;
        Dect_SendtoLMAC(&buffer);
		DECT_DEBUG_HIGH("MAC_MCEI_CONFIRM_HMAC  MSG=%d para1=%d para2=%d\n", buffer.MSG, buffer.Parameter1, buffer.Parameter2);
		
      }
      break;

    case MAC_DIS_RQ_LC:  /*  IN LINE CODE T0202    */
      {
        /* TRANSITION:      T202                                                 */
        /* EVENT:           MAC_DIS_RQ_LC / MAC_DIS_RQ_MAC                       */
        /* DESCRIPTION:     LC Layer requires Release                            */
        /* STARTING STATE:  AI_ATAD_AT                                           */
        /* END STATE:       AI_ATAD_AT                                           */
        /* ----------------------------------------------------------------------*/
        BYTE mcei, lbn;
        mcei = value->Parameter1;
		
		DECT_DEBUG_HIGH("\nMAC_DIS_RQ_LC  mcei=%d Mcei_Table[].Mbc_State=%d\n ", mcei, Mcei_Table[mcei].Mbc_State);
		
        if ( mcei < MAX_MCEI )
        {
          switch (Mcei_Table[mcei].Mbc_State)
          {
#ifdef DECT_NG
            case MBC_ST_SM_INVOKING:
#endif
            case MBC_ST_EST:
              Mcei_Table[mcei].Mbc_State = MBC_ST_RELEASE_RQ;
              lbn = Get_Lbn_of_only_TB(mcei);

              buffer.PROCID = LMAC;
              buffer.MSG = MAC_RELEASE_TB_RQ_HMAC;
              buffer.Parameter1 = lbn;
              buffer.Parameter2 = mcei;
              buffer.Parameter3 = 0;
              buffer.Parameter4 = 0;
              Dect_SendtoLMAC(&buffer);
              break;

#ifdef DECT_NG
            case MBC_ST_CM_HO:
#endif
            case MBC_ST_HO:
              Mcei_Table[mcei].Mbc_State = MBC_ST_RELEASE_RQ;

              buffer.PROCID = LMAC;
              buffer.MSG = MAC_RELEASE_TB_RQ_HMAC;
              buffer.Parameter1 = Mcei_Table[mcei].lbn_1;
              buffer.Parameter2 = mcei;
              buffer.Parameter3 = 0;
              buffer.Parameter4 = 0;
              Dect_SendtoLMAC(&buffer);

              buffer.Parameter1 = Mcei_Table[mcei].lbn_2;
              Dect_SendtoLMAC(&buffer);
			  
              break;

            default:
              break;
          }
        }
      }

		DECT_DEBUG_HIGH("MAC_DIS_RQ_LC  MSG=%d para1=%d para2=%d\n", buffer.MSG, buffer.Parameter1, buffer.Parameter2);


	
      break;

    case MAC_CO_DATA_RQ_LC:  /*  IN LINE CODE T0400    */
      {
        /* TRANSITION:      T400                                                 */
        /* EVENT:           MAC_CO_DATA_RQ_LC                                    */
        /* DESCRIPTION:     Data request via CS Channel                          */
        /* STARTING STATE:  AI_ATAD_AT                                           */
        /* END STATE:       AI_ATAD_AT                                           */
        /* ----------------------------------------------------------------------*/
        BYTE mcei;
        mcei = value->Parameter1;
		
		DECT_DEBUG_MED("MAC_CO_DATA_RQ_LC mcei=%d Mbc_State=%d \n", mcei, Mcei_Table[mcei].Mbc_State);
		
        if ( mcei < MAX_MCEI )
        {
          if ( (Mcei_Table[mcei].Mbc_State == MBC_ST_EST) || 
#ifdef DECT_NG
               (Mcei_Table[mcei].Mbc_State == MBC_ST_SM_INVOKING)  ||
               (Mcei_Table[mcei].Mbc_State == MBC_ST_CM_HO)  ||
#endif
             (Mcei_Table[mcei].Mbc_State == MBC_ST_HO) )
          {
            buffer.PROCID = LMAC;
            buffer.MSG = MAC_CO_DATA_RQ_HMAC;
            buffer.Parameter1 = mcei;
            memcpy(buffer.G_PTR_buf, value->G_PTR_buf, G_PTR_MAX_COUNT);
            Dect_SendtoLMAC(&buffer);
            DECT_DEBUG_HIGH("MAC_DATA_RQ mcei=%x [%2x %2x %2x %2x %2x]\n", buffer.Parameter1, buffer.G_PTR_buf[ 0 ],
            buffer.G_PTR_buf[ 1 ], buffer.G_PTR_buf[ 2 ], buffer.G_PTR_buf[ 3 ], buffer.G_PTR_buf[ 4 ] );
          }
        }
      }
	
	
      break;

    case MAC_RELEASED_TB_LMAC:  /*  IN LINE CODE T0600    */
      {
        /* TRANSITION:      T0600                                                */
        /* EVENT:           MAC_RELEASED_TB_LMAC                             */
        /* DESCRIPTION:     A traffic bearer has been released on LMAC      */
        /* STARTING STATE:  AI_ATAD_AT                                           */
        /* END STATE:       AI_ATAD_AT                                           */
        /* ----------------------------------------------------------------------*/
        BYTE lbn, b_type;
        BYTE mcei;
        lbn = value->Parameter1;
        b_type = value->Parameter2;
        mcei = value->Parameter3;

		DECT_DEBUG_HIGH("\nMAC_RELEASED_TB_LMAC lbn=%d b_type=%d mcei=%d\n", lbn, b_type, mcei);

		
        /* Note: if access was not granted  */
        /* an MBC has not been allocated.   */
        if ( mcei != NO_MCEI )
        {
          MBC_Release_TBC (mcei, lbn);
        }
		else
		{
			/* Note: if released just before assigned MCEI reached */
			/* => The MCEI table should be cleared.   */
			mcei = Get_Mcei_of_only_TB(lbn);

			if( mcei != NO_MCEI )
			MBC_Release_TBC (mcei, lbn);
		}
		/* Reset the valid voice packet condition so that for next
		session PLC is not played before a valid voice pkt */
		if(mcei != NO_MCEI)
		  valid_voice_pkt[mcei] = 0;
      }
      break;

    case MAC_ESTABLISHED_TB_LMAC:  /*  IN LINE CODE T0611    */
      {
        /* TRANSITION:      T0611                                                */
        /* EVENT:           MAC_ESTABLISHED_TB_LMAC                          */
        /* DESCRIPTION:     A traffic bearer has been established on LMACl.  */
        /* STARTING STATE:  AI_ATAD_AT                                           */
        /* END STATE:       AI_ATAD_AT                                           */
        /* ----------------------------------------------------------------------*/
        BYTE lbn;
        BYTE mcei;
        //BYTE i;
	#ifdef DECT_NG
        BYTE s_type =  value->Parameter4;
	#endif

        lbn = value->Parameter1;
        mcei = value->Parameter3;


		DECT_DEBUG_HIGH("MAC_ESTABLISHED_TB_LMAC lbn=%d mcei=%d Mbc_State=%d \n", lbn, mcei, Mcei_Table[mcei].Mbc_State);

        switch ( Mcei_Table[mcei].Mbc_State )
        {
          case MBC_ST_CON_RQ:
            {
              Mcei_Table[mcei].Mbc_State = MBC_ST_EST;

              /* inform the DLC                   */
              buffer.PROCID = LC;
              buffer.MSG = LC_CON_IN_MAC;
#ifdef DECT_NG
	      buffer.Parameter1 = s_type;
#endif
              buffer.CurrentInc= mcei;

	  
              Dect_SendtoStack(&buffer);

		
              buffer.MSG = LC_CO_DATA_DTR_MAC;
              Dect_SendtoStack(&buffer);
            }
            break;

          case MBC_ST_EST:
            Mcei_Table[mcei].Mbc_State = MBC_ST_HO;
            break;

#ifdef DECT_NG
          case MBC_ST_SM_INVOKING:
          {
          Mcei_Table[mcei].Mbc_State = MBC_ST_CM_HO;
          break;
          }
#endif

          case MBC_ST_RELEASE_RQ:
          default:
          /* nothing has to be done; the release has already been initiated */
            break;
        }
		DECT_DEBUG_HIGH("MAC_ESTABLISHED_TB_LMAC MSG=%d Mbc_State=%d mcei=%d\n", buffer.MSG, Mcei_Table[mcei].Mbc_State, mcei);
      }
	
      break;

    case MAC_CO_DATA_IN_LMAC:  /*  IN LINE CODE T0700    */
      {
        /* TRANSITION:      T0700                                                */
        /* EVENT:           MAC_CO_DATA_IN_LMAC                                   */
        /* DESCRIPTION:     BMC ISR Level indicates the reception of a Ct        */
        /*                  fragment.                                            */
        /* STARTING STATE:  AI_ATAD_AT                                           */
        /* END STATE:       AI_ATAD_AT                                           */
        /* ----------------------------------------------------------------------*/
        BYTE mcei = value->Parameter1;
        buffer.PROCID = LC;
        buffer.MSG = LC_CO_DATA_IN_MAC;
        memcpy(buffer.G_PTR_buf, value->G_PTR_buf, G_PTR_MAX_COUNT);
        buffer.CurrentInc= mcei;
        Dect_SendtoStack(&buffer);
        DECT_DEBUG_HIGH("MAC_DATA_IN_LMAC mcei=%d G_PTR[%2x %2x %2x %2x %2x]\n", mcei, buffer.G_PTR_buf[0],
        buffer.G_PTR_buf[1],buffer.G_PTR_buf[2],buffer.G_PTR_buf[3],buffer.G_PTR_buf[4] );
      }
      //DECT_DEBUG_HIGH("MAC_CO_DATA_IN_LMAC MSG=%d G_PTR[%x %x %x %x %x]\n", buffer.MSG, buffer.G_PTR_buf[0],
		//buffer.G_PTR_buf[1],buffer.G_PTR_buf[2],buffer.G_PTR_buf[3],buffer.G_PTR_buf[4]);
	
      break;

    case MAC_CO_DATA_DTR_LMAC:  /*  IN LINE CODE T0701    */
      /* TRANSITION:      T0701                                                */
      /* EVENT:           MAC_CO_DATA_DTR_LMAC                                  */
      /* DESCRIPTION:     BMC ISR Level indicates that a Ct fragment has been  */
      /*                  sent and acknowledged by the PT.                     */
      /* STARTING STATE:  AI_ATAD_AT                                           */
      /* END STATE:       AI_ATAD_AT                                           */
      /* ----------------------------------------------------------------------*/
      /* the message is passed on to the  */
      /* LC(PARAMETER1=mcei)              */
      buffer.PROCID = LC;
      buffer.MSG = LC_CO_DATA_DTR_MAC;
      buffer.CurrentInc= value->Parameter1;
      Dect_SendtoStack(&buffer);
      DECT_DEBUG_HIGH("MAC_DATA_DTR_LMAC CurrentInc=%d\n", buffer.CurrentInc );

      break;

    case MAC_PAGE_RQ_LB:  /*  IN LINE CODE T0401    */
      /* TRANSITION:      T401                                                 */
      /* EVENT:           MAC_PAGE_RQ_LB                                       */
      /* DESCRIPTION:     Bs channel data request via Pt paging                */
      /* STARTING STATE:  AI_ATAD_AT                                           */
      /* END STATE:       AI_ATAD_AT                                           */
      /* ----------------------------------------------------------------------*/
#ifdef FT_CLMS
      // For CLMS, we need length of paging buffer message. So, Parameter4 will be assigned for length.
      BsCh_Paging_Request(value->G_PTR_buf, value->Parameter4 );
	   DECT_DEBUG_HIGH( "PAGE_RQ:%d para1=%d para4=%d // %x %x %x %x %x \r\n",
                       value->MSG, value->Parameter1, value->Parameter4,
                       value->G_PTR_buf[ 0 ], value->G_PTR_buf[ 1 ], value->G_PTR_buf[ 2 ],
                       value->G_PTR_buf[ 3 ], value->G_PTR_buf[ 4 ] );
#else
      DECT_DEBUG_HIGH( "PAGE_RQ:%d para1=%d para4=%d // %x %x %x %x %x \r\n",
                       value->MSG, value->Parameter1, value->Parameter1,
                       value->G_PTR_buf[ 0 ], value->G_PTR_buf[ 1 ], value->G_PTR_buf[ 2 ],
                       value->G_PTR_buf[ 3 ], value->G_PTR_buf[ 4 ] );
#if 1  // LOW_DUTY_SUPPORT
      if( LowDuty_Support == FALSE )
      {
         BsCh_Paging_Request( value->G_PTR_buf, value->Parameter1 );
         break;
      }

      buffer.PROCID = LMAC;
      buffer.MSG = MAC_PAGE_RQ_HMAC;
      buffer.Parameter1 = value->Parameter1;
      buffer.Parameter2 = value->Parameter2;
      buffer.Parameter3 = value->Parameter3;
      buffer.Parameter4 = value->Parameter4;
      memcpy(buffer.G_PTR_buf, value->G_PTR_buf, G_PTR_MAX_COUNT);
      Dect_SendtoLMAC(&buffer);
#else
      BsCh_Paging_Request(value->G_PTR_buf, value->Parameter1 );
#endif
#endif
      break;

    case MAC_BS_INFO_SENT_LMAC:  /*  IN LINE CODE T0404    */
      /* TRANSITION:      T404                                                 */
      /* EVENT:           MAC_BS_INFO_SENT_LMAC                                 */
      /* DESCRIPTION:     ISR level reports 'Bs channel data sent'             */
      /* STARTING STATE:  AI_ATAD_AT                                           */
      /* END STATE:       AI_ATAD_AT                                           */
      /* ----------------------------------------------------------------------*/
	  DECT_DEBUG_MED("MAC_BS_INFO_SENT_LMAC\n");

      BsCh_Paging_Response ();

	  
      break;

    case MAC_PAGE_CANCEL_LB:  /*  IN LINE CODE T0405    */
      /* TRANSITION:      T405                                                 */
      /* EVENT:           MAC_PAGE_CANCEL_LB                                       */
      /* DESCRIPTION:     Bs channel data request via Pt paging                */
      /* STARTING STATE:  AI_ATAD_AT                                           */
      /* END STATE:       AI_ATAD_AT                                           */
      /* ----------------------------------------------------------------------*/
  	  DECT_DEBUG_MED("MAC_PAGE_CANCEL_LB \n");
      Discard_BsCh_Queue(TRUE);
      break;

    case MAC_ENABLE_VOICE_EXTERNAL_SWI:  /*  IN LINE CODE T0500    */
      {
        /* TRANSITION:      T500                                                 */
        /* EVENT:           MAC_ENABLE_VOICE_EXTERNAL_SWI                        */
        /* DESCRIPTION:     voice connection for external voice path             */
        /* PARAMETER:       P1: MCEI of the external connection                  */
        /*                  P2...P4: not used                                    */
        /* STARTING STATE:  AI_ATAD_AT                                           */
        /* END STATE:       AI_ATAD_AT                                           */
        /* ----------------------------------------------------------------------*/
        BYTE mcei;
        mcei = value->Parameter1;

        if ( mcei < MAX_MCEI )
        {
          if ( (Mcei_Table[mcei].Mbc_State == MBC_ST_EST) || 
#ifdef DECT_NG
             (Mcei_Table[mcei].Mbc_State == MBC_ST_SM_INVOKING)  ||
             (Mcei_Table[mcei].Mbc_State == MBC_ST_CM_HO)  ||
#endif
             (Mcei_Table[mcei].Mbc_State == MBC_ST_HO) )
          {
            buffer.PROCID = LMAC;
            buffer.MSG = MAC_VOICE_EXTERNAL_HMAC;
            buffer.Parameter1 = mcei;
            buffer.Parameter2 = NO_MCEI;
            buffer.Parameter3 = value->Parameter3;
            buffer.Parameter4 = ON;
            Dect_SendtoLMAC(&buffer);
            if( value->Parameter3 == 0 )  // JONATHAN : No need KPI Channel assignment for Data Call
               xMceiBuffer[mcei].iKpiChan = value->Parameter2;	/* Tapi channel setting  */
	    
	    DECT_DEBUG_LOW("EV %d %d %d\n", mcei, Mcei_Table[mcei].Mbc_State,xMceiBuffer[mcei].iKpiChan);
            DECT_DEBUG_LOW("%d %d %d %d %d %d\n", xMceiBuffer[0].iKpiChan, xMceiBuffer[1].iKpiChan, xMceiBuffer[2].iKpiChan, xMceiBuffer[3].iKpiChan, xMceiBuffer[4].iKpiChan, xMceiBuffer[5].iKpiChan);
          }
        }
      }
	DECT_DEBUG_HIGH("MAC_ENABLE_VOICE_EXTERNAL_SWI MSG=%d para1=%d para4=%d\n", buffer.MSG, buffer.Parameter1, buffer.Parameter4);

      break;

    case MAC_DISABLE_VOICE_EXTERNAL_SWI:  /*  IN LINE CODE T0501    */
      {
        /* TRANSITION:      T501                                                 */
        /* EVENT:           MAC_DISABLE_VOICE_EXTERNAL_SWI                       */
        /* DESCRIPTION:     voice disconnection for external voice path          */
        /* PARAMETER:       P1: MCEI of the external connection                  */
        /*                  P2...P4: not used                                    */
        /* STARTING STATE:  AI_ATAD_AT                                           */
        /* END STATE:       AI_ATAD_AT                                           */
        /* ----------------------------------------------------------------------*/
        BYTE mcei;

		
        mcei = value->Parameter1;

        if ( mcei < MAX_MCEI )
        {
#if 0
          DECT_DEBUG_LOW("DV %d %d\n", mcei, Tapi_Channel_array[mcei]);
        
          Tapi_Channel_array[mcei] = 0xFF;	/* Tapi channel clear  */
#else
          DECT_DEBUG_LOW("DV %d %d\n", mcei, xMceiBuffer[mcei].iKpiChan);
        
          xMceiBuffer[mcei].iKpiChan = 0xFF;	/* Tapi channel clear  */

#endif
          buffer.PROCID = LMAC;
          buffer.MSG = MAC_VOICE_EXTERNAL_HMAC;
          buffer.Parameter1 = mcei;
          buffer.Parameter2 = NO_MCEI;
          buffer.Parameter3 = value->Parameter3;
          buffer.Parameter4 = OFF;
          Dect_SendtoLMAC(&buffer);
          /*printk("%d %d %d %d %d %d\n", xMceiBuffer[0].iKpiChan, xMceiBuffer[1].iKpiChan, xMceiBuffer[2].iKpiChan, xMceiBuffer[3].iKpiChan, xMceiBuffer[4].iKpiChan, xMceiBuffer[5].iKpiChan);*/
		  
        }
      }
	DECT_DEBUG_HIGH("MAC_DISABLE_VOICE_EXTERNAL_SWI MSG=%d para1=%d para4=%d\n\n", buffer.MSG, buffer.Parameter1, buffer.Parameter4);
      break;

    case MAC_ENABLE_VOICE_INTERNAL_SWI:  /*  IN LINE CODE T0502    */
      {
        /* TRANSITION:      T502                                                 */
        /* EVENT:           MAC_ENABLE_VOICE_INTERNAL_SWI                        */
        /* DESCRIPTION:     voice connection for internal voice path             */
        /* PARAMETER:       P1: MCEI of the one connection                       */
        /*                  P2: MCEI of the other connection                     */
        /*                  P3,P4: not used                                      */
        /* STARTING STATE:  AI_ATAD_AT                                           */
        /* END STATE:       AI_ATAD_AT                                           */
        /* ----------------------------------------------------------------------*/
        BYTE mcei_1, mcei_2;
        mcei_1 = value->Parameter1;
        mcei_2 = value->Parameter2;

		DECT_DEBUG_LOW("MAC_ENABLE_VOICE_INTERNAL_SWI mcei_1=%d mcei_2=%d Tapi_Channel_array=%d Mbc_State1=%d Mbc_State1=%d\n", mcei_1, mcei_2, Mcei_Table[mcei_1].Mbc_State, Mcei_Table[mcei_2].Mbc_State);

		
        if ( (mcei_1<MAX_MCEI) && (mcei_2<MAX_MCEI) )
        {
          /* both MBC's must be established !  */
          if (((Mcei_Table[mcei_1].Mbc_State == MBC_ST_EST)||
#ifdef DECT_NG
              (Mcei_Table[mcei_1].Mbc_State == MBC_ST_SM_INVOKING)  ||
              (Mcei_Table[mcei_1].Mbc_State == MBC_ST_CM_HO)  ||
#endif
             (Mcei_Table[mcei_1].Mbc_State == MBC_ST_HO)) &&
             ((Mcei_Table[mcei_2].Mbc_State == MBC_ST_EST)||
#ifdef DECT_NG
             (Mcei_Table[mcei_2].Mbc_State == MBC_ST_SM_INVOKING)  ||
             (Mcei_Table[mcei_2].Mbc_State == MBC_ST_CM_HO)  ||
#endif
             (Mcei_Table[mcei_2].Mbc_State == MBC_ST_HO)))
          /* Note: be aware of the internal    */
          /* B field buffer selection !        */
          {
            buffer.PROCID = LMAC;
            buffer.MSG = MAC_VOICE_INTERNAL_HMAC;
            buffer.Parameter1 = mcei_1;
            buffer.Parameter2 = mcei_2;
            buffer.Parameter3 = NO_MCEI;
            buffer.Parameter4 = ON;
            Dect_SendtoLMAC(&buffer);
          }
        }
      }
		DECT_DEBUG_LOW("MAC_ENABLE_VOICE_INTERNAL_SWI MSG=%d para1=%d para2=%d para4=%d\n", buffer.MSG, buffer.Parameter1,buffer.Parameter2, buffer.Parameter4);
	
      break;

    case MAC_DISABLE_VOICE_INTERNAL_SWI:  /*  IN LINE CODE T0503    */
      {
        /* TRANSITION:      T503                                                 */
        /* EVENT:           MAC_DISABLE_VOICE_INTERNAL_SWI                       */
        /* DESCRIPTION:     voice disconnection for internal voice path          */
        /* PARAMETER:       P1: MCEI of the connection to be disconnected voice  */
        /*                  P2,P3,P4: not used                                   */
        /* STARTING STATE:  AI_ATAD_AT                                           */
        /* END STATE:       AI_ATAD_AT                                           */
        /* ----------------------------------------------------------------------*/
        BYTE mcei_1;
        mcei_1 = value->Parameter1;

		DECT_DEBUG_MED("MAC_DISABLE_VOICE_INTERNAL_SWI mcei_1=%d Mbc_State=%d\n", mcei_1, Mcei_Table[mcei_1].Mbc_State);


        if ( mcei_1 < MAX_MCEI )
        {
          if ((Mcei_Table[mcei_1].Mbc_State == MBC_ST_EST) || 
#ifdef DECT_NG
             (Mcei_Table[mcei_1].Mbc_State == MBC_ST_SM_INVOKING)  ||
             (Mcei_Table[mcei_1].Mbc_State == MBC_ST_CM_HO)  ||
#endif
             (Mcei_Table[mcei_1].Mbc_State == MBC_ST_HO))
          {
            buffer.PROCID = LMAC;
            buffer.MSG = MAC_VOICE_INTERNAL_HMAC;
            buffer.Parameter1 = mcei_1;
            buffer.Parameter2 = NO_MCEI;
            buffer.Parameter3 = NO_MCEI;
            buffer.Parameter4 = OFF;
            Dect_SendtoLMAC(&buffer);
          }
        }
      }
	DECT_DEBUG_HIGH("MAC_DISABLE_VOICE_INTERNAL_SWI MSG=%d para1=%d para4=%d\n", buffer.MSG, buffer.Parameter1, buffer.Parameter4);
	
      break;

    case MAC_ENABLE_VOICE_CONFERENCE_SWI:  /*  IN LINE CODE T0504    */
      {
        /* TRANSITION:      T504                                                 */
        /* EVENT:           MAC_ENABLE_VOICE_CONFERENCE_SWI                       */
        /* DESCRIPTION:     voice connection for external voice path             */
        /* PARAMETER:       P1: MCEI of the external connection                  */
        /*                  P2...P4: not used                                    */
        /* STARTING STATE:  AI_ATAD_AT                                           */
        /* END STATE:       AI_ATAD_AT                                           */
        /* ----------------------------------------------------------------------*/

        BYTE mcei_1, mcei_2;
        mcei_1 = value->Parameter1;
        mcei_2 = value->Parameter2;


	DECT_DEBUG_MED("MAC_ENABLE_VOICE_CONFERENCE_SWI mcei_1=%d mcei_2=%d Mbc_State_1=%d Mbc_State_2=%d\n", mcei_1, mcei_2, Mcei_Table[mcei_1].Mbc_State, Mcei_Table[mcei_2].Mbc_State);

        if ( (mcei_1<MAX_MCEI) && (mcei_2<MAX_MCEI) )
        {
        /* both MBC's must be established !  */
            if ( ( (Mcei_Table[mcei_1].Mbc_State == MBC_ST_EST) ||
#ifdef DECT_NG
                 (Mcei_Table[mcei_1].Mbc_State == MBC_ST_SM_INVOKING)  ||
                 (Mcei_Table[mcei_1].Mbc_State == MBC_ST_CM_HO)  ||
#endif
                 (Mcei_Table[mcei_1].Mbc_State == MBC_ST_HO) )  &&
                 ( (Mcei_Table[mcei_2].Mbc_State == MBC_ST_EST) ||
#ifdef DECT_NG
                 (Mcei_Table[mcei_2].Mbc_State == MBC_ST_SM_INVOKING)  ||
                 (Mcei_Table[mcei_2].Mbc_State == MBC_ST_CM_HO)  ||
#endif
                  (Mcei_Table[mcei_2].Mbc_State == MBC_ST_HO) ) )
/* Note: be aware of the internal    */
/* B field buffer selection !        */
            {
               buffer.PROCID = LMAC;
               buffer.MSG = MAC_VOICE_CONFERENCE_HMAC;
               buffer.Parameter1 = mcei_1;
               buffer.Parameter2 = mcei_2;
               buffer.Parameter3 = NO_MCEI;
               buffer.Parameter4 = ON;
              Dect_SendtoLMAC(&buffer);
            }
        }
      }
	DECT_DEBUG_HIGH("MAC_ENABLE_VOICE_CONFERENCE_SWI MSG=%d para1=%d para2=%d para4=%d\n", buffer.MSG, buffer.Parameter1, buffer.Parameter2, buffer.Parameter4);
	
      break;

    case MAC_DISABLE_VOICE_CONFERENCE_SWI:  /*  IN LINE CODE T0505    */
      {
        /* TRANSITION:      T505                                                 */
        /* EVENT:           MAC_DISABLE_VOICE_CONFERENCE_SWI                      */
        /* DESCRIPTION:     voice disconnection for conference voice path        */
        /* PARAMETER:       P1: MCEI of the one connection                       */
        /*                  P2: MCEI of the other connection                     */
        /*                  P3,P4: not used                                      */
        /* STARTING STATE:  AI_ATAD_AT                                           */
        /* END STATE:       AI_ATAD_AT                                           */
        /* ----------------------------------------------------------------------*/
        BYTE mcei_1, mcei_2;
        mcei_1 = value->Parameter1;
        mcei_2 = value->Parameter2;

		DECT_DEBUG_MED("MAC_DISABLE_VOICE_CONFERENCE_SWI mcei_1=%d mcei_2=%d Mbc_State_1=%d Mbc_State_2=%d\n", mcei_1, mcei_2, Mcei_Table[mcei_1].Mbc_State, Mcei_Table[mcei_2].Mbc_State);


	/* related MBC's must be established !  */
	if (  (Mcei_Table[mcei_1].Mbc_State == MBC_ST_EST) ||
#ifdef DECT_NG
	(Mcei_Table[mcei_1].Mbc_State == MBC_ST_SM_INVOKING)  ||
	(Mcei_Table[mcei_1].Mbc_State == MBC_ST_CM_HO)  ||
#endif
	(Mcei_Table[mcei_1].Mbc_State == MBC_ST_HO) )
	{
		mcei_1 = NO_MCEI;
	}
	if (  (Mcei_Table[mcei_2].Mbc_State == MBC_ST_EST) ||
#ifdef DECT_NG
	(Mcei_Table[mcei_2].Mbc_State == MBC_ST_SM_INVOKING)  ||
	(Mcei_Table[mcei_2].Mbc_State == MBC_ST_CM_HO)  ||
#endif
	(Mcei_Table[mcei_2].Mbc_State == MBC_ST_HO) )
	{
	mcei_2 = NO_MCEI;
	}
        buffer.PROCID = LMAC;
        buffer.MSG = MAC_VOICE_CONFERENCE_HMAC;
        buffer.Parameter1 = mcei_1;
        buffer.Parameter2 = mcei_2;
        buffer.Parameter3 = NO_MCEI;
        buffer.Parameter4 = OFF;
        Dect_SendtoLMAC(&buffer);
      }
	DECT_DEBUG_HIGH("MAC_DISABLE_VOICE_CONFERENCE_SWI MSG=%d para1=%d para2=%d para4=%d\n", buffer.MSG, buffer.Parameter1, buffer.Parameter2, buffer.Parameter4);
	
      break;

    case MAC_ENC_KEY_RQ_LC:  /*  IN LINE CODE T6000    */
      {
        /* TRANSITION:      T6000                                                 */
        /* EVENT:           MAC_ENC_KEY_RQ_LC                                    */
        /* DESCRIPTION:     Encryption key provision                             */
        /* REFERENCE:       ETS 300 175-3:1996                                   */
        /* STARTING STATE:  LINK_ESTABLISHED                                     */
        /* END STATE:       LINK_ESTABLISHED                                     */
        /* ----------------------------------------------------------------------*/
        /* MCEI is transferred in           */
        /* Parameter1                       */
        BYTE mcei;
        mcei = value->Parameter1;

		DECT_DEBUG_MED("MAC_ENC_KEY_RQ_LC mcei=%d Mbc_State=%d\n", mcei, Mcei_Table[mcei].Mbc_State);

        if ( mcei < MAX_MCEI )
        {
          if ((Mcei_Table[mcei].Mbc_State == MBC_ST_EST)||
#ifdef DECT_NG
	(Mcei_Table[mcei].Mbc_State == MBC_ST_SM_INVOKING)  ||
	(Mcei_Table[mcei].Mbc_State == MBC_ST_CM_HO)  ||
#endif
          (Mcei_Table[mcei].Mbc_State == MBC_ST_HO))
          {
            Load_Encryption_Key(value->G_PTR_buf, mcei);
            // Transfer to LMAC to be used for further encryption!!!
            buffer.PROCID = LMAC;
            buffer.MSG = MAC_ENC_KEY_RQ_HMAC;
            buffer.Parameter1 = mcei;
            buffer.Parameter2 = Mcei_Table[mcei].lbn_1;
            buffer.Parameter3 = Mcei_Table[mcei].lbn_2;
            buffer.Parameter4 = 0;
            memcpy(buffer.G_PTR_buf, value->G_PTR_buf, G_PTR_MAX_COUNT);
            Dect_SendtoLMAC(&buffer);
          }
        }
      }

	   DECT_DEBUG_HIGH("MAC_ENC_KEY_RQ_LC MSG=%d para1=%d para2=%d para3=%d GPTR[%x %x %x %x %x]\n", buffer.MSG, buffer.Parameter1, buffer.Parameter2, buffer.Parameter3,
		buffer.G_PTR_buf[0],		buffer.G_PTR_buf[1],		buffer.G_PTR_buf[5],
		buffer.G_PTR_buf[3],		buffer.G_PTR_buf[4]);
	
      break;

    case MAC_ENC_EKS_IN_LMAC:  /*  IN LINE CODE T6002    */
      {
        /* TRANSITION:      T6002                                                */
        /* EVENT:           MAC_ENC_EKS_IN_LMAC                                   */
        /* DESCRIPTION:     Encryption mode confirmation                         */
        /* STARTING STATE:  AI_ATAD_AT                                           */
        /* END STATE:       AI_ATAD_AT                                           */
        /* ----------------------------------------------------------------------*/
        BYTE lbn;
        BYTE mcei;
        /* P1: LBN                          */
        /* P2: MCEI                         */
        /* P3: TRUE / FALSE                */
        /* P4: Crypt Mode / Clear Mode      */
        lbn = value->Parameter1;
        mcei = value->Parameter2;

	DECT_DEBUG_MED("MAC_ENC_EKS_IN_LMAC lbn=%d mcei=%d Mbc_State=%d\n", lbn, mcei, Mcei_Table[mcei].Mbc_State);

		
        if ((Mcei_Table[mcei].Mbc_State == MBC_ST_EST)||
#ifdef DECT_NG
           (Mcei_Table[mcei].Mbc_State == MBC_ST_SM_INVOKING)  ||
           (Mcei_Table[mcei].Mbc_State == MBC_ST_CM_HO)  ||
#endif
        (Mcei_Table[mcei].Mbc_State == MBC_ST_HO))
        {
          /* P1:  TRUE / FALSE                */
          /* P4:  Crypt Mode / Clear Mode     */
          {
              buffer.PROCID = LC;
              buffer.MSG = LC_ENC_EKS_IND_MAC;
              buffer.Parameter1 = TRUE;
              buffer.Parameter2 = 0;
              buffer.Parameter3 = 0;
              buffer.Parameter4 = value->Parameter4;
              buffer.CurrentInc= mcei;
              Dect_SendtoStack(&buffer);
          }
          Mcei_Table[mcei].Enc_State = TRUE;
        }
      }
	
		DECT_DEBUG_HIGH("MAC_ENC_EKS_IN_LMAC MSG=%d para1=%d para4=%d CurrentInc=%d\n", buffer.MSG, buffer.Parameter1, buffer.Parameter4, buffer.CurrentInc);
	
      break;

    case MAC_ENC_EKS_FAIL_LMAC:  /*  IN LINE CODE T6003    */
      {
        /* TRANSITION:      T6003                                                */
        /* EVENT:           MAC_ENC_EKS_FAIL_LMAC                                 */
        /* DESCRIPTION:     Encryption mode setting failure                      */
        /* STARTING STATE:  AI_ATAD_AT                                           */
        /* END STATE:       AI_ATAD_AT                                           */
        /* ----------------------------------------------------------------------*/
        BYTE lbn;
        BYTE mcei;
        /* P1: LBN                          */
        /* P2: MCEI                         */
        lbn = value->Parameter1;
        mcei = value->Parameter2;
        /* After unsuccessful cipher        */
        /* switching, the link must be      */
        /* released !                       */

		DECT_DEBUG_HIGH("\nMAC_ENC_EKS_FAIL_LMAC lbn=%d mcei=%d \n", lbn, mcei);


        /* P1: lbn                                */
        buffer.PROCID = LMAC;
        buffer.MSG = MAC_RELEASE_TB_RQ_HMAC;
        buffer.Parameter1 = lbn;
        Dect_SendtoLMAC(&buffer);
        Mcei_Table[mcei].Enc_State = FALSE;
		
		DECT_DEBUG_HIGH("MAC_ENC_EKS_FAIL_LMAC MSG=%d para1=%d Enc_State=%d \n", buffer.MSG, buffer.Parameter1, Mcei_Table[mcei].Enc_State);
      }
	
      break;

	case MAC_SLOTTYPE_MOD_RQ_SWI:  /*  IN LINE CODE T6100    */
	{
		/* TRANSITION:      T6100                                                 */
		/* EVENT:           MAC_SLOTTYPE_MOD_RQ_SWI                                     */
		/* DESCRIPTION:     SLOT TYPE  setting                            */
		/* REFERENCE:       ETS 300 175-5:1996                                   */
		/* PARAMETER:       P1: MCEI                                        */
		/*                  P2: not used                                         */
		/*                  P3: not used                                         */
		/*                  P4: go long_slot(WBS)/go full slot(NBS)                             */
		/* STARTING STATE:  FREE                                                 */
		/* END STATE:       FREE                                                 */
		/* ----------------------------------------------------------------------*/
		#ifdef DECT_NG
		BYTE lbn;
		BYTE mcei;
		/* Request Encryption mode setting  */
		/* ---------------------------------*/
		/* P1: MCEI                         */
		/* P2: not used                     */
		/* P3: not used                     */
		/* P4: go long_slot(WBS)/go full slot(NBS)         */

		mcei = value->Parameter1;	// hiryu_20071220

		lbn = Get_Lbn_of_only_TB(mcei);

		DECT_DEBUG_HIGH("\nMAC_SLOTTYPE_MOD_RQ_SWI lbn=%d mcei = %d Mbc_State = %d\n", lbn, mcei,Mcei_Table[mcei].Mbc_State );
		switch ( Mcei_Table[mcei].Mbc_State )
		{
			case MBC_ST_EST:


				buffer.PROCID = LMAC;
				buffer.MSG = MAC_SLOTTYPE_MOD_RQ_HMAC;
				buffer.Parameter1 = mcei;
				buffer.Parameter2 = lbn;
				buffer.Parameter4 = value->Parameter4;
				Dect_SendtoLMAC(&buffer);

			     if( value->Parameter4)
				    Mcei_Table[mcei].Mbc_State = MBC_ST_SM_INVOKING; //Slot type Modification
				 
				 Mcei_Table[mcei].wbs_req = value->Parameter4;
				break;

			default:
				Mcei_Table[mcei].wbs_req = value->Parameter4;
				break;
		}
		#endif
	}
	break;

	case MAC_SLOTTYPE_MOD_CFM_LMAC:  /*  IN LINE CODE T6101    */
	{
		/* TRANSITION:      T6101                                                */
		/* EVENT:           MAC_SLOTTYPE_MOD_CFM_HMAC                                     */
		/* DESCRIPTION:     SLOT TYPE  setting confirmation                              */
		/* REFERENCE:       ETS 300 175-4:1996                                   */
		/* PARAMETER:       P1: not used                                         */
		/*                  P2: success/failure                                  */
		/*                  P3: not used                                         */
		/*                  P4: not used                                         */
		/* REMARK:                                                               */
		/************************************************************************/
		#ifdef DECT_NG
		BYTE lbn;
		BYTE mcei;
	        /* P1: LBN                          */
		/* P2: MCEI                         */
		/* P3: slot_type                  */
		/* P4: success/failure.                          */

		lbn = value->Parameter1;
		mcei = value->Parameter2;

		/* Confirm Slottype modification  */
		/* ---------------------------------*/
		/* P1: Logical Connection No (LCN)  */
		/* P2: MCEI                    */
		/* P3: Slot_type                     */
		/* P4: success(1)/failure(0)             */
		DECT_DEBUG_HIGH("\n########### MAC_SLOTTYPE_MOD_CFM_LMAC ##########\n");
		DECT_DEBUG_HIGH("lbn =%d mcei = %d value->Parameter3 = %d value->Parameter4 = %d\n", lbn, mcei,value->Parameter3, value->Parameter4);


		/* it is  special interface */
		/* hmac --> app    derect send command */
		buffer.PROCID = PROCMAX;
		buffer.MSG = FP_SLOTTYPE_MOD_IN_MAC_TO_APP;
		buffer.Parameter1 = mcei;
		
		if(value->Parameter3)
			buffer.Parameter3 = 1;	// slot type
		else
			buffer.Parameter3 = 0;	// slot type
			
		buffer.Parameter4 = value->Parameter4;	// success / fail

		buffer.CurrentInc= mcei;
		Dect_SendtoStack(&buffer);

#if 1  // SLOTTYPE_MOD_FIX
      if( value->Parameter4 == 0 )
         Mcei_Table[mcei].Mbc_State = MBC_ST_EST;     // Change Back to original
#endif

		#endif
	}
	break;;

    case MAC_A44_SET_RQ_ME:  /*  IN LINE CODE T7000    */
      {
        /* TRANSITION:      T7000                                                */
        /* EVENT:           MAC_A44_SET_RQ_ME                                    */
        /* DESCRIPTION:     The FT starts supporting 'access rights requests'.   */
        /* PARAMETER:       none                                                 */
        /* REFERENCE:       ETS 300175-5, annex F1                               */
        /* REMARKS:          Forward to LMAC                    .                  */
        /* ----------------------------------------------------------------------*/
        /* the MAC is requested to set/clear A44 mode   */
        buffer.PROCID = LMAC;
        buffer.MSG = MAC_A44_SET_RQ_HMAC;
        buffer.Parameter1 = value->Parameter1;
        Dect_SendtoLMAC(&buffer);
      }
		DECT_DEBUG_HIGH("MAC_A44_SET_RQ_ME MSG=%d para1=%d \n", buffer.MSG,buffer.Parameter1);
	
      break;

    case MAC_TBR6_MODE_RQ_ME:  /*  IN LINE CODE T7001    */
      {
        /* TRANSITION:      T7001                                                */
        /* EVENT:           MAC_TBR6_MODE_RQ_ME                                    */
        /* DESCRIPTION:     The FT starts supporting 'tbr6 test mode'.   */
        /* PARAMETER:       none                                                 */
        /* REFERENCE:                                                                       */
        /* REMARKS:          Forward to LMAC                    .                  */
        /* ----------------------------------------------------------------------*/
        /* the MAC is requested to set/clear TBR6 mode   */
        buffer.PROCID = LMAC;
        buffer.MSG = MAC_TBR6_MODE_RQ_HMAC;
        buffer.Parameter1 = value->Parameter1;
        Dect_SendtoLMAC(&buffer);
      }
	DECT_DEBUG_HIGH("MAC_TBR6_MODE_RQ_ME MSG=%d para1=%d \n", buffer.MSG,buffer.Parameter1);
	
      break;

    case MAC_BOOT_RQ_ME:  /*  IN LINE CODE T8000    */
      {
        /* TRANSITION:      T8000                                                */
        /* EVENT:           MAC_BOOT_RQ_ME                                    */
        /* DESCRIPTION:     The FT boot LMAC */
        /* PARAMETER:       none                                                 */
        /* REFERENCE:                                                                       */
        /* REMARKS:          Forward to LMAC                    .                  */
        /* ----------------------------------------------------------------------*/
        /* the MAC is requested to boot LMAC */
        buffer.PROCID = LMAC;
        buffer.MSG = MAC_BOOT_RQ_HMAC;
		buffer.Parameter1 = value->Parameter1;
		buffer.Parameter2 = value->Parameter2;
		buffer.Parameter3 = value->Parameter3;
		
        Dect_SendtoLMAC(&buffer);
      }
	  DECT_DEBUG_HIGH("MAC_BOOT_RQ_ME	MSG=%d\n", buffer.MSG);
	
      break;

    case MAC_PARAMETER_PRELOAD_RQ_ME:  /*  IN LINE CODE T8001    */
      {
        /* TRANSITION:      T8001                                                */
        /* EVENT:           MAC_PARAMETER_PRELOAD_RQ_ME                                    */
        /* DESCRIPTION:     The FT load the LMAC parmaters   */
        /* PARAMETER:       none                                                 */
        /* REFERENCE:                                                                       */
        /* REMARKS:          Forward to LMAC                    .                  */
        /* ----------------------------------------------------------------------*/
        /* the MAC is requested to forward LMAC parameters */
        buffer.PROCID = LMAC;
        buffer.MSG = MAC_PARAMETER_PRELOAD_RQ_HMAC;
        buffer.Parameter1 = value->Parameter1;
        buffer.Parameter2 = value->Parameter2;
        buffer.Parameter3 = value->Parameter3;
        buffer.Parameter4 = value->Parameter4;
        memcpy(buffer.G_PTR_buf, value->G_PTR_buf, G_PTR_MAX_COUNT);

        Dect_SendtoLMAC(&buffer);
#if 1  // LOW_DUTY_SUPPORT
        if( (value->G_PTR_buf[ 4 ] & 0x80) == 0x80 )
           LowDuty_Support = TRUE;
        else
           LowDuty_Support = FALSE;
#endif
      }
	   DECT_DEBUG_HIGH("MAC_PARAMETER_PRELOAD_RQ_ME MSG=%d GPTR[%x %x %x %x %x]\n", buffer.MSG,
		   buffer.G_PTR_buf[0], buffer.G_PTR_buf[1], buffer.G_PTR_buf[2], 
		   buffer.G_PTR_buf[3], buffer.G_PTR_buf[4]);
       //printk("\n MAC_PARAMETER_PRELOAD_RQ_ME:\n");	
       iDriverStatus = MAC_BOOT_IND_LMAC;	
      break;

    case MAC_OSC_SET_RQ_ME:  /*  IN LINE CODE T8002    */
      {
        /* TRANSITION:      T8002                                                */
        /* EVENT:           MAC_OSC_SET_RQ_ME                                    */
        /* DESCRIPTION:     The FT boot LMAC */
        /* PARAMETER:       none                                                 */
        /* REFERENCE:                                                                       */
        /* REMARKS:          Forward to LMAC                    .                  */
        /* ----------------------------------------------------------------------*/
        /* the MAC is requested to boot LMAC */
        buffer.PROCID = LMAC;
        buffer.MSG = MAC_OSC_SET_RQ_HMAC;
        buffer.Parameter1 = value->Parameter1;
        buffer.Parameter2 = value->Parameter2;
        buffer.Parameter3 = value->Parameter3;
        buffer.Parameter4 = value->Parameter4;

        Dect_SendtoLMAC(&buffer);
      }
		DECT_DEBUG_HIGH("MAC_OSC_SET_RQ_ME MSG=%d para1=%d para2=%d para3=%d para4=%d\n",
                       buffer.MSG, buffer.Parameter1, buffer.Parameter2, buffer.Parameter3, buffer.Parameter4 );
	
      break;

    case MAC_GFSK_SET_RQ_ME:  /*  IN LINE CODE T8003    */
      {
        /* TRANSITION:      T8003                                                */
        /* EVENT:           MAC_GFSK_SET_RQ_ME                                    */
        /* DESCRIPTION:     The FT boot LMAC */
        /* PARAMETER:       none                                                 */
        /* REFERENCE:                                                                       */
        /* REMARKS:          Forward to LMAC                    .                  */
        /* ----------------------------------------------------------------------*/
        /* the MAC is requested to boot LMAC */
        buffer.PROCID = LMAC;
        buffer.MSG = MAC_GFSK_SET_RQ_HMAC;
        buffer.Parameter1 = value->Parameter1;
        buffer.Parameter2 = value->Parameter2;
        Dect_SendtoLMAC(&buffer);
      }
					
	  DECT_DEBUG_HIGH("MAC_GFSK_SET_RQ_ME MSG=%d para1=%d\n", buffer.MSG, buffer.Parameter1);
	
      break;

    case MAC_OSC_IND_LMAC:  /*  IN LINE CODE T8004    */
      {
        /* TRANSITION:      T8004                                                */
        /* EVENT:           MAC_OSC_IND_LMAC                                    */
        /* DESCRIPTION:     The FT boot LMAC */
        /* PARAMETER:       none                                                 */
        /* REFERENCE:                                                                       */
        /* REMARKS:          Forward to LMAC                    .                  */
        /* ----------------------------------------------------------------------*/
      }
	DECT_DEBUG_HIGH("MAC_OSC_IND_LMAC	MSG=%d\n", buffer.MSG);
      break;

    case MAC_GFSK_IND_LMAC:  /*  IN LINE CODE T8005    */
      {
        /* TRANSITION:      T8005                                                */
        /* EVENT:           MAC_GFSK_IND_LMAC                                    */
        /* DESCRIPTION:     The FT boot LMAC */
        /* PARAMETER:       none                                                 */
        /* REFERENCE:                                                                       */
        /* REMARKS:          Forward to LMAC                    .                  */
        /* ----------------------------------------------------------------------*/
      }
	DECT_DEBUG_HIGH("MAC_GFSK_IND_LMAC	MSG=%d\n", buffer.MSG);
	
      break;

    case MAC_FW_VERSION_IND_LMAC:  /*  IN LINE CODE T8006    */
      {
        /* TRANSITION:      T8006                                                */
        /* EVENT:           MAC_FW_VERSION_IND_LMAC                                    */
        /* DESCRIPTION:     The FT boot LMAC */
        /* PARAMETER:       none                                                 */
        /* REFERENCE:                                                                       */
        /* REMARKS:          Forward to LMAC                    .                  */
        /* ----------------------------------------------------------------------*/
      }
	DECT_DEBUG_HIGH("MAC_FW_VERSION_IND_LMAC	MSG=%d\n", buffer.MSG);
	
      break;
#ifdef CATIQ_NOEMO	  
    case MAC_NOEMO_IND_LMAC:  /*  IN LINE CODE T8006    */
      {
        /* TRANSITION:                                                      */
        /* EVENT:           MAC_NOEMO_IND_LMAC                                    */
        /* DESCRIPTION:     The FT change noemo mode */
        /* PARAMETER:       Mode                                                  */
        /* REFERENCE:                                                                       */
        /* REMARKS:          Forward to LMAC                    .                  */
        /* ----------------------------------------------------------------------*/
#define  MAC_NOEMOM_IDLE                    0x00
#define  MAC_NOEMOM_STOP                    0x01
#define  MAC_NOEMOM_START                   0x02
#define  MAC_NOEMOM_PEND                    0x03
#define  MAC_NOEMOM_ACTIV                   0x04
#define  MAC_NOEMOM_WAKEUP                  0x05
     
        buffer.PROCID = ME;
        buffer.MSG = ME_NOEMO_IN_MAC;
        buffer.Parameter1 = value->Parameter1; // NoEmo status 
        buffer.Parameter2 = value->Parameter2; // NoEmo Cn
        buffer.Parameter3 = 0;
        buffer.Parameter4 = 0;
        buffer.CurrentInc = 0;
        Dect_SendtoStack(&buffer);

  		  DECT_DEBUG_HIGH("ME_NOEMO_IN_MAC MSG=%d para1=%d \n", buffer.MSG,buffer.Parameter1);
      }
      break;

    case MAC_NOEMO_RQ_ME:  /*    */
      {
        /* TRANSITION:                                                    */
        /* EVENT:           MAC_NOEMO_RQ_ME                                    */
        /* DESCRIPTION:     .   */
        /* PARAMETER:       none                                                 */
        /* REFERENCE:                               */
        /* REMARKS:          Forward to LMAC                    .                  */
        /* ----------------------------------------------------------------------*/
        /* the MAC is requested    */
        buffer.PROCID = LMAC;
        buffer.MSG = MAC_NOEMO_RQ_HMAC;
        buffer.Parameter1 = value->Parameter1;
        buffer.Parameter2 = value->Parameter2;
        buffer.Parameter3 = value->Parameter3;
        buffer.Parameter4 = value->Parameter4;
        Dect_SendtoLMAC(&buffer);
        DECT_DEBUG_HIGH("MAC_NOEMO_RQ_HMAC MSG=%d para1=%d para2=%d \n", buffer.MSG,buffer.Parameter1, buffer.Parameter2);
      }	
      break;
#endif

      // Qt Message Modification Request command
    case MAC_QT_SET_RQ_ME:
      {
        /* TRANSITION:                                                           */
        /* EVENT:           MAC_QT_SET_RQ_ME                                     */
        /* DESCRIPTION:     Qt Message Set                                       */
        /* PARAMETER:       Parameter1 = Qt Message Indicator                    */
        /* REMARK:          Forward to LMAC                                      */
        /* ----------------------------------------------------------------------*/
        buffer.PROCID = LMAC;
        buffer.MSG = MAC_QT_SET_RQ_HMAC;
        buffer.Parameter1 = value->Parameter1;
        buffer.Parameter2 = value->Parameter2;
        buffer.Parameter3 = value->Parameter3;
        buffer.Parameter4 = value->Parameter4;
        memcpy( buffer.G_PTR_buf, value->G_PTR_buf, G_PTR_MAX_COUNT );
        Dect_SendtoLMAC(&buffer);
#if 1  // LOW_DUTY_SUPPORT
        if( value->Parameter1 == 1 )
        {
           if( (value->G_PTR_buf[ 1 ] & 0x10) == 0x10 )
              LowDuty_Support = TRUE;
           else
              LowDuty_Support = FALSE;
        }
#endif
        DECT_DEBUG_HIGH( "MAC_QT_SET_RQ_ME : para1=%d Qt[%x %x %x %x %x]\n", buffer.Parameter1,
        buffer.G_PTR_buf[0], buffer.G_PTR_buf[1], buffer.G_PTR_buf[2], buffer.G_PTR_buf[3], buffer.G_PTR_buf[4] );
      }
      break;

	/* It one time when dect module booting */
	case MAC_BOOT_IND_LMAC:
		/* TRANSITION:		T0701												 */
		/* EVENT:			MAC_BOOT_IND_LMAC								 	 */
		/* DESCRIPTION: 	BMC ISR Level indicates that a Ct fragment has been  			*/
		/*					sent and acknowledged by the PT.					 	*/
		/* STARTING STATE:	AI_ATAD_AT											 */
		/* END STATE:		AI_ATAD_AT											 */
		/* ----------------------------------------------------------------------*/
		/* the message is passed on to the	*/
		/* LC(PARAMETER1=mcei)				*/


		if(value->Parameter1 == 0x01)
		{
		buffer.PROCID = PROCMAX;
		buffer.MSG = STACK_INITAL_CMD;
                   buffer.Parameter1 = value->Parameter2;
                   buffer.Parameter2 = value->Parameter3;
                   buffer.Parameter3 = value->Parameter4;
                   buffer.Parameter4 = 0;
                   memcpy(buffer.G_PTR_buf, value->G_PTR_buf, G_PTR_MAX_COUNT);
			Dect_SendtoStack(&buffer);

		   buffer.PROCID = ME;
		   buffer.MSG = ME_RFP_PRELOAD_DIS;
			Dect_SendtoStack(&buffer);

	      DECT_DEBUG_HIGH("COSIC Modem Version = [%1d.%1d.%02x]\n",
		   value->Parameter2, value->Parameter3, value->Parameter4 );
	      DECT_DEBUG_HIGH("COSIC Modem Sub-Version = [%1c%1c%1c%1c%1c%1c%1c%1c%1c%1c]\n",
		   value->G_PTR_buf[0], value->G_PTR_buf[1], value->G_PTR_buf[2], value->G_PTR_buf[3], value->G_PTR_buf[4],
		   value->G_PTR_buf[5], value->G_PTR_buf[6], value->G_PTR_buf[7], value->G_PTR_buf[8], value->G_PTR_buf[9] );
		}
		break;
		

	/* mac layer status check hiryu_20070911 insert */
	case MAC_STATUS_CHECK:


		break;

	case MAC_INIT_CMD:		/* clear HMAC STATUS   hiryu_20070911 */
		DECT_DEBUG_HIGH("\n COSIC Modem is Ready.\n");
		Discard_BsCh_Queue(FALSE);
		HMAC_INIT();
//		Reset_Hmac_Debug_buffer();	
		break;

	case MAC_SOFT_RESET_RQ_LMAC:
		
		  /* TRANSITION:	  										   */
		  /* EVENT: 		  MAC_SOFT_RESET_RQ_LMAC									*/
		  /* DESCRIPTION:	  SOFTWARE RESET  REQUESET  */
		  /* PARAMETER: 	  none												   */
		  /* REFERENCE: 																	  */
		  /* REMARKS:							  . 				 */
		  /* ----------------------------------------------------------------------*/
		  /* the MAC is requested to boot LMAC */
		  buffer.PROCID = LMAC;
		  buffer.MSG = MAC_MODULE_RESET_RQ_HMAC;
		  Dect_SendtoLMAC(&buffer);
		break;


	case MAC_HARD_RESET_RQ_LMAC:
#if 0
		ssc_dect_haredware_reset(0);
		printk("\nMAC_HARD_RESET_RQ_LMAC LOW\n");
		udelay(1000);	// 1ms
		udelay(1000);	// 2ms
		udelay(1000);	// 3ms
		udelay(1000);	// 4ms
		udelay(1000);	// 5ms
		udelay(1000);	// 6ms
		udelay(1000);	// 7ms
		udelay(1000);	// 8ms
		udelay(1000);	// 9ms
		udelay(1000);	// 10ms
		ssc_dect_haredware_reset(1);		
		printk("\nMAC_HARD_RESET_RQ_LMAC HIGH\n");
#endif

		break;


	


	case MAC_DEBUG_MESSAGE_IND_LMAC:
				/* EVENT:			MAC_DEBUG_MESSAGE_IND_LMAC	                             	*/
				/* DESCRIPTION: 		debug message indicator  							 	*/
				/* PARAMETER:		P1: main loop status								 	*/
				/*					P2: channel number(high nibble : wideband channel nuber,	*/
				/*			                                            low nibble : narrow band channel nuber)	*/				
				/*					P3: SPI Buffer Status(high nibble : SPI RX Buffer Cnt,		*/
				/*			                                            low nibble : SPI TX Buffer Cnt)			*/				
				/*					P4: not used										 */
				/* REMARK:															 */
				/************************************************************************/

				/* it is  special interface */
				/* hmac --> app    derect send command */
				buffer.PROCID = PROCMAX;
				buffer.MSG = FP_DEBUG_MSG_IN_MAC_TO_APP;
				buffer.Parameter1 = value->Parameter1;
				buffer.Parameter2 = value->Parameter2;
				buffer.Parameter3 = value->Parameter3;
				buffer.Parameter4 = value->Parameter4;
				buffer.CurrentInc= value->CurrentInc;
				Dect_SendtoStack(&buffer);

				break;
	case MAC_SLOTTYPE_MOD_TRIGGERED_PP_LMAC:
		/* T6102 ----------------------------------------------------------------*/
		/* TRANSITION:	   T6102												*/
		/* EVENT:		   MAC_SLOTTYPE_MOD_TRIGGERED_PP_LMAC									   */
		/* DESCRIPTION:	   The BMC ISR level has inform a slot_type modification proc will triggered by PP	*/
		/* STARTING STATE:  IL_AL												*/
		/* END STATE:	   IL_AL												*/
		/* ----------------------------------------------------------------------*/
#ifdef DECT_NG
		{
		BYTE lbn;
		BYTE mcei;
		/* P1: LBN							*/
		/* P2: MCEI 						*/
		/* P3: slot_type				  */

		lbn = value->Parameter1;
		mcei = value->Parameter2;
		if( value->Parameter3 == MT_ATTRIBUTES_SLOT_TYPE_LONG )
			Mcei_Table[mcei].Mbc_State = MBC_ST_SM_INVOKING; //Slot type Modification
		}
#endif
		break;


    default:
      break;

  }										/* end of switch message				*/
}		/* end of DECODE_HMAC()					*/

