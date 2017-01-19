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

#include "htc_host_internal.h"

a_uint8_t MapSvc2ULPipe(HTC_SERVICE_ID ServiceID)    /* UL: host --> target */
{
    a_uint8_t UL_Pipe = 0;

#ifdef MAGPIE_SINGLE_CPU_CASE

   adf_os_print("UL Pipes: %x\n", nPipes);
   UL_Pipe = 0;

#elif defined(MAGPIE_HIF_USB)
    switch(ServiceID) {
        case WMI_CONTROL_SVC:
            UL_Pipe = 4;  //HIF_USB_PIPE_COMMAND = 4 => WMI_CONTROL should use USB command pipe
            break;
        case WMI_BEACON_SVC:
          #ifdef __linux__
            UL_Pipe = 5;  //HIF_USB_PIPE_HP_TX
          #else
            UL_Pipe = 1;  //HIF_USB_PIPE_TX
          #endif     
            break;
        case WMI_CAB_SVC:
            UL_Pipe = 1;  //HIF_USB_PIPE_TX 
            break;
        case WMI_UAPSD_SVC:
            UL_Pipe = 1;  //HIF_USB_PIPE_TX
            break;
        case WMI_MGMT_SVC:
            UL_Pipe = 1;  //HIF_USB_PIPE_TX
            break;
        case WMI_DATA_VO_SVC:
            UL_Pipe = 1;  //HIF_USB_PIPE_TX
            break;
        case WMI_DATA_VI_SVC:
            UL_Pipe = 1;  //HIF_USB_PIPE_TX
            break;
        case WMI_DATA_BE_SVC:
            UL_Pipe = 1;  //HIF_USB_PIPE_TX
            break;
        case WMI_DATA_BK_SVC:
            UL_Pipe = 1;  //HIF_USB_PIPE_TX
            break;
        default:
            adf_os_print("Svc %x not supported.\n", ServiceID);
            adf_os_assert(0);
    }

#else    /* PCI, GMII */

    a_uint8_t nPipes = HIFGetULPipeNum();

    if (nPipes == 1) {    /* 1 TX pipe */
        UL_Pipe = 0;
    }
    else if (nPipes == 4) {    /* 4 TX pipes */
        switch(ServiceID) {
            case WMI_CONTROL_SVC:
                UL_Pipe = 0;/* Mgmt*/
                break;
            case WMI_BEACON_SVC:
                UL_Pipe = 3;/* High Priority*/
                break;
            case WMI_CAB_SVC:
                UL_Pipe = 1;/* Low Priority*/
                break; 
            case WMI_UAPSD_SVC:
                UL_Pipe = 1;/* Low Priority*/
                break;
            case WMI_MGMT_SVC:
                UL_Pipe = 1;/* Low Priority*/
                break;
            case WMI_DATA_VO_SVC:
                UL_Pipe = 1;/* Low Priority*/
                break;
            case WMI_DATA_VI_SVC:
                UL_Pipe = 1;/* Low Priority*/
                break;
            case WMI_DATA_BE_SVC:
                UL_Pipe = 1;/* Low Priority*/
                break;
            case WMI_DATA_BK_SVC:
                UL_Pipe = 1;/* Low Priority*/
                break;
            default:
                adf_os_print("Svc %x not supported.\n", ServiceID);
                adf_os_assert(0);
        }
    }
    else {
        adf_os_print("Not support %u pipes\n", nPipes);
        adf_os_assert(0);
    }

#endif

    return UL_Pipe;
    
}

a_uint8_t MapSvc2DLPipe(HTC_SERVICE_ID ServiceID)    /* DL: host <-- target */
{
    a_uint8_t DL_Pipe = 0;

#ifdef MAGPIE_SINGLE_CPU_CASE

   adf_os_print("DL Pipes: %x\n", nPipes);
   DL_Pipe = 0;

#elif defined(MAGPIE_HIF_USB)

    switch(ServiceID) {
        case WMI_CONTROL_SVC:
            DL_Pipe = 3;  //HIF_USB_PIPE_INTERRUPT = 3 => WMI_CONTROL should use USB interrupt pipe
            break;
        case WMI_BEACON_SVC:
            DL_Pipe = 2;  //HIF_USB_PIPE_RX
            break;
        case WMI_CAB_SVC:
            DL_Pipe = 2;  //HIF_USB_PIPE_RX 
            break;
        case WMI_UAPSD_SVC:
            DL_Pipe = 2;  //HIF_USB_PIPE_RX
            break;
        case WMI_MGMT_SVC:
            DL_Pipe = 2;  //HIF_USB_PIPE_RX
            break;
        case WMI_DATA_VO_SVC:
            DL_Pipe = 2;  //HIF_USB_PIPE_RX
            break;
        case WMI_DATA_VI_SVC:
            DL_Pipe = 2;  //HIF_USB_PIPE_RX
            break;
        case WMI_DATA_BE_SVC:
            DL_Pipe = 2;  //HIF_USB_PIPE_RX
            break;
        case WMI_DATA_BK_SVC:
            DL_Pipe = 2;  //HIF_USB_PIPE_RX
            break;
        default:
            adf_os_print("Svc %x not supported.\n", ServiceID);
            adf_os_assert(0);
    }

#else    /* PCI, GMII */

    a_uint8_t nPipes = HIFGetDLPipeNum();

    if (nPipes == 1) {    /* 1 RX pipe */
        DL_Pipe = 0;
    }
    else if (nPipes == 2) {    /* 2 RX pipes */
        switch(ServiceID) {
            case WMI_CONTROL_SVC:
                DL_Pipe = 0;
                break;
            case WMI_BEACON_SVC:
                DL_Pipe = 1;
                break;
            case WMI_CAB_SVC:
                DL_Pipe = 1;
                break;
            case WMI_UAPSD_SVC:
                DL_Pipe = 1;
                break;
            case WMI_MGMT_SVC:
                DL_Pipe = 1;
                break;
            case WMI_DATA_VO_SVC:
                DL_Pipe = 1;
                break;
            case WMI_DATA_VI_SVC:
                DL_Pipe = 1;
                break;
            case WMI_DATA_BE_SVC:
                DL_Pipe = 1;
                break;
            case WMI_DATA_BK_SVC:
                DL_Pipe = 1;
                break;
            default:
                adf_os_print("Svc %x not supported.\n", ServiceID);
                adf_os_assert(0);
        }
    }
    else {
        adf_os_print("Not support %u pipes\n", nPipes);
        adf_os_assert(0);
    }

#endif

    return DL_Pipe;

}


