/******************************************************************************

                              Copyright (c) 2009
                            Lantiq Deutschland GmbH
                     Am Campeon 3; 85579 Neubiberg, Germany

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/
/*****************************************************************************
*                                                                           *
*     Workfile   :  FMBC.C                                                  *
*     Date       :  18 Nov, 2005                                       *
*     Contents   :  Contents : Multi Bearer Control Functions               *
*     Hardware   :  IFX 87xx                                               *
*                                                                           *
*****************************************************************************
*/
#include <linux/string.h>
#include <linux/slab.h>

#include "drv_dect.h"
#include "fmbc.h"
#include "fhmac.h"

#include "FMAC_DEF.H"
#include "FDEF.H"

/* ===============                                                      */
/* Global variables                                                     */
/* ===============                                                      */
/* ================================  */
/* MCEI table                                                          */
/* ================================ */
MCEI_TAB_STRUC Mcei_Table[MAX_MCEI];

static unsigned char KNL_STATE_OF_STACK[MAX_LINK];
static unsigned char KNL_STATE_OF_CC[ MAX_LINK ];

/* BMC register table array(ATE)    */
/* -------------------------------- */

/* ===============                                                      */
/* Local variables                                                      */
/* ===============                                                      */
/* BS queue                         */
/* Curr_BS_Ptr points to the        */
/* current element which is (will   */
/* be) broadcasted                  */

#if 1
#define MAX_PAGE_QUEUE_COUNT     15

typedef struct
{
    unsigned char     hli_frame[ G_PTR_MAX_COUNT ];
    unsigned char     cnt;
#ifdef FT_CLMS
    unsigned char     length;
    unsigned char     section;
    unsigned char     current_ptr;
#else
    unsigned char     page_p1;
#endif
} PAGE_FIFO_QUEUE;

static PAGE_FIFO_QUEUE   BS_Queue[ MAX_PAGE_QUEUE_COUNT ];
static unsigned char     BS_Queue_Head;
static unsigned char     BS_Queue_End;
static unsigned char     BS_Queue_Count;

#else
static unsigned char *Curr_BS_Ptr;
static unsigned char *BS_Head;
static unsigned char *BS_End;
#endif

#ifdef FT_CLMS
static unsigned char BS_Fragment[ G_PTR_MAX_COUNT ];
#endif

/* ==========================                                           */
/* Local Function Declaration                                           */
/* ==========================                                           */


/* ===================                                                  */
/* Function definition                                                  */
/* ===================                                                  */

/*
*****************************************************************************
*                                                                           *
*     Function :  Init_MBC                                                  *
*                                                                           *
*****************************************************************************
*                                                                           *
*     Purpose  :  Initialization of the MBC struct                 *
*     Parms    :  none                                                      *
*     Returns  :  none                                   *
*     Remarks  :                                                            *
*                                                                           *
*****************************************************************************
*/
void Init_MBC(void)
{
  unsigned char i;
  /* Initialization of the MCEI Table */
  for ( i=0; i<MAX_LINK; i++ )
  {
    Init_Mcei_Table_Element(i);
  }
}

/*
*****************************************************************************
*                                                                           *
*     Function :  Init_Mcei_Table_Element                                   *
*                                                                           *
*****************************************************************************
*                                                                           *
*     Purpose  :  initialisation of a MCEI table element                    *
*     Parms    :  nr: number of MCEI table element to be initialized        *
*     Returns  :  none                                                      *
*     Remarks  :                                                            *
*                                                                           *
*****************************************************************************
*/
void Init_Mcei_Table_Element (unsigned char nr)
{
  unsigned char i;

  KNL_STATE_OF_STACK[nr] = TRUE;		/* hiryu_20070822 add element KNL_STATE_OF_STACK */
  KNL_STATE_OF_CC[nr]    = 0;

  Mcei_Table[nr].Mbc_State = MBC_ST_IDLE;
  Mcei_Table[nr].lbn_1 = NO_LBN;
  Mcei_Table[nr].lbn_2 = NO_LBN;
  for (i = 0; i < 8; i++)
    Mcei_Table[nr].dck_array[i] = 0xFF;
  for (i = 0; i < 3; i++)
    Mcei_Table[nr].pmid[i] = 0;
  
  Mcei_Table[nr].Enc_State = FALSE;
}

