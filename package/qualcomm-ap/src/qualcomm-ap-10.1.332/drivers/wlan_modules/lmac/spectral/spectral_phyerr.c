/*
 * Copyright (c) 2002-2009 Atheros Communications, Inc.
 * All Rights Reserved.
 *
 * Copyright (c) 2011 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 *
 */

#include <osdep.h>
#include "spectral.h"

#if ATH_SUPPORT_SPECTRAL

/*
 * Function     : print_buf
 * Description  : Prints given buffer for given length
 * Input        : Pointer to buffer and length
 * Output       : Void
 *
 */
void print_buf(u_int8_t* pbuf, int len)
{
    int i = 0;
    for (i = 0; i < len; i++) {
        printk("%02X ", pbuf[i]);
        if (i % 32 == 31) {
            printk("\n");
        }
    }
}

/*
 * Function     : process_search_fft_report
 * Description  : Process Search FFT Report
 * Input        : Pointer to Spectral Phyerr TLV and Length and pointer to search fft info
 * Output       : Success/Failure
 *
 */
int process_search_fft_report(SPECTRAL_PHYERR_TLV* ptlv, int tlvlen, SPECTRAL_SEARCH_FFT_INFO* p_fft_info)
{
    /* For simplicity, everything is defined as uint32_t (except one). Proper code will later use the right sizes. */
    /* For easy comparision between MDK team and OS team, the MDK script variable names have been used */
    uint32_t relpwr_db;
    uint32_t num_str_bins_ib;
    uint32_t base_pwr;
    uint32_t total_gain_info;

    uint32_t fft_chn_idx;
    int16_t peak_inx;
    uint32_t avgpwr_db;
    uint32_t peak_mag;

    uint32_t fft_summary_A;
    uint32_t fft_summary_B;
    u_int8_t *tmp = (u_int8_t*)ptlv;
    SPECTRAL_PHYERR_HDR* phdr = (SPECTRAL_PHYERR_HDR*)(tmp + sizeof(SPECTRAL_PHYERR_TLV));

    /* Relook this */
    if (tlvlen < 8) {
        printk("SPECTRAL : Unexpected TLV length %d for Spectral Summary Report! Hexdump follows\n", tlvlen);
        print_buf((u_int8_t*)ptlv, tlvlen + 4);
        return -1;
    }

    /* Doing copy as the contents may not be aligned */
    OS_MEMCPY(&fft_summary_A, (u_int8_t*)phdr, sizeof(int));
    OS_MEMCPY(&fft_summary_B, (u_int8_t*)((u_int8_t*)phdr + sizeof(int)), sizeof(int));

    relpwr_db       = ((fft_summary_B >>26) & 0x3f);
    num_str_bins_ib = fft_summary_B & 0xff;
    base_pwr        = ((fft_summary_A >> 14) & 0x1ff);
    total_gain_info = ((fft_summary_A >> 23) & 0x1ff);

    fft_chn_idx     = ((fft_summary_A >>12) & 0x3);
    peak_inx        = fft_summary_A & 0xfff;

    if (peak_inx > 2047) {
        peak_inx = peak_inx - 4096;
    }

    avgpwr_db = ((fft_summary_B >> 18) & 0xff);
    peak_mag = ((fft_summary_B >> 8) & 0x3ff);

    /* Populate the Search FFT Info */
    if (p_fft_info) {
        p_fft_info->relpwr_db       = relpwr_db;
        p_fft_info->num_str_bins_ib = num_str_bins_ib;
        p_fft_info->base_pwr        = base_pwr;
        p_fft_info->total_gain_info = total_gain_info;
        p_fft_info->fft_chn_idx     = fft_chn_idx;
        p_fft_info->peak_inx        = peak_inx;
        p_fft_info->avgpwr_db       = avgpwr_db;
        p_fft_info->peak_mag        = peak_mag;
    }

    return 0;
}

/*
 * Function     : dump_adc_report
 * Description  : Dump ADC Reports
 * Input        : Pointer to Spectral Phyerr TLV and Length
 * Output       : Success/Failure
 *
 */
