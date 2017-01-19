/*
 * Copyright (c) 2008-2010, Atheros Communications Inc. 
 * All Rights Reserved.
 * 
 * Copyright (c) 2011 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 * 
 */


#include "opt_ah.h"

#ifdef AH_SUPPORT_AR9300

#include "ah.h"
#include "ah_internal.h"
#include "ah_desc.h"
#include "ar9300.h"
#include "ar9300desc.h"
#include "ar9300reg.h"
#include "ar9300phy.h"

#ifdef ATH_SUPPORT_TxBF
#include "ar9300_txbf.h"

/* number of carrier mappings under different bandwidth and grouping, ex: 
   bw = 1 (40M), Ng=0 (no group), number of carrier = 114*/
static u_int8_t const Ns[NUM_OF_BW][NUM_OF_Ng] = {
    { 56, 30, 16},
    {114, 58, 30}};

static u_int8_t const Valid_bits[MAX_BITS_PER_SYMBOL] = {
    0x1, 0x3, 0x7, 0xf, 0x1f, 0x3f, 0x7f, 0xff};

static u_int8_t Num_bits_on[NUM_OF_CHAINMASK] = {
    0 /* 000 */,
    1 /* 001 */,
    1 /* 010 */,
    2 /* 011 */,
    1 /* 100 */,
    2 /* 101 */,
    2 /* 110 */,
    3 /* 111 */
};

u_int8_t
ar9300_get_ntx(struct ath_hal *ah)
{
    return Num_bits_on[AH9300(ah)->ah_tx_chainmask];
}

u_int8_t
ar9300_get_nrx(struct ath_hal *ah)
{
    return Num_bits_on[AH9300(ah)->ah_rx_chainmask];
}

void ar9300_set_hw_cv_timeout(struct ath_hal *ah, bool opt)
{
    struct ath_hal_private  *ap = AH_PRIVATE(ah);

    /* 
     * if true use H/W settings to update cv timeout values;
     * otherwise set the cv timeout value to 1 ms to trigger S/W timer
     */
    if (opt) {
        AH_PRIVATE(ah)->ah_txbf_hw_cvtimeout = ap->ah_config.ath_hal_cvtimeout;
    } else {
        AH_PRIVATE(ah)->ah_txbf_hw_cvtimeout = ONE_MS;
    }
    OS_REG_WRITE(ah, AR_TXBF_TIMER,
        (AH_PRIVATE(ah)->ah_txbf_hw_cvtimeout << AR_TXBF_TIMER_ATIMEOUT_S) |
        (AH_PRIVATE(ah)->ah_txbf_hw_cvtimeout << AR_TXBF_TIMER_TIMEOUT_S));
}

void ar9300_init_txbf(struct ath_hal *ah)
{
    u_int32_t tmp;
    struct ath_hal_private *ahpriv = AH_PRIVATE(ah);
    u_int8_t txbf_ctl;

    /* set default settings for tx_bf */
    OS_REG_WRITE(ah, AR_TXBF,
        /* size of codebook entry set to psi 3bits, phi 6bits */
        (AR_TXBF_PSI_4_PHI_6 << AR_TXBF_CB_TX_S)    |
        /* set Number of bit_to 8 bits */
        (AR_TXBF_NUMBEROFBIT_8 << AR_TXBF_NB_TX_S)  |
        /* NG_RPT_TX set t0 No_GROUP */
        (AR_TXBF_No_GROUP << AR_TXBF_NG_RPT_TX_S)   |
        /* TXBF_NG_CVCACHE set to 16 clients */
        (AR_TXBF_SIXTEEN_CLIENTS << AR_TXBF_NG_CVCACHE_S)  |
        /* set weighting method to max power */
        (SM(AR_TXBF_MAX_POWER, AR_TXBF_TXCV_BFWEIGHT_METHOD)));

    /* when ah_txbf_hw_cvtimeout = 0, use setting values */
    if (AH_PRIVATE(ah)->ah_txbf_hw_cvtimeout == 0) {
        ar9300_set_hw_cv_timeout(ah, true);
    } else {
        ar9300_set_hw_cv_timeout(ah, false);
    }

    /*
     * Set CEC to 2 stream for self_gen.
     * Set spacing to 8 us.
     * Initial selfgen Minimum MPDU.
     */
    tmp = OS_REG_READ(ah, AR_SELFGEN);
    tmp |= SM(AR_SELFGEN_CEC_TWO_SPACETIMESTREAM, AR_CEC);
    tmp |= SM(AR_SELFGEN_MMSS_EIGHT_us, AR_MMSS);
    OS_REG_WRITE(ah, AR_SELFGEN, tmp);
   
    /*  set initial for basic set */
    ahpriv->ah_lowest_txrate = MAXMCSRATE;
    ahpriv->ah_basic_set_buf = (1 << (ahpriv->ah_lowest_txrate+ 1))- 1;
    OS_REG_WRITE(ah, AR_BASIC_SET, ahpriv->ah_basic_set_buf);

#if 0
    tmp = OS_REG_READ(ah, AR_PCU_MISC_MODE2);
    /* force H upload */
    tmp |= (1 << 28);
    OS_REG_WRITE(ah, AR_PCU_MISC_MODE2, tmp);
#endif

    /* enable HT fine timing */
    tmp = OS_REG_READ(ah, AR_PHY_TIMING2);
    tmp |= AR_PHY_TIMING2_HT_Fine_Timing_EN;
    OS_REG_WRITE(ah, AR_PHY_TIMING2, tmp);

    /* enable description decouple */
    tmp = OS_REG_READ(ah, AR_PCU_MISC_MODE2);
    tmp |= AR_DECOUPLE_DECRYPTION;
    OS_REG_WRITE(ah, AR_PCU_MISC_MODE2, tmp);

    /* enable flt SVD */
    tmp = OS_REG_READ(ah, AR_PHY_SEARCH_START_DELAY);
    tmp |= AR_PHY_ENABLE_FLT_SVD;
    OS_REG_WRITE(ah, AR_PHY_SEARCH_START_DELAY, tmp);

    /* initial sequence generate for auto reply mgmt frame*/
    OS_REG_WRITE(ah, AR_MGMT_SEQ, AR_MIN_HW_SEQ | 
        SM(AR_MAX_HW_SEQ,AR_MGMT_SEQ_MAX));
    
    txbf_ctl = ahpriv->ah_config.ath_hal_txbf_ctl;
    if ((txbf_ctl & TXBF_CTL_NON_EX_BF_IMMEDIATELY_RPT) ||
        (txbf_ctl & TXBF_CTL_COMP_EX_BF_IMMEDIATELY_RPT)) {
        tmp = OS_REG_READ(ah, AR_H_XFER_TIMEOUT);
        tmp |= AR_EXBF_IMMDIATE_RESP;
        tmp &= ~(AR_EXBF_NOACK_NO_RPT);
        /* enable immediately report */
        OS_REG_WRITE(ah, AR_H_XFER_TIMEOUT, tmp);
    }
}

int
ar9300_get_ness(struct ath_hal *ah, u_int8_t code_rate, u_int8_t cec)
{
    u_int8_t ndltf = 0, ness = 0, ntx;

    ntx = ar9300_get_ntx(ah);

    /* cec+1 remote cap's for channel estimation in stream.*/
    if (ntx > (cec + 2)) {  /* limit by remote's cap */
        ntx = cec + 2;
    }

    if (code_rate < MIN_TWO_STREAM_RATE) {
        ndltf = 1;
    } else if (code_rate < MIN_THREE_STREAM_RATE) {
        ndltf = 2;
    } else {

        ndltf = 4;
    }

    /* NESS is used for setting neltf and NTX =<NDLTF + NELTF, NDLTF 
     * is 2^(stream-1), if NTX < NDLTF, NESS=0, other NESS = NTX-NDLTF */
    if (code_rate >= MIN_HT_RATE) { /* HT rate */
        if (ntx > ndltf) {
            ness = ntx - ndltf;
        }
    }
    return ness;
}

/*
 * function: ar9300_set_11n_txbf_sounding
 * purpose:  Set sounding frame
 * inputs:   
 *           series: rate series of sounding frame
 *           cec: Channel Estimation Capability . Extract from subfields
 *               of the Transmit Beamforming Capabilities field of remote.
 *           opt: control flag of current frame
 */