/*
*****************************************************************************
*                                                                           *
*     Function :  New_Mcei                                                  *
*                                                                           *
*****************************************************************************
*                                                                           *
*     Purpose  :  get new MCEI table element for new bearer                 *
*     Parms    :  none                                                      *
*     Returns  :  MCEI, if a MBC is free                                    *
*                 NO_MCEI, if all MCEI slots are already allocated.         *
*     Remarks  :                                                            *
*                                                                           *
*****************************************************************************
*/
unsigned char New_Mcei (void)
{
  unsigned char cid;

  DECT_DEBUG_LOW("KNL_STATE_OF_STACK[%d %d %d %d %d %d] , Mcei_Table[cid].Mbc_State[%d %d %d %d %d %d]\n",
	  KNL_STATE_OF_STACK[0], KNL_STATE_OF_STACK[1], KNL_STATE_OF_STACK[2], KNL_STATE_OF_STACK[3],
	   KNL_STATE_OF_STACK[4], KNL_STATE_OF_STACK[5], Mcei_Table[0].Mbc_State, Mcei_Table[1].Mbc_State,
	    Mcei_Table[2].Mbc_State, Mcei_Table[3].Mbc_State, Mcei_Table[4].Mbc_State, Mcei_Table[5].Mbc_State);

  for( cid = 0; cid < MAX_LINK; cid++ )
  {
    if (( KNL_STATE_OF_STACK[cid] == TRUE)
        && (Mcei_Table[cid].Mbc_State == MBC_ST_IDLE))
    {
      /* A free MBC, DLC and NTW-Layer    */
      /* is found.                        */
      return cid;
    }
  }
  return NO_MCEI;
}

/*
*****************************************************************************
*                                                                           *
*     Function :  Get_Mcei                                                  *
*                                                                           *
*****************************************************************************
*                                                                           *
*     Purpose  :  get MCEI table element for (handover) bearer              *
*     Parms    :  LBN - bearer number of the HO-Bearer                      *
*     Returns  :  MCEI of the found MBC if a MBC can be found with the same *
*                 PMID                                                      *
*                 NO_MCEI if no element can be found                        *
*     Remarks  :  If a MCEI slot can be found the slot is allocated.        *
*                                                                           *
*****************************************************************************
*/
unsigned char Get_Mcei (unsigned char lbn,unsigned char *pmid_ptr)
{
  unsigned char index=lbn;

  ASSERT (lbn < MAX_BEARERS, "WRONG LBN");

  for (index = 0; index < MAX_MCEI; index++)
  {
    if ((Mcei_Table[index].pmid[0] == (pmid_ptr[0] & 0x0F))
      && (Mcei_Table[index].pmid[1] == pmid_ptr[1])
      && (Mcei_Table[index].pmid[2] == pmid_ptr[2]))
    {
      /* found !                          */
      return index;
    }
  }
  return NO_MCEI;
}

/*
*****************************************************************************
*                                                                           *
*     Function :  Get_Used_Pmid                                             *
*                                                                           *
*****************************************************************************
*                                                                           *
*     Purpose  :  Function returns the PMID of a MAC connection.            *
*     Parms    :  cid: identifies the MAC connection                        *
*                 pos: identifies one of the three bytes of the PMID        *
*     Returns  :  one byte of the PMID                                      *
*     Remarks  :                                                            *
*                                                                           *
*****************************************************************************
*/
unsigned char Get_Used_Pmid( unsigned char cid, unsigned char *pmid )
{

//unsigned char i;

		pmid[0] = Mcei_Table[ cid ].pmid[0];
		pmid[1] = Mcei_Table[ cid ].pmid[1];
		pmid[2] = Mcei_Table[ cid ].pmid[2];

//         printk("GET PMID Mcei_Table[%d].pmid[%d %d %d]\n", cid, pmid[0], pmid[1], pmid[2] );

#if 0
         for(i=0;i<6;i++)
           printk("GET PMID Mcei_Table[%d].pmid[%d %d %d]\n", i, Mcei_Table[ i ].pmid[0], Mcei_Table[ i ].pmid[1], Mcei_Table[ i ].pmid[2] );
#endif	
  return 0;//( Mcei_Table[ cid ].pmid[ pos ] );
}

/*
*****************************************************************************
*                                                                           *
*     Function :  Get_Lbn_of_only_TB                                        *
*                                                                           *
*****************************************************************************
*                                                                           *
*     Purpose  :  Returns the LBN of the last TB of a connection.           *
*     Parms    :  mcei: identifies the MAC connection.                      *
*     Returns  :  LBN of only TB                                            *
*     Remarks  :                                                            *
*                                                                           *
*****************************************************************************
*/
unsigned char Get_Lbn_of_only_TB ( unsigned char mcei )
{
  if (Mcei_Table[mcei].lbn_1 != NO_LBN)
    return (Mcei_Table[mcei].lbn_1);
  else if (Mcei_Table[mcei].lbn_2 != NO_LBN)
    return (Mcei_Table[mcei].lbn_2);

  return NO_LBN;
}

