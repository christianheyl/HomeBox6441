/******************************************************************************

                              Copyright (c) 2009
                            Lantiq Deutschland GmbH
                     Am Campeon 3; 85579 Neubiberg, Germany

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/
#ifndef _INC_FHMAC
#define _INC_FHMAC



extern unsigned char Tapi_Channel_array[];


void HMAC_INIT(void);
void DECODE_HMAC(HMAC_QUEUES *value);



#endif /* endif of #ifndef _INC_FHMAC       */