void
ar9300_set_11n_txbf_sounding(
    struct ath_hal *ah,
    void *ds,
    HAL_11N_RATE_SERIES series[],
    u_int8_t cec,
    u_int8_t opt)
{
    struct ar9300_txc *ads = AR9300TXC(ds);
    u_int8_t ness = 0, ness1 = 0, ness2 = 0, ness3 = 0;

    ads->ds_ctl19 &= (~AR_not_sounding); /*set sounding frame */

    if (opt == HAL_TXDESC_STAG_SOUND) {
        ness  = ar9300_get_ness(ah, series[0].Rate, cec);
        ness1 = ar9300_get_ness(ah, series[1].Rate, cec);
        ness2 = ar9300_get_ness(ah, series[2].Rate, cec);
        ness3 = ar9300_get_ness(ah, series[3].Rate, cec);
    }
    ads->ds_ctl19 |= SM(ness, AR_ness);
    ads->ds_ctl20 |= SM(ness1, AR_ness1);
    ads->ds_ctl21 |= SM(ness2, AR_ness2);
    ads->ds_ctl22 |= SM(ness3, AR_ness3);

#if 0
    HDPRINTF(AH_NULL, HAL_DBG_UNMASKABLE,
        "==>%s:sounding rate series %x,%x,%x,%x \n",
        __func__,
        series[0].Rate, series[1].Rate, series[2].Rate, series[3].Rate);
    HDPRINTF(AH_NULL, HAL_DBG_UNMASKABLE,
        "==>%s:Ness1 %d, NESS2 %d, NESS3 %d, NESS4 %d\n",
        __func__, ness, ness1, ness2, ness3);
#endif

    /* disable other series that don't support sounding at legacy rate */
    if (series[1].Rate < MIN_HT_RATE) {
        ads->ds_ctl13 &=
            ~(AR_xmit_data_tries1 | AR_xmit_data_tries2 | AR_xmit_data_tries3);
    } else if (series[2].Rate < MIN_HT_RATE) {
        ads->ds_ctl13 &= ~(AR_xmit_data_tries2 | AR_xmit_data_tries3);
    } else if (series[3].Rate < MIN_HT_RATE) {
        ads->ds_ctl13 &= ~(AR_xmit_data_tries3);
    }
}

/* search CV cache address according to key index */
static u_int16_t
ar9300_txbf_lru_search(struct ath_hal *ah, u_int8_t key_idx)
{
    u_int32_t tmp = 0;
    u_int16_t cv_cache_idx = 0;

    /* make sure LRU search is initially disabled */
    OS_REG_WRITE(ah, AR_TXBF_SW, 0);
    tmp = (SM(key_idx, AR_DEST_IDX) | AR_LRU_EN);
    /* enable LRU search */
    OS_REG_WRITE(ah, AR_TXBF_SW, tmp);

    /* wait for LRU search to finish */
    do {
        tmp = OS_REG_READ(ah, AR_TXBF_SW);
    } while ((tmp & AR_LRU_ACK) != 1);
    
    cv_cache_idx = MS(tmp, AR_LRU_ADDR);  

    /* disable LRU search */
    OS_REG_WRITE(ah, AR_TXBF_SW, 0);
    return cv_cache_idx;
}

#ifdef TXBF_TODO
void
ar9300_set_11n_txbf_cal(
    struct ath_hal *ah,
    void *ds,
    u_int8_t cal_pos,
    u_int8_t code_rate,
    u_int8_t cec,
    u_int8_t opt)
{
    struct ar9300_txc *ads = AR9300TXC(ds);

    /* set descriptor for calibration*/
    if (cal_pos == 1) {             
        /* cal start frame */
        /* set vmf to enable burst mode */
        ads->ds_ctl11 |= AR_virt_more_frag; 
    } else if (cal_pos == 3) {      
        /* cal complete frame */
        /* set calibrating indicator to BB */
        ads->ds_ctl17 |= AR_calibrating; 
        /*ar9300_set_11n_txbf_sounding(ah, ds, code_rate, cec, opt);*/
    }
}

static void
ar9300_txbf_write_cache(struct ath_hal *ah, u_int32_t cv_tmp, u_int16_t idx)
{
    u_int32_t tmp;

    /* indicate to the HW that this operation writes to the CV cache */
    cv_tmp |= AR_CVCACHE_WRITE;
    OS_REG_WRITE(ah, AR_CVCACHE(idx), cv_tmp);
    /* wait for CV cache write to complete */
    do {
        tmp = OS_REG_READ(ah, AR_TXBF_SW);
    }   while ((tmp & AR_LRU_WR_ACK) == 0);
}

/*
 * Function ar9300_txbf_save_cv_compress
 * purpose: save compress report from segmeneted compress report frame into
 *     CVcache.
 * input:
 *     ah:
 *     key_idx: key cache index
 *     mimo_control: copy of first two bytes of MIMO control field
 *     compress_rpt: pointer to the buffer which collect segmented compressed
 *         beamforming report together and keep beamforming feedback matrix
 *         only. (remove SNR report)
 * output: false: incorrect parameters for Nc and Nr.
 *         true:  success.
 */
bool
ar9300_txbf_save_cv_compress(struct ath_hal *ah, u_int8_t key_idx,
    u_int16_t mimo_control, u_int8_t *compress_rpt)
{
    u_int8_t  nc_idx, nr_idx, bandwidth, ng, nb, ci, psi_bit, phi_bit, i, j;
    u_int32_t cv_tmp = 0;
    u_int16_t cv_cache_idx = 0;
    u_int16_t idx;
    u_int8_t  psi[3], phi[3];
    u_int8_t  ng_sys = 0;
    u_int8_t  bits_left = 8, rpt_idx = 0, na_half, ns, ns_rpt, current_data;
    u_int8_t  samples, sample_idx, average_shift;
    u_int8_t const  na[3][3] = { /* number of angles */
        {0, 0, 0}, /* index 0: N/A */
        {2, 2, 0},
        {4, 6, 6}};

    /* get required parameters from MIMO control field */
    nc_idx = (mimo_control & AR_nc_idx) + 1;
    nr_idx = (MS(mimo_control, AR_nr_idx)) + 1;
    if ((nc_idx >= 3) || (nr_idx >= 3)) {
        return false; /* out-of-bounds due to incorrect mimo_control */
    }

    na_half = na[nc_idx][nr_idx] / 2;
    if (na_half == 0) {
        return false; /* incorrect mimo_control / Na */
    }

    bandwidth = MS(mimo_control, AR_bandwith);
    ng = MS(mimo_control, AR_ng);
    nb = MS(mimo_control, AR_nb);
    ci = MS(mimo_control, AR_CI);
    psi_bit = ci + 1;       /* # of bits for each psi */
    phi_bit = ci + 3;       /* # of bits for each phi */

    /* search CV cache by key cache index */
    cv_cache_idx = ar9300_txbf_lru_search(ah, key_idx);

    idx = cv_cache_idx + 4;  /* first CV address past header */
    ng_sys = MS(OS_REG_READ(ah, AR_TXBF), AR_TXBF_NG_CVCACHE);

    ns = Ns[bandwidth][ng_sys]; /* ns stores desired final carrier size */
    if (ng <= ng_sys) {
        /* keep ng of current report */
        samples = 1;  /* no down sampling required */
        average_shift = 0;
    } else {
        /* reduce the ng of current report */
        average_shift = ng - ng_sys;
        samples = 1 << average_shift; /* down sampling amount */
        ng = ng_sys;
    }

    bits_left = 8;
    rpt_idx = 0;
    current_data = compress_rpt[rpt_idx];
    sample_idx = 0;
    ns_rpt = Ns[bandwidth][ng]; /* carrier size in original report */
    for (i = 0; i < ns_rpt; i++) {
        for (j = 0; j < na_half; j++) {
            if (bits_left < phi_bit) {
                rpt_idx++;
                /* use the next byte from the report */
                current_data += compress_rpt[rpt_idx] << bits_left;
                bits_left += 8;
            }
            phi[j] = (current_data & Valid_bits[phi_bit - 1]);
            current_data = current_data >> phi_bit;
            bits_left -= phi_bit;

            if (bits_left < psi_bit) {
                rpt_idx++;
                /* use the next byte from the report */
                current_data += compress_rpt[rpt_idx] << bits_left;
                bits_left += 8;
            }
            psi[j] = (current_data & Valid_bits[psi_bit - 1]);
            current_data = current_data >> psi_bit;
            bits_left -= psi_bit;
        }
        sample_idx++;
        if (samples == sample_idx) {
            sample_idx = 0;
            /* pack phi and psi into CV cache format */
            cv_tmp =
                 phi[0]        |
                (phi[1] <<  6) |
                (psi[0] << 12) |
                (psi[1] << 16) |
                (phi[2] << 20) |
                (psi[2] << 24);
            ar9300_txbf_write_cache(ah, cv_tmp, idx);
            idx += 4; /* next address in CV cache */
        }
    }
    /* construct and write the CV cache header */
    cv_tmp =
        SM(ng, AR_CVCACHE_Ng_IDX)      |
        SM(bandwidth, AR_CVCACHE_BW40) |
        SM(0, AR_CVCACHE_IMPLICIT)     |
        SM(nc_idx, AR_CVCACHE_Nc_IDX)  |
        SM(nr_idx, AR_CVCACHE_Nr_IDX);
    ar9300_txbf_write_cache(ah, cv_tmp, cv_cache_idx);

    return true;
}