int dump_adc_report(SPECTRAL_PHYERR_TLV* ptlv, int tlvlen)
{
    int i;
    uint32_t *pdata;
    uint32_t data;

    /* For simplicity, everything is defined as uint32_t (except one). Proper code will later use the right sizes. */
    uint32_t samp_fmt;
    uint32_t chn_idx;
    uint32_t recent_rfsat;
    uint32_t agc_mb_gain;
    uint32_t agc_total_gain;

    uint32_t adc_summary;

    u_int8_t* ptmp = (u_int8_t*)ptlv;

    printk("SPECTRAL : ADC REPORT\n");

    /* Relook this */
    if (tlvlen < 4) {
        printk("Unexpected TLV length %d for ADC Report! Hexdump follows\n", tlvlen);
        print_buf((u_int8_t*)ptlv, tlvlen + 4);
        return -1;
    }

    OS_MEMCPY(&adc_summary, (u_int8_t*)(ptlv + 4), sizeof(int));

    samp_fmt= ((adc_summary >> 28) & 0x1);
    chn_idx= ((adc_summary >> 24) & 0x3);
    recent_rfsat = ((adc_summary >> 23) & 0x1);
    agc_mb_gain = ((adc_summary >> 16) & 0x7f);
    agc_total_gain = adc_summary & 0x3ff;

    printk("samp_fmt= %u, chn_idx= %u, recent_rfsat= %u, agc_mb_gain=%u agc_total_gain=%u\n", samp_fmt, chn_idx, recent_rfsat, agc_mb_gain, agc_total_gain);

    for (i=0; i<(tlvlen/4); i++) {
        pdata = (uint32_t*)(ptmp + 4 + i*4);
        data = *pdata;

        /* Interpreting capture format 1 */
        if (1) {
            uint8_t i1;
            uint8_t q1;
            uint8_t i2;
            uint8_t q2;
            int8_t si1;
            int8_t sq1;
            int8_t si2;
            int8_t sq2;


            i1 = data & 0xff;
            q1 = (data >> 8 ) & 0xff;
            i2 = (data >> 16 ) & 0xff;
            q2 = (data >> 24 ) & 0xff;

            if (i1 > 127) {
                si1 = i1 - 256;
            } else {
                si1 = i1;
            }

            if (q1 > 127) {
                sq1 = q1 - 256;
            } else {
                sq1 = q1;
            }

            if (i2 > 127) {
                si2 = i2 - 256;
            } else {
                si2 = i2;
            }

            if (q2 > 127) {
                sq2 = q2 - 256;
            } else {
                sq2 = q2;
            }

            printk("SPECTRAL ADC : Interpreting capture format 1\n");
            printk("adc_data_format_1 # %d %d %d\n", 2*i, si1, sq1);
            printk("adc_data_format_1 # %d %d %d\n", 2*i+1, si2, sq2);
        }

        /* Interpreting capture format 0 */
        if (1) {
            uint16_t i1;
            uint16_t q1;
            int16_t si1;
            int16_t sq1;
            i1 = data & 0xffff;
            q1 = (data >> 16 ) & 0xffff;
            if (i1 > 32767) {
                si1 = i1 - 65536;
            } else {
                si1 = i1;
            }

            if (q1 > 32767) {
                sq1 = q1 - 65536;
            } else {
                sq1 = q1;
            }
            printk("SPECTRAL ADC : Interpreting capture format 0\n");
            printk("adc_data_format_2 # %d %d %d\n", i, si1, sq1);
        }
    }

    printk("\n");

    return 0;
}

/*
 * Function     : dump_search_fft_report
 * Description  : Process Search FFT Report
 * Input        : Pointer to Spectral Phyerr TLV and Length
 * Output       : Success/Failure
 *
 */
