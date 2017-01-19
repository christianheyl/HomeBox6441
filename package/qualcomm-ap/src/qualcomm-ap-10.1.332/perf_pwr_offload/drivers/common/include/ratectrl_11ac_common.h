/*
 * Copyright (c) 2012 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 * $ATH_LICENSE_TARGET_C$
 */

/*
 * This file holds all the structures that should have a common definition
 * for rate-control, irrespective to whether it is host-/target-based.
 */

#ifndef _RATECTRL_COMMON_H_
#define _RATECTRL_COMMON_H_

#if defined(ATH_TARGET)

#include "wlan_peer.h"
#include "wlan_vdev.h"

#else

#endif


#if defined(ATH_TARGET)

#define ratectrl_peer_t         wlan_peer_t
#define ratectrl_vdev_t         wlan_vdev_t
#define ratectrl_wal_peer_t     wal_peer_t
#define ratectrl_wal_vdev_t     wal_vdev_t

#else

#define ratectrl_peer_t         struct ol_txrx_peer_t
#define ratectrl_vdev_t         struct ol_txrx_vdev_t
#define ratectrl_wal_peer_t     ratectrl_peer_t
#define ratectrl_wal_vdev_t     ratectrl_vdev_t

#endif

#if defined(ATH_TARGET)
#define RC_GET_TX_RETRIES(tries, peer)  \
        do { \
            (tries) = wal_rc_get_vdev_tx_tries(((wal_peer_t *)((peer)->wal_peer))->vdev); \
        } while(0);

#define RC_GET_TPC(tpc, peer)   \
        do { \
            (tpc) = wal_rc_get_vdev_tpc(((wal_peer_t *)((peer)->wal_peer))->vdev); \
        } while(0);

#define RC_IS_DYN_BW(pdev)  wal_rc_is_dyn_bw((pdev))

#define IS_CONN_ACTIVE(co)   (co) 

#define RC_GET_PEER_BW(bw, wlan_peer) do {                                \
             struct rate_node *an = RATE_NODE(wlan_peer);                 \
             wlan_vdev_t *dev = (wlan_vdev_t *)(wlan_peer)->pDev;         \
             A_UINT8 vdev_bw = wal_rc_get_vdev_bw(DEV_GET_WAL_VDEV(dev)); \
             (bw) = (an->peer_params.ni_flags & RC_PEER_80MHZ)? 2:        \
                    (an->peer_params.ni_flags & RC_PEER_40MHZ)?1:0;       \
             if ((bw) > vdev_bw) {                                        \
                 (bw) = vdev_bw;                                          \
             }                                                            \
       } while (0);

#else
#define RC_GET_TX_RETRIES(tries, peer)  \
        do { \
            (tries) = (peer)->vdev->vdev_rc_info.def_tries; \
        } while(0);

#define RC_GET_TPC(tpc, peer)   \
        do { \
            (tpc) = (peer)->vdev->vdev_rc_info.def_tpc; \
        } while(0);

#define RC_IS_DYN_BW(pdev)  ((struct ol_txrx_pdev_t *)pdev)->ratectrl.dyn_bw

#define IS_CONN_ACTIVE(co)  1

#define RC_GET_PEER_BW(bw, wlan_peer) do {                                \
             struct rate_node *an = RATE_NODE(wlan_peer);                 \
           /*struct ol_txrx_vdev_t  *dev = (wlan_peer)->vdev; */          \
             A_UINT8 vdev_bw = 2; /* TODO: hook this to get vdev bw */    \
             (bw) = (an->peer_params.ni_flags & RC_PEER_80MHZ)? 2:        \
                    (an->peer_params.ni_flags & RC_PEER_40MHZ)?1:0;       \
             if ((bw) > vdev_bw) {                                        \
                 (bw) = vdev_bw;                                          \
             }                                                            \
       } while (0);


#endif


#define RATE_NODE(_ni)    ((_ni) ? ((struct rate_node *)((_ni)->rc_node)) : NULL)

typedef enum {
    RC_VDEV_M_IBSS  = 0,        /* IBSS (adhoc) station */
    RC_VDEV_M_STA   = 1,        /* infrastructure station */
    RC_VDEV_M_AP    = 2,        /* infrastructure AP */
    RC_VDEV_M_MONITOR = 3,      /* Monitor mode operation */
} RC_VDEV_OPMODE;

/**
 * @brief Various flags that need to be set for the peers in the target
 * @details
 * IMPORTANT: Make sure the bit definitions here are consistent
 * with the ni_flags definitions in wlan_peer.h
 */
#define RC_PEER_AUTH           0x00000001  /* Authorized for data */
#define RC_PEER_QOS            0x00000002  /* QoS enabled */
#define RC_PEER_NEED_PTK_4_WAY 0x00000004  /* Needs PTK 4 way handshake for authorization */
#define RC_PEER_APSD           0x00000800  /* U-APSD power save enabled */
#define RC_PEER_HT             0x00001000  /* HT enabled */
#define RC_PEER_40MHZ          0x00002000  /* 40MHz enabld */
#define RC_PEER_HT_ALLOWED     0x00004000  /* HT mode can be disallowed by firmware */
#define RC_PEER_STBC           0x00008000  /* STBC Enabled */
#define RC_PEER_LDPC           0x00010000  /* LDPC ENabled */
#define RC_PEER_DYN_MIMOPS     0x00020000  /* Dynamic MIMO PS Enabled */
#define RC_PEER_STATIC_MIMOPS  0x00040000  /* Static MIMO PS enabled */
#define RC_PEER_SPATIAL_MUX    0x00200000  /* SM Enabled */
#define RC_PEER_TXBF           0x00400000   /* TXBF */
#define RC_PEER_VHT            0x02000000  /* VHT Enabled */
#define RC_PEER_80MHZ          0x04000000  /* 80MHz enabld */

#endif /* _RATECTRL_COMMON_H_ */
