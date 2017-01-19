/*
 * Copyright (c) 2012 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 * $ATH_LICENSE_TARGET_C$
 */

/*
 * Data-types used in rate-control on host and target.
 */

#ifndef _RATECTRL_TYPES_H_
#define _RATECTRL_TYPES_H_

#if defined(ATH_TARGET)

#include <athdefs.h>
#include <osapi.h>

#else

#include <a_types.h>
#include <a_osapi.h>

#define A_MILLISECONDS()      CONVERT_SYSTEM_TIME_TO_MS((OS_GET_TICKS()))
#define A_RATEMASK            A_UINT32
#define A_RSSI                A_UINT8 //Verify, not u

#ifndef WIN32  
#define A_MAX(_x, _y) \
        ({ __typeof__ (_x) __x = (_x); __typeof__ (_y) __y = (_y); __x > __y ? __x : __y; })
#endif

#ifdef WIN32
 #define A_MIN(_x, _y) ((_x) < (_y) ? (_x) : (_y))
#else
#define A_MIN(_x, _y) \
        ({ __typeof__ (_x) __x = (_x); __typeof__ (_y) __y = (_y); __x < __y ? __x : __y; })
#endif

#endif

#if 0

/*TODO: hbrc: the condition below should be made OS-dependent*/
#if !(ATH_TARGET)

#include <adf_os_util.h>   /* adf_os_assert */
#include <adf_nbuf.h>      /* adf_nbuf_t */
#include <adf_os_mem.h>    /* adf_os_mem_set */

#include "a_types.h"

#include "osdep_adf.h"


#define A_MAX(_x, _y)   adf_os_max((_x), (_y))

#define A_MIN(_x, _y)   adf_os_min((_x), (_y))


#define A_ASSERT(condition)    adf_os_assert((condition))

#define A_MEMCPY(_a,_b,_c)    adf_os_mem_copy((_a), (_b), (_c))

//#define A_MEMZERO

#define A_MEMSET(_a,_b,_c)    adf_os_mem_set((_a), (_b), (_c))

#define A_MILLISECONDS()      CONVERT_SYSTEM_TIME_TO_MS((OS_GET_TICKS()))

//#define A_ALLOCRAM  adf_os_mem_alloc

//#define A_MEMCMP    adf_os_mem_cmp

//#define adf_os_mem_free(buf)

#endif
#endif

#endif /* _RATECTRL_TYPES_H_ */
