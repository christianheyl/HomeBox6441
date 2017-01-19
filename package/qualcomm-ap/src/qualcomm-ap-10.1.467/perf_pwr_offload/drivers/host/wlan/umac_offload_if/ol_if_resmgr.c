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
/*
 * UMAC resmgr specific offload interface functions - for power and performance offload model
 */
#include "ol_if_athvar.h"
#include "ol_if_vap.h"
#include "ieee80211_sm.h"

#if ATH_PERF_PWR_OFFLOAD

/* Resource Manager structure */
struct ieee80211_resmgr {
    /* the function indirection SHALL be the first fileds */
    struct ieee80211_resmgr_func_table              resmgr_func_table; 
    struct ieee80211com                             *ic;               /* Back pointer to ic */
    ieee80211_resmgr_mode                           mode;
    adf_os_spinlock_t rm_lock;   /* Lock ensures that only one thread runs res mgr at a time */
    /* Add one lock for notif_handler register/deregister operation, notif_handler will be modified by 
       wlan_scan_run() call and cause schedule-while-atomic in split driver. */
    adf_os_spinlock_t                                      rm_handler_lock;
    ieee80211_hsm_t                                 hsm_handle;        /* HSM Handle */
    ieee80211_resmgr_notification_handler           notif_handler[IEEE80211_MAX_RESMGR_EVENT_HANDLERS];
    void                                            *notif_handler_arg[IEEE80211_MAX_RESMGR_EVENT_HANDLERS];
};

#define IEEE80211_RESMGR_REQID     0x5555
#define IEEE80211_RESMGR_NOTIFICATION(_resmgr,_notif)  do {                 \
        int i;                                                              \
        for(i = 0;i < IEEE80211_MAX_RESMGR_EVENT_HANDLERS; ++i) {                  \
            if (_resmgr->notif_handler[i]) {                                \
                (* _resmgr->notif_handler[i])                               \
                    (_resmgr, _notif, _resmgr->notif_handler_arg[i]);       \
             }                                                              \
        }                                                                   \
    } while(0)


static void _ieee80211_resmgr_create_complete(ieee80211_resmgr_t resmgr)
{
    return;
}

static void _ieee80211_resmgr_delete(ieee80211_resmgr_t resmgr)
{
    struct ieee80211com *ic;
    struct ol_ath_softc_net80211 *scn;
    
    if (!resmgr)
        return ;

    ic = resmgr->ic;
    scn = OL_ATH_SOFTC_NET80211(ic);

    /* Register WMI event handlers */
    wmi_unified_unregister_event_handler(scn->wmi_handle, WMI_VDEV_START_RESP_EVENTID);

    adf_os_spinlock_destroy(&resmgr->rm_lock);
    adf_os_spinlock_destroy(&resmgr->rm_handler_lock);

    adf_os_mem_free(resmgr);

    ic->ic_resmgr = NULL;

    return;
}

static void _ieee80211_resmgr_delete_prepare(ieee80211_resmgr_t resmgr)
{
    return;
}

static int _ieee80211_resmgr_request_offchan(ieee80211_resmgr_t resmgr,
                                     struct ieee80211_channel *chan,
                                     u_int16_t reqid,
                                     u_int16_t max_bss_chan,
                                     u_int32_t max_dwell_time)
{
    return EOK;
}

static int _ieee80211_resmgr_request_bsschan(ieee80211_resmgr_t resmgr,
                                     u_int16_t reqid)
{
    return EOK;
}

static int _ieee80211_resmgr_request_chanswitch(ieee80211_resmgr_t resmgr,
                                        ieee80211_vap_t vap, 
                                        struct ieee80211_channel *chan,
                                        u_int16_t reqid)
{
    return EOK;
}