/*
 * Function ar9300_txbf_save_cv_non_compress
 * purpose: save compress report from segmeneted compress report frame into
 *     CVcache.
 * input:
 *     ah:
 *     key_idx: key cache index
 *     mimo_control: copy of first two bytes of MIMO control field
 *     non_compress_rpt: pointer to the buffer which collect segmented
 *         Noncompressed beamforming report together and keep beamforming
 *         feedback matrix only. (remove SNR report)
 * output: false: incorrect parameters for Nc and Nr.
 *         true:  success.
 */
bool
ar9300_txbf_save_cv_non_compress(struct ath_hal *ah, u_int8_t key_idx,
        u_int16_t mimo_control, u_int8_t *non_compress_rpt)
{
    u_int8_t  nc_idx, nr_idx, bandwidth, ng, nb, i, j, k;
    u_int32_t cv_tmp = 0;
    u_int16_t cv_cache_idx = 0;
    u_int16_t idx;
    u_int8_t  psi[3], phi[3];
    u_int8_t  ng_sys = 0;
    u_int8_t  bits_left = 8, rpt_idx = 0, ns, ns_rpt, current_data;
    u_int8_t  samples, sample_idx, average_shift;
    COMPLEX   v[3][3];

    /* get required parameters from MIMO control field */
    nc_idx = (mimo_control & AR_nc_idx) + 1;
    nr_idx = (MS(mimo_control, AR_nr_idx)) + 1;
    if ((nc_idx >= 3) | (nr_idx >= 3)) {
        return false; /* out-of-bounds due to incorrect mimo_control */
    }

    bandwidth = MS(mimo_control, AR_bandwith);
    ng = MS(mimo_control, AR_ng);
    switch (MS(mimo_control, AR_nb)) {
    case 0:
        nb = 4;
        break;
    case 1:
        nb = 2;
        break;
    case 2:
        nb = 6;
        break;
    case 3:
        nb = 8;
        break;
    default:
        nb = 8;
        break;
    }

    /* search CV cache by key cache index */
    cv_cache_idx = ar9300_txbf_lru_search(ah, key_idx);

    idx = cv_cache_idx + 4;  /* first CV address past header */
    ng_sys = MS(OS_REG_READ(ah, AR_TXBF), AR_TXBF_NG_CVCACHE);

    ns = Ns[bandwidth][ng_sys]; /* ns stores desired final carrier size */

    if (ng <= ng_sys) {
        /* keep ng of current report */
        samples = 1;  /* no down sampling is required */
        average_shift = 0;
    } else {
        /* reduce the ng of current report */
        average_shift = ng - ng_sys;
        samples = 1 << average_shift; /* down sampling amount */
        ng = ng_sys;
    }

    current_data = non_compress_rpt[rpt_idx];
    sample_idx = 0;
    ns_rpt = Ns[bandwidth][ng]; /* carrier size in original report */
    for (i = 0; i < ns_rpt; i++) {
        for (j = 0; j < nr_idx; j++) {
            for (k = 0; k < nc_idx; k++) {
                if (bits_left < nb) {
                    rpt_idx++;
                    /* use the next byte from the report */
                    current_data += non_compress_rpt[rpt_idx] << bits_left;
                    bits_left += 8;
                }    
                v[j][k].real = current_data & Valid_bits[nb - 1];
                current_data = current_data >> nb;
                bits_left -= nb;
                if (bits_left < nb) {
                    rpt_idx++;
                    /* use the next byte from the report */
                    current_data += non_compress_rpt[rpt_idx] << bits_left;
                    bits_left += 8;
                }    
                v[j][k].imag = current_data & Valid_bits[nb - 1];
                current_data = current_data >> nb;
                bits_left -= nb;
            }
        }            

        compress_v(v, nr_idx, nc_idx, phi, psi);

        sample_idx++;
        if (samples == sample_idx) {
            sample_idx = 0;
            /* pack phi and psi into CV cache format */
            cv_tmp =
                 phi[0]        |
                (phi[1] <<  6) |
                (psi[0] << 12) |
                (psi[1] << 16) |
                (phi[2] << 20) |
                (psi[2] << 24);
            ar9300_txbf_write_cache(ah, cv_tmp, idx);
            idx += 4; /* next address in CV cache */
        }
    }

    /* construct and write the CV cache header */
    cv_tmp =
        SM(ng, AR_CVCACHE_Ng_IDX)      |
        SM(bandwidth, AR_CVCACHE_BW40) |
        SM(0, AR_CVCACHE_IMPLICIT)     |
        SM(nc_idx, AR_CVCACHE_Nc_IDX)  |
        SM(nr_idx, AR_CVCACHE_Nr_IDX);
    ar9300_txbf_write_cache(ah, cv_tmp, cv_cache_idx);

    return true;
}
/* Function ar9300_signbit_convert
 * purpose : convert sign bit to int format
 */
int
ar9300_signbit_convert(
    int data,
    int maxbit)
{
    if (data & (1 << (maxbit - 1))) { /* negative */
        data -= (1 << maxbit);     
    }
    return data;
}

/*
 * Function: ar9300_get_local_h
 * purpose: Get H from local buffer, separate it into complex format
 */
void
ar9300_get_local_h(
    struct ath_hal *ah,
    u_int8_t *local_h,
    int local_h_length,
    int num_tones,
    COMPLEX(*output_h)[3][TONE_40M])
{
    u_int8_t  k;
    u_int8_t  nc, nr, bits_left, nc_idx, nr_idx;
    u_int32_t bitmask, idx, current_data, h_data, h_idx;

    nr = ar9300_get_nrx(ah);
    /* total bits = 20 * nr * nc * num_tones */
    nc = (int) (local_h_length * BITS_PER_BYTE) / 
            (int) (BITS_PER_COMPLEX_SYMBOL * nr * num_tones);

    bits_left = 16; /* process 16 bits at a time */

    /* 10 bit resoluation for H real and imag */
    bitmask = (1 << BITS_PER_SYMBOL) - 1;
    idx = h_idx = 0;
    h_data = local_h[idx++];
    h_data += (local_h[idx++] << BITS_PER_BYTE);
    current_data = h_data & ((1 << 16) - 1); /* get 16 LSBs first */
    
    for (k = 0; k < num_tones; k++) {
        for (nc_idx = 0; nc_idx < nc; nc_idx++) {
            for (nr_idx = 0; nr_idx < nr; nr_idx++) {
                if ((bits_left - BITS_PER_SYMBOL) < 0) {
                    /* get the next 16 bits */
                    h_data = local_h[idx++];
                    h_data += (local_h[idx++] << BITS_PER_BYTE);
                    current_data += h_data << bits_left;
                    bits_left += 16;
                }
                output_h[nr_idx][nc_idx][k].imag = current_data & bitmask;
                
                output_h[nr_idx][nc_idx][k].imag = 
                    ar9300_signbit_convert(output_h[nr_idx][nc_idx][k].imag,
                         BITS_PER_SYMBOL);

                bits_left -= BITS_PER_SYMBOL;
                /* shift out used bits */
                current_data = current_data >> BITS_PER_SYMBOL; 

                if ((bits_left - BITS_PER_SYMBOL) < 0) {
                    /* get the next 16 bits */
                    h_data = local_h[idx++];
                    h_data += (local_h[idx++] << BITS_PER_BYTE);
                    current_data += h_data << bits_left;
                    bits_left += 16;
                }
                output_h[nr_idx][nc_idx][k].real = current_data & bitmask;
                
                output_h[nr_idx][nc_idx][k].real = 
                    ar9300_signbit_convert(output_h[nr_idx][nc_idx][k].real,
                         BITS_PER_SYMBOL);

                bits_left -= BITS_PER_SYMBOL;
                
                /* shift out used bits */
                current_data = current_data >> BITS_PER_SYMBOL;
            }
        }
    }
}