int dump_search_fft_report(SPECTRAL_PHYERR_TLV* ptlv, int tlvlen)
{
    int i;
    uint32_t fft_mag;

    /* For simplicity, everything is defined as uint32_t (except one). Proper code will later use the right sizes. */
    /* For easy comparision between MDK team and OS team, the MDK script variable names have been used */
    uint32_t relpwr_db;
    uint32_t num_str_bins_ib;
    uint32_t base_pwr;
    uint32_t total_gain_info;

    uint32_t fft_chn_idx;
    int16_t peak_inx;
    uint32_t avgpwr_db;
    uint32_t peak_mag;

    uint32_t fft_summary_A;
    uint32_t fft_summary_B;
    u_int8_t *tmp = (u_int8_t*)ptlv;
    SPECTRAL_PHYERR_HDR* phdr = (SPECTRAL_PHYERR_HDR*)(tmp + sizeof(SPECTRAL_PHYERR_TLV));

    printk("SPECTRAL : SEARCH FFT REPORT\n");

    /* Relook this */
    if (tlvlen < 8) {
        printk("SPECTRAL : Unexpected TLV length %d for Spectral Summary Report! Hexdump follows\n", tlvlen);
        print_buf((u_int8_t*)ptlv, tlvlen + 4);
        return -1;
    }


    /* Doing copy as the contents may not be aligned */
    OS_MEMCPY(&fft_summary_A, (u_int8_t*)phdr, sizeof(int));
    OS_MEMCPY(&fft_summary_B, (u_int8_t*)((u_int8_t*)phdr + sizeof(int)), sizeof(int));

    relpwr_db       = ((fft_summary_B >>26) & 0x3f);
    num_str_bins_ib = fft_summary_B & 0xff;
    base_pwr        = ((fft_summary_A >> 14) & 0x1ff);
    total_gain_info = ((fft_summary_A >> 23) & 0x1ff);

    fft_chn_idx     = ((fft_summary_A >>12) & 0x3);
    peak_inx        = fft_summary_A & 0xfff;

    if (peak_inx > 2047) {
        peak_inx = peak_inx - 4096;
    }

    avgpwr_db = ((fft_summary_B >> 18) & 0xff);
    peak_mag = ((fft_summary_B >> 8) & 0x3ff);

    printk("HA = 0x%x HB = 0x%x\n", phdr->hdr_a, phdr->hdr_b);
    printk("Base Power= 0x%x, Total Gain= %d, relpwr_db=%d, num_str_bins_ib=%d fft_chn_idx=%d peak_inx=%d avgpwr_db=%d peak_mag=%d\n", base_pwr, total_gain_info, relpwr_db, num_str_bins_ib, fft_chn_idx, peak_inx, avgpwr_db, peak_mag);

    for (i = 0; i < (tlvlen-8); i++){
        fft_mag = ((u_int8_t*)ptlv)[12 + i];
        printk("%d %d, ", i, fft_mag);
    }

    printk("\n");

    return 0;
}

/*
 * Function     : dump_summary_report
 * Description  : Dump Spectral Summary Report
 * Input        : Pointer to Spectral Phyerr TLV and Length
 * Output       : Success/Failure
 *
 */
int dump_summary_report(SPECTRAL_PHYERR_TLV* ptlv, int tlvlen)
{
    /* For simplicity, everything is defined as uint32_t (except one). Proper code will later use the right sizes. */

    /* For easy comparision between MDK team and OS team, the MDK script variable names have been used */

    uint32_t agc_mb_gain;
    uint32_t sscan_gidx;
    uint32_t agc_total_gain;
    uint32_t recent_rfsat;
    uint32_t ob_flag;
    uint32_t nb_mask;
    uint32_t peak_mag;
    int16_t peak_inx;

    uint32_t ss_summary_A;
    uint32_t ss_summary_B;
    SPECTRAL_PHYERR_HDR* phdr = (SPECTRAL_PHYERR_HDR*)((u_int8_t*)ptlv + sizeof(SPECTRAL_PHYERR_TLV));

    printk("SPECTRAL : SPECTRAL SUMMARY REPORT\n");

    if (tlvlen != 8) {
        printk("SPECTRAL : Unexpected TLV length %d for Spectral Summary Report! Hexdump follows\n", tlvlen);
        print_buf((u_int8_t*)ptlv, tlvlen + 4);
        return -1;
    }

    /* Doing copy as the contents may not be aligned */
    OS_MEMCPY(&ss_summary_A, (u_int8_t*)phdr, sizeof(int));
    OS_MEMCPY(&ss_summary_B, (u_int8_t*)((u_int8_t*)phdr + sizeof(int)), sizeof(int));

    nb_mask = ((ss_summary_B >> 22) & 0xff);
    ob_flag = ((ss_summary_B >> 30) & 0x1);
    peak_inx = (ss_summary_B  & 0xfff);

    if (peak_inx > 2047) {
        peak_inx = peak_inx - 4096;
    }

    peak_mag = ((ss_summary_B >> 12) & 0x3ff);
    agc_mb_gain = ((ss_summary_A >> 24)& 0x7f);
    agc_total_gain = (ss_summary_A  & 0x3ff);
    sscan_gidx = ((ss_summary_A >> 16) & 0xff);
    recent_rfsat = ((ss_summary_B >> 31) & 0x1);

    printk("nb_mask=0x%.2x, ob_flag=%d, peak_index=%d, peak_mag=%d, agc_mb_gain=%d, agc_total_gain=%d, sscan_gidx=%d, recent_rfsat=%d\n", nb_mask, ob_flag, peak_inx, peak_mag, agc_mb_gain, agc_total_gain, sscan_gidx, recent_rfsat);

    return 0;
}

