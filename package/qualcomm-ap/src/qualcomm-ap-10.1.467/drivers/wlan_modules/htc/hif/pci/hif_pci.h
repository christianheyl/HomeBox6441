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

#ifndef __HIF_PCI_H
#define __HIF_PCI_H

#include  "hif_pci_zdma.h"

#define HIF_PCI_MAX_DEVS        4
#define MAX_TXDESC_SHIFT        7
#define MAX_RXDESC_SHIFT        8
#define HIF_PCI_MAX_TX_DESC     (1 << MAX_TXDESC_SHIFT)
#define HIF_PCI_MAX_RX_DESC     (1 << MAX_RXDESC_SHIFT)
#define MAX_NBUF_SIZE           1664

#endif
