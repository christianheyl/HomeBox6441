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

#ifndef _ADF_OS_MODULE_PVT_H
#define _ADF_OS_MODULE_PVT_H

#include <linux/version.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <adf_os_types.h>

#define adf_os_virt_module_init(_x)  \
    static int _x##_mod(void) \
    {                   \
        a_status_t st;  \
        st = (_x)();         \
        if (st != A_STATUS_OK)  \
          return -1;            \
        else                    \
          return 0;             \
    }                           \
    module_init(_x##_mod);

#define adf_os_virt_module_exit(_x)   module_exit(_x)

#define adf_os_virt_module_name(_name) MODULE_LICENSE("GPL");

#define adf_os_module_dep(_name, _dep)

#define adf_os_export_symbol(_sym) EXPORT_SYMBOL(_sym)

#define ADF_OS_PARAM_TYPE_INT32
#define ADF_OS_PARAM_TYPE_STRING

#define adf_os_declare_param(_name, _type)
#define adf_os_read_param(_osdev, _name, _type, _pval)

#endif /*_ADF_OS_MODULE_PVT_H*/