/*
 * Function     : spectral_dump_tlv
 * Description  : Dump Spectral TLV
 * Input        : Pointer to Spectral Phyerr TLV
 * Output       : Success/Failure
 *
 */
int spectral_dump_tlv(SPECTRAL_PHYERR_TLV* ptlv)
{
    if (ptlv->signature != SPECTRAL_PHYERR_SIGNATURE) {
        printk("DRIVER : Invalid signature 0x%x!\n", ptlv->signature);
        return -1;
    }

    /* TODO : Do not delete the following print
     *        The scripts used to validate Spectral depend on this Print
     */
    printk("SPECTRAL : TLV Length is 0x%x (%d)\n", ptlv->length, ptlv->length);

    switch(ptlv->tag) {
        case TLV_TAG_SPECTRAL_SUMMARY_REPORT:
        dump_summary_report(ptlv, ptlv->length);
        break;

        case TLV_TAG_SEARCH_FFT_REPORT:
        dump_search_fft_report(ptlv, ptlv->length);
        break;

        case TLV_TAG_ADC_REPORT:
        dump_adc_report(ptlv, ptlv->length);
        break;

        default:
        printk("SPECTRAL : INVALID TLV\n");
        break;
    }
    return 0;
}

/*
 * Function     : spectral_dump_header
 * Description  : Dump Spectral header
 * Input        : Pointer to Spectral Phyerr Header
 * Output       : Success/Failure
 *
 */
int spectral_dump_header(SPECTRAL_PHYERR_HDR* phdr)
{

    u_int32_t a;
    u_int32_t b;

    OS_MEMCPY(&a, (u_int8_t*)phdr, sizeof(int));
    OS_MEMCPY(&b, (u_int8_t*)((u_int8_t*)phdr + sizeof(int)), sizeof(int));

    printk("SPECTRAL : HEADER A 0x%x (%d)\n", a, a);
    printk("SPECTRAL : HEADER B 0x%x (%d)\n", b, b);
    return 0;
}

/*
 * Function     : spectral_dump_fft
 * Description  : Dump Spectral FFT
 * Input        : Pointer to Spectral Phyerr FFT
 * Output       : Success/Failure
 *
 */
int spectral_dump_fft(SPECTRAL_PHYERR_FFT* pfft, int fftlen)
{
    int i = 0;

    /* TODO : Do not delete the following print
     *        The scripts used to validate Spectral depend on this Print
     */
    printk("SPECTRAL : FFT Length is 0x%x (%d)\n", fftlen, fftlen);

    printk("fft_data # ");
    for (i = 0; i < fftlen; i++) {
        printk("%d ", pfft->buf[i]);
#if 0
        if (i % 32 == 31)
            printk("\n");
#endif
    }
    printk("\n");
    return 0;
}


/*
 * Function     : spectral_send_tlv_to_host
 * Description  : Send the TLV information to Host
 * Input        : Pointer to the TLV
 * Output       : Success/Failure
 *
 */
