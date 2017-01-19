/***************************************************************************
 *
 * <:copyright-arcadyan
 * Copyright 2013 Arcadyan Technology 
 * All Rights Reserved. 
 * 
 * Arcadyan Confidential; Need to Know only. Protected as an unpublished work.
 * 
 * The computer program listings, specifications and documentation herein are
 * the property of Arcadyan Technology and shall not be reproduced, copied,
 * disclosed, or used in whole or in part for any reason without the prior
 * express written permission of Arcadyan Technology
 * :>
 *
 * File Name  : entry.c
 *
 * Description: leave this for future enhancement of other platform usage. 
 *				
 * Updates    : 13/08/2013  bchan.  Created.
 *               
 ***************************************************************************/

#ifdef CONFIG_HDR_MKIMG
#include "mkimg_head.h"
#endif	// CONFIG_HDR_MKIMG //

int main (int argc, char **argv){
#if 0
#ifdef CONFIG_HDR_MKIMG
		return MKIMG_HDR_MAIN(argc, argv);
#endif	// CONFIG_HDR_MKIMG //
#endif   // #if 0

	return MKIMG_MAIN(argc, argv);

}