/*
*****************************************************************************
*                                                                           *
*     Function :  Get_Mcei_of_only_TB                                        *
*                                                                           *
*****************************************************************************
*                                                                           *
*     Purpose  :  Returns the LBN of the last TB of a connection.           *
*     Parms    :  mcei: identifies the MAC connection.                      *
*     Returns  :  LBN of only TB                                            *
*     Remarks  :                                                            *
*                                                                           *
*****************************************************************************
*/
EXPORT BYTE
Get_Mcei_of_only_TB ( BYTE lbn )
{
   BYTE mcei;
   
   if( lbn == NO_LBN )
       return NO_MCEI;
   
   for( mcei = 0; mcei< MAX_MCEI; mcei++)
   {
      if (Mcei_Table[mcei].lbn_1 == lbn)
         return (mcei);
      else if (Mcei_Table[mcei].lbn_2 == lbn)
         return (mcei);
   }
   return NO_MCEI;
}

#ifdef DECT_NG

EXPORT BYTE
Get_no_of_mcei_assigned (void)
{
   BYTE i, cnt=0;

   for( i = 0; i < MAX_LINK; i++ )
   {
      if ( Mcei_Table[i].Mbc_State != MBC_ST_IDLE ) 
         cnt++;
   }

   if( cnt > 4 ) return TRUE;  // if more than 4 HS are in progress.. 

   else return FALSE;
}
#endif



unsigned char Get_Enc_State( unsigned char mcei)
{
  return (Mcei_Table[mcei].Enc_State);
}

unsigned char Get_CC_State( unsigned char mcei)
{
  return ( KNL_STATE_OF_CC[ mcei ] );
}

void Set_KNL_State(unsigned char cid, unsigned char state)
{
  KNL_STATE_OF_STACK[cid] = (state & 0x0F);
  KNL_STATE_OF_CC   [cid] = (state >> 4) & 0x0F;
}
/*
*****************************************************************************
*                                                                           *
*     Function :  Load_Encryption_Key                                       *
*                                                                           *
*****************************************************************************
*                                                                           *
*     Purpose  :  Loads the cipher key of a MAC connection into the MCEI    *
*                 table.                                                    *
*     Parms    :  dck_ptr: pointer to the cipher key                        *
*                 mcei: identifies the MAC connection                       *
*     Returns  :  none                                                      *
*     Remarks  :                                                            *
*                                                                           *
*****************************************************************************
*/
void Load_Encryption_Key(unsigned char *dck_ptr, unsigned char mcei )
{
  /* Reasonable value for mcei ?      */
  if( mcei >= MAX_MCEI )
    return;

  /* The DCK is stored in the MCEI    */
  /* Table ...                        */
  memcpy( Mcei_Table[mcei].dck_array, dck_ptr, 8);
}

/*
*****************************************************************************
*                                                                           *
*     Function :  Attach_TBC_to_MBC                                         *
*                                                                           *
*****************************************************************************
*                                                                           *
*     Purpose  :  attaches a TBC to a MBC                                   *
*     Parms    :  mcei: identifies the MAC connection                       *
*                 lbn: identifies the TBC                                   *
*     Returns  :  none                                                      *
*     Remarks  :                                                            *
*                                                                           *
*****************************************************************************
*/
void Attach_TBC_to_MBC (unsigned char mcei, unsigned char lbn)
{
  if (Mcei_Table[mcei].lbn_1 == NO_LBN)
    Mcei_Table[mcei].lbn_1 = lbn;
  else if (Mcei_Table[mcei].lbn_2 == NO_LBN)
    Mcei_Table[mcei].lbn_2 = lbn;
}

/*
*****************************************************************************
*                                                                           *
*     Function :  Detach_TBC_from_MBC                                       *
*                                                                           *
*****************************************************************************
*                                                                           *
*     Purpose  :  detaches a TBC from a MBC                                 *
*     Parms    :  mcei: identifies the MAC connection                       *
*                 lbn: identifies the TBC                                   *
*     Returns  :  none                                                      *
*     Remarks  :                                                            *
*                                                                           *
*****************************************************************************
*/
void Detach_TBC_from_MBC (unsigned char mcei, unsigned char lbn)
{
  if (Mcei_Table[mcei].lbn_1 == lbn)
    Mcei_Table[mcei].lbn_1 = NO_LBN;
  else  if (Mcei_Table[mcei].lbn_2 == lbn)
    Mcei_Table[mcei].lbn_2 = NO_LBN;
}