int spectral_send_tlv_to_host(struct ath_spectral* spectral, u_int8_t* data, u_int32_t datalen)
{

    int status = AH_TRUE;
    spectral_prep_skb(spectral);
    if (spectral->spectral_skb != NULL) {
        spectral->spectral_nlh = (struct nlmsghdr*)spectral->spectral_skb->data;
        memcpy(NLMSG_DATA(spectral->spectral_nlh), data, datalen);
        spectral_bcast_msg(spectral);
    } else {
      status = AH_FALSE;
    }
    return status;
}EXPORT_SYMBOL(spectral_send_tlv_to_host);

/*
 * Function     : spectral_dump_phyerr_data
 * Description  : Dump PHY Error
 * Input        : Pointer to buffer
 * Output       : Success/Failure
 *
 */
int spectral_dump_phyerr_data(u_int8_t* data)
{
    SPECTRAL_PHYERR_TLV* ptlv = (SPECTRAL_PHYERR_TLV*)data;
    SPECTRAL_PHYERR_HDR* phdr = (SPECTRAL_PHYERR_HDR*)(data + sizeof(SPECTRAL_PHYERR_TLV));
    SPECTRAL_PHYERR_FFT* pfft = (SPECTRAL_PHYERR_FFT*)(data + sizeof(SPECTRAL_PHYERR_TLV) + sizeof(SPECTRAL_PHYERR_HDR));
    int fftlen  = ptlv->length - sizeof(SPECTRAL_PHYERR_HDR);
    u_int8_t* next_tlv;

    if (spectral_dump_tlv(ptlv) == -1) {
        printk("DRIVER : Invalid signature 0x%x!\n", ptlv->signature);
        return -1;
    }

    next_tlv = (u_int8_t*)ptlv + ptlv->length + sizeof(int);

    /*
     * Make a more correct check.
     * Summary Report always follows Spectral Report
     */
    if (ptlv->length > 128) {
        spectral_dump_tlv((SPECTRAL_PHYERR_TLV*)next_tlv);
    }

    spectral_dump_header(phdr);

    if (fftlen) {
        spectral_dump_fft(pfft, fftlen);
    }

    return 0;

}EXPORT_SYMBOL(spectral_dump_phyerr_data);

/*
 * Function     : dbg_print_SAMP_param
 * Description  : Print contents of SAMP struct
 * Input        : Pointer to SAMP message
 * Output       : Void
 *
 */
void dbg_print_SAMP_param(struct samp_msg_params* p)
{
    printk("\nSAMP Packet : -------------------- START --------------------\n");
    printk("Freq        = %d\n", p->freq);
    printk("RSSI        = %d\n", p->rssi);
    printk("Bin Count   = %d\n", p->pwr_count);
    printk("Timestamp   = %d\n", p->tstamp);
    printk("SAMP Packet : -------------------- END -----------------------\n");
}

/*
 * Function     : dbg_print_SAMP_msg
 * Description  : Print contents of SAMP Message
 * Input        : Pointer to SAMP message
 * Output       : Void
 *
 */
