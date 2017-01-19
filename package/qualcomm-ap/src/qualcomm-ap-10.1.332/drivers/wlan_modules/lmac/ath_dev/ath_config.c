/*****************************************************************************/
/* \file ath_config.c
** \brief Configuration code for controlling the ATH layer.
**
**  This file contains the routines used to configure and control the ATH
**  object once instantiated.
**
** Copyright (c) 2009, Atheros Communications Inc. 
 * All Rights Reserved.
 * 
 * Copyright (c) 2011 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 * 
*/

#include "ath_internal.h"

/******************************************************************************/
/*!
**  \brief Set ATH configuration parameter
**
**  This routine will set ATH parameters to the provided values.  The incoming
**  values are always encoded as integers -- conversions are done here.
**
**  \param dev Handle for the ATH device
**  \param ID Parameter ID value, as defined in the ath_param_ID_t type
**  \param buff Buffer containing the value, currently always an integer
**  \return 0 on success
**  \return -1 for unsupported values
*/

int
ath_set_config(ath_dev_t dev, ath_param_ID_t ID, void *buff)
{
    struct ath_softc *sc = ATH_DEV_TO_SC(dev);
    int retval = 0;
    u_int32_t hal_chainmask;
#ifdef ATH_RB
    int value = *(int *)buff;
#endif

    switch(ID)
    {
        case ATH_PARAM_TXCHAINMASK:
             (void) ath_hal_gettxchainmask(sc->sc_ah, &hal_chainmask);
             if (!*(u_int32_t *)buff) {
                 sc->sc_config.txchainmask = hal_chainmask;
             } else {
                 sc->sc_config.txchainmask = *(u_int32_t *)buff & hal_chainmask;
             }
        break;

        case ATH_PARAM_RXCHAINMASK:
             (void) ath_hal_getrxchainmask(sc->sc_ah, &hal_chainmask);
             if (!*(u_int32_t *)buff) {
                 sc->sc_config.rxchainmask = hal_chainmask;
             } else {
                 sc->sc_config.rxchainmask = *(u_int32_t *)buff & hal_chainmask;
             }
        break;

        case ATH_PARAM_TXCHAINMASKLEGACY:
            sc->sc_config.txchainmasklegacy = *(int *)buff;
        break;

        case ATH_PARAM_RXCHAINMASKLEGACY:
            sc->sc_config.rxchainmasklegacy = *(int *)buff;
        break;

        case ATH_PARAM_CHAINMASK_SEL:
            sc->sc_config.chainmask_sel = *(int *)buff;
        break;

        case ATH_PARAM_AMPDU:
            sc->sc_txaggr = *(int *)buff;
        break;

        case ATH_PARAM_AMPDU_LIMIT:
            sc->sc_config.ampdu_limit = *(int *)buff;
        break;

        case ATH_PARAM_AMPDU_SUBFRAMES:
            sc->sc_config.ampdu_subframes = *(int *)buff;
        break;

        case ATH_PARAM_AMPDU_RX_BSIZE:
            sc->sc_config.ampdu_rx_bsize = *(int *)buff;
        break;

        case ATH_PARAM_AGGR_PROT:
            sc->sc_config.ath_aggr_prot = *(int *)buff;
        break;

        case ATH_PARAM_AGGR_PROT_DUR:
            sc->sc_config.ath_aggr_prot_duration = *(int *)buff;
        break;

        case ATH_PARAM_AGGR_PROT_MAX:
            sc->sc_config.ath_aggr_prot_max = *(int *)buff;
        break;

#if ATH_SUPPORT_AGGR_BURST
        case ATH_PARAM_AGGR_BURST:
            sc->sc_aggr_burst = *(int *)buff;
            if (sc->sc_aggr_burst) {
                ath_txq_burst_update(sc, HAL_WME_AC_BE, ATH_BURST_DURATION);
            } else {
                ath_txq_burst_update(sc, HAL_WME_AC_BE, 0);
            }

            sc->sc_aggr_burst_duration = ATH_BURST_DURATION;
        break;

        case ATH_PARAM_AGGR_BURST_DURATION:
            if (sc->sc_aggr_burst) {
                sc->sc_aggr_burst_duration = *(int *)buff;
                ath_txq_burst_update(sc, HAL_WME_AC_BE,
                                     sc->sc_aggr_burst_duration);
            } else {
                printk("Error: enable bursting before setting duration\n");
            }
        break;
#endif

        case ATH_PARAM_TXPOWER_LIMIT2G:
            if (*(int *)buff > ATH_TXPOWER_MAX_2G) {
                retval = -1;
            } else {
                sc->sc_config.txpowlimit2G = *(int *)buff;
            }
        break;

        case ATH_PARAM_TXPOWER_LIMIT5G:
            if (*(int *)buff > ATH_TXPOWER_MAX_5G) {
                retval = -1;
            } else {
                sc->sc_config.txpowlimit5G = *(int *)buff;
            }
        break;

        case ATH_PARAM_TXPOWER_OVERRIDE:
            sc->sc_config.txpowlimit_override = *(int *)buff;
        break;

        case ATH_PARAM_PCIE_DISABLE_ASPM_WK:
            sc->sc_config.pcieDisableAspmOnRfWake = *(int *)buff;
        break;

        case ATH_PARAM_PCID_ASPM:
            sc->sc_config.pcieAspm = *(int *)buff;
        break;

        case ATH_PARAM_BEACON_NORESET:
            sc->sc_noreset = *(int *)buff;
        break;

        case ATH_PARAM_CAB_CONFIG:
            sc->sc_config.cabqReadytime = *(int *)buff;
            ath_cabq_update(sc);
        break;

        case ATH_PARAM_ATH_DEBUG:
        {
            int cat, mask = *(int *)buff, debug_any = 0;
            for (cat = 0; cat < ATH_DEBUG_NUM_CATEGORIES; cat++) {
                int category_mask = (1 << cat) & mask;
                asf_print_mask_set(&sc->sc_print, cat, category_mask);
                debug_any = category_mask ? 1 : debug_any;
            }
            /* Update the ATH_DEBUG_ANY debug mask bit */
            asf_print_mask_set(&sc->sc_print, ATH_DEBUG_ANY, debug_any);
        }
        break;

        case ATH_PARAM_ATH_TPSCALE:
           sc->sc_config.tpscale = *(int *)buff;
           ath_update_tpscale(sc);
        break;

        case ATH_PARAM_ACKTIMEOUT:
           /* input is in usec */
           ath_hal_setacktimeout(sc->sc_ah, (*(int *)buff));
        break;

        case ATH_PARAM_RIMT_FIRST:
           /* input is in usec */
           ath_hal_setintrmitigation_timer(sc->sc_ah, HAL_INT_RX_FIRSTPKT, (*(int *)buff));
        break;

        case ATH_PARAM_RIMT_LAST:
           /* input is in usec */
           ath_hal_setintrmitigation_timer(sc->sc_ah, HAL_INT_RX_LASTPKT, (*(int *)buff));
        break;

        case ATH_PARAM_TIMT_FIRST:
           /* input is in usec */
           ath_hal_setintrmitigation_timer(sc->sc_ah, HAL_INT_TX_FIRSTPKT, (*(int *)buff));
        break;

        case ATH_PARAM_TIMT_LAST:
           /* input is in usec */
           ath_hal_setintrmitigation_timer(sc->sc_ah, HAL_INT_TX_LASTPKT, (*(int *)buff));
        break;

#ifdef ATH_RB
        case ATH_PARAM_RX_RB:
            if (!sc->sc_do_rb_war ||
                ATH_RB_MODE_DETECT == sc->sc_rxrifs ||
                !(value ^ sc->sc_rxrifs))
                return (-1);
            if (value)
                sc->sc_rxrifs = ATH_RB_MODE_FORCE;
            else
                sc->sc_rxrifs = ATH_RB_MODE_OFF;
            ath_rb_set(sc, value);
        break;

        case ATH_PARAM_RX_RB_DETECT:
            if (!sc->sc_do_rb_war ||
                ATH_RB_MODE_FORCE == sc->sc_rxrifs ||
                !(value ^ sc->sc_rxrifs))
                return (-1);
            if (value) {
                ath_rb_reset(sc);
                sc->sc_rxrifs = ATH_RB_MODE_DETECT;
            } else {
                /* Not really off if detection has already triggered.
                 * Settings will be reverted when the timeout routine runs.
                 */
                sc->sc_rxrifs = ATH_RB_MODE_OFF;
            }
        break;

        case ATH_PARAM_RX_RB_TIMEOUT:
            sc->sc_rxrifs_timeout = *(int *)buff;
        break;

        case ATH_PARAM_RX_RB_SKIPTHRESH:
            sc->sc_rxrifs_skipthresh = *(int *)buff;
        break;
#endif
        case ATH_PARAM_AMSDU_ENABLE:
            sc->sc_txamsdu = *(int *)buff;
        break;
#if ATH_SUPPORT_IQUE
		case ATH_PARAM_RETRY_DURATION:
			sc->sc_retry_duration = *(int *)buff;
		break;
		case ATH_PARAM_HBR_HIGHPER:
			sc->sc_hbr_per_high = *(int *)buff;
		break;

		case ATH_PARAM_HBR_LOWPER:
			sc->sc_hbr_per_low = *(int *)buff;
		break;
#endif

#if  ATH_SUPPORT_AOW
        case ATH_PARAM_SW_RETRY_LIMIT:
            sc->sc_aow.sw_retry_limit = *(int *)buff;
        break;
#endif  /* ATH_SUPPORT_AOW */

        case ATH_PARAM_RX_STBC:
            sc->sc_rxstbcsupport = *(int *)buff;
            sc->sc_ieee_ops->set_stbc_config(sc->sc_ieee, sc->sc_rxstbcsupport, 0);
        break;

        case ATH_PARAM_TX_STBC:
            sc->sc_txstbcsupport = *(int *)buff;
            sc->sc_ieee_ops->set_stbc_config(sc->sc_ieee, sc->sc_txstbcsupport, 1);
        break;

        case ATH_PARAM_LDPC:
            sc->sc_ldpcsupport = *(int *)buff;
            sc->sc_ieee_ops->set_ldpc_config(sc->sc_ieee, sc->sc_ldpcsupport);
        break;

        case ATH_PARAM_LIMIT_LEGACY_FRM:
            sc->sc_limit_legacy_frames = *(int *)buff;
        break;

        case ATH_PARAM_TOGGLE_IMMUNITY:
            sc->sc_toggle_immunity = *(int *)buff;
        break;

        case ATH_PARAM_GPIO_LED_CUSTOM:
	        sc->sc_reg_parm.gpioLedCustom = *(int *)buff;
	        ath_led_reinit(sc);
	    break;

	    case ATH_PARAM_SWAP_DEFAULT_LED:
	        sc->sc_reg_parm.swapDefaultLED =  *(int *)buff;
	        ath_led_reinit(sc);
	    break;

#if ATH_SUPPORT_WIRESHARK
        case ATH_PARAM_TAPMONITOR:
            sc->sc_tapmonenable = *(int *)buff;
        break;
#endif
#ifdef VOW_TIDSCHED
        case ATH_PARAM_TIDSCHED:
            sc->tidsched = *(int *)buff;
        break;
        case ATH_PARAM_TIDSCHED_VOQW:
            sc->acqw[3] = *(int *)buff;
        break;
        case ATH_PARAM_TIDSCHED_VIQW:
            sc->acqw[2] = *(int *)buff;
        break;
        case ATH_PARAM_TIDSCHED_BKQW:
            sc->acqw[1] = *(int *)buff;
        break;
        case ATH_PARAM_TIDSCHED_BEQW:
            sc->acqw[0] = *(int *)buff;
        break;
        case ATH_PARAM_TIDSCHED_VOTO:
            sc->acto[3] = *(int *)buff;
        break;
        case ATH_PARAM_TIDSCHED_VITO:
            sc->acto[2] = *(int *)buff;
        break;
        case ATH_PARAM_TIDSCHED_BKTO:
            sc->acto[1] = *(int *)buff;
        break;
        case ATH_PARAM_TIDSCHED_BETO:
            sc->acto[0] = *(int *)buff;
        break;
#endif
#ifdef VOW_LOGLATENCY
        case ATH_PARAM_LOGLATENCY:
            sc->loglatency = *(int *)buff;
        break;
#endif
#if ATH_TX_DUTY_CYCLE
        case ATH_PARAM_TX_DUTY_CYCLE:
            if(*(int *)buff < ATH_TX_DUTY_CYCLE_MIN) {
                DPRINTF(sc, ATH_DEBUG_UNMASKABLE , "Setting tx_duty_cycle should be (tx_duty_cycle >= %d)\n", ATH_TX_DUTY_CYCLE_MIN);
            	retval = -1;
            }
            else{
                sc->sc_tx_dc_active_pct = *(int *)buff;
                if(sc->sc_tx_dc_active_pct >= ATH_TX_DUTY_CYCLE_MAX){
                    sc->sc_tx_dc_active_pct = 0;
                    ath_set_tx_duty_cycle(sc, sc->sc_tx_dc_active_pct, false);
                }else 
                    ath_set_tx_duty_cycle(sc, sc->sc_tx_dc_active_pct, true);
            }
        break;
 #endif
#if ATH_SUPPORT_VOW_DCS
        case ATH_PARAM_DCS_COCH:
            sc->sc_coch_intr_thresh = *(int *)buff;
        break;
        case ATH_PARAM_DCS_TXERR:
            sc->sc_tx_err_thresh = *(int *)buff;
        break;
        case ATH_PARAM_DCS_PHYERR:
            sc->sc_phyerr_penalty = *(int *)buff;
        break;
#endif
#if ATH_SUPPORT_VOWEXT
        case ATH_PARAM_VOWEXT:
            printk("Setting new VOWEXT parameters buffers with %d\n",ATH_TXDESC);
            sc->sc_vowext_flags = (*(u_int32_t*)buff) & ATH_CAP_VOWEXT_MASK; 
            printk("VOW enabled features: \n");
            printk("\tfair_queuing: %d\n\t"\
                   "retry_delay_red %d,\n\tbuffer reordering %d\n\t"\
			       "enhanced_rate_control_and_aggr %d\n",
                   (ATH_IS_VOWEXT_FAIRQUEUE_ENABLED(sc) ? 1 : 0),
                   (ATH_IS_VOWEXT_AGGRSIZE_ENABLED(sc) ? 1 : 0),
                   (ATH_IS_VOWEXT_BUFFREORDER_ENABLED(sc) ? 1 : 0),
			       (ATH_IS_VOWEXT_RCA_ENABLED(sc) ? 1 : 0));
        break;

        case ATH_PARAM_VSP_ENABLE:
            /* VSP feature uses ATH_TX_BUF_FLOW_CNTL feature so enable this feature only when ATH_TX_BUF_FLOW_CNTL is defined*/
            #if ATH_TX_BUF_FLOW_CNTL
	        sc->vsp_enable = *(int *)buff;
            #else
            if(*(int *)buff)
            {
                printk ("ATH_TX_BUF_FLOW_CNTL is not compiled in so can't enable VSP\n");
            }
            #endif
        break;

        case ATH_PARAM_VSP_THRESHOLD:
	        sc->vsp_threshold = (*(int *)buff)*1024*100; /* Convert from mbps to goodput (kbps*100) */
        break;

        case ATH_PARAM_VSP_EVALINTERVAL:
	        sc->vsp_evalinterval = *(int *)buff;
        break;

        case ATH_PARAM_VSP_STATSCLR:
            if(*(int *)buff)
            {
                /* clear out VSP Statistics */
                sc->vsp_bependrop    = 0;
                sc->vsp_bkpendrop    = 0;
                sc->vsp_vipendrop    = 0;
                sc->vsp_vicontention = 0;
            }
        break;
#endif
#if ATH_VOW_EXT_STATS
        case ATH_PARAM_VOWEXT_STATS:
            sc->sc_vowext_stats = ((*(u_int32_t*)buff)) ? 1: 0;
        break;
#endif
#ifdef ATH_SUPPORT_TxBF
        case ATH_PARAM_TXBF_SW_TIMER:
            sc->sc_reg_parm.TxBFSwCvTimeout = *(u_int32_t *) buff;
        break;
#endif
#if ATH_SUPPORT_VOWEXT
        case ATH_PARAM_PHYRESTART_WAR:
        {
            int oval=0;
            oval = sc->sc_phyrestart_disable;
            sc->sc_phyrestart_disable = *(int*) buff;
            /* In case if command issued to disable then disable it
             * immediate. Do not try touching the register multiple times.
             */
            if (oval && ((sc->sc_hang_war & HAL_PHYRESTART_CLR_WAR) &&
                (0 == sc->sc_phyrestart_disable))) 
            {
                ath_hal_phyrestart_clr_war_enable(sc->sc_ah, 0);
            }
            else if ((sc->sc_hang_war & HAL_PHYRESTART_CLR_WAR) &&
                (sc->sc_phyrestart_disable) )
            {
                ath_hal_phyrestart_clr_war_enable(sc->sc_ah, 1);
        }
        }
        break;

        case ATH_PARAM_KEYSEARCH_ALWAYS_WAR:
        {
            sc->sc_keysearch_always_enable = *(int*) buff;
            ath_hal_keysearch_always_war_enable(sc->sc_ah,
                    sc->sc_keysearch_always_enable );
        }
        break;
#endif
#if ATH_SUPPORT_DYN_TX_CHAINMASK
        case ATH_PARAM_DYN_TX_CHAINMASK:
            sc->sc_dyn_txchainmask = *(int *)buff;
        break;
#endif /* ATH_SUPPORT_DYN_TX_CHAINMASK */
#if ATH_SUPPORT_FLOWMAC_MODULE
        case ATH_PARAM_FLOWMAC:
            sc->sc_osnetif_flowcntrl = *(int *)buff;
            sc->sc_ieee_ops->notify_flowmac_state(sc->sc_ieee, sc->sc_osnetif_flowcntrl);
            break;
#endif

#if UMAC_SUPPORT_SMARTANTENNA
        case ATH_PARAM_SMARTANTENNA:
            /* Enable smart antenna if hardware supports and software also enables it */
            if (0 == *(int *)buff) {
                /* disable smart antenna in hardware also */
                ath_disable_smartantenna(sc);
                DPRINTF(sc, ATH_DEBUG_UNMASKABLE , "Smart Antenna Disabled \n");
            } else {
                if(HAL_OK == ath_hal_getcapability(sc->sc_ah, HAL_CAP_SMARTANTENNA, 0, 0)) {
                    /* Do hardware initialization now */
                    ath_enable_smartantenna(sc, *(int *)buff);
                    DPRINTF(sc, ATH_DEBUG_UNMASKABLE, "Smart Antenna Enabled \n");
                } else {
                    DPRINTF(sc, ATH_DEBUG_UNMASKABLE , "Hardware doest not support smartantenna Mode\n");
                    sc->sc_smartant_enable = 0;
                }
            }
            break; 
        case ATH_PARAM_SMARTANT_TRAIN_MODE:
        case ATH_PARAM_SMARTANT_TRAIN_TYPE:
        case ATH_PARAM_SMARTANT_PKT_LEN:
        case ATH_PARAM_SMARTANT_NUM_PKTS:
        case ATH_PARAM_SMARTANT_TRAIN_START:
        case ATH_PARAM_SMARTANT_NUM_ITR:
        case ATH_PARAM_SMARTANT_RETRAIN_THRESHOLD:
        case ATH_PARAM_SMARTANT_RETRAIN_INTERVAL:
        case ATH_PARAM_SMARTANT_RETRAIN_DROP:
        case ATH_PARAM_SMARTANT_MIN_GOODPUT_THRESHOLD:
        case ATH_PARAM_SMARTANT_GOODPUT_AVG_INTERVAL:
        case ATH_PARAM_SMARTANT_TRAFFIC_GEN_TIMER:
        case ATH_PARAM_SMARTANT_RETRAIN:
        case ATH_PARAM_SMARTANT_CURRENT_ANTENNA:
        case ATH_PARAM_SMARTANT_DEFAULT_ANTENNA:
            sc->sc_ieee_ops->smartantenna_setparam(sc->sc_ieee, ID,*(u_int32_t *)buff );
            break;
#endif

        case ATH_PARAM_BCN_BURST:
            if (*(int *)buff)
                sc->sc_reg_parm.burst_beacons = 1;
            else
                sc->sc_reg_parm.burst_beacons = 0;
            break; 
#if ATH_ANI_NOISE_SPUR_OPT
        case ATH_PARAM_NOISE_SPUR_OPT:
            sc->sc_config.ath_noise_spur_opt = *(int *)buff;
            break;
#endif /* ATH_ANI_NOISE_SPUR_OPT */
        case ATH_PARAM_DCS_ENABLE:
            sc->sc_dcs_enabled = (*((int *)buff));
            sc->sc_dcs_enabled  &= ATH_CAP_DCS_MASK;
            /* disable dcs-wlan-im for 11G mode */ 
            if ( (sc->sc_dcs_enabled & ATH_CAP_DCS_WLANIM) && (!((sc->sc_curmode == WIRELESS_MODE_11NA_HT20) 
                        ||(sc->sc_curmode == WIRELESS_MODE_11NA_HT40PLUS)
                        ||(sc->sc_curmode == WIRELESS_MODE_11NA_HT40MINUS))) ){
                printk("Disabling DCS for 11G mode");
                sc->sc_dcs_enabled &= (~ATH_CAP_DCS_WLANIM);
            }
             printk("\tDCS for CW interference mitigation:   %d\n\t"\
                   "DCS for WLAN interference mitigation: %d\n",
                   ((sc->sc_dcs_enabled & ATH_CAP_DCS_CWIM) ? 1 : 0),
                   ((sc->sc_dcs_enabled & ATH_CAP_DCS_WLANIM) ? 1 : 0));

            break;
#if UMAC_SUPPORT_PERIODIC_PERFSTATS
        case ATH_PARAM_PRDPERFSTAT_THRPUT_ENAB:
            (*(int *)buff) ? \
                sc->sc_ieee_ops->set_prdperfstat_thrput_enab(sc->sc_ieee, 1):
                sc->sc_ieee_ops->set_prdperfstat_thrput_enab(sc->sc_ieee, 0);
            break;
        case ATH_PARAM_PRDPERFSTAT_THRPUT_WIN:
            sc->sc_ieee_ops->set_prdperfstat_thrput_win(sc->sc_ieee,
                                                        (*(int *)buff));
            break;
        case ATH_PARAM_PRDPERFSTAT_PER_ENAB:
            (*(int *)buff) ? \
                sc->sc_ieee_ops->set_prdperfstat_per_enab(sc->sc_ieee, 1):   \
                sc->sc_ieee_ops->set_prdperfstat_per_enab(sc->sc_ieee, 0);
            break;
        case ATH_PARAM_PRDPERFSTAT_PER_WIN:
            sc->sc_ieee_ops->set_prdperfstat_per_win(sc->sc_ieee,
                                                     (*(int *)buff));
            break;
#endif /* UMAC_SUPPORT_PERIODIC_PERFSTATS */
#if ATH_SUPPORT_RX_PROC_QUOTA
        case ATH_PARAM_CNTRX_NUM:
            if (*(int *)buff)
                sc->sc_process_rx_num = (*((int *)buff));
            /* converting to even number as we 
               devide this by two for edma case */
            if((sc->sc_process_rx_num % 2)
                    && sc->sc_enhanceddmasupport) {
                sc->sc_process_rx_num++;
                printk("Adjusting to Nearest Even Value %d \n",sc->sc_process_rx_num);
            }
            break;
#endif            
#if ATH_RX_LOOPLIMIT_TIMER
        case ATH_PARAM_LOOPLIMIT_NUM:
            if (*(int *)buff)
                sc->sc_rx_looplimit_timeout = (*((int *)buff));
            break;
#endif
        case ATH_PARAM_RTS_CTS_RATE:
            if ((*(int *)buff) < 4)
                sc->sc_protrix=*(int *)buff;
            else
                printk ("ATH_PARAM_RTS_CTS_RATE not support\n");
            break;  
        case ATH_PARAM_NODEBUG:
            sc->sc_nodebug = *(int *)buff;
            printk("sc nodebug %d \n",sc->sc_nodebug);
            break;
        default:
            return (-1);
    }
    return (retval);
}