/*
 * Function ar9300_txbf_rc_update
 * purpose: Calculation the radio coefficients from CSI1 and CSI2 and save it
 *     into hardware.
 * input:
 *     ah:
 *     rx_status: pointer to Rx status of Rx CSI frame.
 *     local_h: pointer to RXDP+12. Which is the beginning point of h
 *         reported by hardware.
 *     CSIframe: pointer to mimo control field of CSI frame, it should
 *         include mimo control field and CSI report field
 *     ness_a: Local NESS;
 *     ness_b: Rx NESS;
 *     bandwidth: Bandwidth of this calibration.
 *         1: 40 MHz, 2: 20 M lower, 3: 20M upper.
 * output: false : calibration fails.
 *         true  : success.
 */
bool
ar9300_txbf_rc_update(
    struct ath_hal *ah,
    struct ath_rx_status *rx_status,
    u_int8_t *local_h,
    u_int8_t *csi_frame,
    u_int8_t ness_a,
    u_int8_t ness_b,
    int bandwidth)
{
    u_int8_t  ntx_a, nrx_a, ntx_b, nrx_b;
    u_int16_t i, k, num_tones;

    bool      okay;

    COMPLEX   (*h_ba_eff)[MAX_STREAMS][TONE_40M];
    COMPLEX   (*h_eff_quan)[MAX_STREAMS][TONE_40M];
    COMPLEX   (*ka)[TONE_40M];
    u_int8_t  m_h[TONE_40M], snr[MAX_STREAMS];
    u_int8_t  nc, nr, bits_left, nc_idx, nr_idx, ng, nb;
    u_int32_t bitmask, idx, current_data, rc_tmp;
    int       local_h_length;
    u_int16_t mimo_control;
    char      evm[MAX_PILOTS][MAX_STREAMS];
    u_int32_t tmp;
    u_int16_t j;
    char      count, sum;

    tmp = OS_REG_READ(ah, AR_PHY_GEN_CTRL);
    #if 0
    HDPRINTF(AH_NULL, HAL_DBG_UNMASKABLE,
        "==>%s: Current channel flag %x, BB reg setting %x, Rx flag %x\n",
        __func__, chan->channel_flags, tmp, rx_status->rs_flags);
    HDPRINTF(AH_NULL, HAL_DBG_UNMASKABLE,
        "==>%s: Rx evm: %x %x %x %x %x \n",
        __func__, rx_status->evm0, rx_status->evm1,
        rx_status->evm2, rx_status->evm3, rx_status->evm4);
    #endif

    if (rx_status->rs_flags & HAL_RX_2040) {
        bandwidth = BW_40M;
    } else {
        if (tmp & AR_PHY_GC_DYN2040_PRI_CH) {
            bandwidth = BW_20M_LOW;
        } else {
            bandwidth = BW_20M_UP;
        }
    }
    /* check Rx EVM first */
    evm[0][0] =  rx_status->evm0        & 0xff;
    evm[0][1] = (rx_status->evm0 >>  8) & 0xff;
    evm[0][2] = (rx_status->evm0 >> 16) & 0xff;
    evm[1][0] = (rx_status->evm0 >> 24) & 0xff;

    evm[1][1] =  rx_status->evm1        & 0xff;
    evm[1][2] = (rx_status->evm1 >>  8) & 0xff;
    evm[2][0] = (rx_status->evm1 >> 16) & 0xff;
    evm[2][1] = (rx_status->evm1 >> 24) & 0xff;

    evm[2][2] =  rx_status->evm2        & 0xff;
    evm[3][0] = (rx_status->evm2 >>  8) & 0xff;
    evm[3][1] = (rx_status->evm2 >> 16) & 0xff;
    evm[3][2] = (rx_status->evm2 >> 24) & 0xff;
 
    evm[4][0] =  rx_status->evm3        & 0xff;
    evm[4][1] = (rx_status->evm3 >>  8) & 0xff;
    evm[4][2] = (rx_status->evm3 >> 16) & 0xff;
    evm[5][0] = (rx_status->evm3 >> 24) & 0xff;

    evm[5][1] =  rx_status->evm4        & 0xff;
    evm[5][2] = (rx_status->evm4 >>  8) & 0xff;

    #if 0
    HDPRINTF(AH_NULL, HAL_DBG_UNMASKABLE, "==>%s:", __func__);
    #endif

    for (i = 0; i < MAX_PILOTS; i++) {
        #if 0
        HDPRINTF(AH_NULL, HAL_DBG_UNMASKABLE, "EVM%d ", i);
        #endif
        sum = count = 0;
        for (j = 0; j < MAX_STREAMS; j++) {
            #if 0
            HDPRINTF(AH_NULL, HAL_DBG_UNMASKABLE,
                "stream%d %d ,", j, evm[i][j]);
            #endif
            if (evm[i][j] != EVM_MIN) {
                if (evm[i][j] != 0) {
                    #if 0
                    HDPRINTF(AH_NULL, HAL_DBG_UNMASKABLE,
                        "stream%d %d ,", j, evm[i][j]);
                    #endif
                    sum += evm[i][j];
                    count++;
                }
            }
            #if 0
            HDPRINTF(AH_NULL, HAL_DBG_UNMASKABLE, "sum %d ;", sum);
            #endif
        }
        if (count != 0) {
            sum /= count;
            if (sum < EVM_TH) {
                #if 0
                HDPRINTF(AH_NULL, HAL_DBG_UNMASKABLE,
                    "==>%s:RC calculation fail!! EVM too small!! "
                    "sum %d, count %d\n",
                    __func__, sum, count);
                #endif
                return false;
            }
        }
    }
    #if 0
    HDPRINTF(AH_NULL, HAL_DBG_UNMASKABLE, "\n");
    #endif

    ntx_a = ar9300_get_ntx(ah);
    nrx_a = ar9300_get_nrx(ah);

    num_tones = (bandwidth == BW_40M) ? TONE_40M : TONE_20M;
    local_h_length = rx_status->rs_datalen;
#if DEBUG_WIN_RC
    HDPRINTF(AH_NULL, HAL_DBG_UNMASKABLE,
        "==>%s: local H:length %d", __func__, local_h_length);
    for (i = 0; i < (local_h_length / 9); i++) {
        HDPRINTF(AH_NULL, HAL_DBG_UNMASKABLE,
            "%#x, %#x, %#x, %#x, %#x, %#x, %#x, %#x, %#x,\n",
            local_h[9 * i], local_h[9 * i + 1], local_h[9 * i + 2],
            local_h[9 * i + 3], local_h[9 * i + 4], local_h[9 * i + 5],
            local_h[9 * i + 6], local_h[9 * i + 7], local_h[9 * i + 8]);
    }
    HDPRINTF(AH_NULL, HAL_DBG_UNMASKABLE, "\n");
#endif
#if DEBUG_RC
    HDPRINTF(AH_NULL, HAL_DBG_UNMASKABLE,
        "==>%s: local H:length %d", __func__, local_h_length);
    for (i = 0; i < local_h_length; i++) {
        if (i % 16 == 0) {
            HDPRINTF(AH_NULL, HAL_DBG_UNMASKABLE, "\n");
        }
        HDPRINTF(AH_NULL, HAL_DBG_UNMASKABLE, " %#x,", local_h[i]);
    }
    HDPRINTF(AH_NULL, HAL_DBG_UNMASKABLE, "\n");
#endif

    nr = nrx_a;

    /* total bits = 20 * nr * nc * num_tones */
    nc = (local_h_length * BITS_PER_BYTE) / 
            (BITS_PER_COMPLEX_SYMBOL * nr * num_tones);
    ntx_b = nc;
    if ((nc < 1) || (nc > MAX_STREAMS)) {
        return false;
    }

    /*
     * Get CSI2 from CSI report.
     * Get required parameters from the CSI frame's MIMO control field.
     */
    mimo_control = csi_frame[0] + (csi_frame[1] << 8);
    nr = MS(mimo_control, AR_nr_idx) + 1;
    nrx_b = nr;
    nc = (mimo_control & AR_nc_idx) + 1;

    if (nc != ntx_a) {
        /* Bfee, BFer mismatched */
        #if 0
        HDPRINTF(AH_NULL, HAL_DBG_UNMASKABLE,
            "==>%s:Nc %d, Ntx_a %d, mismatched\n",
            __func__, nc, ntx_a);
        #endif
        return false;
    }
    if ((nr > MAX_STREAMS) || (nc > MAX_STREAMS)) {
        #if 0
        HDPRINTF(AH_NULL, HAL_DBG_UNMASKABLE,
            "==>%s: Nr %d, Nc %d >3 ",
            __func__, nr, nc);
        #endif
        return false; /* out of bounds */
    }
    ng = MS(mimo_control, AR_ng);
    /* confirm that there's no group */
    if (ng != 0) {
        #if 0
        HDPRINTF(AH_NULL, HAL_DBG_UNMASKABLE,
            "==>%s: Ng %d, !=0 ", __func__, ng);
        #endif
        return false;
    }
    switch (MS(mimo_control, AR_nb)) {
    case 0:
        nb = 4;
        break;
    case 1:
        nb = 5;
        break;
    case 2:
        nb = 6;
        break;
    case 3:
        nb = 8;
        break;
    default:
        nb = 8;
        break;
    }

    h_ba_eff = ath_hal_malloc(ah, MAX_STREAMS * MAX_STREAMS * 
                TONE_40M * sizeof(COMPLEX));
    h_eff_quan = ath_hal_malloc(ah, MAX_STREAMS * MAX_STREAMS * 
                    TONE_40M * sizeof(COMPLEX));
    ka = ath_hal_malloc(ah, MAX_STREAMS * TONE_40M * sizeof(COMPLEX));   
    /* get CSI1 from local H */
    ar9300_get_local_h(ah, local_h, local_h_length, num_tones, h_ba_eff);

    idx = 6; /* CSI report field from byte 6 */

    for (i = 0; i < nr; i++) {
        snr[i] = csi_frame[idx++];
    }
    bits_left = 8;
    current_data = csi_frame[idx++];
    bitmask = (1 << nb) - 1;
    for (k = 0; k < num_tones; k++) {
        /* need 3 bits for the magnitude */
        if ((bits_left - 3) < 0) {
            /* get the next 8 bits */
            current_data += csi_frame[idx++] << bits_left;
            bits_left += 8;
        }
        /* extract the 3-bit magnitude and shift out the used bits */
        m_h[k] = current_data & ((1 << 3) - 1);
        bits_left -= 3;
        current_data = current_data >> 3;

        for (nr_idx = 0; nr_idx < nr; nr_idx++) {
            for (nc_idx = 0; nc_idx < nc; nc_idx++) {
                if ((bits_left - nb) < 0) {
                    /* get the next 8 bits */
                    current_data += csi_frame[idx++] << bits_left;
                    bits_left += 8;
                }
                /* extract nb bits and shift out the used bits */
                h_eff_quan[nr_idx][nc_idx][k].real = current_data & bitmask;

                h_eff_quan[nr_idx][nc_idx][k].real = 
                    ar9300_signbit_convert(h_eff_quan[nr_idx][nc_idx][k].real,
                         BITS_PER_BYTE);

                bits_left -= nb;
                current_data = current_data >> nb;

                if ((bits_left - nb) < 0) {
                    /* get the next 8 bits */
                    current_data += csi_frame[idx++] << bits_left;
                    bits_left += 8;
                }
                /* extract nb bits and shift out the used bits */
                h_eff_quan[nr_idx][nc_idx][k].imag = current_data & bitmask;
                h_eff_quan[nr_idx][nc_idx][k].imag = 
                    ar9300_signbit_convert(h_eff_quan[nr_idx][nc_idx][k].imag,
                         BITS_PER_BYTE);

                bits_left -= nb;
                current_data = current_data >> nb;
            }
        }
    }
#if DEBUG_WIN_RC
    HDPRINTF(AH_NULL, HAL_DBG_UNMASKABLE,
        "==>%s: CSI report, length %d ", __func__, idx);
    i = 0;
    while ((i + 16) < idx) {
        HDPRINTF(AH_NULL, HAL_DBG_UNMASKABLE,
            " %#x, %#x, %#x, %#x, %#x, %#x, %#x, %#x, "
            "%#x, %#x, %#x, %#x, %#x, %#x, %#x, %#x,\n",
            csi_frame[i], csi_frame[i + 1], csi_frame[i + 2],
            csi_frame[i + 3], csi_frame[i + 4], csi_frame[i + 5],
            csi_frame[i + 6], csi_frame[i + 7], csi_frame[i + 8],
            csi_frame[i + 9], csi_frame[i + 10], csi_frame[i + 11],
            csi_frame[i + 12], csi_frame[i + 13], csi_frame[i + 14],
            csi_frame[i + 15]);
        i += 16;
    }
    while (i < idx) {
        HDPRINTF(AH_NULL, HAL_DBG_UNMASKABLE, " %#x,", csi_frame[i]);
        i++;
    }
    HDPRINTF(AH_NULL, HAL_DBG_UNMASKABLE, "\n");
#endif
#if DEBUG_RC
    HDPRINTF(AH_NULL, HAL_DBG_UNMASKABLE,
        "==>%s: CSI report, length %d ", __func__, idx);
    for (i = 0; i < idx; i++) {
        if (i % 16 == 0) {
            HDPRINTF(AH_NULL, HAL_DBG_UNMASKABLE, "\n");
        }
        HDPRINTF(AH_NULL, HAL_DBG_UNMASKABLE, " %#x,", csi_frame[i]);
    }
    HDPRINTF(AH_NULL, HAL_DBG_UNMASKABLE, "\n");
#endif

    OS_MEMZERO(ka, sizeof(ka));
    #if 0
    HDPRINTF(AH_NULL, HAL_DBG_UNMASKABLE,
        "==>%s, Ntx_A %x, Nrx_A %x, Ntx_B %x, Nrx_B %x, "
        "NESSA %x, NESSB %x, BW %x \n",
        __func__, ntx_a, nrx_a, ntx_b, nrx_b, ness_a, ness_b, bandwidth);
    #endif
    okay = ka_calculation(
        ah, ntx_a, nrx_a, ntx_b, nrx_b,
        h_ba_eff, h_eff_quan, m_h, ka, ness_a, ness_b, bandwidth);
    if (okay != true) {
        goto fail;
    }

#if (DEBUG_RC | DEBUG_WIN_RC)
    HDPRINTF(AH_NULL, HAL_DBG_UNMASKABLE, "==>%s ka: \n", __func__);
    for (i = 0; i < num_tones; i++) {
        HDPRINTF(AH_NULL, HAL_DBG_UNMASKABLE,
            "          %4d %4d, %4d %4d,%4d %4d;\n",
            ka[0][i].real, ka[0][i].imag,
            ka[1][i].real, ka[1][i].imag,
            ka[2][i].real, ka[2][i].imag);
    }
#endif
    #if 0
    HDPRINTF(AH_NULL, HAL_DBG_UNMASKABLE,
        "==>%s: BW is %d \n", __func__, bandwidth);
    #endif
    if (bandwidth == BW_40M) {
        /* at the start, clear 40M done */
        rc_tmp = OS_REG_READ(ah, AR_TXBF);
        rc_tmp &= (~AR_TXBF_RC_40_DONE);
        OS_REG_WRITE(ah, AR_TXBF, rc_tmp);

        idx = 2 * 4;  /* skip -60,-59 */
        for (i = 0; i < TONE_40M; i++) {
            rc_tmp =
                (ka[0][i].real & 0xff)        |
               ((ka[0][i].imag & 0xff) <<  8) |
               ((ka[1][i].real & 0xff) << 16) |
               ((ka[1][i].imag & 0xff) << 24);
            OS_REG_WRITE(ah, AR_RC0(idx), rc_tmp);
            #if 0
            HDPRINTF(AH_NULL, HAL_DBG_UNMASKABLE,
                "tone %d: RC0 %x,", i, rc_tmp);
            #endif
            rc_tmp = OS_REG_READ(ah, AR_RC0(idx));
            #if 0
            HDPRINTF(AH_NULL, HAL_DBG_UNMASKABLE,
                "read back address %x value %x,", AR_RC0(idx), rc_tmp);
            #endif

            rc_tmp = (ka[2][i].real & 0xff) | ((ka[2][i].imag & 0xff) << 8);
            OS_REG_WRITE(ah, AR_RC1(idx), rc_tmp);

            #if 0
            HDPRINTF(AH_NULL, HAL_DBG_UNMASKABLE, " RC1 %x,", rc_tmp);
            #endif
            rc_tmp = OS_REG_READ(ah, AR_RC1(idx));
            #if 0
            HDPRINTF(AH_NULL, HAL_DBG_UNMASKABLE,
                "read back address %x value %x\n", AR_RC1(idx), rc_tmp);
            #endif

            idx += 4;
        }

        /* at the end, set 40M done */
        rc_tmp = OS_REG_READ(ah, AR_TXBF);
        rc_tmp |= AR_TXBF_RC_40_DONE;
        OS_REG_WRITE(ah, AR_TXBF, rc_tmp);
    } else if (bandwidth == BW_20M_UP) {
        /* at the start, clear upper 20M done */
        rc_tmp = OS_REG_READ(ah, AR_TXBF);
        rc_tmp &= (~AR_TXBF_RC_20_L_DONE);
        OS_REG_WRITE(ah, AR_TXBF, rc_tmp);

        idx = 61 * 4; /* 58+3 ; place from tone +4 to +60 */
        for (i = 0; i < TONE_20M / 2; i++) {
            rc_tmp =
                 (ka[0][i].real & 0xff)        |
                ((ka[0][i].imag & 0xff) <<  8) |
                ((ka[1][i].real & 0xff) << 16) |
                ((ka[1][i].imag & 0xff) << 24);
            OS_REG_WRITE(ah, AR_RC0(idx), rc_tmp);

            #if 0
            HDPRINTF(AH_NULL, HAL_DBG_UNMASKABLE,
                "tone %d: RC0 %x,", i, rc_tmp);
            #endif
            rc_tmp = OS_REG_READ(ah, AR_RC0(idx));
            #if 0
            HDPRINTF(AH_NULL, HAL_DBG_UNMASKABLE,
                "read back address %x value %x,", AR_RC0(idx), rc_tmp);
            #endif

            rc_tmp = (ka[2][i].real & 0xff) | ((ka[2][i].imag & 0xff) << 8);
            OS_REG_WRITE(ah, AR_RC1(idx), rc_tmp);

            #if 0
            HDPRINTF(AH_NULL, HAL_DBG_UNMASKABLE, " RC1 %x,", rc_tmp);
            #endif
            rc_tmp = OS_REG_READ(ah, AR_RC1(idx));
            #if 0
            HDPRINTF(AH_NULL, HAL_DBG_UNMASKABLE,
                "read back address %x value %x\n", AR_RC1(idx), rc_tmp);
            #endif

            idx += 4;
        }
        idx = 90 * 4;
        k = i;
        for (i = k; i < (TONE_20M / 2 + k); i++) {
            rc_tmp =
                 (ka[0][i].real & 0xff)        |
                ((ka[0][i].imag & 0xff) <<  8) |
                ((ka[1][i].real & 0xff) << 16) |
                ((ka[1][i].imag & 0xff) << 24);
            OS_REG_WRITE(ah, AR_RC0(idx), rc_tmp);
            #if 0
            HDPRINTF(AH_NULL, HAL_DBG_UNMASKABLE,
                "tone %d: RC0 %x,", i, rc_tmp);
            #endif
            rc_tmp = OS_REG_READ(ah, AR_RC0(idx));
            #if 0
            HDPRINTF(AH_NULL, HAL_DBG_UNMASKABLE,
                "read back address %x value %x,", AR_RC0(idx), rc_tmp);
            #endif

            rc_tmp = (ka[2][i].real & 0xff) | ((ka[2][i].imag & 0xff) << 8);
            OS_REG_WRITE(ah, AR_RC1(idx), rc_tmp);

            #if 0
            HDPRINTF(AH_NULL, HAL_DBG_UNMASKABLE, "RC1 %x,", rc_tmp);
            #endif
            rc_tmp = OS_REG_READ(ah, AR_RC1(idx));
            #if 0
            HDPRINTF(AH_NULL, HAL_DBG_UNMASKABLE,
                "read back address %x value %x \n", AR_RC1(idx), rc_tmp);
            #endif

            idx += 4;
        }
        /* at the end, set upper 20M done */
        rc_tmp = OS_REG_READ(ah, AR_TXBF);
        rc_tmp |= AR_TXBF_RC_20_U_DONE;
        OS_REG_WRITE(ah, AR_TXBF, rc_tmp);
    } else if (bandwidth == BW_20M_LOW) {
        /* at the start, clear lower 20M done */
        rc_tmp = OS_REG_READ(ah, AR_TXBF);
        rc_tmp &= (~AR_TXBF_RC_20_L_DONE);
        OS_REG_WRITE(ah, AR_TXBF, rc_tmp);

        idx = 0;
        for (i = 0; i < TONE_20M / 2; i++) {
            rc_tmp =
                 (ka[0][i].real & 0xff)        |
                ((ka[0][i].imag & 0xff) <<  8) |
                ((ka[1][i].real & 0xff) << 16) |
                ((ka[1][i].imag & 0xff) << 24);
            OS_REG_WRITE(ah, AR_RC0(idx), rc_tmp);
            #if 0 
            HDPRINTF(AH_NULL, HAL_DBG_UNMASKABLE,
                "tone %d: RC0 %x,", i, rc_tmp);
            #endif
            rc_tmp = OS_REG_READ(ah, AR_RC0(idx));
            #if 0
            HDPRINTF(AH_NULL, HAL_DBG_UNMASKABLE,
                "read back address %x value %x,", AR_RC0(idx), rc_tmp);
            #endif

            rc_tmp = (ka[2][i].real & 0xff) | ((ka[2][i].imag & 0xff) << 8);
            OS_REG_WRITE(ah, AR_RC1(idx), rc_tmp);

            #if 0
            HDPRINTF(AH_NULL, HAL_DBG_UNMASKABLE, " RC1 %x,", rc_tmp);
            #endif
            rc_tmp = OS_REG_READ(ah, AR_RC1(idx));
            #if 0
            HDPRINTF(AH_NULL, HAL_DBG_UNMASKABLE,
                "read back address %x value %x\n", AR_RC1(idx), rc_tmp);
            #endif

            idx += 4;
        }
        idx = 29 * 4;
        k = i;
        for (i = k; i < (TONE_20M / 2 + k); i++) {
            rc_tmp =
                 (ka[0][i].real & 0xff)        |
                ((ka[0][i].imag & 0xff) <<  8) |
                ((ka[1][i].real & 0xff) << 16) |
                ((ka[1][i].imag & 0xff) << 24);
            OS_REG_WRITE(ah, AR_RC0(idx), rc_tmp);
            #if 0
            HDPRINTF(AH_NULL, HAL_DBG_UNMASKABLE,
                "tone %d: RC0 %x,", i, rc_tmp);
            #endif
            rc_tmp = OS_REG_READ(ah, AR_RC0(idx));
            #if 0
            HDPRINTF(AH_NULL, HAL_DBG_UNMASKABLE,
                "read back address %x value %x,", AR_RC0(idx), rc_tmp);
            #endif
            rc_tmp = (ka[2][i].real & 0xff) | ((ka[2][i].imag & 0xff) << 8);
            OS_REG_WRITE(ah, AR_RC1(idx), rc_tmp);

            #if 0
            HDPRINTF(AH_NULL, HAL_DBG_UNMASKABLE, "RC1 %x,", rc_tmp);
            #endif
            rc_tmp = OS_REG_READ(ah, AR_RC1(idx));
            #if 0
            HDPRINTF(AH_NULL, HAL_DBG_UNMASKABLE,
                "read back address %x value %x \n", AR_RC1(idx), rc_tmp);
            #endif

            idx += 4;
        }
        /* at the end, set lower 20M done */
        rc_tmp = OS_REG_READ(ah, AR_TXBF);
        rc_tmp |= AR_TXBF_RC_20_L_DONE;
        OS_REG_WRITE(ah, AR_TXBF, rc_tmp);
    }
    idx = 0;
    #if 0
    HDPRINTF(AH_NULL, HAL_DBG_UNMASKABLE,
        "==>%s:RC read back for 120", __func__);
    #endif
    #if 0
    for (i = 0; i < 120; i++) {
        if (i % 4 == 0) {
            HDPRINTF(AH_NULL, HAL_DBG_UNMASKABLE, "\n");
        }
        rc_tmp = OS_REG_READ(ah, AR_RC0(idx));
        HDPRINTF(AH_NULL, HAL_DBG_UNMASKABLE,
            " addr %x:%x", AR_RC0(idx), rc_tmp);
        rc_tmp = OS_REG_READ(ah, AR_RC1(idx));
        HDPRINTF(AH_NULL, HAL_DBG_UNMASKABLE,
            " addr %x:%x;", AR_RC1(idx), rc_tmp);
        idx += 4;
    }
    HDPRINTF(AH_NULL, HAL_DBG_UNMASKABLE, "\n");
    #endif

fail:
    ath_hal_free(ah, h_ba_eff);
    ath_hal_free(ah, h_eff_quan);
    ath_hal_free(ah, ka);
    return okay;
}

