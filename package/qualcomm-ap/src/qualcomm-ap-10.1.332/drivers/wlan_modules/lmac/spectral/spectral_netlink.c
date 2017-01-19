/*
 * Copyright (c) 2010, Atheros Communications Inc.
 * All Rights Reserved.
 *
 * Copyright (c) 2011 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 *
 */
#include <osdep.h>
#include "ah.h"

#if ATH_SUPPORT_SPECTRAL

#include "spectral_data.h"
#include "spec_msg_proto.h"
#include "spectral.h"

#ifdef SPECTRAL_USE_NETLINK_SOCKETS
struct sock *spectral_nl_sock;
static atomic_t spectral_nl_users = ATOMIC_INIT(0);
int spectral_init_netlink(struct ath_spectral* spectral)
{

    if (spectral == NULL) {
        printk(  "%s: sc_spectral is NULL\n", __func__);
        return -EIO;
    }

    spectral->spectral_sent_msg=0;

    if (spectral_nl_sock == NULL) {
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,31))
        spectral_nl_sock = (struct sock *)netlink_kernel_create(NETLINK_ATHEROS, 1,&spectral_nl_data_ready, THIS_MODULE);
#else
        extern struct net init_net;
        spectral_nl_sock = (struct sock *)netlink_kernel_create(&init_net,NETLINK_ATHEROS, 1,&spectral_nl_data_ready, NULL, THIS_MODULE);
#endif
        if (spectral_nl_sock == NULL) {
            printk( "%s NETLINK_KERNEL_CREATE FAILED\n", __func__);
            return -ENODEV;
        }
    }
    atomic_inc(&spectral_nl_users);
    spectral->spectral_sock = spectral_nl_sock;

    if ((spectral==NULL) || (spectral->spectral_sock==NULL)) {
        printk( "%s NULL pointers (spectral=%d) (sock=%d) \n", __func__, (spectral==NULL),(spectral->spectral_sock==NULL));
        return -ENODEV;
    }
   if (spectral->spectral_skb == NULL) {
             printk( "%s %d NULL SKB\n", __func__, __LINE__);
    }

    return 0;

}

int spectral_destroy_netlink(struct ath_spectral* spectral)
{

    spectral->spectral_sock = NULL;
    if (atomic_dec_and_test(&spectral_nl_users)) {
        sock_release(spectral_nl_sock->sk_socket);
        spectral_nl_sock = NULL;
    }
    return 0;
}



#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,31))
void spectral_nl_data_ready(struct sock *sk, int len)
{
        printk("%s %d\n", __func__, __LINE__);
        return;
}

#else
void spectral_nl_data_ready(struct sk_buff *skb )
{
        printk("%s %d\n", __func__, __LINE__);
        return;
}

#endif /* VERSION*/
#endif /* SPECTRAL_USE_NETLINK_SOCKETS */