/******************************************************************************/
/*!
**  \brief Get ATH configuration parameter
**
**  This returns the current value of the indicated parameter.  Conversion
**  to integer done here.
**
**  \param dev Handle for the ATH device
**  \param ID Parameter ID value, as defined in the ath_param_ID_t type
**  \param buff Buffer to place the integer value into.
**  \return 0 on success
**  \return -1 for unsupported values
*/

int
ath_get_config(ath_dev_t dev, ath_param_ID_t ID, void *buff)
{
    struct ath_softc *sc = ATH_DEV_TO_SC(dev);

    switch(ID)
    {
        case ATH_PARAM_TXCHAINMASK:
            *(int *)buff = sc->sc_config.txchainmask;
        break;

        case ATH_PARAM_GET_IF_ID:
            *(int *)buff = IF_ID_DIRECT_ATTACH;
        break;

        case ATH_PARAM_RXCHAINMASK:
            *(int *)buff = sc->sc_config.rxchainmask;
        break;

        case ATH_PARAM_TXCHAINMASKLEGACY:
            *(int *)buff = sc->sc_config.txchainmasklegacy;
        break;

        case ATH_PARAM_RXCHAINMASKLEGACY:
            *(int *)buff = sc->sc_config.rxchainmasklegacy;
        break;
        case ATH_PARAM_CHAINMASK_SEL:
            *(int *)buff = sc->sc_config.chainmask_sel;
        break;

        case ATH_PARAM_AMPDU:
            *(int *)buff = sc->sc_txaggr;
        break;

        case ATH_PARAM_AMPDU_LIMIT:
            *(int *)buff = sc->sc_config.ampdu_limit;
        break;

        case ATH_PARAM_AMPDU_SUBFRAMES:
            *(int *)buff = sc->sc_config.ampdu_subframes;
        break;

        case ATH_PARAM_AMPDU_RX_BSIZE:
            *(int *)buff = sc->sc_config.ampdu_rx_bsize;
        break;

        case ATH_PARAM_AGGR_PROT:
            *(int *)buff = sc->sc_config.ath_aggr_prot;
        break;

        case ATH_PARAM_AGGR_PROT_DUR:
            *(int *)buff = sc->sc_config.ath_aggr_prot_duration;
        break;

        case ATH_PARAM_AGGR_PROT_MAX:
            *(int *)buff = sc->sc_config.ath_aggr_prot_max;
        break;

        case ATH_PARAM_TXPOWER_LIMIT2G:
            *(int *)buff = sc->sc_config.txpowlimit2G;
        break;

        case ATH_PARAM_TXPOWER_LIMIT5G:
            *(int *)buff = sc->sc_config.txpowlimit5G;
        break;

        case ATH_PARAM_TXPOWER_OVERRIDE:
            *(int *)buff = sc->sc_config.txpowlimit_override;
        break;

        case ATH_PARAM_PCIE_DISABLE_ASPM_WK:
            *(int *)buff = sc->sc_config.pcieDisableAspmOnRfWake;
        break;

        case ATH_PARAM_PCID_ASPM:
            *(int *)buff = sc->sc_config.pcieAspm;
        break;

        case ATH_PARAM_BEACON_NORESET:
            *(int *)buff = sc->sc_noreset;
        break;

        case ATH_PARAM_CAB_CONFIG:
            *(int *)buff = sc->sc_config.cabqReadytime;
        break;

        case ATH_PARAM_ATH_DEBUG:
        {
            /*
             * asf_print: the asf print mask is (probably) 64 bits long, but 
             * we are expected to return 32 bits. For now, just return the 32
             * LSBs from the print mask. Currently, all used ATH print 
             * categories fit within 32 bits anyway.
             */
            u_int8_t *mask_ptr = &sc->sc_print.category_mask[0];
            *(int *)buff = 
                mask_ptr[0] | 
                (mask_ptr[1] << 8) | 
                (mask_ptr[2] << 16) | 
                (mask_ptr[3] << 24);
        }
        break;

        case ATH_PARAM_ATH_TPSCALE:
           *(int *) buff = sc->sc_config.tpscale;
        break;

        case ATH_PARAM_ACKTIMEOUT:
           *(int *) buff = ath_hal_getacktimeout(sc->sc_ah) /* usec */;
        break;

        case ATH_PARAM_RIMT_FIRST:
           *(int *)buff = ath_hal_getintrmitigation_timer(sc->sc_ah, HAL_INT_RX_FIRSTPKT);
        break;
        
        case ATH_PARAM_RIMT_LAST:
           *(int *)buff = ath_hal_getintrmitigation_timer(sc->sc_ah, HAL_INT_RX_LASTPKT);
        break;
        
        case ATH_PARAM_TIMT_FIRST:
           *(int *)buff = ath_hal_getintrmitigation_timer(sc->sc_ah, HAL_INT_TX_FIRSTPKT);
        break;
        
        case ATH_PARAM_TIMT_LAST:
           *(int *)buff = ath_hal_getintrmitigation_timer(sc->sc_ah, HAL_INT_TX_LASTPKT);
        break;

#ifdef ATH_RB
        case ATH_PARAM_RX_RB:
            *(int *)buff = (sc->sc_rxrifs & ATH_RB_MODE_FORCE) ? 1 : 0;
        break;

        case ATH_PARAM_RX_RB_DETECT:
            *(int *)buff = (sc->sc_rxrifs & ATH_RB_MODE_DETECT) ? 1 : 0;
        break;

        case ATH_PARAM_RX_RB_TIMEOUT:
            *(int *)buff = sc->sc_rxrifs_timeout;
        break;

        case ATH_PARAM_RX_RB_SKIPTHRESH:
            *(int *)buff = sc->sc_rxrifs_skipthresh;
        break;
#endif
        case ATH_PARAM_AMSDU_ENABLE:
            *(int *)buff = sc->sc_txamsdu;
        break;
#if ATH_SUPPORT_IQUE
		case ATH_PARAM_RETRY_DURATION:
			*(int *)buff = sc->sc_retry_duration;
		break;
		case ATH_PARAM_HBR_HIGHPER:
			*(int *)buff = sc->sc_hbr_per_high;
		break;

		case ATH_PARAM_HBR_LOWPER:
			*(int *)buff = sc->sc_hbr_per_low;
		break;
#endif

#if  ATH_SUPPORT_AOW
        case ATH_PARAM_SW_RETRY_LIMIT:
            *(int *)buff = sc->sc_aow.sw_retry_limit;
        break;
#endif  /* ATH_SUPPORT_AOW */
        case ATH_PARAM_RX_STBC:
            *(int *)buff = sc->sc_rxstbcsupport;
        break;

        case ATH_PARAM_TX_STBC:
            *(int *)buff = sc->sc_txstbcsupport;
        break;

        case ATH_PARAM_LDPC:
            *(int *)buff = sc->sc_ldpcsupport;
        break;

        case ATH_PARAM_LIMIT_LEGACY_FRM:
            *(int *)buff = sc->sc_limit_legacy_frames;
        break;

        case ATH_PARAM_TOGGLE_IMMUNITY:
            *(int *)buff = sc->sc_toggle_immunity;
        break;

        case ATH_PARAM_WEP_TKIP_AGGR_TX_DELIM:
            (void)ath_hal_gettxdelimweptkipaggr(sc->sc_ah, (u_int32_t *)buff);
            break;

        case ATH_PARAM_WEP_TKIP_AGGR_RX_DELIM:
            (void)ath_hal_getrxdelimweptkipaggr(sc->sc_ah, (u_int32_t *)buff);
            break;

	case ATH_PARAM_GPIO_LED_CUSTOM:
            *(int *)buff = sc->sc_reg_parm.gpioLedCustom;
            break;

        case ATH_PARAM_SWAP_DEFAULT_LED:
            *(int *)buff = sc->sc_reg_parm.swapDefaultLED;
            break;
#ifdef VOW_TIDSCHED
        case ATH_PARAM_TIDSCHED:
            *(int *)buff = sc->tidsched;
        break;
        case ATH_PARAM_TIDSCHED_VOQW:
            *(int *)buff = sc->acqw[3];
        break;
        case ATH_PARAM_TIDSCHED_VIQW:
            *(int *)buff = sc->acqw[2];
        break;
        case ATH_PARAM_TIDSCHED_BKQW:
            *(int *)buff = sc->acqw[1];
        break;
        case ATH_PARAM_TIDSCHED_BEQW:
            *(int *)buff = sc->acqw[0];
        break;
        case ATH_PARAM_TIDSCHED_VOTO:
            *(int *)buff = sc->acto[3];
        break;
        case ATH_PARAM_TIDSCHED_VITO:
            *(int *)buff = sc->acto[2];
        break;
        case ATH_PARAM_TIDSCHED_BKTO:
            *(int *)buff = sc->acto[1];
        break;
        case ATH_PARAM_TIDSCHED_BETO:
            *(int *)buff = sc->acto[0];
        break;
#endif
#ifdef VOW_LOGLATENCY
        case ATH_PARAM_LOGLATENCY:
            *(int *)buff = sc->loglatency;
        break;
#endif
#if ATH_TX_DUTY_CYCLE
        case ATH_PARAM_TX_DUTY_CYCLE:
            *(int *)buff = sc->sc_tx_dc_enable? sc->sc_tx_dc_active_pct : ATH_TX_DUTY_CYCLE_MAX ;
        break;
#endif
#if ATH_SUPPORT_VOW_DCS
        case ATH_PARAM_DCS_COCH:
            *(int *)buff = sc->sc_coch_intr_thresh;
        break;
        case ATH_PARAM_DCS_TXERR:
            *(int *)buff = sc->sc_tx_err_thresh;
        break;
        case ATH_PARAM_DCS_PHYERR:
            *(int *)buff = sc->sc_phyerr_penalty;
        break;
#endif
#if ATH_SUPPORT_VOWEXT
        case ATH_PARAM_VOWEXT:
            *(int *)buff = sc->sc_vowext_flags;
        break;

        case ATH_PARAM_VSP_ENABLE:
	         *(int *)buff = sc->vsp_enable;
        break;

        case ATH_PARAM_VSP_THRESHOLD:
	        *(int *)buff = (sc->vsp_threshold)/(100*1024);  /* Convert from goodput fromat (kbps*100) to mbps format */
        break;

        case ATH_PARAM_VSP_EVALINTERVAL:
	        *(int *)buff = sc->vsp_evalinterval;
        break;

        case ATH_PARAM_VSP_STATS:
            printk("VSP Statistics:\n\t"\
                    "vi contention: %d\n\tbe(penalized) drop : %d\n\t"\
                    "bk(penalized) drop : %d\n\tvi(penalized) drop : %d\n",\
                    sc->vsp_vicontention, sc->vsp_bependrop, 
                    sc->vsp_bkpendrop, sc->vsp_vipendrop);
        break;
#endif
#if ATH_VOW_EXT_STATS
        case ATH_PARAM_VOWEXT_STATS:
            *(int*)buff = sc->sc_vowext_stats;
            break;
#endif
//        case ATH_PARAM_USE_EAP_LOWEST_RATE:
//            *(int *)buff = sc->sc_eap_lowest_rate;

#if ATH_SUPPORT_WIRESHARK
        case ATH_PARAM_TAPMONITOR:
            *(int *)buff = sc->sc_tapmonenable;
        break;
#endif
#ifdef ATH_SUPPORT_TxBF
        case ATH_PARAM_TXBF_SW_TIMER:
            *(int *)buff = sc->sc_reg_parm.TxBFSwCvTimeout  ;
        break;            
#endif
#if ATH_SUPPORT_VOWEXT
        case ATH_PARAM_PHYRESTART_WAR:
            *(int*) buff  = sc->sc_phyrestart_disable;
            break;
        case ATH_PARAM_KEYSEARCH_ALWAYS_WAR:
            *(int*) buff  = sc->sc_keysearch_always_enable;
            break;
#endif
        case ATH_PARAM_CHANNEL_SWITCHING_TIME_USEC:
            (void)ath_hal_getchannelswitchingtime(sc->sc_ah, (u_int32_t *)buff);
            break;
#if ATH_SUPPORT_DYN_TX_CHAINMASK            
        case ATH_PARAM_DYN_TX_CHAINMASK:
            *(int *)buff = sc->sc_dyn_txchainmask;
            break;            
#endif /* ATH_SUPPORT_DYN_TX_CHAINMASK */
#if ATH_SUPPORT_FLOWMAC_MODULE
        case ATH_PARAM_FLOWMAC:
            *(int *)buff = sc->sc_osnetif_flowcntrl;
            break;
#endif
#if ATH_SUPPORT_AGGR_BURST
        case ATH_PARAM_AGGR_BURST:
            *(int *)buff = sc->sc_aggr_burst;
            break;
        case ATH_PARAM_AGGR_BURST_DURATION:
            *(int *)buff = sc->sc_aggr_burst_duration;
        break;
#endif

#if UMAC_SUPPORT_SMARTANTENNA
        case ATH_PARAM_SMARTANTENNA:
            *(int*)buff = sc->sc_smartant_enable;
            break;

        case ATH_PARAM_SMARTANT_TRAIN_MODE:
        case ATH_PARAM_SMARTANT_TRAIN_TYPE:
        case ATH_PARAM_SMARTANT_PKT_LEN:
        case ATH_PARAM_SMARTANT_NUM_PKTS:
        case ATH_PARAM_SMARTANT_TRAIN_START:
        case ATH_PARAM_SMARTANT_NUM_ITR:
        case ATH_PARAM_SMARTANT_RETRAIN_THRESHOLD:
        case ATH_PARAM_SMARTANT_RETRAIN_INTERVAL:
        case ATH_PARAM_SMARTANT_RETRAIN_DROP:
        case ATH_PARAM_SMARTANT_MIN_GOODPUT_THRESHOLD:
        case ATH_PARAM_SMARTANT_GOODPUT_AVG_INTERVAL:
        case ATH_PARAM_SMARTANT_TRAFFIC_GEN_TIMER:
        case ATH_PARAM_SMARTANT_RETRAIN:
        case ATH_PARAM_SMARTANT_CURRENT_ANTENNA:
        case ATH_PARAM_SMARTANT_DEFAULT_ANTENNA:
            *(int*)buff  = sc->sc_ieee_ops->smartantenna_getparam(sc->sc_ieee, ID);
            break;	
#endif

        case ATH_PARAM_BCN_BURST:
            *(int *)buff = sc->sc_reg_parm.burst_beacons;
            break;
        case ATH_PARAM_DCS_ENABLE:
            *(int *)buff = sc->sc_dcs_enabled;
        break;
        
#if UMAC_SUPPORT_PERIODIC_PERFSTATS
        case ATH_PARAM_PRDPERFSTAT_THRPUT_ENAB:
            *(int *)buff =
                sc->sc_ieee_ops->get_prdperfstat_thrput_enab(sc->sc_ieee);
            break;
        case ATH_PARAM_PRDPERFSTAT_THRPUT_WIN:
            *(int *)buff =
                sc->sc_ieee_ops->get_prdperfstat_thrput_win(sc->sc_ieee);
            break;
        case ATH_PARAM_PRDPERFSTAT_THRPUT:
            *(int *)buff =
                sc->sc_ieee_ops->get_prdperfstat_thrput(sc->sc_ieee);
            break;
        case ATH_PARAM_PRDPERFSTAT_PER_ENAB:
            *(int *)buff =
                sc->sc_ieee_ops->get_prdperfstat_per_enab(sc->sc_ieee);
            break;
        case ATH_PARAM_PRDPERFSTAT_PER_WIN:
            *(int *)buff =
                sc->sc_ieee_ops->get_prdperfstat_per_win(sc->sc_ieee);
            break;
        case ATH_PARAM_PRDPERFSTAT_PER:
            *(int *)buff =
                sc->sc_ieee_ops->get_prdperfstat_per(sc->sc_ieee);
            break;
#endif /* UMAC_SUPPORT_PERIODIC_PERFSTATS */
        case ATH_PARAM_TOTAL_PER:
            *(int *)buff =
                sc->sc_ieee_ops->get_total_per(sc->sc_ieee);
            break;
#if ATH_SUPPORT_RX_PROC_QUOTA
        case ATH_PARAM_CNTRX_NUM:
            *(int *)buff = sc->sc_process_rx_num;
            break;
#endif
#if ATH_RX_LOOPLIMIT_TIMER
        case ATH_PARAM_LOOPLIMIT_NUM:
            *(int *)buff = sc->sc_rx_looplimit_timeout;
            break;
#endif
        case ATH_PARAM_RTS_CTS_RATE:
            *(int *)buff = sc->sc_protrix;
            break;  
        default:
            return (-1);
    }

    return (0);
}
