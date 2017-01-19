/*
 * Copyright (c) 2011, Atheros Communications Inc.
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

#include "ol_if_athvar.h"
#include "ol_defines.h"
#include "sw_version.h"
#include "targaddrs.h"
#include "ol_helper.h"
#include "adf_os_mem.h"   /* adf_os_mem_alloc,free */
#include "adf_os_types.h" /* adf_os_vprint */

#ifdef  QVIT
#include <qvit/qvit_defs.h>

static int ol_ath_qvit_event(ol_scn_t scn,
                             u_int8_t *data,
                             u_int16_t datalen,
                             void *context)
{
    QVIT_SEG_HDR_INFO_STRUCT segHdrInfo;
    u_int8_t totalNumOfSegments,currentSeq;

    segHdrInfo = *(QVIT_SEG_HDR_INFO_STRUCT *)&(data[0]);

    scn->utf_event_info.currentSeq = (segHdrInfo.segmentInfo & 0xF);

    currentSeq = (segHdrInfo.segmentInfo & 0xF);
    totalNumOfSegments = (segHdrInfo.segmentInfo >>4)&0xF;

    datalen = datalen - sizeof(segHdrInfo);
#ifdef QVIT_DEBUG_EVENT
    printf(KERN_INFO "QVIT: %s: totalNumOfSegments [%d]\n", __FUNCTION__, totalNumOfSegments);
#endif
    if ( currentSeq == 0 )
    {
        scn->utf_event_info.expectedSeq = 0;
        scn->utf_event_info.offset = 0;
    }
    else
    {
        if ( scn->utf_event_info.expectedSeq != currentSeq )
        {
            printk(KERN_ERR "QVIT: Mismatch in expecting seq expected Seq %d got seq %d\n",scn->utf_event_info.expectedSeq,currentSeq);
        }
    }

    OS_MEMCPY(&scn->utf_event_info.data[scn->utf_event_info.offset],&data[sizeof(segHdrInfo)],datalen);
    scn->utf_event_info.offset = scn->utf_event_info.offset + datalen;
    scn->utf_event_info.expectedSeq++;
#ifdef QVIT_DEBUG_EVENT
    printk(KERN_INFO "QVIT: %s: scn->utf_event_info.expectedSeq [%d]\n", __FUNCTION__, scn->utf_event_info.expectedSeq);
#endif
    if ( scn->utf_event_info.expectedSeq == totalNumOfSegments )
    {
        if( scn->utf_event_info.offset != segHdrInfo.len )
        {
            printk(KERN_ERR "QVIT: All segs received total len mismatch .. len %d total len %d\n",scn->utf_event_info.offset,segHdrInfo.len);
        }
#ifdef QVIT_DEBUG_EVENT
        qvit_hexdump((unsigned char *)data, datalen + 4);
#endif
        scn->utf_event_info.length = scn->utf_event_info.offset;
    }

    return 0;
}

void ol_ath_qvit_detach(struct ol_ath_softc_net80211 *scn)
{

#ifdef QVIT
    printk(KERN_INFO "QVIT: %s: called\n", __FUNCTION__);
#endif
    OS_FREE(scn->utf_event_info.data);
    wmi_unified_unregister_event_handler(scn->wmi_handle, WMI_PDEV_QVIT_EVENTID);
    scn->utf_event_info.data = NULL;
    scn->utf_event_info.length = 0;
}

void ol_ath_qvit_attach(struct ol_ath_softc_net80211 *scn)
{
#ifdef QVIT
    printk(KERN_INFO "QVIT: %s: called\n", __FUNCTION__);
#endif
    scn->utf_event_info.data = (unsigned char *)OS_MALLOC((void*)scn->sc_osdev,MAX_QVIT_EVENT_LENGTH,GFP_KERNEL);
    scn->utf_event_info.length = 0;

    wmi_unified_register_event_handler(scn->wmi_handle, WMI_PDEV_QVIT_EVENTID,
                                       ol_ath_qvit_event,
                                       NULL);
}