void dbg_print_SAMP_msg(SPECTRAL_SAMP_MSG* ss_msg)
{
    int i = 0;

    SPECTRAL_SAMP_DATA *p = &ss_msg->samp_data;
    SPECTRAL_CLASSIFIER_PARAMS *pc = &p->classifier_params;
    struct INTERF_SRC_RSP  *pi = &p->interf_list;

    line();
    printk("Spectral Message\n");
    line();
    printk("Signature   :   0x%x\n", ss_msg->signature);
    printk("Freq        :   %d\n", ss_msg->freq);
    printk("Freq load   :   %d\n", ss_msg->freq_loading);
    printk("CW Inter    :   %d\n", ss_msg->cw_interferer);
    line();
    printk("Spectral Data info\n");
    line();
    printk("data length     :   %d\n", p->spectral_data_len);
    printk("rssi            :   %d\n", p->spectral_rssi);
    printk("combined rssi   :   %d\n", p->spectral_combined_rssi);
    printk("upper rssi      :   %d\n", p->spectral_upper_rssi);
    printk("lower rssi      :   %d\n", p->spectral_lower_rssi);
    printk("bw info         :   %d\n", p->spectral_bwinfo);
    printk("timestamp       :   %d\n", p->spectral_tstamp);
    printk("max index       :   %d\n", p->spectral_max_index);
    printk("max exp         :   %d\n", p->spectral_max_exp);
    printk("max mag         :   %d\n", p->spectral_max_mag);
    printk("last timstamp   :   %d\n", p->spectral_last_tstamp);
    printk("upper max idx   :   %d\n", p->spectral_upper_max_index);
    printk("lower max idx   :   %d\n", p->spectral_lower_max_index);
    printk("bin power count :   %d\n", p->bin_pwr_count);
    line();
    printk("Classifier info\n");
    line();
    printk("20/40 Mode      :   %d\n", pc->spectral_20_40_mode);
    printk("dc index        :   %d\n", pc->spectral_dc_index);
    printk("dc in MHz       :   %d\n", pc->spectral_dc_in_mhz);
    printk("upper channel   :   %d\n", pc->upper_chan_in_mhz);
    printk("lower channel   :   %d\n", pc->lower_chan_in_mhz);
    line();
    printk("Interference info\n");
    line();
    printk("inter count     :   %d\n", pi->count);

    for (i = 0; i < pi->count; i++) {
        printk("inter type  :   %d\n", pi->interf[i].interf_type);
        printk("min freq    :   %d\n", pi->interf[i].interf_min_freq);
        printk("max freq    :   %d\n", pi->interf[i].interf_max_freq);
    }


}


/*
 * Function     : spectral_process_phyerr
 * Description  : Process PHY Error
 * Input        : Pointer to buffer
 * Output       : Success/Failure
 *
 */