static void spectral_process_noise_pwr_report(struct ath_spectral *spectral, const SPECTRAL_SAMP_MSG* spec_samp_msg)
{
    int i, done;

#if 0
    printk(
        "%s: #%d/%d datalen=%d tstamp=%x last_tstamp=%x "
        "rssi=%d nb_lower=%d peak=%d\n",
        __func__, spectral->noise_pwr_reports_recv,
        spectral->noise_pwr_reports_reqd,
        spec_samp_msg->samp_data.spectral_data_len,
        spec_samp_msg->samp_data.spectral_tstamp,
        spec_samp_msg->samp_data.spectral_last_tstamp,
        spec_samp_msg->samp_data.spectral_lower_rssi,
        spec_samp_msg->samp_data.spectral_nb_lower,
        spec_samp_msg->samp_data.spectral_lower_max_index);
#endif

    spin_lock(&spectral->noise_pwr_reports_lock);

    if (!spectral->noise_pwr_reports_reqd) {
        spin_unlock(&spectral->noise_pwr_reports_lock);
        return;
    }

    if (spectral->noise_pwr_reports_recv < spectral->noise_pwr_reports_reqd) {
        spectral->noise_pwr_reports_recv++;

#if 0
        printk(
                         "#%d/%d: rssi=%3d,%3d,%3d %3d,%3d,%3d\n",
                         spectral->noise_pwr_reports_recv,
                         spectral->noise_pwr_reports_reqd,
                         spec_samp_msg->samp_data.spectral_chain_ctl_rssi[0],
                         spec_samp_msg->samp_data.spectral_chain_ctl_rssi[1],
                         spec_samp_msg->samp_data.spectral_chain_ctl_rssi[2],
                         spec_samp_msg->samp_data.spectral_chain_ext_rssi[0],
                         spec_samp_msg->samp_data.spectral_chain_ext_rssi[1],
                         spec_samp_msg->samp_data.spectral_chain_ext_rssi[2]);
#endif

        for(i = 0; i < ATH_MAX_ANTENNA; i++) {
            if (spectral->noise_pwr_chain_ctl[i]) {
                spectral->noise_pwr_chain_ctl[i]->pwr[spectral->noise_pwr_chain_ctl[i]->rptcount++] =
                    spec_samp_msg->samp_data.spectral_chain_ctl_rssi[i];
            }
            if (spectral->noise_pwr_chain_ext[i]) {
                spectral->noise_pwr_chain_ext[i]->pwr[spectral->noise_pwr_chain_ext[i]->rptcount++] =
                    spec_samp_msg->samp_data.spectral_chain_ext_rssi[i];
            }
        }
    }

    done = (spectral->noise_pwr_reports_recv >= spectral->noise_pwr_reports_reqd);

    spin_unlock(&spectral->noise_pwr_reports_lock);

    if (done) {
        SPECTRAL_LOCK(spectral);
        stop_current_scan(spectral);
        spectral->sc_spectral_scan = 0;
        SPECTRAL_UNLOCK(spectral);
#if 0
        printk(
                         "%s: done: %d/%d recv - set sc_spectral_scan = 0\n",
                         __func__, spectral->noise_pwr_reports_recv,
                         spectral->noise_pwr_reports_reqd);
#endif
    }
}

/*
 * Function     : spectral_create_samp_msg
 * Description  : create SAMP message and send it host
 * Input        :
 * Output       :
 *
 */

