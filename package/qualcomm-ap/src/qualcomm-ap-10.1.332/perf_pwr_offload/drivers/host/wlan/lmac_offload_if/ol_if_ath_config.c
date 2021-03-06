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
 * Radio interface configuration routines for perf_pwr_offload 
 */
#include <osdep.h>
#include "ol_if_athvar.h"
#include "ol_if_athpriv.h"
#define IF_ID_OFFLOAD (1)
#if ATH_PERF_PWR_OFFLOAD


/* The below mapping is according the NEC doc, which is as follows,

    DSCP        TID     AC
    000000      0       WME_AC_BE
    001000      1       WME_AC_BK
    010000      2       WME_AC_BK
    011000      0       WME_AC_BE
    100000      6       WME_AC_VO
    101000      5       WME_AC_VI
    110000      6       WME_AC_VO
    111000      6       WME_AC_VO
    101110      6       WME_AC_VO
    others      0       WME_AC_BE

*/
static int
ol_ath_set_dscp_tid_map(struct ieee80211com *ic)
{

    A_UINT32 dscp_tid_map[WMI_DSCP_MAP_MAX] = {
                                            0, 0, 0, 0, 0, 0, 0, 0,
                                            1, 0, 0, 0, 0, 0, 0, 0,
                                            2, 0, 0, 0, 0, 0, 0, 0,
                                            0, 0, 0, 0, 0, 0, 0, 0,
                                            6, 0, 0, 0, 0, 0, 0, 0,
                                            5, 0, 0, 0, 0, 0, 6, 0,
                                            6, 0, 0, 0, 0, 0, 0, 0,
                                            6, 0, 0, 0, 0, 0, 0, 0,
                                          };
    struct ol_ath_softc_net80211 *scn = OL_ATH_SOFTC_NET80211(ic);
    wmi_buf_t buf;
    wmi_pdev_set_dscp_tid_map_cmd *cmd;
    int len = sizeof(wmi_pdev_set_dscp_tid_map_cmd);

    buf = wmi_buf_alloc(scn->wmi_handle, len);
    if (!buf) {
        printk("%s:wmi_buf_alloc failed\n", __FUNCTION__);
        return -1;
    }
    cmd = (wmi_pdev_set_dscp_tid_map_cmd *)wmi_buf_data(buf);
    OS_MEMCPY(cmd->dscp_to_tid_map, dscp_tid_map, sizeof(A_UINT32) * WMI_DSCP_MAP_MAX);

    return wmi_unified_cmd_send(scn->wmi_handle, buf, len, WMI_PDEV_SET_DSCP_TID_MAP_CMDID);

}