/*
*****************************************************************************
*                                                                           *
*     Function :  BsCh_Paging_Request                                       *
*                                                                           *
*****************************************************************************
*                                                                           *
*     Purpose  :  called by the state machine to broadcast Bs channel data  *
*     Parms    :  pointer to the Bs channel data                            *
*     Returns  :  none                                                      *
*     Remarks  :                                                            *
*                                                                           *
*****************************************************************************
*/
void BsCh_Paging_Request (unsigned char *frame, unsigned char p1)
{
#if 1
#ifdef FT_CLMS    // CLMS Paging
  HMAC_QUEUES    buffer;
  unsigned char  i, max_cnt;


   if( BS_Queue_Count == 0 )
   {
      BS_Queue_Head = 0;
      BS_Queue_End  = 0;
   }
   memset( &buffer, 0x00, sizeof( HMAC_QUEUES ) );

  DECT_DEBUG_HIGH( "Message : [%d %d %d %d %d %d] \r\n",
                   frame[ 0 ], frame[ 1 ], frame[ 2 ], frame[ 3 ], frame[ 4 ] );
   max_cnt = p1;
   BS_Queue[ BS_Queue_Head ].cnt     = 1;
   BS_Queue[ BS_Queue_Head ].length  = p1;
   for( i = 0;  i < G_PTR_MAX_COUNT;  i++ )
   {
      if( i < max_cnt )
         BS_Queue[ BS_Queue_Head ].hli_frame[ i ] = frame[ i ];
      else
         BS_Queue[ BS_Queue_Head ].hli_frame[ i ] = 0x00;
   }
   BS_Queue[ BS_Queue_Head ].section     = 0xFF;
   BS_Queue[ BS_Queue_Head ].current_ptr = 0;
   
   if( BS_Queue_Count == 0 )
   {
      BS_Queue_Count = 1;
      BS_Queue_Head  = 1;
      BS_Queue_End   = 0;

      buffer.PROCID = LMAC;
      buffer.MSG    = MAC_PAGE_RQ_HMAC;
      if( max_cnt < 5 )          buffer.Parameter1 = INITIALIZED_SHORT_PAGE;
      else if( max_cnt == 5 )    buffer.Parameter1 = INITIALIZED_FULL_PAGE;
      else if( max_cnt > 5 )     buffer.Parameter1 = INITIALIZED_LONG_PAGE_FIRST;

      buffer.Parameter2 = 0;
      buffer.Parameter3 = 0;
      buffer.Parameter4 = 0;
      memcpy( buffer.G_PTR_buf, frame, G_PTR_MAX_COUNT );
      Dect_SendtoLMAC( &buffer );
   }
   else
   {
      if( BS_Queue_Head < (MAX_PAGE_QUEUE_COUNT - 1) )
         BS_Queue_Head++;
      else
         BS_Queue_Head = 0;
      BS_Queue_Count++;
   }

#else  // FT_CLMS

   HMAC_QUEUES    buffer;
   unsigned char  i, max_cnt;

   if( BS_Queue_Count == 0 )
   {
      BS_Queue_Head = 0;
      BS_Queue_End  = 0;
   }
   memset( &buffer, 0x00, sizeof(HMAC_QUEUES) );

   if( p1 )   max_cnt = 5;
   else       max_cnt = 3;

   BS_Queue[ BS_Queue_Head ].cnt     = 1;
   BS_Queue[ BS_Queue_Head ].page_p1 = p1;
   for( i = 0;  i < G_PTR_MAX_COUNT;  i++ )
   {
      if( i < max_cnt )
         BS_Queue[ BS_Queue_Head ].hli_frame[ i ] = frame[ i ];
      else
         BS_Queue[ BS_Queue_Head ].hli_frame[ i ] = 0x00;
   }

   if( BS_Queue_Count == 0 )
   {
      BS_Queue_Count = 1;
      BS_Queue_Head  = 1;
      BS_Queue_End   = 0;

      buffer.PROCID = LMAC;
      buffer.MSG    = MAC_PAGE_RQ_HMAC;
#ifdef DECT_NG_FULL_PAGE
     if( p1 )      buffer.Parameter1 = INITIALIZED_FULL_PAGE;
     else          buffer.Parameter1 = INITIALIZED_SHORT_PAGE;
#else
     buffer.Parameter1 = INITIALIZED_SHORT_PAGE;
#endif
      memcpy( buffer.G_PTR_buf, frame, G_PTR_MAX_COUNT );
      Dect_SendtoLMAC( &buffer );
   }
   else
   {
      if( BS_Queue_Head < (MAX_PAGE_QUEUE_COUNT - 1) )
         BS_Queue_Head++;
      else
         BS_Queue_Head = 0;
      BS_Queue_Count++;
   }
#endif  // FT_CLMS
#else 

  BS_QU *bs_temp;
  HMAC_QUEUES buffer;


  memset(&buffer, 0x00, sizeof(HMAC_QUEUES));

  /* request memory for a new element */
  bs_temp = (BS_QU *) kmalloc (sizeof(BS_QU), GFP_KERNEL);
  bs_temp->hli_frame = (FPTR)kmalloc (G_PTR_MAX_COUNT, GFP_KERNEL);

  bs_temp->previous = NULL;
  bs_temp->next = NULL;

  memset(bs_temp->hli_frame, 0, G_PTR_MAX_COUNT);

  memcpy(bs_temp->hli_frame, frame, G_PTR_MAX_COUNT);      /* store pointer to BS data         */
  bs_temp->cnt       = 1;             /* no repetitions                   */
  if (BS_Head == NULL)
  {

    BS_Head = BS_End = Curr_BS_Ptr = (unsigned char *) bs_temp;
    bs_temp->next = bs_temp->previous = NULL;

    buffer.PROCID = LMAC;
    buffer.MSG = MAC_PAGE_RQ_HMAC;
#ifdef DECT_NG_FULL_PAGE
	if(p1)
    	buffer.Parameter1 = INITIALIZED_FULL_PAGE;
	else
		buffer.Parameter1 = INITIALIZED_SHORT_PAGE;
#else
    buffer.Parameter1 = INITIALIZED_SHORT_PAGE;
#endif
    memcpy(buffer.G_PTR_buf, frame, G_PTR_MAX_COUNT);
    Dect_SendtoLMAC(&buffer);
  }
  else
  {
    /* list has aready entries, store   */
    /* new element at the end of the    */
    /* list                             */
    ((BS_QU *)BS_End)->next = (unsigned char *) bs_temp;
    bs_temp->previous = BS_End;
    bs_temp->next     = NULL;
    BS_End            = (unsigned char *) bs_temp;
  }
#endif
}