extern void dbg_print_SAMP_msg(SPECTRAL_SAMP_MSG* pmsg);
void spectral_create_samp_msg(struct ath_spectral* spectral, struct samp_msg_params *params)
{

    /*
     * XXX : Non-Rentrant. Will be an issue with dual concurrent
     *       operation on multi-processor system
     */

    static int i            = 0;
    int temp_samp_msg_len   = 0;

    static SPECTRAL_SAMP_MSG spec_samp_msg;

    SPECTRAL_SAMP_MSG   *msg        = NULL;
    SPECTRAL_SAMP_DATA *data        = NULL;
    u_int8_t *bin_pwr_data          = NULL;
    SPECTRAL_CLASSIFIER_PARAMS *cp  = NULL;
    SPECTRAL_CLASSIFIER_PARAMS *pcp = NULL;
    SPECTRAL_OPS* p_sops            = NULL;

#ifdef SPECTRAL_USE_NETLINK_SOCKETS
    static int samp_msg_index   =   0;
    int samp_msg_len            =   sizeof(SPECTRAL_SAMP_MSG);
#endif

    p_sops              = GET_SPECTRAL_OPS(spectral);

    temp_samp_msg_len   = sizeof(SPECTRAL_SAMP_MSG) - (MAX_NUM_BINS * sizeof(u_int8_t));
    temp_samp_msg_len  += (params->pwr_count * sizeof(u_int8_t));
    bin_pwr_data        = *(params->bin_pwr_data);

    memset(&spec_samp_msg, 0, sizeof(SPECTRAL_SAMP_MSG));

    data = &spec_samp_msg.samp_data;

    spec_samp_msg.signature                     = SPECTRAL_SIGNATURE;
    spec_samp_msg.freq                          = params->freq;
    spec_samp_msg.freq_loading                  = params->freq_loading;
    spec_samp_msg.samp_data.spectral_data_len   = params->datalen;
    spec_samp_msg.samp_data.spectral_rssi       = params->rssi;
    spec_samp_msg.samp_data.ch_width            = spectral->ch_width;

    spec_samp_msg.samp_data.spectral_combined_rssi  = (u_int8_t)params->rssi;
    spec_samp_msg.samp_data.spectral_upper_rssi     = params->upper_rssi;
    spec_samp_msg.samp_data.spectral_lower_rssi     = params->lower_rssi;

    OS_MEMCPY(spec_samp_msg.samp_data.spectral_chain_ctl_rssi, params->chain_ctl_rssi, sizeof(params->chain_ctl_rssi));
    OS_MEMCPY(spec_samp_msg.samp_data.spectral_chain_ext_rssi, params->chain_ext_rssi, sizeof(params->chain_ext_rssi));

    spec_samp_msg.samp_data.spectral_bwinfo         = params->bwinfo;
    spec_samp_msg.samp_data.spectral_tstamp         = params->tstamp;
    spec_samp_msg.samp_data.spectral_max_index      = params->max_index;

    /* Classifier in user space needs access to these */
    spec_samp_msg.samp_data.spectral_lower_max_index    = params->max_lower_index;
    spec_samp_msg.samp_data.spectral_upper_max_index    = params->max_upper_index;
    spec_samp_msg.samp_data.spectral_nb_lower           = params->nb_lower;
    spec_samp_msg.samp_data.spectral_nb_upper           = params->nb_upper;
    spec_samp_msg.samp_data.spectral_last_tstamp        = params->last_tstamp;
    spec_samp_msg.samp_data.spectral_max_mag            = params->max_mag;
    spec_samp_msg.samp_data.bin_pwr_count               = params->pwr_count;
    spec_samp_msg.samp_data.spectral_combined_rssi      = params->rssi;
    spec_samp_msg.samp_data.spectral_max_scale          = params->max_exp;

#ifdef SPECTRAL_USE_NETLINK_SOCKETS

    /*
     * This is a dirty hack to get the Windows build pass.
     * Currently Windows and Linux builds source spectral_data.h
     * form two different place. The windows version do not
     * have noise_floor member in it.
     *
     * As a temp workaround this variable is set under the
     * SPECTRAL_USE_NETLINK_SOCKETS as this is called only
     * under the linux build and this saves the day
     *
     * The plan to sync of header files in under the way
     *
     */

    spec_samp_msg.samp_data.noise_floor = params->noise_floor;
#endif  /* SPECTRAL_USE_NETLINK_SOCKETS */

    /* Classifier in user space needs access to these */
    cp  = &(spec_samp_msg.samp_data.classifier_params);
    pcp = &(params->classifier_params);

    OS_MEMCPY(cp, pcp, sizeof(SPECTRAL_CLASSIFIER_PARAMS));

#ifdef REVERSE_ORDER
    OS_MEMCPY(&data->bin_pwr[0], (bin_pwr_data+64), 64);
    OS_MEMCPY(&data->bin_pwr[64], (bin_pwr_data), 64);
#else
    OS_MEMCPY(&data->bin_pwr[0], bin_pwr_data, params->pwr_count);
#endif

#ifdef SPECTRAL_CLASSIFIER_IN_KERNEL
    if(params->interf_list.count) {
        OS_MEMCPY(&data->interf_list, &params->interf_list, sizeof(struct INTERF_SRC_RSP));
    } else
#endif
    data->interf_list.count = 0;

#ifdef  SPECTRAL_USE_NETLINK_SOCKETS
    spectral_prep_skb(spectral);

    if (spectral->spectral_skb != NULL) {
        p_sops->get_mac_address(spectral, spec_samp_msg.macaddr);
        spectral->spectral_nlh = (struct nlmsghdr*)spectral->spectral_skb->data;
        memcpy(NLMSG_DATA(spectral->spectral_nlh), &spec_samp_msg, sizeof(SPECTRAL_SAMP_MSG));
        msg = (SPECTRAL_SAMP_MSG*) NLMSG_DATA(spectral->spectral_nlh);
        spectral_bcast_msg(spectral);
        samp_msg_index++;
    }

#else
    /*
     * call the indicate function to pass the data to the net layer
     * Windows will pass to a spectral WIN32 service
     */
    msg = (SPECTRAL_SAMP_MSG *)OS_MALLOC(params->sc->sc_osdev, sizeof(SPECTRAL_SAMP_MSG), GFP_KERNEL);
    if (msg) {
        OS_MEMCPY(msg, &spec_samp_msg, sizeof(SPECTRAL_SAMP_MSG));
        ath_spectral_indicate(params->sc, (void*)msg, sizeof(SPECTRAL_SAMP_MSG));
        OS_FREE(msg);
        msg = NULL;
    } else {
        printk("No buffer\n");
    }
#endif  /* SPECTRAL_USE_NETLINK_SOCKETS */

    if (spectral->sc_spectral_noise_pwr_cal) {
        spectral_process_noise_pwr_report(spectral, &spec_samp_msg);
    }
}