/* Implement wmi_unified_vdev_start_cmd() here */
static int _ieee80211_resmgr_vap_start(ieee80211_resmgr_t resmgr,
                               ieee80211_vap_t vap, 
                               struct ieee80211_channel *chan,
                               u_int16_t reqid,
                               u_int16_t max_start_time)
{
    struct ieee80211com *ic = resmgr->ic;
    struct ol_ath_softc_net80211 *scn = OL_ATH_SOFTC_NET80211(ic);
    struct ol_ath_vap_net80211 *avn = OL_ATH_VAP_NET80211(vap);
    u_int32_t freq;
    bool disable_hw_ack= false;

printk("OL vap_start +\n");

    freq = ieee80211_chan2freq(resmgr->ic, chan);
    if (!freq) {
        printk("ERROR : INVALID Freq \n");
        return 0;
    }
#if ATH_SUPPORT_DFS
    if ( vap->iv_opmode == IEEE80211_M_HOSTAP &&
        IEEE80211_IS_CHAN_DFS(chan)) {
           disable_hw_ack = true;
    }
#endif
    if (wmi_unified_vdev_start_send(scn->wmi_handle, avn->av_if_id, chan, freq, disable_hw_ack)) {
        printk("Unable to bring up the interface for ath_dev.\n");
        return -1;
    }

    /* Interface is up, UMAC is waiting for
     * target response     
     */ 
   avn->av_ol_resmgr_wait = TRUE;

   /* The channel configured in target is not same always with the vap desired channel
      due to 20/40 coexistence scenarios, so, channel is saved to configure on VDEV START RESP */
   avn->av_ol_resmgr_chan = chan;

printk("OL vap_start -\n");
    return EBUSY;
}

static void _ieee80211_resmgr_vattach(ieee80211_resmgr_t resmgr, ieee80211_vap_t vap)
{
    return;
}

static void _ieee80211_resmgr_vdetach(ieee80211_resmgr_t resmgr, ieee80211_vap_t vap)
{
    return;
}

static const char *_ieee80211_resmgr_get_notification_type_name(ieee80211_resmgr_t resmgr, ieee80211_resmgr_notification_type type)
{
    return "unknown";
}

static int _ieee80211_resmgr_register_noa_event_handler(ieee80211_resmgr_t resmgr,
    ieee80211_vap_t vap, ieee80211_resmgr_noa_event_handler handler, void *arg)
{

    return EOK;
}

static int _ieee80211_resmgr_unregister_noa_event_handler(ieee80211_resmgr_t resmgr,
    ieee80211_vap_t vap)
{
    return EOK;
}

static void ieee80211_notify_vap_start_complete(ieee80211_resmgr_t resmgr,
                             struct ieee80211vap *vap,  ieee80211_resmgr_notification_status status)
{
    ieee80211_resmgr_notification notif;

    /* Intimate start completion to VAP module */
    notif.type = IEEE80211_RESMGR_VAP_START_COMPLETE;
    notif.req_id = IEEE80211_RESMGR_REQID;
    notif.status = status;
    notif.vap = vap;
    vap->iv_rescan = 0;
    IEEE80211_RESMGR_NOTIFICATION(resmgr, &notif);

    printk("Notification to UMAC VAP layer\n");
}


/*
 * Register a resmgr notification handler.
 */
static int 
_ieee80211_resmgr_register_notification_handler(ieee80211_resmgr_t resmgr, 
                                               ieee80211_resmgr_notification_handler notificationhandler,
                                               void *arg)
{
    int i;

    /* unregister if there exists one already */
    ieee80211_resmgr_unregister_notification_handler(resmgr, notificationhandler,arg);
    adf_os_spin_lock(&resmgr->rm_handler_lock);
    for (i=0; i<IEEE80211_MAX_RESMGR_EVENT_HANDLERS; ++i) {
        if (resmgr->notif_handler[i] == NULL) {
            resmgr->notif_handler[i] = notificationhandler;
            resmgr->notif_handler_arg[i] = arg;
            adf_os_spin_unlock(&resmgr->rm_handler_lock);
            return 0;
        }
    }
    adf_os_spin_unlock(&resmgr->rm_handler_lock);
    return -ENOMEM;
}

/*
 * Unregister a resmgr event handler.
 */
static int _ieee80211_resmgr_unregister_notification_handler(ieee80211_resmgr_t resmgr, 
                                                     ieee80211_resmgr_notification_handler handler,
                                                     void  *arg)
{
    int i;
    adf_os_spin_lock(&resmgr->rm_handler_lock);
    for (i=0; i<IEEE80211_MAX_RESMGR_EVENT_HANDLERS; ++i) {
        if (resmgr->notif_handler[i] == handler && resmgr->notif_handler_arg[i] == arg ) {
            resmgr->notif_handler[i] = NULL;
            resmgr->notif_handler_arg[i] = NULL;
            adf_os_spin_unlock(&resmgr->rm_handler_lock);
            return 0;
        }
    }
    adf_os_spin_unlock(&resmgr->rm_handler_lock);
    return -EEXIST;
}


static int _ieee80211_resmgr_off_chan_sched_set_air_time_limit(ieee80211_resmgr_t resmgr,
    ieee80211_vap_t vap,
    u_int32_t scheduled_air_time_limit
    )
{
    return EINVAL;
}