/*
 * function: ar9300_fill_csi_frame
 * purpose: Use local_h and Rx status report to form MIMO control and CSI
 *     report field of CSI action frame.
 * input:
 *     ah:
 *     rx_status: rx status report of received pos_3 frame
 *     bandwidth: bandwitdth:1:40 M, 0:20M
 *     local_h: pointer to RXDP+12. Which is the beginning point of h
 *         reported by hardware.
 *     local_h_length: length of h;
 *     csi_frame_body: point to a buffer for CSI frame;
 * output: int: CSI framelength
 */
int
ar9300_fill_csi_frame(
    struct ath_hal *ah,
    struct ath_rx_status *rx_status,
    u_int8_t bandwidth,
    u_int8_t *local_h,
    u_int8_t *csi_frame_body)
{
    u_int8_t    nc, nr, nr_idx, nc_idx;
    u_int32_t   idx = 0;
    u_int8_t    bit_idx = 0;
    u_int16_t   mimo_control, num_tones, i;
    u_int16_t   current_data = 0;

    COMPLEX     (*h)[MAX_STREAMS][TONE_40M];
    u_int8_t    *m_h;
    int         local_h_length;

    local_h_length = rx_status->rs_datalen;
    if (rx_status->rs_flags & HAL_RX_2040) {
        bandwidth = BW_40M;
    }
#if DEBUG_WIN_RC
    HDPRINTF(AH_NULL, HAL_DBG_UNMASKABLE, "==>%s: original H: \n", __func__);
    for (i = 0; i < (local_h_length / 9); i++) {
        HDPRINTF(AH_NULL, HAL_DBG_UNMASKABLE,
            "%#x, %#x, %#x, %#x, %#x, %#x, %#x, %#x, %#x,\n",
            local_h[i], local_h[i + 1], local_h[i + 2], local_h[i + 3],
            local_h[i + 4], local_h[i + 5], local_h[i + 6],
            local_h[i + 7], local_h[i + 8], local_h[i + 9]);
    }
#endif

    nr = ar9300_get_nrx(ah);
    num_tones = (bandwidth == BW_40M) ?  TONE_40M : TONE_20M;

    /* total bits = 20 * nr * nc * num_tones */
    nc = (local_h_length * BITS_PER_BYTE) /
             (BITS_PER_COMPLEX_SYMBOL * nr * num_tones);
    if ((nc < 1) || (nc > MAX_STREAMS)) {
        return 0;
    }

    h = ath_hal_malloc(ah, MAX_STREAMS * MAX_STREAMS * TONE_40M *
            sizeof(COMPLEX));
    m_h = ath_hal_malloc(ah, TONE_40M);

    mimo_control =
        SM((nc - 1), AR_nc_idx)    |
        SM((nr - 1), AR_nr_idx)    |
        SM(bandwidth, AR_bandwith) |
        SM(3, AR_nb);
    csi_frame_body[idx++] = mimo_control & 0xff;
    csi_frame_body[idx++] = (mimo_control >> 8) & 0xff;
    /* rx sounding timestamp */
    csi_frame_body[idx++] =  rx_status->rs_tstamp        & 0xff;
    csi_frame_body[idx++] = (rx_status->rs_tstamp >> 8)  & 0xff;
    csi_frame_body[idx++] = (rx_status->rs_tstamp >> 16) & 0xff;
    csi_frame_body[idx++] = (rx_status->rs_tstamp >> 24) & 0xff;
    /* rx chain rssi */
    csi_frame_body[idx++] = rx_status->rs_rssi_ctl0;
    if (nr > 1) {
        csi_frame_body[idx++] = rx_status->rs_rssi_ctl1;
    }
    if (nr > 2) {
        csi_frame_body[idx++] = rx_status->rs_rssi_ctl2;
    }
    ar9300_get_local_h(ah, local_h, local_h_length, num_tones, h);
    h_to_csi(ah, nc, nr, h, m_h, num_tones);
    current_data = bit_idx = 0;
    for (i = 0; i < num_tones; i++) {
        /* add 3 bits for m_h */
        current_data += (m_h[i] & 0x7) << bit_idx;
        bit_idx += 3;
        if (bit_idx >= 8) {
            csi_frame_body[idx++] = current_data & 0xff;
            current_data >>= 8;
            bit_idx -= 8;
        }
        for (nr_idx = 0; nr_idx < nr; nr_idx++) {
            for (nc_idx = 0; nc_idx < nc; nc_idx++) {
                /* get a new byte of data from the real component */
                current_data += (h[nr_idx][nc_idx][i].real & 0xff) << bit_idx;
                bit_idx += 8;
                /* form the next byte of the message */
                csi_frame_body[idx++] = current_data & 0xff;
                current_data >>= 8;
                bit_idx -= 8;

                /* get a new byte of data from the imaginary component */
                current_data += (h[nr_idx][nc_idx][i].imag & 0xff) << bit_idx;
                bit_idx += 8;
                /* form the next byte of the message */
                csi_frame_body[idx++] = current_data & 0xff;
                current_data >>= 8;
                bit_idx -= 8;
            }
        }
    }
    ath_hal_free(ah, h);
    ath_hal_free(ah, m_h);
    return idx;
}
#endif

