/*
 * Copyright (c) 2009, Atheros Communications Inc. 
 * All Rights Reserved.
 * 
 * Copyright (c) 2011 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 * 
 *
 *  RIFS Burst (RB) state machine.
 */

#ifdef ATH_RB

#include "ath_internal.h"

/* Static function declarations */
static void ath_rb_starting(void *arg, ath_rb_event_t event);
static void ath_rb_detecting(void *arg, ath_rb_event_t event);
static void ath_rb_inrb(void *arg, ath_rb_event_t event);
static int  ath_rb_timeout(void *arg);

/* Initialize the RB state machine */
void
ath_rb_init(struct ath_softc *sc)
{
    ath_rb_t     *rb = &sc->sc_rb;

    rb->sc = sc;
    OS_MEMZERO(&rb->restore_timer, sizeof(rb->restore_timer));
    ath_initialize_timer(sc->sc_osdev, &rb->restore_timer, 
                         sc->sc_rxrifs_timeout,
                         ath_rb_timeout, rb);
    ATH_RB_SET_STATE(rb, ath_rb_starting);
    ATH_RB_SEND_EVENT(rb, ATH_RB_RESET);
}

/* RB state machine: Populate the RB sequence number history array */
static void
ath_rb_starting(void *arg, ath_rb_event_t event)
{
    ath_rb_t *rb = (ath_rb_t *)arg;
    struct ieee80211_qosframe   *whqos;
    u_int16_t rxseq;

    switch (event) {
        /* Reset state machine: evacuate timer */
        case ATH_RB_RESET:
            ath_cancel_timer(&rb->restore_timer, CANCEL_NO_SLEEP);
            /* Fall through */
        /* Restore state machine: reset variables */
        case ATH_RB_RESTORE_TIMEOUT:
            rb->skip_cnt = 0;
            OS_MEMZERO(rb->seqs, sizeof(rb->seqs));
            break;
        /* Populate the seqs array */
        case ATH_RB_RX_SEQ:
            whqos = (struct ieee80211_qosframe *)ATH_RB_GET_ARG(rb);
            rxseq = le16toh(*(u_int16_t *)whqos->i_seq) >>
                    IEEE80211_SEQ_SEQ_SHIFT;

            /*
             * seqs[1] is the one seen last.
             */
            if (!rb->seqs[1]) {
                ATH_RB_QUEUE_SEQ(rb, rxseq);
                return;
            }

            /*
             * seqs[0] is the oldest.
             */
            KASSERT(!rb->seqs[0],
                    ("%s: unexpected seq %d", __FUNCTION__, rb->seqs[0]));
            ATH_RB_QUEUE_SEQ(rb, rxseq);
            ATH_RB_SET_STATE(rb, ath_rb_detecting);
            break;
        default:
            KASSERT(0, ("%s(): Unhandled event %d", __FUNCTION__, event));
    }
}

/* RB state machine: Determine if the RB sequence pattern from ath_rb_starting
 * resembles a RIFS burst
 */
static void
ath_rb_detecting(void *arg, ath_rb_event_t event)
{
    ath_rb_t *rb = (ath_rb_t *)arg;
    struct ath_softc *sc = rb->sc;
    struct ieee80211_qosframe *whqos;
    u_int16_t rxseq, rxdiff_now, rxdiff_prev;

    switch (event) {
        /* Reset state machine */
        case ATH_RB_RESET:
            ATH_RB_SET_STATE(rb, ath_rb_starting);
            ATH_RB_SEND_EVENT(rb, ATH_RB_RESET);
            break;
        /* Evaluate contents of the seqs array */
        case ATH_RB_RX_SEQ:
            whqos = (struct ieee80211_qosframe *)ATH_RB_GET_ARG(rb);
            rxseq = le16toh(*(u_int16_t *)whqos->i_seq) >> 
                    IEEE80211_SEQ_SEQ_SHIFT; 
            rxdiff_now = (rxseq - rb->seqs[1]) & 
                         (IEEE80211_SEQ_MAX - 1);
            rxdiff_prev = (rb->seqs[1] - rb->seqs[0]) & 
                          (IEEE80211_SEQ_MAX - 1);

            /*
             * Three consecutive sequence numbers or non-1-skips. 
             * Reset and start again.
             */ 
            if ((rxdiff_now  == 1 && rxdiff_prev == 1) ||
                (rxdiff_now > 2 || rxdiff_prev > 2)) {
                ATH_RB_SET_STATE(rb, ath_rb_starting);
                ATH_RB_SEND_EVENT(rb, ATH_RB_RESET);
                break;
            }

            ATH_RB_QUEUE_SEQ(rb, rxseq);

            /*
             * Three sequence numbers skipping by 1. Could this be rb?
             */ 
            if (rxdiff_now == 2 && rxdiff_prev == 2) {
                if (++rb->skip_cnt >= sc->sc_rxrifs_skipthresh) {
                    ATH_RB_SET_STATE(rb, ath_rb_inrb);
                    ATH_RB_SEND_EVENT(rb, ATH_RB_ACTIVATE);
                }
            }
            break;
        /* Invalid states */
        case ATH_RB_ACTIVATE:
        case ATH_RB_RESTORE_TIMEOUT:
        default:
            KASSERT(0, ("%s(): Unhandled event %d", __FUNCTION__, event));
    }
}