static u_int32_t _ieee80211_resmgr_off_chan_sched_get_air_time_limit(ieee80211_resmgr_t resmgr,
    ieee80211_vap_t vap
    )
{
    return 100;
}

/* Implement wmi_unified_vdev_start_cmd() here */
static int _ieee80211_resmgr_vap_stop(ieee80211_resmgr_t resmgr,
                               ieee80211_vap_t vap, 
                               u_int16_t reqid)
{
#if 0        
    struct ieee80211com *ic = resmgr->ic;
    struct ol_ath_softc_net80211 *scn = OL_ATH_SOFTC_NET80211(ic);
    struct ol_ath_vap_net80211 *avn = OL_ATH_VAP_NET80211(vap);
#endif

    printk("OL vap_stop +\n");

    vap->iv_stopping(vap);
    printk("OL vap_stop -\n");
    return EOK;
}

static int 
ol_vdev_wmi_event_handler(ol_scn_t scn, u_int8_t *data, u_int16_t datalen, void *context)
{
    ieee80211_resmgr_t resmgr = (ieee80211_resmgr_t )context;
    wmi_vdev_start_response_event *wmi_vdev_start_resp_ev =
                                                (wmi_vdev_start_response_event *)data;
    wlan_if_t vaphandle;
    struct ieee80211com  *ic = resmgr->ic;
    struct ieee80211_channel *chan = NULL;
    struct ieee80211_node *ni;
    u_int8_t numvaps;
    struct ol_ath_vap_net80211 *avn;
    bool do_notify = true;
    vaphandle = ol_ath_vap_get(scn, wmi_vdev_start_resp_ev->vdev_id);
    avn = OL_ATH_VAP_NET80211(vaphandle);

printk("ol_vdev_start_resp_ev\n");
    switch (vaphandle->iv_opmode) {

        case IEEE80211_M_MONITOR:
               /* Handle same as HOSTAP */
        case IEEE80211_M_HOSTAP:
            /* If vap is not waiting for the WMI event from target
               return here
             */
            if(avn->av_ol_resmgr_wait == FALSE) {
               return 0;
            }
             /* Resetting the ol_resmgr_wait flag*/
            avn->av_ol_resmgr_wait = FALSE;

            numvaps = ieee80211_vaps_active(ic);

            chan =  vaphandle->iv_des_chan[vaphandle->iv_des_mode];

            /* 
             * if there is a vap already running.
             * ignore the desired channel and use the
             * operating channel of the other vap.
             */
            /* so that cwm can do its own crap. need to untie from state */
            /* vap join is called here to wake up the chip if it is in sleep state */
            ieee80211_vap_join(vaphandle);
            
            if (numvaps == 0) {
                //AP_DFS: ieee80211_set_channel(ic, chan);
                if (wmi_vdev_start_resp_ev->resp_type != WMI_VDEV_RESTART_RESP_EVENT) {
                     /* 20/40 Mhz coexistence  handler */
                    if ((avn->av_ol_resmgr_chan != NULL) && (chan != avn->av_ol_resmgr_chan)) {
                        chan = avn->av_ol_resmgr_chan;
                    }

                    ic->ic_prevchan = ic->ic_curchan;
                    ic->ic_curchan = chan;
                    /* update max channel power to max regpower of current channel */
                    ieee80211com_set_curchanmaxpwr(ic, chan->ic_maxregpower);
                    ieee80211_wme_initparams(vaphandle);
                }

                /* ieee80211 Layer - Default Configuration */
                vaphandle->iv_bsschan = ic->ic_curchan;
                /* XXX reset erp state */
                ieee80211_reset_erp(ic, ic->ic_curmode, vaphandle->iv_opmode);
            } else {
                /* ieee80211 Layer - Default Configuration */
                vaphandle->iv_bsschan = ic->ic_curchan;
            }
            /* use the vap bsschan for dfs configure */
            if ( IEEE80211_IS_CHAN_DFS(vaphandle->iv_bsschan)) {
                   extern void ol_if_dfs_configure(struct ieee80211com *ic);
                   ol_if_dfs_configure(ic);
            }
            break;
        case IEEE80211_M_STA:

            ni = vaphandle->iv_bss;

            chan = ni->ni_chan;

            vaphandle->iv_bsschan = chan;
            
            ic->ic_prevchan = ic->ic_curchan;
            ic->ic_curchan = chan;
            /* update max channel power to max regpower of current channel */
            ieee80211com_set_curchanmaxpwr(ic, chan->ic_maxregpower);

            /* ieee80211 Layer - Default Configuration */
            vaphandle->iv_bsschan = ic->ic_curchan;

            /* XXX reset erp state */
            ieee80211_reset_erp(ic, ic->ic_curmode, vaphandle->iv_opmode);
            ieee80211_wme_initparams(vaphandle);
            if (wmi_vdev_start_resp_ev->resp_type == WMI_VDEV_RESTART_RESP_EVENT) {
                do_notify = false;
            }
            
            break;
        default:
            break;
    }

    /* Intimate start completion to VAP module */
    /* if STA, bypass notification for RESTERT EVENT */
    if (do_notify)
    ieee80211_notify_vap_start_complete(resmgr, vaphandle, IEEE80211_RESMGR_STATUS_SUCCESS);

    return 0;
}