/*
*****************************************************************************
*                                                                           *
*     Function :  BsCh_Paging_Response                                      *
*                                                                           *
*****************************************************************************
*                                                                           *
*     Purpose  :  BMC ISR level has finished paging                         *
*     Parms    :  none                                                      *
*     Returns  :  none                                                      *
*     Remarks  :                                                            *
*                                                                           *
*****************************************************************************
*/
void BsCh_Paging_Response (void)
{
#if 1
#ifdef FT_CLMS
   HMAC_QUEUES buffer;

   memset(&buffer, 0x00, sizeof(buffer));

   if( BS_Queue[ BS_Queue_End ].cnt > 0 )
      BS_Queue[ BS_Queue_End ].cnt--;        // decrement repeat counter of current element

   DECT_DEBUG_MED("bs_temp->cnt=%d\n",bs_temp->cnt);

   if( BS_Queue[ BS_Queue_End ].cnt == 0 )
   {
      // Assemble next broadcasting fragments
      if( BS_Queue[ BS_Queue_End ].length > 5 )
      {
         BS_Queue[ BS_Queue_End ].current_ptr += 4;
         BS_Queue[ BS_Queue_End ].length -= 4;

         if( BS_Queue[ BS_Queue_End ].section == 0xFF )
            BS_Queue[ BS_Queue_End ].section = 0;
         else
            BS_Queue[ BS_Queue_End ].section += 1;

         if( BS_Queue[ BS_Queue_End ].length != 0 )
         {
             memset( BS_Fragment, 0x00, G_PTR_MAX_COUNT );     // Fill the remain bytes by 0
             memcpy( &BS_Fragment[ 1 ], &BS_Queue[ BS_Queue_End ].hli_frame[ BS_Queue[ BS_Queue_End ].current_ptr ], 4 );
             BS_Fragment[ 0 ] =  BS_Queue[ BS_Queue_End ].section & 0x07;      // set CLMS header(data section number)

            if( BS_Queue[ BS_Queue_End ].length > 4 )          // Not the last fragment!
            {
               buffer.PROCID = LMAC;
               buffer.MSG    = MAC_PAGE_RQ_HMAC;
               buffer.Parameter1 = INITIALIZED_LONG_PAGE_MID;
               buffer.Parameter2 = 0;
               buffer.Parameter3 = 0;
               buffer.Parameter4 = 0;
               memcpy( buffer.G_PTR_buf, BS_Fragment, G_PTR_MAX_COUNT );
               Dect_SendtoLMAC( &buffer );
            }
            else    //Last fragment!
            {
               buffer.PROCID = LMAC;
               buffer.MSG    = MAC_PAGE_RQ_HMAC;
               buffer.Parameter1 = INITIALIZED_LONG_PAGE_LAST;
               buffer.Parameter2 = 0;
               buffer.Parameter3 = 0;
               buffer.Parameter4 = 0;
               memcpy( buffer.G_PTR_buf, BS_Fragment, G_PTR_MAX_COUNT );
               Dect_SendtoLMAC( &buffer );
            }
            return;
         }
      }
      memset( &BS_Queue[ BS_Queue_End ], 0, sizeof( PAGE_FIFO_QUEUE ) );
      if( BS_Queue_End < (MAX_PAGE_QUEUE_COUNT - 1) )
         BS_Queue_End++;
      else
         BS_Queue_End = 0;

      if( BS_Queue_Count > 0 )
         BS_Queue_Count--;

     if( BS_Queue_Count == 0 )
     {
        BS_Queue_Head = 0;
        BS_Queue_End  = 0;
        buffer.PROCID = LMAC;
        buffer.MSG    = MAC_PAGE_CANCEL_HMAC;
        Dect_SendtoLMAC( &buffer );
     }
     else
     {
         buffer.PROCID = LMAC;
         buffer.MSG    = MAC_PAGE_RQ_HMAC;
         if( BS_Queue[ BS_Queue_End ].length < 5 )          buffer.Parameter1 = INITIALIZED_SHORT_PAGE;
         else if( BS_Queue[ BS_Queue_End ].length == 5 )    buffer.Parameter1 = INITIALIZED_LONG_ALL_PAGE;
         else if( BS_Queue[ BS_Queue_End ].length > 5 )     buffer.Parameter1 = INITIALIZED_LONG_PAGE_FIRST;

         buffer.Parameter2 = 0;
         buffer.Parameter3 = 0;
         buffer.Parameter4 = 0;
         memcpy( buffer.G_PTR_buf, BS_Queue[ BS_Queue_End ].hli_frame, G_PTR_MAX_COUNT );
         Dect_SendtoLMAC( &buffer );
      }
   }
   else     // this is not the case becasue we set cnt = 1 always...
   {
      buffer.PROCID = LMAC;
      buffer.MSG    = MAC_PAGE_RQ_HMAC;
      if( BS_Queue[ BS_Queue_End ].length < 5 )          buffer.Parameter1 = INITIALIZED_SHORT_PAGE;
      else if( BS_Queue[ BS_Queue_End ].length == 5 )    buffer.Parameter1 = INITIALIZED_LONG_ALL_PAGE;
      else if( BS_Queue[ BS_Queue_End ].length > 5 )     buffer.Parameter1 = INITIALIZED_LONG_PAGE_FIRST;

      buffer.Parameter2 = 0;
      buffer.Parameter3 = 0;
      buffer.Parameter4 = 0;
      memcpy( buffer.G_PTR_buf, BS_Queue[ BS_Queue_End ].hli_frame, G_PTR_MAX_COUNT );
      Dect_SendtoLMAC( &buffer );
   }

#else  // FT_CLMS

  HMAC_QUEUES buffer;

  memset(&buffer, 0x00, sizeof(buffer));

  if( BS_Queue[ BS_Queue_End ].cnt > 0 )
     BS_Queue[ BS_Queue_End ].cnt--;      // decrement repeat counter of current element

  DECT_DEBUG_MED("bs_temp->cnt=%d\n",bs_temp->cnt);

  if( BS_Queue[ BS_Queue_End ].cnt == 0 )
  {
     memset( &BS_Queue[ BS_Queue_End ], 0, sizeof( PAGE_FIFO_QUEUE ) );
     if( BS_Queue_End < (MAX_PAGE_QUEUE_COUNT - 1) )
        BS_Queue_End++;
     else
        BS_Queue_End = 0;

     if( BS_Queue_Count > 0 )
        BS_Queue_Count--;

     if( BS_Queue_Count == 0 )
     {
        BS_Queue_Head = 0;
        BS_Queue_End  = 0;
        buffer.PROCID = LMAC;
        buffer.MSG    = MAC_PAGE_CANCEL_HMAC;
        Dect_SendtoLMAC( &buffer );
     }
     else
     {
        buffer.PROCID = LMAC;
        buffer.MSG    = MAC_PAGE_RQ_HMAC;
#ifdef DECT_NG_FULL_PAGE
        if( BS_Queue[ BS_Queue_End ].page_p1 )
           buffer.Parameter1 = INITIALIZED_FULL_PAGE;
        else
           buffer.Parameter1 = INITIALIZED_SHORT_PAGE;
#else
        buffer.Parameter1 = INITIALIZED_SHORT_PAGE;
#endif
        memcpy( buffer.G_PTR_buf, BS_Queue[ BS_Queue_End ].hli_frame, G_PTR_MAX_COUNT );
        Dect_SendtoLMAC(&buffer);
     }
  }
  else     // this is not the case becasue we set cnt = 1 always...
  {
     buffer.PROCID = LMAC;
     buffer.MSG    = MAC_PAGE_RQ_HMAC;
#ifdef DECT_NG_FULL_PAGE
     if( BS_Queue[ BS_Queue_End ].page_p1 )
        buffer.Parameter1 = INITIALIZED_FULL_PAGE;
     else
        buffer.Parameter1 = INITIALIZED_SHORT_PAGE;
#else
     buffer.Parameter1 = INITIALIZED_SHORT_PAGE;
#endif
     memcpy( buffer.G_PTR_buf, BS_Queue[ BS_Queue_End ].hli_frame, G_PTR_MAX_COUNT );
     Dect_SendtoLMAC(&buffer);
  }
#endif  // FT_CLMS
#else

  BS_QU *bs_temp;
  unsigned char *prev;
  unsigned char *next;
  HMAC_QUEUES buffer;
  memset(&buffer, 0x00, sizeof(buffer));

  bs_temp = (BS_QU *)Curr_BS_Ptr;

  /* decrement repeat counter of      */
  /* current element                  */
  (bs_temp->cnt)--;
  /* save 'previous' and 'next'       */
  /* pointer                          */
  prev = bs_temp->previous;
  next = bs_temp->next;



  DECT_DEBUG_MED("bs_temp->cnt=%d\n",bs_temp->cnt);

  if (bs_temp->cnt == 0)
  {
    /* if the paging procedure of this  */
    /* element is completed (after      */
    /* bs_temp->cnt retransmissions)    */
    /* the buffers are released and the */
    /* 'previous' and 'next' pointers   */
    /* of the adjacent list elements    */
    /* are adjusted accordingly         */
    kfree(bs_temp->hli_frame);
    kfree((unsigned char *) bs_temp);

    if (next == NULL)
    {
      /* current element was last element */
      /* of the list                      */
      /* -------------------------------- */
      if (prev == NULL)
      {
        /* and none before means the list   */
        /* is empty now                     */
        Curr_BS_Ptr = BS_Head = BS_End = NULL;
      }
      else
      {
        /* the element before is now the    */
        /* last element                     */
        Curr_BS_Ptr = BS_Head;
        ((BS_QU *)prev)->next = NULL;
        BS_End = prev;
      } /* if (prev == NULL)                                            */
    }
    else
    {
      /* current element was not the last */
      /* element of the list              */
      /* -------------------------------- */
      Curr_BS_Ptr = next;

      if (prev == NULL)
      {
        /* it was the first element. The    */
        /* 'previous' pointer of the next   */
        /* element is zeroed                */
        ((BS_QU *)next)->previous = NULL;
        BS_Head = next;
      }
      else
      {
        /* somewhere between head and end   */
        /* of list -> adjust 'previous' and */
        /* 'next' pointers of the adjacent  */
        /* elements                         */
        ((BS_QU *)prev)->next = next;
        ((BS_QU *)next)->previous = prev;
      }
    } /* if (next == NULL)                                               */
  }
  else // this is not the case becasue we set cnt = 1 always...
  {
    /* paging procedure of this         */
    /* element is not completed,        */
    /* set current pointer to next      */
    /* element                          */
    Curr_BS_Ptr = next;

    if (next == NULL)
    Curr_BS_Ptr = BS_Head;

  } /* if (bs_temp->cnt == 0)                                             */

  /* paging is initiated if           */
  /* 'Curr_BS_Ptr' points to a valid  */
  /* element                          */
  
  if ((unsigned char *) Curr_BS_Ptr != NULL)
  {
    buffer.PROCID = LMAC;
    buffer.MSG = MAC_PAGE_RQ_HMAC;
#ifdef DECT_NG_FULL_PAGE
    buffer.Parameter1 = INITIALIZED_FULL_PAGE;
#else
    buffer.Parameter1 = INITIALIZED_SHORT_PAGE;
#endif
    memcpy(buffer.G_PTR_buf, ((BS_QU *)Curr_BS_Ptr)->hli_frame, G_PTR_MAX_COUNT);
    Dect_SendtoLMAC(&buffer);
	
  }
  else
  {
    buffer.PROCID = LMAC;
//    buffer.MSG = MAC_PAGE_CANCEL_HMAC;
    buffer.MSG = MAC_PAGE_CANCEL_HMAC;
    Dect_SendtoLMAC(&buffer);
//	DECT_DEBUG_MED("MAC_PAGE_CANCEL_HMAC\n");	
  }
#endif
}