int wmi_unified_pdev_qvit_cmd(wmi_unified_t wmi_handle,
                              u_int8_t *utf_payload,
                              u_int32_t len)
{
    wmi_buf_t buf;
    u_int8_t *cmd;
    int ret=0;
    static u_int8_t msgref = 1; /// We can initialize the value and increment..
    u_int8_t segNumber =0,segInfo,numSegments;
    u_int16_t  chunkLen, totalBytes;
    u_int8_t*  bufpos;
    QVIT_SEG_HDR_INFO_STRUCT segHdrInfo;
#ifdef QVIT_DEBUG
    printk(KERN_INFO "QVIT: %s: called\n", __FUNCTION__);
#endif
    bufpos = utf_payload;
    totalBytes = len;
    numSegments = (totalBytes / MAX_WMI_QVIT_LEN );

    if ( len - (numSegments * MAX_WMI_QVIT_LEN) )
        numSegments++;

    while (len)
    {
        if (len > MAX_WMI_QVIT_LEN)
            chunkLen = MAX_WMI_QVIT_LEN; /* MAX messsage.. */
        else
            chunkLen = len;

        buf = wmi_buf_alloc(wmi_handle, (chunkLen + sizeof(segHdrInfo)) );
        if (!buf)
        {
            printk(KERN_ERR "QVIT: %s: wmi_buf_alloc failed\n", __FUNCTION__);
            return -1;
        }

        cmd = (u_int8_t *)wmi_buf_data(buf);

        segHdrInfo.len = totalBytes;
        segHdrInfo.msgref =  msgref;
        segInfo = ((numSegments << 4 ) & 0xF0) | (segNumber & 0xF);
        segHdrInfo.segmentInfo = segInfo;

        segNumber++;

        OS_MEMCPY(cmd, &segHdrInfo, sizeof(segHdrInfo)); // 4 bytes..
        OS_MEMCPY(&cmd[sizeof(segHdrInfo)], bufpos, chunkLen);

        ret =  wmi_unified_cmd_send(wmi_handle, buf, (chunkLen + sizeof(segHdrInfo)),
                                    WMI_PDEV_QVIT_CMDID);

        if (ret != 0 )
        {

            printk(KERN_ERR "QVIT: %s: wmi_unified_cmd_send failed\n", __FUNCTION__);
            break;
        }

        len -= chunkLen;
        bufpos += chunkLen;
    }

    msgref++;

    return ret;
}

int ol_ath_qvit_cmd(ol_scn_t scn_handle, u_int8_t *data, u_int16_t len)
{
    struct ol_ath_softc_net80211 *scn = (struct ol_ath_softc_net80211 *)scn_handle;

    scn->utf_event_info.length = 0;

#ifdef QVIT_DEBUG
    printk(KERN_INFO "QVIT: %s called\n", __FUNCTION__);
#endif
    return wmi_unified_pdev_qvit_cmd(scn->wmi_handle,data,len);
}


int ol_ath_qvit_rsp(ol_scn_t scn, u_int8_t *payload)
{
    int ret = -1;
    if ( scn->utf_event_info.length )
    {
        ret = 0;

        *(A_UINT32*)&(payload[0]) = scn->utf_event_info.length;
        OS_MEMCPY((payload+4), scn->utf_event_info.data, scn->utf_event_info.length);

#ifdef QVIT_DEBUG_RSP
        printk(KERN_INFO "QVIT: %s: Start\n", __FUNCTION__);
        qvit_hexdump((unsigned char *)payload, (unsigned int)(scn->utf_event_info.length + 4));
        printk(KERN_INFO "QVIT: %s: End\n", __FUNCTION__);
#endif
        scn->utf_event_info.length = 0;
    }
#ifdef QVIT_DEBUG_RSP1
    printf(KERN_INFO "QVIT: %s: ret = %d\n", __FUNCTION__, ret);
#endif
    return ret;
}




#endif
