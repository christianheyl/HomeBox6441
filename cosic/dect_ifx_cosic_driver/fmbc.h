/******************************************************************************

                              Copyright (c) 2009
                            Lantiq Deutschland GmbH
                     Am Campeon 3; 85579 Neubiberg, Germany

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/
/*****************************************************************************
*                                                                           *
*     Workfile   :  FMBC.H                                                  *
*     Date       :  18 Nov, 2005                                       *
*     Contents   :  Contents : Function declarations for FMBC.C .           *
*     Hardware   :  IFX 87xx                                               *
*                                                                           *
*****************************************************************************
*/
#ifndef _INC_FMBC
#define _INC_FMBC

#include "TYPEDEF.H"
#include "SYSDEF.H"
#include "ENI_LIB.H"
#include "CONF_CP.H"
#include "DECT.H"
#include "TYPEDEF_CP.H"

                  /* MCEI table                       */
                  /* ================================ */
extern MCEI_TAB_STRUC Mcei_Table[MAX_MCEI];
                  /* RSSI table array(ATE)            */
                  /* BMC register table array(ATE)    */
                  /* -------------------------------- */

   /* ============================ */
   /* Global function declarations */
   /* ============================ */
void Init_MBC (void);

unsigned char New_Mcei( void );
unsigned char Get_Mcei( unsigned char, unsigned char * );
unsigned char Get_Used_Pmid( unsigned char, unsigned char * );
unsigned char Get_Enc_State( unsigned char mcei);
unsigned char Get_CC_State ( unsigned char mcei );
void Set_KNL_State(unsigned char cid, unsigned char state);
void Init_Mcei_Table_Element (unsigned char nr);
unsigned char Get_Lbn_of_only_TB( unsigned char );
void Load_Encryption_Key( unsigned char *, unsigned char );
void Attach_TBC_to_MBC( unsigned char, unsigned char );
void Detach_TBC_from_MBC( unsigned char, unsigned char );

unsigned char Check_Same_BsCH_Page  ( unsigned char *, unsigned char );
BYTE Get_Mcei_of_only_TB   ( BYTE );

void Init_BsCh_Queue(void);
void BsCh_Paging_Request( unsigned char *, unsigned char);
void BsCh_Paging_Response( void );
void Discard_BsCh_Queue(unsigned char how);

#ifdef DECT_NG
BYTE Get_no_of_mcei_assigned (void);
#endif



#endif                                 /* endif of #ifndef _INC_FMBC       */
