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

#include <osdep.h>
#include <adf_os_mem.h>
#include <adf_nbuf.h>

#include <athdefs.h>
#include "a_types.h"
#include "a_osapi.h"

#include "htc_host_api.h"
#include "miscdrv.h"
#ifdef HTC_HOST_CREDIT_DIST
/* initialize and setup credit distribution */
A_STATUS HTCSetupCreditDist(HTC_HANDLE HTCHandle, void  *pCredInfo)
{
    HTC_SERVICE_ID servicepriority[WMI_MAX_SERVICES];
	
	servicepriority[0] = WMI_CONTROL_SVC; /* highest */
    servicepriority[1] = WMI_BEACON_SVC;
    servicepriority[2] = WMI_UAPSD_SVC;
    servicepriority[3] = WMI_MGMT_SVC;
    servicepriority[4] = WMI_DATA_VO_SVC;
    servicepriority[5] = WMI_CAB_SVC;
    servicepriority[6] = WMI_DATA_VI_SVC;
    servicepriority[7] = WMI_DATA_BE_SVC;
    servicepriority[8] = WMI_DATA_BK_SVC; /* lowest */
    
	adf_os_mem_zero(pCredInfo,sizeof(COMMON_CREDIT_STATE_INFO));

  	/* set callbacks and priority list */
    HTCSetCreditDistribution(HTCHandle,
                             pCredInfo,
                             NULL,
                             NULL,
                             servicepriority,
                             WMI_MAX_SERVICES
                            );
    return A_OK;
}
#endif