void
ar9300_fill_txbf_capabilities(struct ath_hal *ah)
{
    struct ath_hal_9300     *ahp = AH9300(ah);
    HAL_TXBF_CAPS           *txbf = &ahp->txbf_caps;
    struct ath_hal_private  *ahpriv = AH_PRIVATE(ah);
    u_int32_t val;
    u_int8_t txbf_ctl;

    OS_MEMZERO(txbf, sizeof(HAL_TXBF_CAPS));
    if (ahpriv->ah_config.ath_hal_txbf_ctl == 0) {
        /* doesn't support tx_bf, let txbf ie = 0*/
        return;
    }
    /* CEC for osprey is always 1 (2 stream) */ 
    txbf->channel_estimation_cap = 1;

    /* For calibration, always 2 (3 stream) for osprey */
    txbf->csi_max_rows_bfer = 2;
    
    /*
     * Compressed Steering Number of Beamformer Antennas Supported is
     * limited by local's antenna
     */
    txbf->comp_bfer_antennas = ar9300_get_ntx(ah)-1;
    
    /*
     * Compressed Steering Number of Beamformer Antennas Supported
     * is limited by local's antenna
     */
    txbf->noncomp_bfer_antennas = ar9300_get_ntx(ah)-1;
    
    /* NOT SUPPORT CSI */
    txbf->csi_bfer_antennas = 0;
    
    /* 1, 2, 4 group is supported */
    txbf->minimal_grouping = ALL_GROUP;
   
    /* Explicit compressed Beamforming Feedback Capable */
    if (ahpriv->ah_config.ath_hal_txbf_ctl & TXBF_CTL_COMP_EX_BF_DELAY_RPT) {
        /* support delay report by settings */
        txbf->explicit_comp_bf |= Delay_Rpt;
    }
    if (ahpriv->ah_config.ath_hal_txbf_ctl &
        TXBF_CTL_COMP_EX_BF_IMMEDIATELY_RPT)
    {
        /* support immediately report by settings.*/
        txbf->explicit_comp_bf |= Immediately_Rpt;
    }

    /* Explicit non-Compressed Beamforming Feedback Capable */
    if (ahpriv->ah_config.ath_hal_txbf_ctl & TXBF_CTL_NON_EX_BF_DELAY_RPT) {
        txbf->explicit_noncomp_bf |= Delay_Rpt;
    }
    if (ahpriv->ah_config.ath_hal_txbf_ctl &
        TXBF_CTL_NON_EX_BF_IMMEDIATELY_RPT)
    {
        txbf->explicit_noncomp_bf |= Immediately_Rpt;
    }

    /* not support csi feekback */
    txbf->explicit_csi_feedback = 0; 

    /* Explicit compressed Steering Capable from settings */
    txbf->explicit_comp_steering =
        MS(ahpriv->ah_config.ath_hal_txbf_ctl, TXBF_CTL_COMP_EX_BF);
        
    /* Explicit Non-compressed Steering Capable from settings */
    txbf->explicit_noncomp_steering =
        MS(ahpriv->ah_config.ath_hal_txbf_ctl, TXBF_CTL_NON_EX_BF);
    
    /* not support CSI */
    txbf->explicit_csi_txbf_capable = false; 
    
#ifdef TXBF_TODO
    /* initial and respond calibration */
    txbf->calibration = INIT_RESP_CAL;
    
    /* set implicit by settings */
    txbf->implicit_txbf_capable = MS(ahpriv->ah_config.ath_hal_txbf_ctl, 
        TXBF_CTL_IM_BF);
    txbf->implicit_rx_capable = MS(ahpriv->ah_config.ath_hal_txbf_ctl, 
        TXBF_CTL_IM_BF_FB);
#else
    /* not support imbf and calibration */
    txbf->calibration = NO_CALIBRATION;
    txbf->implicit_txbf_capable = false;
    txbf->implicit_rx_capable  = false;
#endif
    /* not support NDP */
    txbf->tx_ndp_capable = false;    
    txbf->rx_ndp_capable = false;
    
    /* support stagger sounding. */
    txbf->tx_staggered_sounding = true;  
    txbf->rx_staggered_sounding = true;

    /* set immediately or delay report to H/W */
    val = OS_REG_READ(ah, AR_H_XFER_TIMEOUT);
    txbf_ctl = ahpriv->ah_config.ath_hal_txbf_ctl;
    if (((txbf_ctl & TXBF_CTL_NON_EX_BF_IMMEDIATELY_RPT) == 0) &&
        ((txbf_ctl & TXBF_CTL_COMP_EX_BF_IMMEDIATELY_RPT) == 0)) {
        /* enable delayed report */
        val &= ~(AR_EXBF_IMMDIATE_RESP);
        val |= AR_EXBF_NOACK_NO_RPT;
        OS_REG_WRITE(ah, AR_H_XFER_TIMEOUT, val);
    } else {
        /* enable immediately report */
        val |= AR_EXBF_IMMDIATE_RESP;
        val &= ~(AR_EXBF_NOACK_NO_RPT);
        OS_REG_WRITE(ah, AR_H_XFER_TIMEOUT, val);
    }
    #if 0
    HDPRINTF(AH_NULL, HAL_DBG_UNMASKABLE,
        "==>%sTxBfCtl= %x \n", __func__, ahpriv->ah_config.ath_hal_txbf_ctl);
    HDPRINTF(AH_NULL, HAL_DBG_UNMASKABLE,
        "==>%s:Compress ExBF= %x FB %x\n",
        __func__, txbf->explicit_comp_steering, txbf->explicit_comp_bf);
    HDPRINTF(AH_NULL, HAL_DBG_UNMASKABLE,
        "==>%s:NonCompress ExBF= %x FB %x\n",
        __func__, txbf->explicit_noncomp_steering, txbf->explicit_noncomp_bf);
    HDPRINTF(AH_NULL, HAL_DBG_UNMASKABLE,
        "==>%s:ImBF= %x FB %x\n",
        __func__, txbf->implicit_txbf_capable, txbf->implicit_rx_capable);
    #endif
}