#ifdef SPECTRAL_USE_NETLINK_SOCKETS

void spectral_prep_skb(struct ath_spectral* spectral)
{

    spectral->spectral_skb = dev_alloc_skb(MAX_SPECTRAL_PAYLOAD);

    if (spectral->spectral_skb != NULL) {

        skb_put(spectral->spectral_skb, MAX_SPECTRAL_PAYLOAD);
        spectral->spectral_nlh = (struct nlmsghdr*)spectral->spectral_skb->data;

        /* Possible bug that size of  SPECTRAL_SAMP_MSG and SPECTRAL_MSG
         * differ by 3 bytes  so we miss 3 bytes
         */

        spectral->spectral_nlh->nlmsg_len   = NLMSG_SPACE(sizeof(SPECTRAL_SAMP_MSG));
        spectral->spectral_nlh->nlmsg_pid   = 0;
        spectral->spectral_nlh->nlmsg_flags = 0;
    } else {
        spectral->spectral_skb = NULL;
        spectral->spectral_nlh = NULL;
    }
}

void spectral_unicast_msg(struct ath_spectral *spectral)
{

    if (spectral == NULL) {
        printk("%s Spectral is NULL\n", __func__);
        return;
    }

    if (spectral->spectral_sock == NULL) {
        printk("%s Spectral Socket is invalid\n", __func__);
        dev_kfree_skb(spectral->spectral_skb);
        spectral->spectral_skb = NULL;
        return;
    }


    if (spectral->spectral_skb != NULL) {

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,31))
        NETLINK_CB(spectral->spectral_skb).dst_pid = spectral->spectral_pid;
#endif /* VERSION - field depracated by newer kernel */
        NETLINK_CB(spectral->spectral_skb).pid = 0;  /* from kernel */
     /* to mcast group 1<<0 */
        NETLINK_CB(spectral->spectral_skb).dst_group=0;;

        netlink_unicast(spectral->spectral_sock, spectral->spectral_skb, spectral->spectral_pid, MSG_DONTWAIT);
    }
}

/*
 * Function     : spectral_bcast_msg
 * Description  : Passes the Spectral Message to Host
 * Input        : Pointer to spectral
 * Output       : Void
 *
 */
void spectral_bcast_msg(struct ath_spectral* spectral)
{
    fd_set write_set;
    SPECTRAL_SAMP_MSG *msg  = NULL;
    struct nlmsghdr *nlh    = NULL;
    int status;

    FD_ZERO(&write_set);

    if (spectral == NULL) {
        return;
    }

    if (spectral->spectral_sock == NULL) {
      dev_kfree_skb(spectral->spectral_skb);
      spectral->spectral_skb = NULL;
      return;
    }

    if (spectral->spectral_skb == NULL) {
        return;
    }

    nlh = (struct nlmsghdr*)spectral->spectral_skb->data;
    msg = (SPECTRAL_SAMP_MSG*) NLMSG_DATA(spectral->spectral_nlh);
    //print_samp_msg (msg, sc);

    status = netlink_broadcast(spectral->spectral_sock, spectral->spectral_skb, 0,1, GFP_ATOMIC);
    if (status == 0) {
      spectral->spectral_sent_msg++;
    }

    /* netlink will have freed the skb */
    if (spectral->spectral_skb != NULL) {
      spectral->spectral_skb = NULL;
    }
}

#endif /* SPECTRAL_USE_NETLINK_SOCKETS */
#endif
