/*
 * Copyright (c) 2010, Atheros Communications Inc. 
 * All Rights Reserved.
 * 
 * Copyright (c) 2011 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 * 
 */

#include "ath_internal.h"

#if ATH_SUPPORT_SPECTRAL

#include "spectral.h"

/* Get noise floor compensated control channel RSSI */
int8_t get_nfc_ctl_rssi(struct ath_spectral *spectral, int8_t rssi, int8_t *ctl_nf)
{
    SPECTRAL_OPS* p_sops = GET_SPECTRAL_OPS(spectral);
    int16_t nf = -110;
    int8_t temp;
    *ctl_nf = -110;

    nf = p_sops->get_ctl_noisefloor(spectral);
    temp = 110 + (nf) + (rssi);
    *ctl_nf = nf;
    return temp;
}

/* Get noise floor compensated extension channel RSSI */
int8_t get_nfc_ext_rssi(struct ath_spectral *spectral, int8_t rssi, int8_t *ext_nf)
{
    int16_t nf = -110;
    int8_t temp;
    *ext_nf = -110;
    SPECTRAL_OPS p_sops = GET_SPECTRAL_OPS(spectral);

    nf = p_sops->get_ext_noisefloor(spectral);
    temp = 110 + (nf) + (rssi);
    *ext_nf = nf;
    return temp;
}

static void update_eacs_avg_rssi(struct ath_spectral *spectral, int8_t nfc_ctl_rssi, int8_t nfc_ext_rssi)
{
    int temp=0;

    if(spectral->sc_spectral_20_40_mode) {
        // HT40 mode
        temp = (spectral->ext_eacs_avg_rssi * (spectral->ext_eacs_spectral_reports));
        temp += nfc_ext_rssi;
        spectral->ext_eacs_spectral_reports++;
        spectral->ext_eacs_avg_rssi = (temp / spectral->ext_eacs_spectral_reports);
    }

    temp = (spectral->ctl_eacs_avg_rssi * (spectral->ctl_eacs_spectral_reports));
    temp += nfc_ctl_rssi;
    spectral->ctl_eacs_spectral_reports++;
    spectral->ctl_eacs_avg_rssi = (temp / spectral->ctl_eacs_spectral_reports);
}

void update_eacs_counters(struct ath_spectral *spectral, int8_t nfc_ctl_rssi, int8_t nfc_ext_rssi)
{
    update_eacs_thresholds(spectral);
    update_eacs_avg_rssi(spectral, nfc_ctl_rssi, nfc_ext_rssi);

    if (spectral->sc_spectral_20_40_mode) {
        // HT40 mode
        if (nfc_ext_rssi > spectral->ext_eacs_rssi_thresh){
            spectral->ext_eacs_interf_count++;    
        }
        spectral->ext_eacs_duty_cycle=((spectral->ext_eacs_interf_count * 100)/spectral->eacs_this_scan_spectral_data);
    }

    if (nfc_ctl_rssi > spectral->ctl_eacs_rssi_thresh){
        spectral->ctl_eacs_interf_count++;
    }
    spectral->ctl_eacs_duty_cycle=((spectral->ctl_eacs_interf_count * 100)/spectral->eacs_this_scan_spectral_data);
}

int get_eacs_control_duty_cycle(struct ath_spectral* spectral)
{
    if (spectral == NULL) {
        return 0;
    }
    return spectral->ctl_eacs_duty_cycle;
}

int get_eacs_extension_duty_cycle(struct ath_spectral *spectral)
{
    if (spectral == NULL) {
        return 0;
    }
    spectral->ctl_eacs_interf_count, 
    spectral->ext_eacs_interf_count, 
    spectral->eacs_this_scan_spectral_data);
    return spectral->ext_eacs_duty_cycle;
}

void update_eacs_thresholds(struct ath_spectral* spectral)
{
    int16_t nf = -110;
    SPECTRAL_OPS* p_sops = GET_SPECTRAL_OPS(spectral);

    nf = p_sops->get_ctl_noisefloor(spectral);
    spectral->ctl_eacs_rssi_thresh = nf + 10;

    if (spectral->sc_spectral_20_40_mode) {
 
        nf = p_sops->get_ext_noisefloor(spectral);
        spectral->ext_eacs_rssi_thresh = nf + 10;
    }
    spectral->ctl_eacs_rssi_thresh = 32;
    spectral->ext_eacs_rssi_thresh = 32;
}
#endif