HAL_TXBF_CAPS *
ar9300_get_txbf_capabilities(struct ath_hal *ah)
{
    return &(AH9300(ah)->txbf_caps);
}

/*
 * ar9300_txbf_set_key is used to set TXBF related field in key cache.
 */
void ar9300_txbf_set_key(
    struct ath_hal *ah,
    u_int16_t entry,
    u_int8_t rx_staggered_sounding,
    u_int8_t channel_estimation_cap,
    u_int8_t mmss)
{
    u_int32_t tmp, txbf;

    /* 1 for 2 stream, 0 for 1 stream, should add 1 for H/W */
    channel_estimation_cap += 1;

    txbf = (
        SM(rx_staggered_sounding, AR_KEYTABLE_STAGGED) |
        SM(channel_estimation_cap, AR_KEYTABLE_CEC)    |
        SM(mmss, AR_KEYTABLE_MMSS));
    tmp = OS_REG_READ(ah, AR_KEYTABLE_TYPE(entry));
    if (txbf !=
        (tmp & (AR_KEYTABLE_STAGGED | AR_KEYTABLE_CEC | AR_KEYTABLE_MMSS))) {
        /* update key cache for txbf */
        OS_REG_WRITE(ah, AR_KEYTABLE_TYPE(entry), tmp | txbf);
        #if 0
        HDPRINTF(AH_NULL, HAL_DBG_UNMASKABLE,
            "==>%s: update keyid %d, value %x, orignal %x\n",
            __func__, entry, txbf, tmp);
        #endif
    }
    #if 0
    else {
        HDPRINTF(AH_NULL, HAL_DBG_UNMASKABLE,
        "==>%s: parameters no changes : %x, don't need updtate key!\n",
        __func__, tmp);
    }
    #endif
}