/*
*****************************************************************************
*                                                                           *
*   Function   :  Discard_BsCh_Queue                                        *
*                                                                           *
*****************************************************************************
*                                                                           *
*   Purpose    :  The function discards the broadcasting queue              *
*   Parms      :  none                                                      *
*   Returns    :  none                                                      *
*   Call Level :  Process Level                                             *
*   Remarks    :                                                            *
*                                                                           *
*****************************************************************************
*/
void Discard_BsCh_Queue( unsigned char how )
{
#if 1
  HMAC_QUEUES    buffer;
  unsigned char  i;

  memset(&buffer, 0x00, sizeof(buffer));
  
  if( how )
  {
     buffer.PROCID = LMAC;
     buffer.MSG    = MAC_PAGE_CANCEL_HMAC;
     Dect_SendtoLMAC( &buffer );
  }
 
  for( i = 0;  i < MAX_PAGE_QUEUE_COUNT;  i++ )
     memset( &BS_Queue[ i ], 0, sizeof( PAGE_FIFO_QUEUE ) );

  BS_Queue_Head  = 0;
  BS_Queue_End   = 0;
  BS_Queue_Count = 0;

#else

  BS_QU *bs_temp;
  unsigned char *prev;
  unsigned char *next;
  HMAC_QUEUES buffer;
  memset(&buffer, 0x00, sizeof(buffer));
  
  if( how )
  {
      buffer.PROCID = LMAC;
      buffer.MSG    = MAC_PAGE_CANCEL_HMAC;
      Dect_SendtoLMAC( &buffer );
  }
  else
  {
      if( Curr_BS_Ptr == NULL )  return;
  }
  
  bs_temp = (BS_QU *)  Curr_BS_Ptr;
  /* save 'previous' and 'next'       */
  /* pointer                          */
  prev = bs_temp->previous;
  next = bs_temp->next;

	if( bs_temp != NULL )
	{

		kfree(bs_temp->hli_frame);
		kfree((unsigned char *)(bs_temp));

		while ( (prev != NULL) || (next != NULL) )
		{
			if (next == NULL)
			{
												/* current element was last element */
												/* of the list 					 */
												/* -------------------------------- */
				if (prev == NULL)
				{
												/* and none before means the list	 */
												/* is empty now					 */
					Curr_BS_Ptr = BS_Head = BS_End = NULL;
				}
				else
				{
												/* the element before is now the    */
												/* last element                     */
					Curr_BS_Ptr = BS_Head;
					((BS_QU *)prev)->next = NULL;
					BS_End = prev;
				} /* if (prev == NULL)											 */
			}
			else
			{
												/* current element was not the last */
												/* element of the list              */
												/* -------------------------------- */
				Curr_BS_Ptr = next;
				if (prev == NULL)
				{
													/* it was the first element. The    */
													/* 'previous' pointer of the next   */
													/* element is zeroed                */
					((BS_QU *)next)->previous = NULL;
					BS_Head = next;
				}
				else
				{
													/* somewhere between head and end   */
													/* of list -> adjust 'previous' and */
													/* 'next' pointers of the adjacent  */
													/* elements                         */
					((BS_QU *)prev)->next = next;
					((BS_QU *)next)->previous = prev;
				}
			} /* if (next == NULL) */
			
			bs_temp = (BS_QU XDATA *)	Curr_BS_Ptr;
													/* save 'previous' and 'next'       */
													/* pointer                          */
			prev = bs_temp->previous;
			next = bs_temp->next;

			if ( bs_temp != NULL )
			{
				kfree(bs_temp->hli_frame);
				kfree((unsigned char *)(bs_temp));
			}
		}
	}

  Curr_BS_Ptr = BS_Head = BS_End = NULL;
#endif
}

