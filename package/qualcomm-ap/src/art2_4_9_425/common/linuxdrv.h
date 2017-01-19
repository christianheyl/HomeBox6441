/*
 *  Copyright © 2000-2001 Atheros Communications, Inc.,  All Rights Reserved.
 */

#ident  "ACI $Header: //depot/sw/branches/art2_main_per_cs/src/art2/common/linuxdrv.h#1 $"

/*
DESCRIPTION
This file contains the linux declarations for some 
data structures 
*/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifndef	__INClinuxdrvh
#define	__INClinuxdrvh

/* Atheros include files */

#include "wlantype.h"
#include <sys/time.h>

typedef	A_UINT32	FHANDLE;

/* Atheros declarations mapped to OS specific declarations */

#define A_RAND      rand
#define A_MALLOC(a)	(malloc(a))
#define A_FREE(a)	free(a);a=NULL
#define A_DRIVER_MALLOC(a)	(malloc(a))
#define A_DRIVER_FREE(a, b)	(free(a))
#define A_DRIVER_BCOPY(from, to, len) (bcopy((void *)(from), (void *)(to), (len)))
#define A_BCOPY(from, to, len) (bcopy((void *)(from), (void *)(to), (len)))
#define A_BCOMP(s1, s2, len) (memcmp((void *)(s1), (void *)(s2), (len)))
#define A_MACADDR_COPY(from, to) ((void *) memcpy(&((to)->octets[0]),&((from)->octets[0]),WLAN_MAC_ADDR_SIZE))
#define A_MACADDR_COMP(m1, m2) ((void *) strncmp((char *)&((m1)->octets[0]), (char *)&((m2)->octets[0]), WLAN_MAC_ADDR_SIZE))

/* Locking macros 
 * Currently, they are all empty. They will be eventually
 * written as call to NDIS dependent function.
 */

#define A_SEM_LOCK(sem, param)
#define A_SEM_UNLOCK(sem)
#define A_SIB_TAB_LOCK()
#define A_SIB_TAB_UNLOCK()
#define A_SIB_ENTRY_LOCK(s)
#define A_SIB_ENTRY_UNLOCK(s)

#define A_INIT_TIMER(func, param)
#define A_TIMEOUT(func, period, param)
#define A_UNTIMEOUT(handle)

#endif /* __INCsunosdrvh */

#ifdef __cplusplus
}
#endif /* __cplusplus */