int spectral_process_phyerr(struct ath_spectral* spectral, u_int8_t* data,
                              SPECTRAL_RFQUAL_INFO* p_rfqual,
                              SPECTRAL_CHAN_INFO* p_chaninfo,
                              u_int64_t tsf64)
{


    /*
     * XXX : The classifier do not use all the members of the SAMP
     *       message data format.
     *       The classifier only depends upon the following parameters
     *
     *          1. Frequency (freq, msg->freq)
     *          2. Spectral RSSI (spectral_rssi, msg->samp_data.spectral_rssi)
     *          3. Bin Power Count (bin_pwr_count, msg->samp_data.bin_pwr_count)
     *          4. Bin Power values (bin_pwr, msg->samp_data.bin_pwr[0]
     *          5. Spectral Timestamp (spectral_tstamp, msg->samp_data.spectral_tstamp)
     *          6. MAC Address (macaddr, msg->macaddr)
     *
     *       This function prepares the params structure and populates it with 
     *       relevant values, this is in turn passed to spectral_create_samp_msg()
     *       to prepare fully formatted Spectral SAMP message
     *
     *       XXX : Need to verify
     *          1. Order of FFT bin values
     *
     */

    struct samp_msg_params params;
    SPECTRAL_SEARCH_FFT_INFO search_fft_info;
    SPECTRAL_SEARCH_FFT_INFO* p_sfft = &search_fft_info;

    int8_t rssi_up  = 0;
    int8_t rssi_low = 0;
    int8_t chn_idx  = 0;

    u_int8_t control_rssi   = 0;
    u_int8_t extension_rssi = 0;
    u_int8_t combined_rssi  = 0;

    u_int32_t tstamp    = 0;


    SPECTRAL_OPS* p_sops = GET_SPECTRAL_OPS(spectral);

    SPECTRAL_PHYERR_TLV* ptlv = (SPECTRAL_PHYERR_TLV*)data;
    SPECTRAL_PHYERR_FFT* pfft = (SPECTRAL_PHYERR_FFT*)(data + sizeof(SPECTRAL_PHYERR_TLV) + sizeof(SPECTRAL_PHYERR_HDR));
    int fftlen  = ptlv->length - sizeof(SPECTRAL_PHYERR_HDR);

    /* XXX Extend SPECTRAL_DPRINTK() to use spectral_debug_level,
           and use this facility inside spectral_dump_phyerr_data() 
           and supporting functions. */
    if (spectral_debug_level & ATH_DEBUG_SPECTRAL2) {
        spectral_dump_phyerr_data(data);
    }

    if (ptlv->signature != SPECTRAL_PHYERR_SIGNATURE) {
        
        /* EV# 118023: We tentatively disable the below print
           and provide stats instead. */
        //printk("SPECTRAL : Mismatch\n");
        spectral->diag_stats.spectral_mismatch++;
        return -1;
    }

    OS_MEMZERO(&params, sizeof(params));

    if (ptlv->tag == TLV_TAG_SEARCH_FFT_REPORT) {

        process_search_fft_report(ptlv, ptlv->length, &search_fft_info);

        tstamp = p_sops->get_tsf64(spectral) & SPECTRAL_TSMASK;

        combined_rssi   = p_rfqual->rssi_comb;

        if (spectral->upper_is_control) {
            rssi_up = control_rssi;
        } else {
            rssi_up = extension_rssi;
        }

        if (spectral->lower_is_control) {
            rssi_low = control_rssi;
        } else {
            rssi_low = extension_rssi;
        }

        params.rssi         = p_rfqual->rssi_comb;
        params.lower_rssi   = rssi_low;
        params.upper_rssi   = rssi_up;

        if (spectral->sc_spectral_noise_pwr_cal) {
            params.chain_ctl_rssi[0] = p_rfqual->pc_rssi_info[0].rssi_pri20;
            params.chain_ctl_rssi[1] = p_rfqual->pc_rssi_info[1].rssi_pri20;
            params.chain_ctl_rssi[2] = p_rfqual->pc_rssi_info[2].rssi_pri20;
            params.chain_ext_rssi[0] = p_rfqual->pc_rssi_info[0].rssi_sec20;
            params.chain_ext_rssi[1] = p_rfqual->pc_rssi_info[1].rssi_sec20;
            params.chain_ext_rssi[2] = p_rfqual->pc_rssi_info[2].rssi_sec20;
        }


        /*
         * XXX : This actually depends on the programmed chain mask
         *       There are three chains in Peregrine
         *       This value decides the per-chain enable mask to select
         *       the input ADC for search FTT.
         *       If more than one chain is enabled, the max valid chain
         *       is used. LSB corresponds to chain zero
         *
         *  XXX: The current algorithm do not use these control and extension channel
         *       Instead, it just relies on the combined RSSI values only.
         *       For fool-proof detection algorithm, we should take these RSSI values
         *       in to account. This is marked for future enhancements.
         */
        chn_idx = (spectral->params.ss_chn_mask & 0x4)?3:((spectral->params.ss_chn_mask & 0x2)?1:0);
        control_rssi    = (u_int8_t)p_rfqual->pc_rssi_info[chn_idx].rssi_pri20;
        extension_rssi  = (u_int8_t)p_rfqual->pc_rssi_info[chn_idx].rssi_sec20;

        params.bwinfo   = 0;
        params.tstamp   = 0;
        params.max_mag  = p_sfft->peak_mag;

        params.max_index    = p_sfft->peak_inx;
        params.max_exp      = 0;
        params.peak         = 0;
        params.bin_pwr_data = &pfft;
        params.freq         = p_sops->get_current_channel(spectral);
        params.freq_loading = 0;

        params.interf_list.count = 0;
        params.max_lower_index   = 0;
        params.max_upper_index   = 0;
        params.nb_lower          = 0;
        params.nb_upper          = 0;
        params.noise_floor       = p_rfqual->noise_floor[0]; /* XXX: This should come from antenna mask */
        params.datalen           = ptlv->length;
        params.pwr_count         = ptlv->length - sizeof(SPECTRAL_PHYERR_HDR);
        params.tstamp            = (tsf64 & SPECTRAL_TSMASK);

        OS_MEMCPY(&params.classifier_params, &spectral->classifier_params, sizeof(SPECTRAL_CLASSIFIER_PARAMS));

#ifdef SPECTRAL_DEBUG_SAMP_MSG
        dbg_print_SAMP_param(&params);
#endif

        spectral_create_samp_msg(spectral, &params);
    }

    return 0;
}EXPORT_SYMBOL(spectral_process_phyerr);

#endif  /* ATH_SUPPORT_SPECTRAL */