/*
*****************************************************************************
*                                                                           *
*     Function :  Check_Same_BsCH_Page                                      *
*                                                                           *
*****************************************************************************
*                                                                           *
*     Purpose  :  page frame compare with BsCH queue.                       *
*     Parms    :  pointer to the Bs channel data                            *
*     Returns  :  TRUE/FALSE                                                *
*     Remarks  :                                                            *
*                                                                           *
*****************************************************************************
*/
#if 0
unsigned char Check_Same_BsCH_Page( unsigned char *frame, unsigned char length )
{
   BYTE  i;

   for( i = 0;  i < MAX_PAGE_QUEUE_COUNT;  i++ )
   {
      if ( i != BS_Queue_End )   // Check current data
      {
         // check if this is duplicated paging info(inlcuding TPUI)
         if( BS_Queue[ i ].length == length )
         {
            if ( memcmp( BS_Queue[ i ].hli_frame, frame, length ) )
            {
               return   1;
            }
         }
      }
   }

   return   0;
}
#endif

/*
*****************************************************************************
*                                                                           *
*     Function :  Init_BsCh_Queue                                          *
*                                                                           *
*****************************************************************************
*                                                                           *
*     Purpose  :  Initialization of the Bs paging control data.             *
*     Parms    :  none                                                      *
*     Returns  :  none                                                      *
*     Remarks  :                                                            *
*                                                                           *
*****************************************************************************
*/
void Init_BsCh_Queue(void)
{
#if 1
  unsigned char  i;

  // Initialize structures for paging control
  BS_Queue_Count = 0;
  BS_Queue_Head  = 0;
  BS_Queue_End   = 0;
  for( i = 0;  i < MAX_PAGE_QUEUE_COUNT;  i++ )
     memset( &BS_Queue[ i ], 0, sizeof( PAGE_FIFO_QUEUE ) );

#else

  /* initialise structures for paging */
  /* control                                 */
  /* ----------------------------------*/
  BS_Head = BS_End = Curr_BS_Ptr = NULL;
#endif
}