u_int32_t
ar9300_read_cv_cache(struct ath_hal *ah, u_int32_t addr)
{
    u_int32_t tmp, value;

    OS_REG_WRITE(ah, addr, AR_CVCACHE_RD_EN);
    do {
        tmp = OS_REG_READ(ah, AR_TXBF_SW);
    } while ((tmp & AR_LRU_RD_ACK) == 0);
    
    value = OS_REG_READ(ah, addr);
    tmp &= ~(AR_LRU_RD_ACK);
    OS_REG_WRITE(ah, AR_TXBF_SW, tmp);
    return (value & AR_CVCACHE_DATA);
}

void
ar9300_txbf_get_cv_cache_nr(struct ath_hal *ah, u_int16_t key_idx, u_int8_t *nr)
{
    u_int32_t idx, value;
    u_int8_t  nr_idx;
    
    /* get current CV cache addess offset from key index */
    idx = ar9300_txbf_lru_search(ah, key_idx);
    
    /* read the cvcache header */
    value = ar9300_read_cv_cache(ah, AR_CVCACHE(idx));
    nr_idx = MS(value, AR_CVCACHE_Nr_IDX);  
    *nr = nr_idx + 1;      
}

/*
 * Workaround for HW issue EV [69449] Chip::Osprey HW does not filter 
 * non-directed frame for uploading TXBF delay report 
 */
void 
ar9300_reconfig_h_xfer_timeout(struct ath_hal *ah, bool is_reset)
{
#define DEFAULT_TIMEOUT_VALUE      0xD

    u_int32_t val;

    val = OS_REG_READ(ah, AR_H_XFER_TIMEOUT);

    if (is_reset) {
        val = DEFAULT_TIMEOUT_VALUE + 
            (val & (~AR_H_XFER_TIMEOUT_COUNT));
    } else {
        val = ((val - (1 << AR_H_XFER_TIMEOUT_COUNT_S)) & 
               AR_H_XFER_TIMEOUT_COUNT) + 
            (val & (~AR_H_XFER_TIMEOUT_COUNT));
    }

    OS_REG_WRITE(ah, AR_H_XFER_TIMEOUT, val);
    return;

#undef DEFAULT_TIMEOUT_VALUE
}

/*
 * Limit self-generate frame rate (such as CV report ) by lowest Tx rate to
 * guarantee the success of CV report frame 
 */
void
ar9300_set_selfgenrate_limit(struct ath_hal *ah, u_int8_t rateidx)
{
    struct      ath_hal_private *ahp = AH_PRIVATE(ah);
    u_int32_t   selfgen_rate; 
   
    if (rateidx & HT_RATE){
        rateidx &= ~ (HT_RATE);
        if (rateidx < ahp->ah_lowest_txrate){
            ahp->ah_lowest_txrate = rateidx;
            selfgen_rate = (1 << ((ahp->ah_lowest_txrate) + 1))- 1;
            if (selfgen_rate !=  ahp->ah_basic_set_buf){
                ahp->ah_basic_set_buf = selfgen_rate;
                OS_REG_WRITE(ah, AR_BASIC_SET, ahp->ah_basic_set_buf);
            }
        }
    }
}

/*
 * Reset current lower Tx rate to max MCS 
 */
void
ar9300_reset_lowest_txrate(struct ath_hal *ah)
{
    struct ath_hal_9300 *ahp = AH9300(ah);
    struct ath_hal_private *ahpriv = AH_PRIVATE(ah);

    if (ahp->ah_reduced_self_gen_mask){
        /* limit self gen rate to one stream for BT coex */
        ahpriv->ah_lowest_txrate = MAXONESTREAMRATE;
    }
    else{
        ahpriv->ah_lowest_txrate = MAXMCSRATE;
    }
}   

/*
 * Update basic set according to self gen mask.
 */ 
void
ar9300_txbf_set_basic_set(struct ath_hal *ah)
{
    struct ath_hal_private *ahp = AH_PRIVATE(ah);
    struct ath_hal_9300 *ah9300 = AH9300(ah);

    if (ah9300->ah_reduced_self_gen_mask){
        /* set max rate to mcs7 when self gen mask is single chain. */
        ahp->ah_lowest_txrate = MAXONESTREAMRATE;
        
    }else{
        ahp->ah_lowest_txrate = MAXMCSRATE;
    }
    ahp->ah_basic_set_buf = (1 << (ahp->ah_lowest_txrate+ 1))- 1;
    OS_REG_WRITE(ah, AR_BASIC_SET, ahp->ah_basic_set_buf); 
}

#endif /* ATH_SUPPORT_TxBF*/
#endif /* AH_SUPPORT_AR9300 */