ieee80211_resmgr_t ol_resmgr_create(struct ieee80211com *ic, ieee80211_resmgr_mode mode)
{
    struct ol_ath_softc_net80211 *scn = OL_ATH_SOFTC_NET80211(ic);
    ieee80211_resmgr_t    resmgr;

printk("OL Resmgr Init-ed\n");

    if (ic->ic_resmgr) {
        printk("%s : ResMgr already exists \n", __func__); 
        return NULL;
    }

    /* Allocate ResMgr data structures */
    resmgr = (ieee80211_resmgr_t) adf_os_mem_alloc(scn->adf_dev, 
                                            sizeof(struct ieee80211_resmgr));
    if (resmgr == NULL) {
        printk("%s : ResMgr memory alloction failed\n", __func__); 
        return NULL;
    }

    OS_MEMZERO(resmgr, sizeof(struct ieee80211_resmgr));
    resmgr->ic = ic;
    resmgr->mode = mode;
    /* Indicate the device is capable of multi-chan operation*/
    ic->ic_caps_ext |= IEEE80211_CEXT_MULTICHAN;

    /* initialize function pointer table */
    resmgr->resmgr_func_table.resmgr_create_complete = _ieee80211_resmgr_create_complete; 
    resmgr->resmgr_func_table.resmgr_delete = _ieee80211_resmgr_delete;
    resmgr->resmgr_func_table.resmgr_delete_prepare = _ieee80211_resmgr_delete_prepare; 

    resmgr->resmgr_func_table.resmgr_register_notification_handler = _ieee80211_resmgr_register_notification_handler; 
    resmgr->resmgr_func_table.resmgr_unregister_notification_handler = _ieee80211_resmgr_unregister_notification_handler; 

    resmgr->resmgr_func_table.resmgr_request_offchan = _ieee80211_resmgr_request_offchan; 
    resmgr->resmgr_func_table.resmgr_request_bsschan = _ieee80211_resmgr_request_bsschan; 
    resmgr->resmgr_func_table.resmgr_request_chanswitch = _ieee80211_resmgr_request_chanswitch; 

    resmgr->resmgr_func_table.resmgr_vap_start = _ieee80211_resmgr_vap_start; 

    resmgr->resmgr_func_table.resmgr_vattach = _ieee80211_resmgr_vattach; 
    resmgr->resmgr_func_table.resmgr_vdetach = _ieee80211_resmgr_vdetach; 
    resmgr->resmgr_func_table.resmgr_get_notification_type_name = _ieee80211_resmgr_get_notification_type_name; 
    resmgr->resmgr_func_table.resmgr_register_noa_event_handler = _ieee80211_resmgr_register_noa_event_handler; 
    resmgr->resmgr_func_table.resmgr_unregister_noa_event_handler = _ieee80211_resmgr_unregister_noa_event_handler; 
    resmgr->resmgr_func_table.resmgr_off_chan_sched_set_air_time_limit = _ieee80211_resmgr_off_chan_sched_set_air_time_limit; 
    resmgr->resmgr_func_table.resmgr_off_chan_sched_get_air_time_limit = _ieee80211_resmgr_off_chan_sched_get_air_time_limit; 

    resmgr->resmgr_func_table.resmgr_vap_stop = _ieee80211_resmgr_vap_stop; 

    adf_os_spinlock_init(&resmgr->rm_lock);
    adf_os_spinlock_init(&resmgr->rm_handler_lock);

    /* Register WMI event handlers */
    wmi_unified_register_event_handler(scn->wmi_handle, WMI_VDEV_START_RESP_EVENTID,
                                        ol_vdev_wmi_event_handler, resmgr);

    return resmgr;
}



void
ol_ath_resmgr_attach(struct ieee80211com *ic)
{
    ic->ic_resmgr_create = ol_resmgr_create;
}

#endif /* ATH_PERF_PWR_OFFLOAD */

