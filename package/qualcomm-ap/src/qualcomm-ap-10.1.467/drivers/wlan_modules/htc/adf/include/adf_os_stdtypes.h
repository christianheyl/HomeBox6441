/*
 * Copyright (c) 2010, Atheros Communications Inc.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/**
 * @defgroup adf_os_public OS abstraction API
 */ 

/**
 * @ingroup adf_os_public
 * @file adf_os_stdtypes.h
 * This file defines standard types.
 */

#ifndef _ADF_OS_STDTYPES_H
#define _ADF_OS_STDTYPES_H

/**
 * @brief basic data types. 
 */
typedef enum {
    A_FALSE,
    A_TRUE           
} a_bool_t;

typedef unsigned char        a_uint8_t;
typedef char                 a_int8_t;
typedef unsigned short       a_uint16_t;
typedef short                a_int16_t;

#ifdef WIN32
typedef unsigned long        a_uint32_t;
typedef long                 a_int32_t;
#else
typedef unsigned int         a_uint32_t;
typedef int                  a_int32_t;
#endif

typedef unsigned long long   a_uint64_t;
typedef long long            a_int64_t;

#endif