int
ol_ath_set_config_param(struct ol_ath_softc_net80211 *scn, ol_ath_param_t param, void *buff)
{
    int retval = 0;
    u_int32_t value = *(u_int32_t *)buff, param_id;
    struct ieee80211com *ic = &scn->sc_ic;

    switch(param)
    {
        case OL_ATH_PARAM_TXCHAINMASK:
        {
            u_int8_t cur_mask = ieee80211com_get_tx_chainmask(ic);
            if (!value) {
                /* value is 0 - set the chainmask to be the default 
                 * supported tx_chain_mask value 
                 */
                retval = wmi_unified_pdev_set_param(scn->wmi_handle,
                             WMI_PDEV_PARAM_TX_CHAIN_MASK, 
                             scn->wlan_resource_config.tx_chain_mask);
                if (retval == EOK) {
                    /* Update the ic_chainmask */
                    ieee80211com_set_tx_chainmask(ic, 
                        (u_int8_t) (scn->wlan_resource_config.tx_chain_mask));
                }
            }
            else if (cur_mask != value) {
                /* Update chainmask only if the current chainmask is different */
                if (value > scn->wlan_resource_config.tx_chain_mask) {
                    printk("ERROR - value is greater than supported chainmask 0x%x \n",
                            scn->wlan_resource_config.tx_chain_mask);
                    return -1;
                }
                retval = wmi_unified_pdev_set_param(scn->wmi_handle,
                             WMI_PDEV_PARAM_TX_CHAIN_MASK, value);
                if (retval == EOK) {
                    /* Update the ic_chainmask */
                    ieee80211com_set_tx_chainmask(ic, (u_int8_t) (value));
                }
                /* FIXME - Currently the assumption is no active vaps 
                 * when executing this command 
                 * Need to restart all the active VAPs
                 */
            }
        }
        break;
		case OL_ATH_PARAM_DSCP_TID_MAP:
			ol_ath_set_dscp_tid_map(ic);
		break;	
        case OL_ATH_PARAM_RXCHAINMASK:
        {
            u_int8_t cur_mask = ieee80211com_get_rx_chainmask(ic);
            if (!value) {
                /* value is 0 - set the chainmask to be the default 
                 * supported rx_chain_mask value 
                 */
                retval = wmi_unified_pdev_set_param(scn->wmi_handle,
                             WMI_PDEV_PARAM_RX_CHAIN_MASK, 
                             scn->wlan_resource_config.rx_chain_mask);
                if (retval == EOK) {
                    /* Update the ic_chainmask */
                    ieee80211com_set_rx_chainmask(ic, 
                        (u_int8_t) (scn->wlan_resource_config.rx_chain_mask));
                }
            }
            else if (cur_mask != value) {
                /* Update chainmask only if the current chainmask is different */
                if (value > scn->wlan_resource_config.rx_chain_mask) {
                    printk("ERROR - value is greater than supported chainmask 0x%x \n",
                            scn->wlan_resource_config.rx_chain_mask);
                    return -1;
                }
                retval = wmi_unified_pdev_set_param(scn->wmi_handle,
                             WMI_PDEV_PARAM_RX_CHAIN_MASK, value);
                if (retval == EOK) {
                    /* Update the ic_chainmask */
                    ieee80211com_set_rx_chainmask(ic, (u_int8_t) (value));
                }
                /* FIXME - Currently the assumption is no active vaps 
                 * when executing this command 
                 * Need to restart all the active VAPs
                 */
            }
        }
        break;
        
        case OL_ATH_PARAM_TXPOWER_LIMIT2G:
        {
            if (!value) {
                value = scn->max_tx_power;
            }
            ic->ic_set_txPowerLimit(ic, value, value, 1);
        }        
        break;
        
        case OL_ATH_PARAM_TXPOWER_LIMIT5G:
        {
            if (!value) {
                value = scn->max_tx_power;
            }
            ic->ic_set_txPowerLimit(ic, value, value, 0);
        }                
        break;    
        
        case OL_ATH_PARAM_TXPOWER_SCALE:
        {
            if((WMI_TP_SCALE_MAX <= value) && (value <= WMI_TP_SCALE_MIN))
            {
                scn->txpower_scale = value;
                return wmi_unified_pdev_set_param(scn->wmi_handle,
                    WMI_PDEV_PARAM_TXPOWER_SCALE, value);
            } else {
                retval = -EINVAL;
            }
        }
        break;

        case OL_ATH_PARAM_NON_AGG_SW_RETRY_TH:
        {
                return wmi_unified_pdev_set_param(scn->wmi_handle,
                             WMI_PDEV_PARAM_NON_AGG_SW_RETRY_TH, value);
        }
        break; 
        case OL_ATH_PARAM_AGG_SW_RETRY_TH:
        {
                return wmi_unified_pdev_set_param(scn->wmi_handle,
                             WMI_PDEV_PARAM_AGG_SW_RETRY_TH, value);
        }
        break; 
        case OL_ATH_PARAM_STA_KICKOUT_TH:
        {
                return wmi_unified_pdev_set_param(scn->wmi_handle,
                             WMI_PDEV_PARAM_STA_KICKOUT_TH, value);
        }
        break; 
        case OL_ATH_PARAM_WLAN_PROF_ENABLE:
        {
                return wmi_unified_wlan_profile_enable(scn->wmi_handle,
                             WMI_WLAN_PROFILE_TRIGGER_CMDID, value);
        }
        break; 
        case OL_ATH_PARAM_BCN_BURST:
        {
                /* value is set to either 1 (bursted) or 0 (staggered).
                 * if value passed is non-zero, convert it to 1 with
                 * double negation 
                 */
                value = !!value;
                if (scn->bcn_mode != (u_int8_t)value) {
                    retval = wmi_unified_pdev_set_param(scn->wmi_handle,
                                 WMI_PDEV_PARAM_BEACON_TX_MODE, value);
                    if (retval == EOK) {
                        scn->bcn_mode = (u_int8_t)value;
                    }
                }
                break;
        }
        break; 
        case OL_ATH_PARAM_ARP_AC_OVERRIDE:
            if ((WME_AC_BE <= value) && (value <= WME_AC_VO)) {
                scn->arp_override = value;
                retval = wmi_unified_pdev_set_param(scn->wmi_handle,
                    WMI_PDEV_PARAM_ARP_AC_OVERRIDE, value);
            } else {
                retval = -EINVAL;
            }
        break;
        case OL_ATH_PARAM_ANI_ENABLE:
        {
                if (value <= 1) {
                    return wmi_unified_pdev_set_param(scn->wmi_handle,
                             WMI_PDEV_PARAM_ANI_ENABLE, value);
                } else {
                    retval = -EINVAL;
                }
        }
        break;

        case OL_ATH_PARAM_DCS:
            {
                /* 
                 * Host and target should always contain the same value. So
                 * avoid talking to target if the values are same.
                 */
                if (value == OL_IS_DCS_ENABLED(scn->scn_dcs.dcs_enable)) {
                    retval = EOK;
                    break;
                }
                /* if already enabled and run state is not running, more
                 * likely that channel change is in progress, do not let
                 * user modify the current status
                 */
                if ((OL_IS_DCS_ENABLED(scn->scn_dcs.dcs_enable)) &&
                        !(OL_IS_DCS_RUNNING(scn->scn_dcs.dcs_enable))) {
                    retval = EINVAL;
                    break;
                }
                retval = wmi_unified_pdev_set_param(scn->wmi_handle, 
                                WMI_PDEV_PARAM_DCS, value);

                /* 
                 * we do not expect this to fail, if failed, eventually
                 * target and host may not be at agreement. Otherway is 
                 * to keep it in same old state. 
                 */
                if (EOK == retval) {
                    scn->scn_dcs.dcs_enable = value;
                    printk("DCS: %s dcs enable value %d return value %d", __func__, value, retval );
                } else {
                    printk("DCS: %s target command fail, setting return value %d",
                            __func__, retval );
                }
                (OL_IS_DCS_ENABLED(scn->scn_dcs.dcs_enable)) ? (OL_ATH_DCS_SET_RUNSTATE(scn->scn_dcs.dcs_enable)) :
                                        (OL_ATH_DCS_CLR_RUNSTATE(scn->scn_dcs.dcs_enable)); 
            }
            break;
        case OL_ATH_PARAM_DCS_COCH_THR: 
            scn->scn_dcs.coch_intr_thresh = value;
            break;
        case OL_ATH_PARAM_DCS_PHYERR_THR: 
            scn->scn_dcs.phy_err_threshold = value;
            break;
        case OL_ATH_PARAM_DCS_PHYERR_PENALTY: 
            scn->scn_dcs.phy_err_penalty = value;         /* phy error penalty*/
            break;
        case OL_ATH_PARAM_DCS_RADAR_ERR_THR:
            scn->scn_dcs.radar_err_threshold = value;
            break;
        case OL_ATH_PARAM_DCS_USERMAX_CU_THR:
            scn->scn_dcs.user_max_cu = value;             /* tx_cu + rx_cu */
            break;
        case OL_ATH_PARAM_DCS_INTR_DETECT_THR:
            scn->scn_dcs.intr_detection_threshold = value;
            break;
        case OL_ATH_PARAM_DCS_SAMPLE_WINDOW:
            scn->scn_dcs.intr_detection_window = value;
            break;
        case OL_ATH_PARAM_DCS_DEBUG:
            if (value < 0 || value > 2) {
                printk("0-disable, 1-critical 2-all, %d-not valid option\n", value);
                return -EINVAL;
            }
            scn->scn_dcs.dcs_debug = value;
            break;

        case OL_ATH_PARAM_DYN_TX_CHAINMASK:
            /*
             * value is set to either 1 (enabled) or 0 (disabled).
             */
            value = !!value;
            if (scn->dtcs != (u_int8_t)value) {
                retval = wmi_unified_pdev_set_param(scn->wmi_handle,
                                 WMI_PDEV_PARAM_DYNTXCHAIN, value);
                if (retval == EOK) {
                    scn->dtcs = (u_int8_t)value;
                }
            }
        break;
        
        case OL_ATH_PARAM_VOW_EXT_STATS:
            {
                scn->vow_extstats = value;
            }
            break; 

        case OL_ATH_PARAM_LTR_ENABLE:
            param_id = WMI_PDEV_PARAM_LTR_ENABLE;
            goto low_power_config;
        case OL_ATH_PARAM_LTR_AC_LATENCY_BE:
            param_id = WMI_PDEV_PARAM_LTR_AC_LATENCY_BE;
            goto low_power_config;
        case OL_ATH_PARAM_LTR_AC_LATENCY_BK:
            param_id = WMI_PDEV_PARAM_LTR_AC_LATENCY_BK;
            goto low_power_config;
        case OL_ATH_PARAM_LTR_AC_LATENCY_VI:
            param_id = WMI_PDEV_PARAM_LTR_AC_LATENCY_VI;
            goto low_power_config;
        case OL_ATH_PARAM_LTR_AC_LATENCY_VO:
            param_id = WMI_PDEV_PARAM_LTR_AC_LATENCY_VO;
            goto low_power_config;
        case OL_ATH_PARAM_LTR_AC_LATENCY_TIMEOUT:
            param_id = WMI_PDEV_PARAM_LTR_AC_LATENCY_TIMEOUT;
            goto low_power_config;
        case OL_ATH_PARAM_LTR_TX_ACTIVITY_TIMEOUT:
            param_id = WMI_PDEV_PARAM_LTR_TX_ACTIVITY_TIMEOUT;
            goto low_power_config;
        case OL_ATH_PARAM_LTR_SLEEP_OVERRIDE:
            param_id = WMI_PDEV_PARAM_LTR_SLEEP_OVERRIDE;
            goto low_power_config;
        case OL_ATH_PARAM_LTR_RX_OVERRIDE:
            param_id = WMI_PDEV_PARAM_LTR_RX_OVERRIDE;
            goto low_power_config;
        case OL_ATH_PARAM_L1SS_ENABLE:
            param_id = WMI_PDEV_PARAM_L1SS_ENABLE;
            goto low_power_config;
        case OL_ATH_PARAM_DSLEEP_ENABLE:
            param_id = WMI_PDEV_PARAM_DSLEEP_ENABLE;
            goto low_power_config;
low_power_config:
            retval = wmi_unified_pdev_set_param(scn->wmi_handle,
                         param_id, value);
            break;
        default:
            return (-1);
    }
    return retval;
}