/* RB state machine: State machine routine used for setting, unsetting,
 * and maintaining a RIFS burst.
 */
static void 
ath_rb_inrb(void *arg, ath_rb_event_t event)
{
    ath_rb_t *rb = (ath_rb_t *)arg;
    struct ath_softc *sc = rb->sc;
    struct ieee80211_qosframe *whqos;
    u_int16_t rxseq;

    switch (event) {
        /* Activating RB Rx */
        case ATH_RB_ACTIVATE:
            ath_rb_set(sc, 1);
            ath_set_timer_period(&rb->restore_timer, sc->sc_rxrifs_timeout);
            ath_start_timer(&rb->restore_timer);
            break;
        /* Maintaining RB Rx */
        case ATH_RB_RX_SEQ:
            whqos = (struct ieee80211_qosframe *)ATH_RB_GET_ARG(rb);
            rxseq = le16toh(*(u_int16_t *)whqos->i_seq) >> 
                    IEEE80211_SEQ_SEQ_SHIFT; 

            ATH_RB_QUEUE_SEQ(rb, rxseq);

            /* 
             * restart timer if seq found
             */
            ath_cancel_timer(&rb->restore_timer, CANCEL_NO_SLEEP);
            ath_set_timer_period(&rb->restore_timer, sc->sc_rxrifs_timeout);
            ath_start_timer(&rb->restore_timer);
            break;
        /* Timeout RB Rx */
        case ATH_RB_RESTORE_TIMEOUT:
            ath_rb_set(sc, 0);
        /* Reset state machine */
        case ATH_RB_RESET:
            ATH_RB_SET_STATE(rb, ath_rb_starting);
            ATH_RB_SEND_EVENT(rb, event);
            break;
        default:
            KASSERT(0, ("%s(): Unhandled event %d", __FUNCTION__, event));
    }
}

/* RB state machine: Timer for unsetting RB Rx */
static int
ath_rb_timeout(void *arg)
{
    ath_rb_t *rb = (ath_rb_t *) arg;

    ATH_RB_SEND_EVENT(rb, ATH_RB_RESTORE_TIMEOUT);
    return 1;
}

/* RB state machine: Reset RB state machine as part of reset,chan change,etc */
void
ath_rb_reset(struct ath_softc *sc)
{
    if (!sc->sc_do_rb_war)
        return;

    if (ATH_RB_MODE_DETECT == sc->sc_rxrifs) {
        ATH_RB_SEND_EVENT(&sc->sc_rb, ATH_RB_RESET);
    }    
}

/* RB state machine: Entry point, conditionally called from recv */
void
ath_rb_detect(ath_rb_t *rb, struct ieee80211_qosframe *whqos)
{
    ATH_RB_SET_ARG(rb, whqos);
    ATH_RB_SEND_EVENT(rb, ATH_RB_RX_SEQ);
}

/* RB state machine: Enable/Disable */
void
ath_rb_set(struct ath_softc *sc, int enable)
{
    if (!sc->sc_do_rb_war)
        return;

    ath_hal_set_rifs(sc->sc_ah, enable);

    if (enable)
        sc->sc_stats.ast_11n_stats.rx_rb_on++;
    else
        sc->sc_stats.ast_11n_stats.rx_rb_off++;
}

#endif