int
ol_ath_get_config_param(struct ol_ath_softc_net80211 *scn, ol_ath_param_t param, void *buff)
{
    int retval = 0;
    struct ieee80211com *ic = &scn->sc_ic;

    switch(param)
    {
        case OL_ATH_PARAM_GET_IF_ID:
            *(int *)buff = IF_ID_OFFLOAD;
        break;

        case OL_ATH_PARAM_TXCHAINMASK:
            *(int *)buff = ieee80211com_get_tx_chainmask(ic);
        break;
        case OL_ATH_PARAM_RXCHAINMASK:
            *(int *)buff = ieee80211com_get_rx_chainmask(ic);
        break;
        case OL_ATH_PARAM_BCN_BURST:
            *(int *)buff = scn->bcn_mode;
        break;
        case OL_ATH_PARAM_ARP_AC_OVERRIDE:
            *(int *)buff = scn->arp_override;
        break;
        
        case OL_ATH_PARAM_TXPOWER_LIMIT2G:
            *(int *)buff = scn->txpowlimit2G;
        break;
        
        case OL_ATH_PARAM_TXPOWER_LIMIT5G:
            *(int *)buff = scn->txpowlimit5G;
        break;

        case OL_ATH_PARAM_TXPOWER_SCALE:
            *(int *)buff = scn->txpower_scale;
        break;

        case OL_ATH_PARAM_DYN_TX_CHAINMASK:
            *(int *)buff = scn->dtcs;
        break; 
    case OL_ATH_PARAM_VOW_EXT_STATS:
        *(int *)buff = scn->vow_extstats;
        break;
        case OL_ATH_PARAM_DCS:
        /* do not need to talk to target */
        *(int *)buff = OL_IS_DCS_ENABLED(scn->scn_dcs.dcs_enable);
            break;
        case OL_ATH_PARAM_DCS_COCH_THR: 
            *(int *)buff = scn->scn_dcs.coch_intr_thresh ;
            break;
        case OL_ATH_PARAM_DCS_PHYERR_THR: 
            *(int *)buff = scn->scn_dcs.phy_err_threshold ;
            break;
        case OL_ATH_PARAM_DCS_PHYERR_PENALTY: 
            *(int *)buff = scn->scn_dcs.phy_err_penalty ;  
            break;
        case OL_ATH_PARAM_DCS_RADAR_ERR_THR:
            *(int *)buff = scn->scn_dcs.radar_err_threshold ;
            break;
        case OL_ATH_PARAM_DCS_USERMAX_CU_THR:
            *(int *)buff = scn->scn_dcs.user_max_cu ;         
            break;
        case OL_ATH_PARAM_DCS_INTR_DETECT_THR:
            *(int *)buff = scn->scn_dcs.intr_detection_threshold ;
            break;
        case OL_ATH_PARAM_DCS_SAMPLE_WINDOW:
            *(int *)buff = scn->scn_dcs.intr_detection_window ;
            break;
        case OL_ATH_PARAM_DCS_DEBUG:
            *(int *)buff = scn->scn_dcs.dcs_debug ;
            break;
        default:
            return (-1);
    }
    return retval;
}


int
ol_hal_set_config_param(struct ol_ath_softc_net80211 *scn, ol_hal_param_t param, void *buff)
{
    return -1;
}

int
ol_hal_get_config_param(struct ol_ath_softc_net80211 *scn, ol_hal_param_t param, void *address)
{
    return -1;
}

#endif
