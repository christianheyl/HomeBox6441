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
 * LMAC offload interface functions for UMAC - for power and performance offload model
 */
#include "ol_if_athvar.h"
#include "ol_if_athpriv.h"
#include "ol_if_athutf.h"
#include "bmi.h"
#include "sw_version.h"
#include "targaddrs.h"
#include "ol_helper.h"
#include "ol_txrx_ctrl_api.h"
#include "adf_os_mem.h"   /* adf_os_mem_alloc,free */
#include "adf_os_lock.h"  /* adf_os_spinlock_* */
#include "adf_os_types.h" /* adf_os_vprint */
/* FIX THIS: the HL vs. LL selection will need to be done 
 * at runtime rather than compile time
 */
#if defined(CONFIG_HL_SUPPORT)
#include "wlan_tgt_def_config_hl.h"    /* TODO: check if we need a seperated config file */
#else
#include "wlan_tgt_def_config.h"
#endif
#include "dbglog_host.h"
#include "ol_if_wow.h"
#include "a_debug.h"
#include "epping_test.h"

#if ATH_SUPPORT_DFS
#include "ol_if_dfs.h"
#endif

#include "pktlog_ac.h"
#ifndef REMOVE_PKT_LOG
struct ol_pl_os_dep_funcs *g_ol_pl_os_dep_funcs = NULL;
#endif
#include "ol_regdomain.h"

#if ATH_SUPPORT_SPECTRAL
#include "ol_if_spectral.h"
#endif
#include "ol_ath.h"

#if ATH_SUPPORT_GREEN_AP
#include "ol_if_greenap.h"
#endif  /* ATH_SUPPORT_GREEN_AP */

#include "ol_if_stats.h"
#include "ol_ratetable.h"
#include "ol_if_vap.h"

#if ATH_PERF_PWR_OFFLOAD

// Disabling scan offload
#define UMAC_SCAN_OFFLOAD 1
#if defined(EPPING_TEST) && !defined(HIF_USB)
unsigned int eppingtest = 1;
unsigned int bypasswmi = 1;
#else
unsigned int eppingtest = 0;
unsigned int bypasswmi = 0;
#endif

int wmi_unified_pdev_tpc_config_event_handler (ol_scn_t scn, u_int8_t *data, u_int16_t datalen, void *context);
int wmi_unified_gpio_input_event_handler (ol_scn_t scn, u_int8_t *data, u_int16_t datalen, void *context);

static int
wmi_unified_dcs_interference_handler (ol_scn_t scn,
			  u_int8_t *data, u_int16_t datalen, void *context);
__inline__
u_int32_t host_interest_item_address(u_int32_t target_type, u_int32_t item_offset)
{
    switch (target_type)
    {
        default:
            ASSERT(0);
        case TARGET_TYPE_AR6002:
            return (AR6002_HOST_INTEREST_ADDRESS + item_offset);
        case TARGET_TYPE_AR6003:
            return (AR6003_HOST_INTEREST_ADDRESS + item_offset);
        case TARGET_TYPE_AR6004:
            return (AR6004_HOST_INTEREST_ADDRESS + item_offset);
        case TARGET_TYPE_AR6006:
            return (AR6006_HOST_INTEREST_ADDRESS + item_offset);
        case TARGET_TYPE_AR9888:
            return (AR9888_HOST_INTEREST_ADDRESS + item_offset);
        case TARGET_TYPE_AR6320:
            return (AR6320_HOST_INTEREST_ADDRESS + item_offset);
    }
}

/* WORDs, derived from AR600x_regdump.h */
#define REG_DUMP_COUNT_AR9888   60
#define REG_DUMP_COUNT_AR6320   60
#define REGISTER_DUMP_LEN_MAX   60

#if REG_DUMP_COUNT_AR9888 > REGISTER_DUMP_LEN_MAX
#error "REG_DUMP_COUNT_AR9888 too large"
#endif
#if REG_DUMP_COUNT_AR6320 > REGISTER_DUMP_LEN_MAX
#error "REG_DUMP_COUNT_AR6320 too large"
#endif

void 
ol_target_send_suspend_complete(void *ctx)
{
    struct ol_ath_softc_net80211 *scn = (struct ol_ath_softc_net80211 *)ctx;

    scn->is_target_paused = TRUE;
    __ol_target_paused_event(scn);
}

#ifdef BIG_ENDIAN_HOST
void swap_bytes(void *pv, size_t n)
{
	int noWords;
	int i;
	A_UINT32 *wordPtr;

	noWords =   n/sizeof(u_int32_t);
	wordPtr = (u_int32_t *)pv;
	for (i=0;i<noWords;i++)
	{
		*(wordPtr + i) = __cpu_to_le32(*(wordPtr + i));
	}
}
#define SWAPME(x, len) swap_bytes(&x, len);
#endif

void
ol_target_failure(void *instance, A_STATUS status)
{
    struct ol_ath_softc_net80211 *scn = (struct ol_ath_softc_net80211 *)instance;
    A_UINT32 reg_dump_area = 0;
    A_UINT32 reg_dump_values[REGISTER_DUMP_LEN_MAX];
    A_UINT32 reg_dump_cnt = 0;
    A_UINT32 i;

    printk("XXX TARGET ASSERTED XXX\n");
    scn->target_status = OL_TRGET_STATUS_RESET;
    if (HIFDiagReadMem(scn->hif_hdl,
                host_interest_item_address(scn->target_type, offsetof(struct host_interest_s, hi_failure_state)),
                (A_UCHAR *)&reg_dump_area,
                sizeof(A_UINT32))!= A_OK)
    {
        printk("HifDiagReadiMem FW Dump Area Pointer failed\n");
        return;
    }

    printk("Target Register Dump Location 0x%08X\n", reg_dump_area);

    if (scn->target_type == TARGET_TYPE_AR6320) {
        reg_dump_cnt = REG_DUMP_COUNT_AR6320;
    } else  if (scn->target_type == TARGET_TYPE_AR9888) {
        reg_dump_cnt = REG_DUMP_COUNT_AR9888;
    } else {
        A_ASSERT(0);
    }

    if (HIFDiagReadMem(scn->hif_hdl,
                reg_dump_area,
                (A_UCHAR*)&reg_dump_values[0],
                reg_dump_cnt * sizeof(A_UINT32))!= A_OK)
    {
        printk("HifDiagReadiMem for FW Dump Area failed\n");
        return;
    }

    printk("Target Register Dump\n");
    for (i = 0; i < reg_dump_cnt; i++) {
        printk("[%02d]   :  0x%08X\n", i, reg_dump_values[i]);
    }

    return;

}

int
ol_ath_configure_target(struct ol_ath_softc_net80211 *scn)
{
    u_int32_t param;

#if 0
    if (enableuartprint) {
        param = 1;
        if (BMIWriteMemory(ar->arHifDevice,
                           HOST_INTEREST_ITEM_ADDRESS(ar->arTargetType, hi_serial_enable),
                           (A_UCHAR *)&param,
                           4)!= A_OK)
        {
             AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("BMIWriteMemory for enableuartprint failed \n"));
             return A_ERROR;
        }
        AR_DEBUG_PRINTF(ATH_DEBUG_INFO,("Serial console prints enabled\n"));
    }
#endif

    /* Tell target which HTC version it is used*/
    param = HTC_PROTOCOL_VERSION;
    if (BMIWriteMemory(scn->hif_hdl,
                       host_interest_item_address(scn->target_type, offsetof(struct host_interest_s, hi_app_host_interest)),
                       (u_int8_t *)&param,
                       4, scn)!= A_OK)
    {
         printk("BMIWriteMemory for htc version failed \n");
         return -1;
    }

#if 0
    if (enabletimerwar) {
        u_int32_t param;

        if (BMIReadMemory(ar->arHifDevice,
            HOST_INTEREST_ITEM_ADDRESS(ar->arTargetType, hi_option_flag),
            (A_UCHAR *)&param,
            4)!= A_OK)
        {
            AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("BMIReadMemory for enabletimerwar failed \n"));
            return A_ERROR;
        }

        param |= HI_OPTION_TIMER_WAR;

        if (BMIWriteMemory(ar->arHifDevice,
            HOST_INTEREST_ITEM_ADDRESS(ar->arTargetType, hi_option_flag),
            (A_UCHAR *)&param,
            4) != A_OK)
        {
            AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("BMIWriteMemory for enabletimerwar failed \n"));
            return A_ERROR;
        }
        AR_DEBUG_PRINTF(ATH_DEBUG_INFO,("Timer WAR enabled\n"));
    }
#endif

    /* set the firmware mode to STA/IBSS/AP */
    {
        if (BMIReadMemory(scn->hif_hdl,
            host_interest_item_address(scn->target_type, offsetof(struct host_interest_s, hi_option_flag)),
            (A_UCHAR *)&param,
            4, scn)!= A_OK)
        {
            printk("BMIReadMemory for setting fwmode failed \n");
            return A_ERROR;
        }

    /* TODO following parameters need to be re-visited. */
        param |= (1 << HI_OPTION_NUM_DEV_SHIFT); //num_device
        param |= (HI_OPTION_FW_MODE_AP << HI_OPTION_FW_MODE_SHIFT); //Firmware mode ??
        param |= (1 << HI_OPTION_MAC_ADDR_METHOD_SHIFT); //mac_addr_method
        param |= (0 << HI_OPTION_FW_BRIDGE_SHIFT);  //firmware_bridge
        param |= (0 << HI_OPTION_FW_SUBMODE_SHIFT); //fwsubmode

        printk("NUM_DEV=%d FWMODE=0x%x FWSUBMODE=0x%x FWBR_BUF %d\n",
                            1, HI_OPTION_FW_MODE_AP, 0, 0);

        if (BMIWriteMemory(scn->hif_hdl,
            host_interest_item_address(scn->target_type, offsetof(struct host_interest_s, hi_option_flag)),
            (A_UCHAR *)&param,
            4, scn) != A_OK)
        {
            printk("BMIWriteMemory for setting fwmode failed \n");
            return A_ERROR;
        }
    }

#if (CONFIG_DISABLE_CDC_MAX_PERF_WAR)
    {
        /* set the firmware to disable CDC max perf WAR */
        if (BMIReadMemory(scn->hif_hdl,
            host_interest_item_address(scn->target_type, offsetof(struct host_interest_s, hi_option_flag2)),
            (A_UCHAR *)&param,
            4, scn)!= A_OK)
        {
            printk("BMIReadMemory for setting cdc max perf failed \n");
            return A_ERROR;
        }

        param |= HI_OPTION_DISABLE_CDC_MAX_PERF_WAR;
        if (BMIWriteMemory(scn->hif_hdl,
            host_interest_item_address(scn->target_type, offsetof(struct host_interest_s, hi_option_flag2)),
            (A_UCHAR *)&param,
            4, scn) != A_OK)
        {
            printk("BMIWriteMemory for setting cdc max perf failed \n");
            return A_ERROR;
        }
    }
#endif /* CONFIG_CDC_MAX_PERF_WAR */

    /* If host is running on a BE CPU, set the host interest area */
    {
#ifdef BIG_ENDIAN_HOST 
        param = 1;
#else
        param = 0;
#endif
        if (BMIWriteMemory(scn->hif_hdl,
            host_interest_item_address(scn->target_type, offsetof(struct host_interest_s, hi_be)),
            (A_UCHAR *)&param,
            4, scn) != A_OK)
        {
            printk("BMIWriteMemory for setting host CPU BE mode failed \n");
            return A_ERROR;
        }
    }

    /* FW descriptor/Data swap flags */
    {
        param = 0;
        if (BMIWriteMemory(scn->hif_hdl,
            host_interest_item_address(scn->target_type, offsetof(struct host_interest_s, hi_fw_swap)),
            (A_UCHAR *)&param,
            4, scn) != A_OK)
        {
            printk("BMIWriteMemory for setting FW data/desc swap flags failed \n");
            return A_ERROR;
        }
    }

#if 0
#ifdef ATH6KL_DISABLE_TARGET_DBGLOGS
    {
        u_int32_t param;

        if (BMIReadMemory(ar->arHifDevice,
            HOST_INTEREST_ITEM_ADDRESS(ar->arTargetType, hi_option_flag),
            (A_UCHAR *)&param,
            4)!= A_OK)
        {
            AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("BMIReadMemory for disabling debug logs failed\n"));
            return A_ERROR;
        }

        param |= HI_OPTION_DISABLE_DBGLOG;

        if (BMIWriteMemory(ar->arHifDevice,
            HOST_INTEREST_ITEM_ADDRESS(ar->arTargetType, hi_option_flag),
            (A_UCHAR *)&param,
            4) != A_OK)
        {
            AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("BMIWriteMemory for HI_OPTION_DISABLE_DBGLOG\n"));
            return A_ERROR;
        }
        AR_DEBUG_PRINTF(ATH_DEBUG_INFO,("Firmware mode set\n"));
    }
#endif /* ATH6KL_DISABLE_TARGET_DBGLOGS */

    if (regscanmode) {
        u_int32_t param;

        if (BMIReadMemory(ar->arHifDevice,
            HOST_INTEREST_ITEM_ADDRESS(ar->arTargetType, hi_option_flag),
            (A_UCHAR *)&param,
            4)!= A_OK)
        {
            AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("BMIReadMemory for setting regscanmode failed\n"));
            return A_ERROR;
        }

        if (regscanmode == 1) {
            param |= HI_OPTION_SKIP_REG_SCAN;
        } else if (regscanmode == 2) {
            param |= HI_OPTION_INIT_REG_SCAN;
        }

        if (BMIWriteMemory(ar->arHifDevice,
            HOST_INTEREST_ITEM_ADDRESS(ar->arTargetType, hi_option_flag),
            (A_UCHAR *)&param,
            4) != A_OK)
        {
            AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("BMIWriteMemory for setting regscanmode failed\n"));
            return A_ERROR;
        }
        AR_DEBUG_PRINTF(ATH_DEBUG_INFO,("Regulatory scan mode set\n"));
    }

#if defined(AR6003_REV2_BOARD_EXT_DATA_ADDRESS)
    /*
     * Hardcode the address use for the extended board data
     * Ideally this should be pre-allocate by the OS at boot time
     * But since it is a new feature and board data is loaded
     * at init time, we have to workaround this from host.
     * It is difficult to patch the firmware boot code,
     * but possible in theory.
     */
    if (ar->arTargetType == TARGET_TYPE_AR6003) {
        u_int32_t ramReservedSz;
        if (ar->arVersion.target_ver == AR6003_REV2_VERSION) {
            param = AR6003_REV2_BOARD_EXT_DATA_ADDRESS;
            ramReservedSz =  AR6003_REV2_RAM_RESERVE_SIZE;
        } else {
            param = AR6003_REV3_BOARD_EXT_DATA_ADDRESS;
            ramReservedSz =  AR6003_REV3_RAM_RESERVE_SIZE;
        }
        if (BMIWriteMemory(ar->arHifDevice,
            HOST_INTEREST_ITEM_ADDRESS(ar->arTargetType, hi_board_ext_data),
            (A_UCHAR *)&param,
            4) != A_OK)
        {
            AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("BMIWriteMemory for hi_board_ext_data failed \n"));
            return A_ERROR;
        }
        if (BMIWriteMemory(ar->arHifDevice,
              HOST_INTEREST_ITEM_ADDRESS(ar->arTargetType, hi_end_RAM_reserve_sz),
              (A_UCHAR *)&ramReservedSz, 4) != A_OK)
        {
            AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("BMIWriteMemory for hi_end_RAM_reserve_sz failed \n"));
            return A_ERROR;
        }
    }
#endif /* AR6003_REV2_BOARD_EXT_DATA_ADDRESS */
    /* For AR6004: Size reserved at the end of RAM is done by wlansetup app */

        /* since BMIInit is called in the driver layer, we have to set the block
         * size here for the target */

    if (A_FAILED(ar6000_set_htc_params(ar->arHifDevice,
                                       ar->arTargetType,
                                       mbox_yield_limit,
                                       0 /* use default number of control buffers */
                                       ))) {
        return A_ERROR;
    }

    if (setupbtdev != 0) {
        if (A_FAILED(ar6000_set_hci_bridge_flags(ar->arHifDevice,
                                                 ar->arTargetType,
                                                 setupbtdev))) {
            return A_ERROR;
        }
    }
#endif
    return A_OK;
}

int
ol_check_dataset_patch(struct ol_ath_softc_net80211 *scn, u_int32_t *address)
{
    /* Check if patch file needed for this target type/version. */
    return 0;
}


#ifdef HIF_SDIO
static A_STATUS
ol_sdio_extra_initialization(struct ol_ath_softc_net80211 *scn)
{
    A_STATUS status;
    do{
        A_UINT32 blocksizes[HTC_MAILBOX_NUM_MAX];
        unsigned int MboxIsrYieldValue = 99;
        A_UINT32 TargetType = TARGET_TYPE_AR6320;
        /* get the block sizes */
        status = HIFConfigureDevice(scn->hif_hdl, HIF_DEVICE_GET_MBOX_BLOCK_SIZE,
                                    blocksizes, sizeof(blocksizes));

        if (A_FAILED(status)) {
            printk("Failed to get block size info from HIF layer...\n");
            break;
        }else{
            printk("get block size info from HIF layer:%x,%x,%x,%x\n",
                    blocksizes[0], blocksizes[1], blocksizes[2], blocksizes[3]);
        }
            /* note: we actually get the block size for mailbox 1, for SDIO the block
             * size on mailbox 0 is artificially set to 1 */
            /* must be a power of 2 */
        A_ASSERT((blocksizes[1] & (blocksizes[1] - 1)) == 0);

            /* set the host interest area for the block size */
        status = BMIWriteMemory(scn->hif_hdl,
                                HOST_INTEREST_ITEM_ADDRESS(TargetType, hi_mbox_io_block_sz),
                                (A_UCHAR *)&blocksizes[1],
                                4,
                                scn);

        if (A_FAILED(status)) {
            printk("BMIWriteMemory for IO block size failed \n");
            break;
        }else{
            printk("BMIWriteMemory for IO block size succeeded \n");
        }

        printk("Block Size Set: %d (target address:0x%X)\n",
                blocksizes[1], HOST_INTEREST_ITEM_ADDRESS(TargetType, hi_mbox_io_block_sz));

        if (MboxIsrYieldValue != 0) {
                /* set the host interest area for the mbox ISR yield limit */
            status = BMIWriteMemory(scn->hif_hdl,
                                    HOST_INTEREST_ITEM_ADDRESS(TargetType, hi_mbox_isr_yield_limit),
                                    (A_UCHAR *)&MboxIsrYieldValue,
                                    4,
                                    scn);

            if (A_FAILED(status)) {
                printk("BMIWriteMemory for yield limit failed \n");
                break;
            }
        }
    }while(FALSE);
    return status;
}
#endif

int
ol_ath_download_firmware(struct ol_ath_softc_net80211 *scn)
{
    u_int32_t param, address = 0;
    int status = !EOK;

    /* Transfer Board Data from Target EEPROM to Target RAM */
    /* Determine where in Target RAM to write Board Data */
    if (scn->target_version != AR6004_VERSION_REV1_3) {
         BMIReadMemory(scn->hif_hdl,
         host_interest_item_address(scn->target_type, offsetof(struct host_interest_s, hi_board_data)),
                                   (u_int8_t *)&address, 4, scn);
    }

    if (!address) {
         if (scn->target_version == AR6004_REV1_VERSION)  {
             address = AR6004_REV1_BOARD_DATA_ADDRESS;
         } else if (scn->target_version == AR6004_VERSION_REV1_3) {
              address = AR6004_REV5_BOARD_DATA_ADDRESS;
         }
        printk("%s: Target address not known! Using 0x%x\n", __func__, address);
    }

    if (scn->cal_in_flash) {
        /* Write EEPROM or Flash data to Target RAM */
        status = ol_transfer_bin_file(scn, ATH_FLASH_FILE, address, FALSE);
    }
   
    if (status == EOK) {
        /* Record the fact that Board Data is initialized */
        if (scn->target_version != AR6004_VERSION_REV1_3) {
            param = 1;
            BMIWriteMemory(scn->hif_hdl,
                           host_interest_item_address(scn->target_type,
                               offsetof(struct host_interest_s, hi_board_data_initialized)),
                           (u_int8_t *)&param, 4, scn);
        }
    } else {
        /* Flash is either not available or invalid */
        if (ol_transfer_bin_file(scn, ATH_BOARD_DATA_FILE, address, FALSE) != EOK) {
            return -1;
        }

        /* Record the fact that Board Data is initialized */
        if (scn->target_version != AR6004_VERSION_REV1_3) {
            param = 1;
            BMIWriteMemory(scn->hif_hdl,
                           host_interest_item_address(scn->target_type,
                               offsetof(struct host_interest_s, hi_board_data_initialized)),
                           (u_int8_t *)&param, 4, scn);
        }

        /* Transfer One Time Programmable data */
        address = BMI_SEGMENTED_WRITE_ADDR;
        printk("%s: Using 0x%x for the remainder of init\n", __func__, address);

        status = ol_transfer_bin_file(scn, ATH_OTP_FILE, address, TRUE);
        if (status == EOK) {
            /* Execute the OTP code only if entry found and downloaded */
            param = 0;
            BMIExecute(scn->hif_hdl, address, &param, scn);
        } else if (status == -1) {
            return status;
        }
    }

    /* Download Target firmware - TODO point to target specific files in runtime */
    address = BMI_SEGMENTED_WRITE_ADDR;
    if (ol_transfer_bin_file(scn, ATH_FIRMWARE_FILE, address, TRUE) != EOK) {
        return -1;
    }

    /* Apply the patches */
    if (ol_check_dataset_patch(scn, &address))
    {
        if ((ol_transfer_bin_file(scn, ATH_PATCH_FILE, address, FALSE)) != EOK) {
            return -1;
        }
        BMIWriteMemory(scn->hif_hdl,
                     host_interest_item_address(scn->target_type, offsetof(struct host_interest_s, hi_dset_list_head)),
                     (u_int8_t *)&address, 4, scn);
    }

    if (scn->enableuartprint) {
        /* Configure GPIO AR9888 UART */
        if (scn->target_version == AR6004_VERSION_REV1_3) {
            param = 15;
        } else {
            param = 7;
        }
        BMIWriteMemory(scn->hif_hdl,
                host_interest_item_address(scn->target_type, offsetof(struct host_interest_s, hi_dbg_uart_txpin)),
                (u_int8_t *)&param, 4, scn);
        param = 1;
        BMIWriteMemory(scn->hif_hdl,
                host_interest_item_address(scn->target_type, offsetof(struct host_interest_s, hi_serial_enable)),
                (u_int8_t *)&param, 4, scn);
    }else {
        /*
         * Explicitly setting UART prints to zero as target turns it on
         * based on scratch registers.
         */
        param = 0;
        BMIWriteMemory(scn->hif_hdl,
                host_interest_item_address(scn->target_type, offsetof(struct host_interest_s,hi_serial_enable)),
                (u_int8_t *)&param, 4, scn);
    }

    if (scn->target_version == AR6004_VERSION_REV1_3) {
        A_UINT32 blocksizes[HTC_MAILBOX_NUM_MAX] = {0x10,0x10,0x10,0x10};
        BMIWriteMemory(scn->hif_hdl,
                host_interest_item_address(scn->target_type, offsetof(struct host_interest_s, hi_mbox_io_block_sz)),
                 (A_UCHAR *)&blocksizes[1], 4, scn); 
    }
#ifdef HIF_SDIO
    return ol_sdio_extra_initialization(scn);
#else    
    return EOK;
#endif    
}

int
ol_ath_set_host_app_area(struct ol_ath_softc_net80211 *scn)
{
    printk("ol_ath_set_host_app_area TODO\n");
#if 0
    u_int32_t address, data;
    struct host_app_area_s host_app_area;

    /* Fetch the address of the host_app_area_s instance in the host interest area */
    address = TARG_VTOP(scn->target_type, HOST_INTEREST_ITEM_ADDRESS(scn->target_type, hi_app_host_interest));
    if (ar6000_ReadRegDiag(scn->hif_hdl, &address, &data) != A_OK) {
        return A_ERROR;
    }
    address = TARG_VTOP(scn->target_type, data);
    host_app_area.wmi_protocol_ver = WMI_PROTOCOL_VERSION;
    if (ar6000_WriteDataDiag(scn->hif_hdl, address,
                             (A_UCHAR *)&host_app_area,
                             sizeof(struct host_app_area_s)) != A_OK)
    {
        return A_ERROR;
    }
#endif
    return A_OK;
}
A_STATUS HIF_USB_connect_service(struct ol_ath_softc_net80211 *scn)
{
    int status;
    HTC_SERVICE_CONNECT_REQ connect;
    HTC_SERVICE_CONNECT_RESP response;
    A_MEMZERO(&connect,sizeof(connect));

    connect.EpCallbacks.EpSendFull        = NULL;
    connect.EpCallbacks.EpRecv            = NULL;
    connect.LocalConnectionFlags |= HTC_LOCAL_CONN_FLAGS_ENABLE_SEND_BUNDLE_PADDING;
    connect.MaxSendMsgSize =  1664;
    connect.ServiceID = WMI_DATA_BE_SVC;
    if ((status = HTCConnectService(scn->htc_handle, &connect, &response))
            != EOK) {
        printk("Failed to connect to Endpoint Ping BE service status:%d \n", status);
        return -1;;
    } else {
        printk("eppingtest BE endpoint:%d\n", response.Endpoint);
    }
    connect.ServiceID = WMI_DATA_BK_SVC;
    if ((status = HTCConnectService(scn->htc_handle, &connect, &response))
            != EOK) {
        printk("Failed to connect to Endpoint Ping BK service status:%d \n", status);
        return -1;;
    } else {
        printk("eppingtest BK endpoint:%d\n", response.Endpoint);
    }
    connect.ServiceID = WMI_DATA_VI_SVC;
    if ((status = HTCConnectService(scn->htc_handle, &connect, &response))
            != EOK) {
        printk("Failed to connect to Endpoint Ping VI service status:%d \n", status);
        return -1;;
    } else {
        printk("eppingtest VI endpoint:%d\n", response.Endpoint);
    }
    connect.ServiceID = WMI_DATA_VO_SVC;
    if ((status = HTCConnectService(scn->htc_handle, &connect, &response))
            != EOK) {
        printk("Failed to connect to Endpoint Ping VO service status:%d \n", status);
        return -1;;
    } else {
        printk("eppingtest VO endpoint:%d\n", response.Endpoint);
    }
    return EOK;
}
int
ol_ath_connect_htc(struct ol_ath_softc_net80211 *scn)
{
    int status;
    HTC_SERVICE_CONNECT_REQ connect;

    OS_MEMZERO(&connect,sizeof(connect));

    /* meta data is unused for now */
    connect.pMetaData = NULL;
    connect.MetaDataLength = 0;
    /* these fields are the same for all service endpoints */
    connect.EpCallbacks.pContext = scn;
    connect.EpCallbacks.EpTxCompleteMultiple = NULL /* Control path completion ar6000_tx_complete */;
    connect.EpCallbacks.EpRecv = NULL /* Control path rx */;
    connect.EpCallbacks.EpRecvRefill = NULL /* ar6000_rx_refill */;
    connect.EpCallbacks.EpSendFull = NULL /* ar6000_tx_queue_full */;
#if 0
    /* set the max queue depth so that our ar6000_tx_queue_full handler gets called.
     * Linux has the peculiarity of not providing flow control between the
     * NIC and the network stack. There is no API to indicate that a TX packet
     * was sent which could provide some back pressure to the network stack.
     * Under linux you would have to wait till the network stack consumed all sk_buffs
     * before any back-flow kicked in. Which isn't very friendly.
     * So we have to manage this ourselves */
    connect.MaxSendQueueDepth = MAX_DEFAULT_SEND_QUEUE_DEPTH;
    connect.EpCallbacks.RecvRefillWaterMark = AR6000_MAX_RX_BUFFERS / 4; /* set to 25 % */
    if (0 == connect.EpCallbacks.RecvRefillWaterMark) {
        connect.EpCallbacks.RecvRefillWaterMark++;
    }
#endif
#if 0
    /* connect to control service */
    connect.ServiceID = WMI_CONTROL_SVC;
    if ((status = ol_ath_connectservice(scn, &connect, "WMI CONTROL")) != EOK)
        goto conn_fail;
#endif
    if (!bypasswmi) {
        if ((status = wmi_unified_connect_htc_service(scn->wmi_handle, scn->htc_handle)) != EOK)
             goto conn_fail;
    }
#if defined(EPPING_TEST) && !defined(HIF_USB)
    if (eppingtest){
        extern A_STATUS epping_connect_service(struct ol_ath_softc_net80211 *scn);
        if ((status = epping_connect_service(scn)) != EOK)
             goto conn_fail;
    }
#endif	
    if (scn->target_version == AR6004_VERSION_REV1_3) {
      if ((status = HIF_USB_connect_service(scn)) != EOK)
               goto conn_fail;
    }
    /*
     * give our connected endpoints some buffers
     */
#if 0
    ar6000_rx_refill(scn, scn->htt_control_ep);
    ar6000_rx_refill(scn, scn->htt_data_ep);
#endif

    /*
     * Since cookies are used for HTC transports, they should be
     * initialized prior to enabling HTC.
     */
    ol_cookie_init((void *)scn);


    /*
     * Start HTC
     */
    if ((status = HTCStart(scn->htc_handle)) != A_OK) {
        goto conn_fail;
    }

    if (!bypasswmi) {
        /*
         * Wait for WMI event to be ready
         */
        if (scn->target_version == AR6004_VERSION_REV1_3) {
            scn->wmi_ready = TRUE;
            scn->wlan_init_status = WLAN_INIT_STATUS_SUCCESS;
        } else {
            if ((status = __ol_ath_check_wmi_ready(scn)) != EOK) {
                goto conn_fail1;
            }
            printk("%s() WMI is ready\n", __func__);
        
            if(scn->wlan_init_status != WLAN_INIT_STATUS_SUCCESS)
            {
              printk("%s Target wmi init failed with status %d\n", __func__,scn->wlan_init_status);
              status = ENODEV; 
              goto conn_fail1;
            }   
        }
        /* Communicate the wmi protocol verision to the target */
        if ((ol_ath_set_host_app_area(scn)) != EOK) {
            printk("Unable to set the host app area\n");
        }
    }

    // TODO is this needed
//            ar6000_target_config_wlan_params(arPriv);
    return EOK;

conn_fail1:
    HTCStop(scn->htc_handle);
conn_fail:
    return status;
}

int
ol_ath_disconnect_htc(struct ol_ath_softc_net80211 *scn)
{
    if (scn->htc_handle != NULL) {
        HTCStop(scn->htc_handle);
    }
    return 0;
}

static void 
ol_ath_update_caps(struct ieee80211com *ic, wmi_service_ready_event *ev,
                           A_UINT32 *wmi_service_bitmap)
{
    struct ol_ath_softc_net80211 *scn = OL_ATH_SOFTC_NET80211(ic);
    u_int32_t ampdu_exp = 0;
    u_int16_t caps  = 0;

    /* setup ieee80211 flags */
    ieee80211com_clear_cap(ic, -1);
    ieee80211com_clear_athcap(ic, -1);
    ieee80211com_clear_athextcap(ic, -1);

    ieee80211com_set_phytype(ic, IEEE80211_T_OFDM);

    ieee80211com_set_cap(ic,IEEE80211_C_SHPREAMBLE);
   
    ieee80211com_set_cap(ic,
                     IEEE80211_C_IBSS           /* ibss, nee adhoc, mode */
                     | IEEE80211_C_HOSTAP       /* hostap mode */
                     | IEEE80211_C_MONITOR      /* monitor mode */
                     | IEEE80211_C_SHSLOT       /* short slot time supported */
                     | IEEE80211_C_PMGT         /* capable of power management*/
                     | IEEE80211_C_WPA          /* capable of WPA1+WPA2 */
                     | IEEE80211_C_BGSCAN       /* capable of bg scanning */ 
                  );
    
    /* WMM enable */
    ieee80211com_set_cap(ic, IEEE80211_C_WME);

    if (WMI_SERVICE_IS_ENABLED(wmi_service_bitmap, WMI_SERVICE_AP_UAPSD)) {
        ieee80211com_set_cap(ic, IEEE80211_C_UAPSD);
        IEEE80211_UAPSD_ENABLE(ic);
    }

    /* Default 11h to start enabled  */
    ieee80211_ic_doth_set(ic);

    /* setup the chainmasks */
    ieee80211com_set_tx_chainmask(ic,
        (u_int8_t) (scn->wlan_resource_config.tx_chain_mask));

    ieee80211com_set_rx_chainmask(ic,
        (u_int8_t) (scn->wlan_resource_config.rx_chain_mask));

#ifdef ATH_SUPPORT_WAPI
    /*WAPI HW engine support upto 300 Mbps (MCS15h),
      limiting the chains to 2*/
    ic->ic_num_wapi_rx_maxchains = 2;
    ic->ic_num_wapi_tx_maxchains = 2;
#endif
    /* 11n Capabilities */
    ieee80211com_set_num_tx_chain(ic,1);
    ieee80211com_set_num_rx_chain(ic,1);
    ieee80211com_clear_htcap(ic, -1);
    ieee80211com_clear_htextcap(ic, -1);
    if (ev->ht_cap_info & WMI_HT_CAP_ENABLED) {
        ieee80211com_set_cap(ic, IEEE80211_C_HT);
        ieee80211com_set_htcap(ic, IEEE80211_HTCAP_C_SHORTGI40
                        | IEEE80211_HTCAP_C_CHWIDTH40
                        | IEEE80211_HTCAP_C_DSSSCCK40);
        if (ev->ht_cap_info & WMI_HT_CAP_HT20_SGI)  {
            ieee80211com_set_htcap(ic, IEEE80211_HTCAP_C_SHORTGI20);
        }
        if (ev->ht_cap_info & WMI_HT_CAP_DYNAMIC_SMPS) {
            ieee80211com_set_htcap(ic, IEEE80211_HTCAP_C_SMPOWERSAVE_DYNAMIC);
        } else {
            ieee80211com_set_htcap(ic, IEEE80211_HTCAP_C_SM_ENABLED);
        }
        ieee80211com_set_htextcap(ic, IEEE80211_HTCAP_EXTC_TRANS_TIME_5000
                        | IEEE80211_HTCAP_EXTC_MCS_FEEDBACK_NONE);
        ieee80211com_set_maxampdu(ic, IEEE80211_HTCAP_MAXRXAMPDU_65536);

        /* Force this to 8usec for now, instead of checking min_pkt_size_enable */
        if(0) {
            ieee80211com_set_mpdudensity(ic,IEEE80211_HTCAP_MPDUDENSITY_NA);
        }
        else {
            ieee80211com_set_mpdudensity(ic,IEEE80211_HTCAP_MPDUDENSITY_8);
        }

        IEEE80211_ENABLE_AMPDU(ic);

        ieee80211com_set_num_rx_chain(ic, ev->num_rf_chains); 
        ieee80211com_set_num_tx_chain(ic, ev->num_rf_chains);
    }

    /* Tx STBC is a 2-bit mask. Convert to ieee definition. */
    caps = (ev->ht_cap_info & WMI_HT_CAP_TX_STBC) >> WMI_HT_CAP_TX_STBC_MASK_SHIFT;
    ieee80211com_set_htcap(ic, IEEE80211_HTCAP_C_TXSTBC & (caps << IEEE80211_HTCAP_C_TXSTBC_S));
  

    /* Rx STBC is a 2-bit mask. Convert to ieee definition. */
    caps = (ev->ht_cap_info & WMI_HT_CAP_RX_STBC) >> WMI_HT_CAP_RX_STBC_MASK_SHIFT;
    ieee80211com_set_htcap(ic, IEEE80211_HTCAP_C_RXSTBC & (caps << IEEE80211_HTCAP_C_RXSTBC_S));

    if (ev->ht_cap_info & WMI_HT_CAP_LDPC) {
        ieee80211com_set_htcap(ic, IEEE80211_HTCAP_C_ADVCODING);
    }

    /* 11n configuration */
    ieee80211com_clear_htflags(ic, -1);
    if (ieee80211com_has_htcap(ic, IEEE80211_HTCAP_C_SHORTGI40))
            ieee80211com_set_htflags(ic, IEEE80211_HTF_SHORTGI40);
    if (ieee80211com_has_htcap(ic, IEEE80211_HTCAP_C_SHORTGI20))
            ieee80211com_set_htflags(ic, IEEE80211_HTF_SHORTGI20);

    /*
     * Note that in the offload architecture chain_masks
     * and spatial_streams are synonymous 
     */
    ieee80211com_set_spatialstreams(ic, ev->num_rf_chains);

    /*
     * Indicate we need the 802.11 header padded to a
     * 32-bit boundary for 4-address and QoS frames.
     */
    IEEE80211_ENABLE_DATAPAD(ic);

    /* Check whether the hardware is VHT capable */
    ieee80211com_clear_vhtcap(ic, -1);
    if (WMI_SERVICE_IS_ENABLED(wmi_service_bitmap, WMI_SERVICE_11AC)) {

        /* Copy the VHT capabilities information */
        ieee80211com_set_vhtcap(ic, ev->vht_cap_info);

        /* Adjust HT AMSDU len based on VHT MPDU len */
        if (ev->vht_cap_info & IEEE80211_VHTCAP_MAX_MPDU_LEN_3839) {
            ieee80211com_set_htcap(ic,IEEE80211_HTCAP_C_MAXAMSDUSIZE);
        } else {
            ieee80211com_clear_htcap(ic,IEEE80211_HTCAP_C_MAXAMSDUSIZE);
        }

        /* Adjust HT AMPDU len Exp  based on VHT MPDU len */
        ampdu_exp = ev->vht_cap_info >> IEEE80211_VHTCAP_MAX_AMPDU_LEN_EXP_S;
        switch (ampdu_exp) {
            case 0:
            case 1:
            case 2:
            case 3:
                ieee80211com_set_maxampdu(ic, ampdu_exp);
            break;
     
            default:
                ieee80211com_set_maxampdu(ic, IEEE80211_HTCAP_MAXRXAMPDU_65536);
            break;
        }
 

        /* Set the VHT  rate information */
        {
            /*  11ac spec states it is mandatory to support MCS 0-7 and NSS=1 */ 
            u_int16_t basic_mcs = 0xfffc;
            ol_ath_vht_rate_setup(ic, ev->vht_supp_mcs, 0, basic_mcs );

        }

    }

    if (WMI_SERVICE_IS_ENABLED(wmi_service_bitmap, WMI_SERVICE_RATECTRL)) {
        ol_txrx_enable_host_ratectrl(
                (OL_ATH_SOFTC_NET80211(ic))->pdev_txrx_handle, 1);
    }

    /* ToDo, check ev->sys_cap_info for  WMI_SYS_CAP_ENABLE and WMI_SYS_CAP_TXPOWER when it is available from FW */
    ieee80211com_set_cap(ic, IEEE80211_C_TXPMGT);
   
}

void
ol_ath_set_default_tgt_config(struct ol_ath_softc_net80211 *scn)
{

    wmi_resource_config  tgt_cfg = {
        CFG_TGT_NUM_VDEV,
        CFG_TGT_NUM_PEERS + CFG_TGT_NUM_VDEV, /* need to reserve an additional peer for each VDEV */
        CFG_TGT_NUM_PEER_KEYS,
        CFG_TGT_NUM_TIDS,
        CFG_TGT_AST_SKID_LIMIT,
        CFG_TGT_DEFAULT_TX_CHAIN_MASK,
        CFG_TGT_DEFAULT_RX_CHAIN_MASK,
        { CFG_TGT_RX_TIMEOUT_LO_PRI, CFG_TGT_RX_TIMEOUT_LO_PRI, CFG_TGT_RX_TIMEOUT_LO_PRI, CFG_TGT_RX_TIMEOUT_HI_PRI },
#ifdef ATHR_WIN_NWF
        CFG_TGT_RX_DECAP_MODE_NWIFI,
#else
        CFG_TGT_RX_DECAP_MODE,
#endif
        CFG_TGT_DEFAULT_SCAN_MAX_REQS,
        CFG_TGT_DEFAULT_BMISS_OFFLOAD_MAX_VDEV,
        CFG_TGT_DEFAULT_ROAM_OFFLOAD_MAX_VDEV,
        CFG_TGT_DEFAULT_ROAM_OFFLOAD_MAX_PROFILES,
        CFG_TGT_DEFAULT_NUM_MCAST_GROUPS,
        CFG_TGT_DEFAULT_NUM_MCAST_TABLE_ELEMS,
        CFG_TGT_DEFAULT_MCAST2UCAST_MODE,
        CFG_TGT_DEFAULT_TX_DBG_LOG_SIZE,
        CFG_TGT_WDS_ENTRIES,
        CFG_TGT_DEFAULT_DMA_BURST_SIZE,
        CFG_TGT_DEFAULT_MAC_AGGR_DELIM,
        CFG_TGT_DEFAULT_RX_SKIP_DEFRAG_TIMEOUT_DUP_DETECTION_CHECK,
        CFG_TGT_DEFAULT_VOW_CONFIG,
        CFG_TGT_NUM_MSDU_DESC
    };

    /* reduce the peer/vdev if CFG_TGT_NUM_MSDU_DESC exceeds 1000 */
    // TODO:

    /* Override VoW specific configuration, if VoW is enabled (no of peers, 
     * no of vdevs, No of wds entries, No of Vi Nodes, No of Descs per Vi Node)
     */
    if(scn->vow_config >> 16)
    {
        tgt_cfg.num_vdevs = CFG_TGT_NUM_VDEV_VOW;
        tgt_cfg.num_peers = CFG_TGT_NUM_PEERS_VOW + CFG_TGT_NUM_VDEV_VOW;
        tgt_cfg.num_tids = CFG_TGT_NUM_TIDS_VOW;
        tgt_cfg.num_wds_entries = CFG_TGT_WDS_ENTRIES_VOW;

        tgt_cfg.vow_config = scn->vow_config;        
    }

#if PERE_IP_HDR_ALIGNMENT_WAR
    if (scn->host_80211_enable) {
        /*
         * To make the IP header begins at dword aligned address,
         * we make the decapsulation mode as Native Wifi.
         */
        tgt_cfg.rx_decap_mode = CFG_TGT_RX_DECAP_MODE_NWIFI;
    }
#endif

    scn->wlan_resource_config = tgt_cfg;

}

static void dbg_print_wmi_service_11ac(wmi_service_ready_event *ev)
{
    if (WMI_SERVICE_IS_ENABLED(ev->wmi_service_bitmap, WMI_SERVICE_11AC)) {
        printk("num_rf_chain : %08x\n",ev->num_rf_chains);
        printk("ht_cap_info: : %08x\n",ev->ht_cap_info);
        printk("vht_cap_info : %08x\n",ev->vht_cap_info);
        printk("vht_supp_mcs : %08x\n",ev->vht_supp_mcs);
    }
    else {
        printk("\n No WMI 11AC service event received\n");
    }
}


/**
 * allocate a chunk of memory at the index indicated and 
 * if allocation fail allocate smallest size possiblr and
 * return number of units allocated.
 */
u_int32_t
ol_ath_alloc_host_mem_chunk(ol_scn_t scn, u_int32_t req_id, u_int32_t idx, u_int32_t num_units, u_int32_t unit_len)
{
    adf_os_dma_addr_t paddr;
    if (!num_units  || !unit_len)  {
        return 0;
    }
    scn->mem_chunks[idx].vaddr = NULL ;
    /** reduce the requested allocation by half until allocation succeeds */
    while(scn->mem_chunks[idx].vaddr == NULL && num_units ) {
        scn->mem_chunks[idx].vaddr = adf_os_mem_alloc_consistent(
            scn->adf_dev, num_units*unit_len, &paddr,
            adf_os_get_dma_mem_context((&(scn->mem_chunks[idx])), memctx));
        if(scn->mem_chunks[idx].vaddr == NULL) {
            num_units = (num_units >> 1) ; /* reduce length by half */
        } else {
           scn->mem_chunks[idx].paddr = paddr;
           scn->mem_chunks[idx].len = num_units*unit_len;
           scn->mem_chunks[idx].req_id =  req_id;
        }
    }
    return num_units;
}


#define HOST_MEM_SIZE_UNIT 4

/*
 * allocate amount of memory requested by FW. 
 */ 
void
ol_ath_alloc_host_mem (ol_scn_t scn, u_int32_t req_id, u_int32_t num_units, u_int32_t unit_len)
{
    u_int32_t remaining_units,allocated_units,idx;
    /* adjust the length to nearest multiple of unit size */
    unit_len = (unit_len + (HOST_MEM_SIZE_UNIT - 1)) & (~(HOST_MEM_SIZE_UNIT - 1)); 
    idx = scn->num_mem_chunks ;
    remaining_units = num_units;
    while(remaining_units) {
        allocated_units = ol_ath_alloc_host_mem_chunk(scn,req_id,  idx, remaining_units,unit_len);
        if (allocated_units == 0) {
            printk("FAILED TO ALLOCATED memory unit len %d units requested %d units allocated %d \n",unit_len, num_units,(num_units - remaining_units));
            scn->num_mem_chunks = idx;
            break;
        }
        remaining_units -= allocated_units;
        ++idx;
        if (idx == MAX_MEM_CHUNKS ) {
            printk("RWACHED MAX CHUNK LIMIT for memory units %d unit len %d requested by FW, only allocated %d \n", 
                   num_units,unit_len, (num_units - remaining_units));
            scn->num_mem_chunks = idx;
            break;
        }
    }
    scn->num_mem_chunks = idx;
}

void
ol_ath_service_ready_event(ol_scn_t scn_handle, wmi_service_ready_event *ev)
{
    struct ol_ath_softc_net80211 *scn = (struct ol_ath_softc_net80211 *)scn_handle;
    struct ieee80211com *ic = &scn->sc_ic;
    wmi_init_cmd *cmd;
    struct ol_ath_target_cap target_cap; 
    wmi_buf_t buf;
    int len = sizeof(wmi_init_cmd),idx;

    scn->phy_capability = ev->phy_capability;
	scn->max_frag_entry = ev->max_frag_entry;

    /* Dump service ready event for debugging */
    dbg_print_wmi_service_11ac(ev);

    /* wmi service is ready */
    OS_MEMCPY(scn->wmi_service_bitmap,ev->wmi_service_bitmap,sizeof(scn->wmi_service_bitmap));
    buf = wmi_buf_alloc(scn->wmi_handle, len + (sizeof(wlan_host_memory_chunk) * MAX_MEM_CHUNKS) );
    if (!buf) {
        printk("%s:wmi_buf_alloc failed\n", __FUNCTION__);
        return ;
    }
    
    if (WMI_SERVICE_IS_ENABLED(scn->wmi_service_bitmap, WMI_SERVICE_RATECTRL_CACHE) &&
        WMI_SERVICE_IS_ENABLED(scn->wmi_service_bitmap, WMI_SERVICE_IRAM_TIDS) &&
        !(scn->vow_config >> 16)) {

        if (scn->max_peers) {
            scn->wlan_resource_config.num_peers = scn->max_peers;
        }
        else {
            scn->wlan_resource_config.num_peers = CFG_TGT_NUM_PEERS_MAX;
        }
        if ((scn->wlan_resource_config.num_peers * 2) > CFG_TGT_NUM_TIDS_MAX) {
            /* one data tid per peer */
            scn->wlan_resource_config.num_tids = scn->wlan_resource_config.num_peers;
        }
        else if ((scn->wlan_resource_config.num_peers * 4) > CFG_TGT_NUM_TIDS_MAX) {
            /* two tids per peer */
            scn->wlan_resource_config.num_tids = scn->wlan_resource_config.num_peers * 2;
        }
        else {
            /* four tids per peer */
            scn->wlan_resource_config.num_tids = scn->wlan_resource_config.num_peers * 4;
        }
        if (scn->max_vdevs) {
            scn->wlan_resource_config.num_vdevs = scn->max_vdevs;
            scn->wlan_resource_config.num_peers += scn->max_vdevs;
        }
        else {
            scn->wlan_resource_config.num_peers += CFG_TGT_NUM_VDEV;
        }
        printk("LARGE_AP enabled. num_peers %d, num_vdevs %d, num_tids %d\n",
            scn->wlan_resource_config.num_peers,
            scn->wlan_resource_config.num_vdevs,
            scn->wlan_resource_config.num_tids);
    }

    OS_MEMCPY(target_cap.wmi_service_bitmap,scn->wmi_service_bitmap,sizeof(scn->wmi_service_bitmap));
    target_cap.wlan_resource_config = scn->wlan_resource_config;

    /* call back into  os shim with the services bitmap and resource config to let
     * the os shim layer modify it according to its needs and requirements */
    if (scn->cfg_cb) {
        scn->cfg_cb(scn, &target_cap);
        OS_MEMCPY(scn->wmi_service_bitmap,target_cap.wmi_service_bitmap, sizeof(scn->wmi_service_bitmap));
        scn->wlan_resource_config = target_cap.wlan_resource_config;
    }

    ol_ath_update_caps(ic, ev, scn->wmi_service_bitmap);
    OS_MEMCPY(&scn->hal_reg_capabilities, &ev->hal_reg_capabilities, sizeof(HAL_REG_CAPABILITIES)); 
    scn->max_tx_power = ev->hw_max_tx_power;
    scn->min_tx_power = ev->hw_min_tx_power;

    scn->txpowlimit2G = scn->max_tx_power;
    scn->txpowlimit5G = scn->max_tx_power;
    scn->txpower_scale = WMI_TP_SCALE_MAX;

    ieee80211com_set_txpowerlimit(ic, scn->max_tx_power);

    ol_regdmn_attach(scn);

    ol_regdmn_set_regdomain(scn->ol_regdmn_handle, scn->hal_reg_capabilities.eeprom_rd);
    ol_regdmn_set_regdomain_ext(scn->ol_regdmn_handle, scn->hal_reg_capabilities.eeprom_rd_ext);
  
    cmd = (wmi_init_cmd *)wmi_buf_data(buf);
    cmd->resource_config = scn->wlan_resource_config;
    /* allocate memory requested by FW */
    ASSERT (ev->num_mem_reqs <= WMI_MAX_MEM_REQS); 
    cmd->num_host_mem_chunks = 0;   
    if (ev->num_mem_reqs) {
        u_int32_t num_units;
        for(idx=0;idx < ev->num_mem_reqs; ++idx) {  
            num_units = ev->mem_reqs[idx].num_units;
            if ( ev->mem_reqs[idx].num_unit_info ) {
               if  ( ev->mem_reqs[idx].num_unit_info & NUM_UNITS_IS_NUM_PEERS ) {
	            /* number of units to allocate is number of peers, 1 extra for self peer on target */
                   /* this needs to be fied, host and target can get out of sync */
                    num_units = cmd->resource_config.num_peers + 1; 
               } 
            }
           printk("idx %d req %d  num_units %d num_unit_info %d unit size %d actual units %d \n",idx, 
                                   ev->mem_reqs[idx].req_id,
                                   ev->mem_reqs[idx].num_units,
                                   ev->mem_reqs[idx].num_unit_info,
                                   ev->mem_reqs[idx].unit_size,
                                   num_units);
            ol_ath_alloc_host_mem(scn_handle, ev->mem_reqs[idx].req_id,
                                   num_units,
                                   ev->mem_reqs[idx].unit_size);
        }
        for(idx=0;idx<scn->num_mem_chunks; ++idx) {  
           cmd->host_mem_chunks[idx].ptr = scn->mem_chunks[idx].paddr;
           cmd->host_mem_chunks[idx].size = scn->mem_chunks[idx].len;
           cmd->host_mem_chunks[idx].req_id = scn->mem_chunks[idx].req_id;
           printk("chunk %d len %d requested ,ptr  0x%x \n",idx, 
           cmd->host_mem_chunks[idx].size ,
              cmd->host_mem_chunks[idx].ptr ) ;
        }
        cmd->num_host_mem_chunks = scn->num_mem_chunks;   
        if (scn->num_mem_chunks > 1 ) { 
            len += ((scn->num_mem_chunks-1) * sizeof(wlan_host_memory_chunk)) ;
        }
    }
    wmi_unified_cmd_send(scn->wmi_handle, buf, len, WMI_INIT_CMDID);
}

void
ol_ath_ready_event(ol_scn_t scn_handle, wmi_ready_event *ev)
{
    struct ol_ath_softc_net80211 *scn = (struct ol_ath_softc_net80211 *)scn_handle;

    scn->version.wlan_ver = ev->sw_version;
    scn->version.abi_ver = ev->abi_version;

    /*
     * Indicate to the waiting thread that the ready
     * event was received
     */
    scn->wmi_ready = TRUE;
    scn->wlan_init_status = ev->status;
    /* copy the mac addr */
    WMI_MAC_ADDR_TO_CHAR_ARRAY (&ev->mac_addr, scn->sc_ic.ic_myaddr);
    WMI_MAC_ADDR_TO_CHAR_ARRAY (&ev->mac_addr, scn->sc_ic.ic_my_hwaddr);
    __ol_ath_wmi_ready_event(scn);

}

/*
 *  WMI API for getting TPC configuration
 */
int
wmi_unified_pdev_get_tpc_config(wmi_unified_t wmi_handle, u_int32_t param)
{
    wmi_pdev_get_tpc_config_cmd *cmd;
    wmi_buf_t buf;
    int32_t len = sizeof(wmi_pdev_get_tpc_config_cmd);

    buf = wmi_buf_alloc(wmi_handle, len);
    if (!buf) {
        printk("%s:wmi_buf_alloc failed\n", __FUNCTION__);
        return -1;
    }
    cmd = (wmi_pdev_get_tpc_config_cmd *)wmi_buf_data(buf);
    cmd->param = param;
    return wmi_unified_cmd_send(wmi_handle, buf, len, WMI_PDEV_GET_TPC_CONFIG_CMDID);
}

/*
 *  WMI API for setting device params
 */
int
wmi_unified_pdev_set_param(wmi_unified_t wmi_handle, u_int32_t param_id,
                           u_int32_t param_value) 
{
    wmi_pdev_set_param_cmd *cmd;
    wmi_buf_t buf;
    int len = sizeof(wmi_pdev_set_param_cmd);
    buf = wmi_buf_alloc(wmi_handle, len);
    if (!buf) {
        printk("%s:wmi_buf_alloc failed\n", __FUNCTION__);
        return -1;
    }
    cmd = (wmi_pdev_set_param_cmd *)wmi_buf_data(buf);
    cmd->param_id = param_id;
    cmd->param_value = param_value;
    return wmi_unified_cmd_send(wmi_handle, buf, len, WMI_PDEV_SET_PARAM_CMDID);
}

int
wmi_unified_wlan_profile_enable(wmi_unified_t wmi_handle, u_int32_t param_id,
                           u_int32_t param_value)
{
    wmi_wlan_profile_trigger_cmd *cmd;
    wmi_buf_t buf;
    int len = sizeof(wmi_wlan_profile_trigger_cmd);
    buf = wmi_buf_alloc(wmi_handle, len);
    if (!buf) {
        printk("%s:wmi_buf_alloc failed\n", __FUNCTION__);
        return -1;
    }
    cmd = (wmi_wlan_profile_trigger_cmd *)wmi_buf_data(buf);
    cmd->enable = param_value;

    return wmi_unified_cmd_send(wmi_handle, buf, len, WMI_WLAN_PROFILE_TRIGGER_CMDID);
}

int
wmi_unified_pdev_set_channel(wmi_unified_t wmi_handle, 
                           struct ieee80211_channel *chan, u_int32_t freq)
{
    wmi_set_channel_cmd *cmd;
    wmi_buf_t buf;
    u_int32_t chan_mode;
    static const u_int modeflags[] = {
        0,                            /* IEEE80211_MODE_AUTO           */
        MODE_11A,         /* IEEE80211_MODE_11A            */
        MODE_11B,         /* IEEE80211_MODE_11B            */
        MODE_11G,         /* IEEE80211_MODE_11G            */
        0,                            /* IEEE80211_MODE_FH             */
        0,                            /* IEEE80211_MODE_TURBO_A        */
        0,                            /* IEEE80211_MODE_TURBO_G        */
        MODE_11NA_HT20,   /* IEEE80211_MODE_11NA_HT20      */
        MODE_11NG_HT20,   /* IEEE80211_MODE_11NG_HT20      */
        MODE_11NA_HT40,   /* IEEE80211_MODE_11NA_HT40PLUS  */
        MODE_11NA_HT40,   /* IEEE80211_MODE_11NA_HT40MINUS */
        MODE_11NG_HT40,   /* IEEE80211_MODE_11NG_HT40PLUS  */
        MODE_11NG_HT40,   /* IEEE80211_MODE_11NG_HT40MINUS */
        MODE_11NG_HT40,   /* IEEE80211_MODE_11NG_HT40      */
        MODE_11NA_HT40,   /* IEEE80211_MODE_11NA_HT40      */
        MODE_11AC_VHT20,  /* IEEE80211_MODE_11AC_VHT20     */
        MODE_11AC_VHT40,  /* IEEE80211_MODE_11AC_VHT40PLUS */
        MODE_11AC_VHT40,  /* IEEE80211_MODE_11AC_VHT40MINUS*/
        MODE_11AC_VHT40,  /* IEEE80211_MODE_11AC_VHT40     */
        MODE_11AC_VHT80,  /* IEEE80211_MODE_11AC_VHT80     */
    };
    
    int len = sizeof(wmi_set_channel_cmd);

    buf = wmi_buf_alloc(wmi_handle, len);
    if (!buf) {
        printk("%s:wmi_buf_alloc failed\n", __FUNCTION__);
        return -1;
    }

    cmd = (wmi_set_channel_cmd *)wmi_buf_data(buf);

    cmd->chan.mhz = freq;

    chan_mode = ieee80211_chan2mode(chan);
    WMI_SET_CHANNEL_MODE(&cmd->chan, modeflags[chan_mode]);
    
    if(chan_mode == IEEE80211_MODE_11AC_VHT80) {
            if (chan->ic_ieee < 20)
                cmd->chan.band_center_freq1 = ieee80211_ieee2mhz(chan->ic_vhtop_ch_freq_seg1, IEEE80211_CHAN_2GHZ); 
            else
                cmd->chan.band_center_freq1 = ieee80211_ieee2mhz(chan->ic_vhtop_ch_freq_seg1, IEEE80211_CHAN_5GHZ);            
    } else if((chan_mode == IEEE80211_MODE_11NA_HT40PLUS) || (chan_mode == IEEE80211_MODE_11NG_HT40PLUS) ||
        (chan_mode == IEEE80211_MODE_11AC_VHT40PLUS)) {
            cmd->chan.band_center_freq1 = freq + 10;
    } else if((chan_mode == IEEE80211_MODE_11NA_HT40MINUS) || (chan_mode == IEEE80211_MODE_11NG_HT40MINUS) ||
        (chan_mode == IEEE80211_MODE_11AC_VHT40MINUS)) {
            cmd->chan.band_center_freq1 = freq - 10;
    } else {
            cmd->chan.band_center_freq1 = freq;
    }
    /* we do not support HT80PLUS80 yet */
    cmd->chan.band_center_freq2=0;
        
    WMI_SET_CHANNEL_MIN_POWER(&cmd->chan, chan->ic_minpower);
    WMI_SET_CHANNEL_MAX_POWER(&cmd->chan, chan->ic_maxpower);
    WMI_SET_CHANNEL_REG_POWER(&cmd->chan, chan->ic_maxregpower);
    WMI_SET_CHANNEL_ANTENNA_MAX(&cmd->chan, chan->ic_antennamax);
    WMI_SET_CHANNEL_REG_CLASSID(&cmd->chan, chan->ic_regClassId);
    
    if (IEEE80211_IS_CHAN_DFS(chan))
        WMI_SET_CHANNEL_FLAG(&cmd->chan, WMI_CHAN_FLAG_DFS);
    if (IEEE80211_IS_CHAN_HALF(chan))
        WMI_SET_CHANNEL_FLAG(&cmd->chan, WMI_CHAN_FLAG_HALF);
    if (IEEE80211_IS_CHAN_QUARTER(chan))
        WMI_SET_CHANNEL_FLAG(&cmd->chan, WMI_CHAN_FLAG_QUARTER);

    printk("WMI channel freq=%d, mode=%x band_center_freq1=%d\n", cmd->chan.mhz, 
        WMI_GET_CHANNEL_MODE(&cmd->chan), cmd->chan.band_center_freq1);

    return wmi_unified_cmd_send(wmi_handle, buf, len, 
                                WMI_PDEV_SET_CHANNEL_CMDID);
}

int
wmi_unified_pdev_set_ht_ie(wmi_unified_t wmi_handle, u_int32_t ie_len, u_int8_t *ie_data)
{
    wmi_pdev_set_ht_ie_cmd *cmd;
    wmi_buf_t buf;
    /* adjust length to be next multiple of four */
    int len = (ie_len + (sizeof(u_int32_t) - 1)) & (~(sizeof(u_int32_t) - 1)); 
    len += (sizeof(wmi_pdev_set_ht_ie_cmd) - 4 /* to account for extra four bytes of ie data in the struct */);
    
    buf = wmi_buf_alloc(wmi_handle, len);
    if (!buf) {
        printk("%s:wmi_buf_alloc failed\n", __FUNCTION__);
        return -1;
    }
    cmd = (wmi_pdev_set_ht_ie_cmd  *)wmi_buf_data(buf);
    cmd->ie_len = ie_len;
    OS_MEMCPY(cmd->ie_data,ie_data,ie_len);
#ifdef BIG_ENDIAN_HOST
	SWAPME(cmd->ie_data,len-(offsetof(wmi_pdev_set_ht_ie_cmd, ie_data)));
#endif
    return wmi_unified_cmd_send(wmi_handle, buf, len, WMI_PDEV_SET_HT_CAP_IE_CMDID);
}

int
wmi_unified_pdev_set_vht_ie(wmi_unified_t wmi_handle, u_int32_t ie_len, u_int8_t *ie_data)
{
    wmi_pdev_set_vht_ie_cmd *cmd;
    wmi_buf_t buf;
    /* adjust length to be next multiple of four */
    int len = (ie_len + (sizeof(u_int32_t) - 1)) & (~(sizeof(u_int32_t) - 1)); 
    len += (sizeof(wmi_pdev_set_vht_ie_cmd) - 4 /* to account for extra four bytes of ie data in the struct */);
    
    buf = wmi_buf_alloc(wmi_handle, len);
    if (!buf) {
        printk("%s:wmi_buf_alloc failed\n", __FUNCTION__);
        return -1;
    }
    cmd = (wmi_pdev_set_vht_ie_cmd *)wmi_buf_data(buf);
    cmd->ie_len = ie_len;
    OS_MEMCPY(cmd->ie_data,ie_data,ie_len);
#ifdef BIG_ENDIAN_HOST
	SWAPME(cmd->ie_data,len-(offsetof(wmi_pdev_set_vht_ie_cmd, ie_data)));
#endif
    return wmi_unified_cmd_send(wmi_handle, buf, len, WMI_PDEV_SET_VHT_CAP_IE_CMDID);
}

#define MAX_IE_SIZE 512 

void ol_ath_set_ht_vht_ies(struct ieee80211_node *ni)
{
    struct ieee80211com *ic = ni->ni_ic;
    struct ieee80211vap *vap = ni->ni_vap;
    struct ol_ath_softc_net80211 *scn = OL_ATH_SOFTC_NET80211(ic);
    if (!scn->set_ht_vht_ies) {
        u_int8_t *buf=adf_os_mem_alloc(scn->adf_dev, MAX_IE_SIZE);
        u_int8_t *buf_end;
        if (buf) {
            buf_end = ieee80211_add_htcap(buf, vap->iv_bss,IEEE80211_FC0_SUBTYPE_PROBE_REQ);
            if ((buf_end - buf ) <= MAX_HT_IE_LEN ) {
                wmi_unified_pdev_set_ht_ie(scn->wmi_handle,buf_end-buf, buf);
            } else {
                printk("%s: HT IE length %d is more than expected\n",__func__, (buf_end-buf));
            }
            buf_end = ieee80211_add_vhtcap(buf, vap->iv_bss,ic, IEEE80211_FC0_SUBTYPE_PROBE_REQ);
            if ((buf_end - buf ) <= MAX_VHT_IE_LEN ) {
                wmi_unified_pdev_set_vht_ie(scn->wmi_handle,buf_end-buf,buf);
            } else {
                printk("%s: VHT IE length %d is more than expected\n",__func__, (buf_end-buf));
            }
            scn->set_ht_vht_ies = 1;
            adf_os_mem_free(buf);
        }
    }
}

struct ieee80211_channel *
ol_ath_find_full_channel(struct ieee80211com *ic, u_int32_t freq) 
{
    struct ieee80211_channel    *c;
    c = NULL;
#define IEEE80211_2GHZ_FREQUENCY_THRESHOLD    3000            // in kHz
    if (freq < IEEE80211_2GHZ_FREQUENCY_THRESHOLD) { /* 2GHZ channel */
        if (IEEE80211_SUPPORT_PHY_MODE(ic, IEEE80211_MODE_11NG_HT20)) {
            c = ieee80211_find_channel(ic, freq, IEEE80211_CHAN_11NG_HT20); 
        }

        if (c == NULL) {
            c = ieee80211_find_channel(ic, freq, IEEE80211_CHAN_G);
        }


        if (c == NULL) {
            c = ieee80211_find_channel(ic, freq, IEEE80211_CHAN_PUREG);
        }

        if (c == NULL) {
            c = ieee80211_find_channel(ic, freq, IEEE80211_CHAN_B);
        }
    } else {
        if (IEEE80211_SUPPORT_PHY_MODE(ic, IEEE80211_MODE_11NA_HT20)) {
            c = ieee80211_find_channel(ic, freq, IEEE80211_CHAN_11NA_HT20); 
        }

        if (c == NULL) {
            u_int32_t halfquarter = ic->ic_chanbwflag & (IEEE80211_CHAN_HALF | IEEE80211_CHAN_QUARTER);
            c = ieee80211_find_channel(ic, freq, IEEE80211_CHAN_A | halfquarter);
        }
    }
    return c;
#undef IEEE80211_2GHZ_FREQUENCY_THRESHOLD    
}

/* Offload Interface functions for UMAC */
static int
ol_ath_init(struct ieee80211com *ic)
{
    /* TBD */
    return 0;
}

static int
ol_ath_reset_start(struct ieee80211com *ic, bool no_flush)
{
    /* TBD */
    return 0;
}

static int
ol_ath_reset_end(struct ieee80211com *ic, bool no_flush)
{
    /* TBD */
    return 0;
}

static int
ol_ath_reset(struct ieee80211com *ic)
{
    /* TBD */
    return 0;
}

static void
ol_ath_updateslot(struct ieee80211com *ic)
{
    struct ol_ath_softc_net80211 *scn = OL_ATH_SOFTC_NET80211(ic);
    int slottime;

    /* TODO: Pass this info correctly */
    slottime = (IEEE80211_IS_SHSLOT_ENABLED(ic)) ?  9 : 20;
    wmi_unified_pdev_set_param(scn->wmi_handle,
        WMI_PDEV_PARAM_PROTECTION_MODE, slottime);

    return;
}

static int
ol_ath_wmm_update(struct ieee80211com *ic)
{
#define ATH_EXPONENT_TO_VALUE(v)    ((1<<v)-1)
#define ATH_TXOP_TO_US(v)           (v<<5)
    struct ol_ath_softc_net80211 *scn = OL_ATH_SOFTC_NET80211(ic);
    wmi_buf_t buf;
    wmi_pdev_set_wmm_params_cmd *cmd;
    wmi_wmm_params *wmi_param = 0;
    int ac;
    struct wmeParams *wmep;
    int len = sizeof(wmi_pdev_set_wmm_params_cmd);

    buf = wmi_buf_alloc(scn->wmi_handle, len);
    printk("%s:\n", __FUNCTION__);
    if (!buf) {
        printk("%s:wmi_buf_alloc failed\n", __FUNCTION__);
        return 0;
    }
    cmd = (wmi_pdev_set_wmm_params_cmd*)wmi_buf_data(buf);

    for (ac = 0; ac < WME_NUM_AC; ac++) {
        wmep = ieee80211com_wmm_chanparams(ic, ac);

        switch (ac) {
        case WME_AC_BE:
            wmi_param = &cmd->wmm_params_ac_be;
            break;
        case WME_AC_BK:
            wmi_param = &cmd->wmm_params_ac_bk;
            break;
        case WME_AC_VI:
            wmi_param = &cmd->wmm_params_ac_vi;
            break;
        case WME_AC_VO:
            wmi_param = &cmd->wmm_params_ac_vo;
            break;
        default:
            break;
        }

        wmi_param->aifs = wmep->wmep_aifsn;
        wmi_param->cwmin = ATH_EXPONENT_TO_VALUE(wmep->wmep_logcwmin);
        wmi_param->cwmax = ATH_EXPONENT_TO_VALUE(wmep->wmep_logcwmax);
        wmi_param->txoplimit = ATH_TXOP_TO_US(wmep->wmep_txopLimit);
        wmi_param->acm = wmep->wmep_acm;
        wmi_param->no_ack = wmep->wmep_noackPolicy;
    
#if 0
        printk("WMM PARAMS AC [%d]: AIFS %d Min %d Max %d TXOP %d ACM %d NOACK %d\n", 
                ac,
                wmi_param->aifs,
                wmi_param->cwmin,
                wmi_param->cwmax,
                wmi_param->txoplimit,
                wmi_param->acm,
                wmi_param->no_ack
                );
#endif
    }

    wmi_unified_cmd_send(scn->wmi_handle, buf, len, WMI_PDEV_SET_WMM_PARAMS_CMDID);
    return 0;
}


static u_int32_t
ol_ath_txq_depth(struct ieee80211com *ic)
{
    /* TBD */
    return 0;
}

static u_int32_t
ol_ath_txq_depth_ac(struct ieee80211com *ic,int ac)
{
    /* TBD */
    return 0;
}

/*
 * Function to set 802.11 protection mode
 */
static void
ol_ath_update_protmode(struct ieee80211com *ic)
{
    struct ol_ath_softc_net80211 *scn = OL_ATH_SOFTC_NET80211(ic);

    wmi_unified_pdev_set_param(scn->wmi_handle,
        WMI_PDEV_PARAM_PROTECTION_MODE, ic->ic_protmode);
    return;
}

static void
ol_net80211_chwidth_change(struct ieee80211_node *ni)
{
    struct ieee80211com *ic = ni->ni_ic;
    struct ol_ath_softc_net80211 *scn = OL_ATH_SOFTC_NET80211(ic);
    struct ol_ath_vap_net80211 *avn = OL_ATH_VAP_NET80211(ni->ni_vap);

    if(wmi_unified_node_set_param(scn->wmi_handle,ni->ni_macaddr,WMI_PEER_CHWIDTH,
            ni->ni_chwidth,avn->av_if_id)) {
        printk("%s:Unable to change peer bandwidth\n", __func__);
    }
}

static void
ol_net80211_nss_change(struct ieee80211_node *ni)
{
    struct ieee80211com *ic = ni->ni_ic;
    struct ol_ath_softc_net80211 *scn = OL_ATH_SOFTC_NET80211(ic);
    struct ol_ath_vap_net80211 *avn = OL_ATH_VAP_NET80211(ni->ni_vap);

    if(wmi_unified_node_set_param(scn->wmi_handle,ni->ni_macaddr,WMI_PEER_NSS,
            ni->ni_streams,avn->av_if_id)) {
        printk("%s:Unable to change peer Nss\n", __func__);
    }

    ol_ath_node_update(ni);
}

struct ol_vap_mode_count {
    u_int32_t non_monitor_mode_vap_cnt;
    u_int32_t monitor_mode_vap_cnt;
};

static void check_monitor_only_vapmode(void *arg, struct ieee80211vap *vap)
{
    struct ol_vap_mode_count *mode_cnt = (struct ol_vap_mode_count *)arg;

    if (IEEE80211_M_MONITOR != vap->iv_opmode) {
        mode_cnt->non_monitor_mode_vap_cnt++;
    } else if (IEEE80211_M_MONITOR == vap->iv_opmode) {
        mode_cnt->monitor_mode_vap_cnt++;
    }

    return;
}


static void get_monitor_mode_vap(void *arg, struct ieee80211vap *vap)
{
    ieee80211_vap_t *ppvap=(ieee80211_vap_t *)arg;

    if (IEEE80211_M_MONITOR == vap->iv_opmode) {
        *ppvap = vap;
    }

    return;
}

static void get_ap_mode_vap(void *arg, struct ieee80211vap *vap)
{
    ieee80211_vap_t *ppvap=(ieee80211_vap_t *)arg;

    if (IEEE80211_M_HOSTAP == vap->iv_opmode) {
        *ppvap = vap;
    }

    return;
}

static int
ol_ath_set_channel(struct ieee80211com *ic)
{
    struct ol_ath_softc_net80211 *scn = OL_ATH_SOFTC_NET80211(ic);
    struct ieee80211_channel *chan;
    u_int32_t freq;
    ieee80211_vap_t vap = NULL;
    struct ol_ath_vap_net80211 *avn = NULL;
    struct ol_vap_mode_count mode_cnt;

    chan = ieee80211_get_current_channel(ic);
    
    freq = ieee80211_chan2freq(ic, chan);
    if (!freq) {
        printk("ERROR : INVALID Freq \n");
        return 0;
    }
    
    printk("set channel freq %d \n",freq);

    /* update max channel power to max regpower of current channel */
    ieee80211com_set_curchanmaxpwr(ic, chan->ic_maxregpower);

    /* Update the channel for monitor mode path */
    ol_txrx_set_curchan(scn->pdev_txrx_handle, freq);

    /* Call a new channel change WMI cmd for monitor mode only. For all other
     * modes the vdev_start would happen via the resmgr_vap_start() path.
     */
    mode_cnt.non_monitor_mode_vap_cnt = 0;
    mode_cnt.monitor_mode_vap_cnt = 0;

    wlan_iterate_vap_list(ic, check_monitor_only_vapmode ,(void *)&mode_cnt);

    if (mode_cnt.non_monitor_mode_vap_cnt == 0 && mode_cnt.monitor_mode_vap_cnt >= 1) {

        wlan_iterate_vap_list(ic, get_monitor_mode_vap ,(void *)&vap );
        if (vap) {
            avn = OL_ATH_VAP_NET80211(vap);
            /* TODO: To be replaced with a new WMI cmd to be used by host for
             * channel change. The host would wait for a Asycn resp from the 
             * target confirming/denying channel change.
             */ 
            if (wmi_unified_vdev_restart_send(scn->wmi_handle, 
                     avn->av_if_id, chan, 
                     freq, IEEE80211_IS_CHAN_DFS(chan))) {
                printk("%s[%d] Unable to bring up the interface for ath_dev.\n", __func__, __LINE__);
                return -1;
            }
        }
    } else {

        /* Call a new channel change WMI cmd for AP mode.
        */
        vap = NULL;

        wlan_iterate_vap_list(ic, get_ap_mode_vap ,(void *)&vap );
    
        if (vap) {
            avn = OL_ATH_VAP_NET80211(vap);
            if (wmi_unified_vdev_restart_send(scn->wmi_handle, 
                     avn->av_if_id, chan, 
                     freq, IEEE80211_IS_CHAN_DFS(chan))) {
                printk("%s[%d] Unable to bring up the interface for ath_dev.\n", __func__, __LINE__);
                return -1;
            }
        }
    }
    if (avn) {
       avn->av_ol_resmgr_wait = TRUE;
    } else {
       printk("%s[%d] Error avn = NULL \n", __func__, __LINE__);
    }
#if ATH_SUPPORT_DFS
    ol_if_dfs_configure(ic);
#endif
    /* once the channel change is complete, turn on the dcs, 
     * use the same state as what the current enabled state of the dcs. Also
     * set the run state accordingly.
     */
    (void)wmi_unified_pdev_set_param(scn->wmi_handle, WMI_PDEV_PARAM_DCS, scn->scn_dcs.dcs_enable&0x0f);
    printk("DCS previous state is restored \n");

    (OL_IS_DCS_ENABLED(scn->scn_dcs.dcs_enable)) ? (OL_ATH_DCS_SET_RUNSTATE(scn->scn_dcs.dcs_enable)) :
                            (OL_ATH_DCS_CLR_RUNSTATE(scn->scn_dcs.dcs_enable)); 

    return 0;
}

static void 
ol_ath_log_text(struct ieee80211com *ic, char *text)
{
    /* TBD */
    return;
}

static void
ol_ath_pwrsave_set_state(struct ieee80211com *ic, IEEE80211_PWRSAVE_STATE newstate)
{
    /* The host does not manage the HW power state with offload FW. This function
     * exists solely for completeness.
     */
}

u_int
ol_ath_mhz2ieee(struct ieee80211com *ic, u_int freq, u_int flags)
{
#define IS_CHAN_IN_PUBLIC_SAFETY_BAND(_c) ((_c) > 4940 && (_c) < 4990)
struct ol_ath_softc_net80211 *scn = OL_ATH_SOFTC_NET80211(ic);

    /* TBD */
     /* FIXME: This change has been added for testing 
      * This below code block is copied from direct attach architecture
      * Some of the code has been disabled because those APIs and Macros yet
      * to be defined
      */
        
#if 0
    if (flags & CHANNEL_2GHZ) { /* 2GHz band */
        if (freq == 2484)
            return 14;
        if (freq < 2484)
            return (freq - 2407) / 5;
        else
            return 15 + ((freq - 2512) / 20);
    } else if (flags & CHANNEL_5GHZ) {/* 5Ghz band */
     /*   if (ath_hal_ispublicsafetysku(ah) &&
            IS_CHAN_IN_PUBLIC_SAFETY_BAND(freq)) {
            return ((freq * 10) + 
                (((freq % 5) == 2) ? 5 : 0) - 49400) / 5;
        } else */if ((flags & CHANNEL_A) && (freq <= 5000)) {
            return (freq - 4000) / 5;
        } else {
            return (freq - 5000) / 5;
        }
    } else 
#endif
    {            /* either, guess */
        if (freq == 2484)
            return 14;
        if (freq < 2484)
            return (freq - 2407) / 5;
        if (freq < 5000) {
            if (ol_regdmn_ispublicsafetysku(scn->ol_regdmn_handle)
                && IS_CHAN_IN_PUBLIC_SAFETY_BAND(freq)) {
                return ((freq * 10) +
                    (((freq % 5) == 2) ? 5 : 0) - 49400)/5;
            } else if (freq > 4900) {
                return (freq - 4000) / 5;
            } else {
                return 15 + ((freq - 2512) / 20);
            }
        }
        return (freq - 5000) / 5;
    }
}


static int16_t ol_ath_get_noisefloor (struct ieee80211com *ic, struct ieee80211_channel *chan)
{
    /* TBD */
    return 0;
}

static int16_t ol_ath_net80211_get_cur_chan_noisefloor(struct ieee80211com *ic)
{
    struct ol_ath_softc_net80211 *scn = OL_ATH_SOFTC_NET80211(ic);
    return scn->chan_nf;
}

static void
ol_ath_get_chainnoisefloor(struct ieee80211com *ic, struct ieee80211_channel *chan, int16_t *nfBuf)
{
    /* TBD */
    return;
}

void
ol_ath_setTxPowerLimit(struct ieee80211com *ic, u_int32_t limit, u_int16_t tpcInDb, u_int32_t is2GHz)
{   
    int retval = 0;
    struct ol_ath_softc_net80211 *scn = OL_ATH_SOFTC_NET80211(ic);
    u_int16_t cur_tx_power = ieee80211com_get_txpowerlimit(ic);
            
    if (cur_tx_power != limit) {
        /* Update max tx power only if the current max tx power is different */
        if (limit > scn->max_tx_power) {
                printk("ERROR - Tx power value is greater than supported max tx power %d \n",
                    scn->max_tx_power);
		return;
        }
        if (is2GHz) {
            retval = wmi_unified_pdev_set_param(scn->wmi_handle,
                WMI_PDEV_PARAM_TXPOWER_LIMIT2G, limit);
        } else {
            retval = wmi_unified_pdev_set_param(scn->wmi_handle,
                WMI_PDEV_PARAM_TXPOWER_LIMIT5G, limit);
        }
        if (retval == EOK) {
            /* Update the ic_txpowlimit */
            if (is2GHz) {
                scn->txpowlimit2G = limit;              
            } else {
                scn->txpowlimit5G = limit;
            }	
            if ((is2GHz && IEEE80211_IS_CHAN_2GHZ(ic->ic_curchan)) ||
                (!is2GHz && !IEEE80211_IS_CHAN_2GHZ(ic->ic_curchan)))
            {
                ieee80211com_set_txpowerlimit(ic, (u_int16_t) (limit));
            }
        }
    }
}

static u_int8_t
ol_ath_get_common_power(struct ieee80211com *ic, struct ieee80211_channel *chan)
{
    /* TBD */
    return 0;
}

static u_int32_t
ol_ath_getTSF32(struct ieee80211com *ic)
{
    /* TBD */
    return 0;
}

static int
ol_ath_getrmcounters(struct ieee80211com *ic, struct ieee80211_mib_cycle_cnts *pCnts)
{
    struct ol_ath_softc_net80211 *scn = OL_ATH_SOFTC_NET80211(ic);

    /* "ic->ic_mib_cycle_cnts" gets updated during a periodic stats event */
    pCnts->tx_frame_count = scn->mib_cycle_cnts.tx_frame_count;
    pCnts->rx_frame_count = scn->mib_cycle_cnts.rx_frame_count;
    pCnts->rx_clear_count = scn->mib_cycle_cnts.rx_clear_count;
    pCnts->cycle_count = scn->mib_cycle_cnts.cycle_count;

    /* "is_rx_active" and "is_tx_active" not being used, but for safety, set it to 0 */
    pCnts->is_rx_active = 0;
    pCnts->is_tx_active = 0;

    return 0;
}

static u_int32_t
ol_ath_wpsPushButton(struct ieee80211com *ic)
{
    /* TBD */
    return 0;
}

static void
ol_ath_clear_phystats(struct ieee80211com *ic)
{
    /* TBD */
    return;
}

static int
ol_ath_set_macaddr(struct ieee80211com *ic, u_int8_t *macaddr)
{
    /* TBD */
    return 0;
}

static int
ol_ath_set_chain_mask(struct ieee80211com *ic, ieee80211_device_param type, u_int32_t mask)
{
    /* TBD */
    return 0;
}

static u_int32_t
ol_ath_getmfpsupport(struct ieee80211com *ic)
{
    return IEEE80211_MFP_HW_CRYPTO;
}

static void
ol_ath_setmfpQos(struct ieee80211com *ic, u_int32_t dot11w)
{
    struct ol_ath_softc_net80211 *scn = OL_ATH_SOFTC_NET80211(ic);
    wmi_unified_pdev_set_param(scn->wmi_handle,
                               WMI_PDEV_PARAM_PMF_QOS, dot11w);
    return;
}

static u_int64_t 
ol_ath_get_tx_hw_retries(struct ieee80211com *ic)
{
    /* TBD */
    return 0;
}

static u_int64_t 
ol_ath_get_tx_hw_success(struct ieee80211com *ic)
{
    /* TBD */
    return 0;
}

static void
ol_ath_mcast_group_update(
    struct ieee80211com *ic,
    int action,
    int wildcard,
    u_int8_t *mcast_ip_addr,
    int mcast_ip_addr_bytes,
    u_int8_t *ucast_mac_addr)
{
    struct ol_ath_softc_net80211 *scn = OL_ATH_SOFTC_NET80211(ic);
    wmi_peer_mcast_group_cmd *cmd;
    wmi_buf_t buf;
    int len;
    int offset;

    len = sizeof(wmi_peer_mcast_group_cmd);
    buf = wmi_buf_alloc(scn->wmi_handle, len);
    if (!buf) {
        printk("%s: wmi_buf_alloc failed\n", __FUNCTION__);
        return;
    }
    cmd = (wmi_peer_mcast_group_cmd *) wmi_buf_data(buf);
    /* confirm the buffer is 4-byte aligned */
    ASSERT((((size_t) cmd) & 0x3) == 0);

    /* construct the message assuming our endianness matches the target */
    cmd->fields.flags |= WMI_PEER_MCAST_GROUP_FLAG_ACTION_M &
        (action << WMI_PEER_MCAST_GROUP_FLAG_ACTION_S);
    cmd->fields.flags |= WMI_PEER_MCAST_GROUP_FLAG_WILDCARD_M &
        (wildcard << WMI_PEER_MCAST_GROUP_FLAG_WILDCARD_S);

    /* unicast address spec only applies for non-wildcard cases */
    if (!wildcard) {
        OS_MEMCPY(
            &cmd->fields.ucast_mac_addr,
            ucast_mac_addr,
            sizeof(cmd->fields.ucast_mac_addr));
    }
    ASSERT(mcast_ip_addr_bytes <= sizeof(cmd->fields.mcast_ip_addr));
    offset = sizeof(cmd->fields.mcast_ip_addr) - mcast_ip_addr_bytes;
    OS_MEMCPY(
        ((u_int8_t *) &cmd->fields.mcast_ip_addr) + offset,
        mcast_ip_addr,
        mcast_ip_addr_bytes);

    /* now correct for endianness, if necessary */
    ol_bytestream_endian_fix(
        (u_int32_t *)cmd, sizeof(*cmd) / sizeof(u_int32_t));

    wmi_unified_cmd_send(
        scn->wmi_handle, buf, len, WMI_PEER_MCAST_GROUP_CMDID);
}


	static int
wmi_unified_debug_print_event_handler (ol_scn_t scn, u_int8_t *data, u_int16_t datalen, void *context)
{
#ifdef BIG_ENDIAN_HOST
	char dbgbuf[500] = {0};
	memcpy(dbgbuf, data, datalen);
	SWAPME(dbgbuf, datalen);
	printk("FIRMWARE:%s \n",dbgbuf);
	return 0;
#else
	printk("FIRMWARE:%s \n",data);
	return 0;
#endif
}

static void
ol_ath_set_config(struct ieee80211vap* vap)
{
    /* Currently Not used for Offload */
}

static void
ol_ath_set_safemode(struct ieee80211vap* vap, int val)
{
    ol_txrx_vdev_handle vdev = (ol_txrx_vdev_handle) vap->iv_txrx_handle;
    if (vdev) {
        ol_txrx_set_safemode(vdev, val);
    }
    return;
}

static void
ol_ath_scan_start(struct ieee80211com *ic)
{
#ifdef DEPRECATED
    /*
     * this command was added to support host scan egine which is deprecated.
     * now  the scan engine is in FW and host directly isssues a scan request 
     * to perform scan and provide results back to host 
     */
    struct ol_ath_softc_net80211 *scn = OL_ATH_SOFTC_NET80211(ic);
    wmi_buf_t buf;
    wmi_pdev_scan_cmd *cmd;
    int len = sizeof(wmi_pdev_scan_cmd);
    buf = wmi_buf_alloc(scn->wmi_handle, len);
    printk("%s:\n", __FUNCTION__);
    if (!buf) {
        printk("%s:wmi_buf_alloc failed\n", __FUNCTION__);
        return ;
    }
    cmd = (wmi_pdev_scan_cmd *)wmi_buf_data(buf);
    cmd->scan_start = TRUE;
    wmi_unified_cmd_send(scn->wmi_handle, buf, len, WMI_PDEV_SCAN_CMDID);
#endif
}

static void
ol_ath_scan_end(struct ieee80211com *ic)
{
#ifdef DEPRECATED
    /*
     * this command was added to support host scan egine which is deprecated.
     * now  the scan engine is in FW and host directly isssues a scan request 
     * to perform scan and provide results back to host 
     */
    struct ol_ath_softc_net80211 *scn = OL_ATH_SOFTC_NET80211(ic);
    wmi_pdev_scan_cmd *cmd;
    wmi_buf_t buf;
    int len = sizeof(wmi_pdev_scan_cmd);
    buf = wmi_buf_alloc(scn->wmi_handle, len);
    printk("%s:\n", __FUNCTION__);
    if (!buf) {
        printk("%s:wmi_buf_alloc failed\n", __FUNCTION__);
        return ;
    }
    cmd = (wmi_pdev_scan_cmd *)wmi_buf_data(buf);
    cmd->scan_start = FALSE;
    wmi_unified_cmd_send(scn->wmi_handle, buf, len, WMI_PDEV_SCAN_CMDID);
#endif
}

#if ATH_SUPPORT_IQUE
void
ol_ath_set_acparams(struct ieee80211com *ic, u_int8_t ac, u_int8_t use_rts,
                          u_int8_t aggrsize_scaling, u_int32_t min_kbps)
{
    struct ol_ath_softc_net80211 *scn = OL_ATH_SOFTC_NET80211(ic);
    wmi_pdev_set_param_cmd *cmd;
    wmi_buf_t buf;
    u_int32_t param_value = 0;
    int len = sizeof(wmi_pdev_set_param_cmd);

    buf = wmi_buf_alloc(scn->wmi_handle, len);
    if (!buf) {
        printk("%s:wmi_buf_alloc failed\n", __FUNCTION__);
        return;
    }

    cmd = (wmi_pdev_set_param_cmd *)wmi_buf_data(buf);
    cmd->param_id = WMI_PDEV_PARAM_AC_AGGRSIZE_SCALING;
    param_value = ac;
    param_value |= (aggrsize_scaling << 8);
    cmd->param_value = param_value;

    wmi_unified_cmd_send(scn->wmi_handle, buf, len, WMI_PDEV_SET_PARAM_CMDID);
    return;
}

void
ol_ath_set_rtparams(struct ieee80211com *ic, u_int8_t ac, u_int8_t perThresh,
                          u_int8_t probeInterval)
{
    /* TBD */
    return;
}

void
ol_ath_get_iqueconfig(struct ieee80211com *ic)
{
    /* TBD */
    return;
}

void
ol_ath_set_hbrparams(struct ieee80211vap *iv, u_int8_t ac, u_int8_t enable, u_int8_t per)
{
    /* TBD */
    return;
}
#endif /*ATH_SUPPORT_IQUE*/

/* 
 * Disable the dcs im when the intereference is detected too many times. for
 * thresholds check umac
 */
static void 
ol_ath_disable_dcsim(struct ieee80211com *ic)
{
    struct ol_ath_softc_net80211 *scn = OL_ATH_SOFTC_NET80211(ic);
    /* clear the run state, only when cwim is not set */
    if (!(OL_IS_DCS_ENABLED(scn->scn_dcs.dcs_enable) & ATH_CAP_DCS_CWIM)) {
        OL_ATH_DCS_CLR_RUNSTATE(scn->scn_dcs.dcs_enable);
    }

    OL_ATH_DCS_DISABLE(scn->scn_dcs.dcs_enable, ATH_CAP_DCS_WLANIM);

    /* send target to disable and then disable in host */
    wmi_unified_pdev_set_param(scn->wmi_handle, WMI_PDEV_PARAM_DCS, scn->scn_dcs.dcs_enable);

}
#ifdef ATH_SUPPORT_TxBF
#ifdef TXBF_TODO
static void	
ol_ath_net80211_get_pos2_data(struct ieee80211com *ic, u_int8_t **p_data, 
    u_int16_t* p_len,void **rx_status)
{
    /* TBD */
    return;
}

static bool 
ol_ath_net80211_txbfrcupdate(struct ieee80211com *ic,void *rx_status,u_int8_t *local_h,
    u_int8_t *CSIFrame, u_int8_t NESSA, u_int8_t NESSB, int BW)
{
    /* TBD */
    return 1;
}

static void
ol_ath_net80211_ap_save_join_mac(struct ieee80211com *ic, u_int8_t *join_macaddr)
{
	struct ath_softc_net80211 *scn = ATH_SOFTC_NET80211(ic);

	scn->sc_ops->ap_save_join_mac(scn->sc_dev, join_macaddr);
}

static void
ol_ath_net80211_start_imbf_cal(struct ieee80211com *ic)
{
    return;
}
#endif

static int
ol_ath_net80211_txbf_alloc_key(struct ieee80211com *ic, struct ieee80211_node *ni)
{
    return 0;
}

static void
ol_ath_net80211_txbf_set_key(struct ieee80211com *ic, struct ieee80211_node *ni)
{
    return;
}

static void
ol_ath_net80211_init_sw_cv_timeout(struct ieee80211com *ic, struct ieee80211_node *ni)
{
    return;
}

static int
ol_ath_set_txbfcapability(struct ieee80211com *ic)
{
    return 0;
}

#ifdef TXBF_DEBUG
static void
ol_ath_net80211_txbf_check_cvcache(struct ieee80211com *ic, struct ieee80211_node *ni)
{
    return 0;
}
#endif

static void
ol_ath_net80211_CSI_Frame_send(struct ieee80211_node *ni,
						u_int8_t	*CSI_buf,
                        u_int16_t	buf_len,
						u_int8_t    *mimo_control)
{
    return;
}

static void
ol_ath_net80211_v_cv_send(struct ieee80211_node *ni,
                       u_int8_t *data_buf,
                       u_int16_t buf_len)
{
    return;
}
static void
ol_ath_net80211_txbf_stats_rpt_inc(struct ieee80211com *ic, 
                                struct ieee80211_node *ni)
{
    return;
}
static void
ol_ath_net80211_txbf_set_rpt_received(struct ieee80211com *ic, 
                                struct ieee80211_node *ni)
{
    return;
}
#endif 

static bool
ol_ath_net80211_is_mode_offload(struct ieee80211com *ic)
{
    /*
     * If this function executes, it is offload mode
     */
    return TRUE;
}

/*
 * Register the DCS functionality 
 * As such this is very small function and is not going to contain too many 
 * functions, right now continuing in the same file. Once it grows bigger, 
 * move to different file. 
 *  
 *  # register event handler to receive non-wireless lan interference event
 *  # register event handler to receive the extended stats that are meant for
 *    receiving the timed extra stats
 *        - right now this is not implemented and would implement
 *          as we go with second implementation
 *  # initialize the initial enabled state
 *  # initialize the host data strucutres that are meant for handling
 *    the wireless lan interference.
 *          - right now these variables would not be used
 *  # Keep the initialized state as disabled, and enable
 *    when first channel gets activated. 
 *  # Keep the status as disabled until completely qualified
 */
void
ol_ath_dcs_attach(struct ieee80211com *ic)
{ 
    struct ol_ath_softc_net80211 *scn = OL_ATH_SOFTC_NET80211(ic);
    /* Register WMI event handlers */
	scn->scn_dcs.dcs_enable                 = 0;
    OL_ATH_DCS_CLR_RUNSTATE(scn->scn_dcs.dcs_enable);
	scn->scn_dcs.phy_err_penalty            = DCS_PHYERR_PENALTY;
	scn->scn_dcs.phy_err_threshold          = DCS_PHYERR_THRESHOLD ;
	scn->scn_dcs.radar_err_threshold        = DCS_RADARERR_THRESHOLD;
	scn->scn_dcs.coch_intr_thresh           = DCS_COCH_INTR_THRESHOLD ;
	scn->scn_dcs.user_max_cu                = DCS_USER_MAX_CU; /* tx_cu + rx_cu */
	scn->scn_dcs.intr_detection_threshold   = DCS_INTR_DETECTION_THR;
	scn->scn_dcs.intr_detection_window      = DCS_SAMPLE_SIZE;
	scn->scn_dcs.scn_dcs_im_stats.im_intr_cnt = 0;
	scn->scn_dcs.scn_dcs_im_stats.im_samp_cnt = 0;
    scn->scn_dcs.dcs_debug                  = DCS_DEBUG_DISABLE;

    wmi_unified_register_event_handler(scn->wmi_handle, WMI_DCS_INTERFERENCE_EVENTID,
                                            wmi_unified_dcs_interference_handler, NULL);
    return;
}

#if ATH_OL_FAST_CHANNEL_RESET_WAR
#define DISABLE_FAST_CHANNEL_RESET 1
     /*WAR for EV#117307, MSDU_DONE is not set for data packet,
      to fix this issue, fast channel change is disabled for x86 platform*/
void ol_ath_fast_chan_change(struct ol_ath_softc_net80211 *scn)
{
    printk("Disabling fast channel reset \n");
    if(wmi_unified_pdev_set_param(scn->wmi_handle,
                       WMI_PDEV_PARAM_FAST_CHANNEL_RESET,
                       DISABLE_FAST_CHANNEL_RESET)) {
        printk(" Failed to disable fast channel reset \n");
    }
}
#endif


void
ol_ath_dev_attach(struct ol_ath_softc_net80211 *scn,
                   IEEE80211_REG_PARAMETERS *ieee80211_conf_parm)
{
    struct ieee80211com *ic = &scn->sc_ic;
    int error = 0;
    spin_lock_init(&ic->ic_lock);
    IEEE80211_STATE_LOCK_INIT(ic);


    /* attach channel width management */
    error = ol_ath_cwm_attach(scn);
    if (error) {
        printk("%s : ol_ath_cwm_attach failed \n", __func__);
        return;
    }

    /* XXX not right but it's not used anywhere important */
    ieee80211com_set_phytype(ic, IEEE80211_T_OFDM);

    /*
     * Set the Atheros Advanced Capabilities from station config before
     * starting 802.11 state machine.
     */
     /* TBD */

    /* this matches the FW default value */
    scn->arp_override = WME_AC_VO;

    /* Set the mac address */

    /* Setup Min frame size */
    ic->ic_minframesize = sizeof(struct ieee80211_frame_min);

     /*
     * Setup some device specific ieee80211com methods
     */
    ic->ic_init = ol_ath_init;
    ic->ic_reset_start = ol_ath_reset_start;
    ic->ic_reset = ol_ath_reset;
    ic->ic_reset_end = ol_ath_reset_end;
    ic->ic_updateslot = ol_ath_updateslot;
    ic->ic_wme.wme_update = ol_ath_wmm_update;
    ic->ic_txq_depth = ol_ath_txq_depth;
    ic->ic_txq_depth_ac = ol_ath_txq_depth_ac;
    ic->ic_update_protmode = ol_ath_update_protmode;
    ic->ic_chwidth_change = ol_net80211_chwidth_change;
    ic->ic_nss_change = ol_net80211_nss_change;
    ic->ic_start_csa = ieee80211_start_csa;

    /* dummy scan start/end commands */
    ic->ic_scan_start = ol_ath_scan_start;
    ic->ic_scan_end = ol_ath_scan_end;
#if ATH_SUPPORT_VOW_DCS
	/* host side umac compiles with this flag, so we have no
	   option than writing this with flag, otherwise the other
	   drivers in oher OSes would fail*/
    ic->ic_disable_dcsim = ol_ath_disable_dcsim;
#endif


    /* Attach resmgr module */
    ol_ath_resmgr_attach(ic);

    /* attach scan module */
#if UMAC_SCAN_OFFLOAD
    ol_ath_scan_attach(ic);
#else
    ieee80211_scan_class_attach(ic);
#endif

    ol_ath_power_attach(ic);

    /*
     * Attach ieee80211com object to net80211 protocal stack.
     */
    ieee80211_ifattach(ic, ieee80211_conf_parm);

     /*
     * Complete device specific ieee80211com methods init
     */
    ic->ic_set_channel = ol_ath_set_channel;
    ic->ic_pwrsave_set_state = ol_ath_pwrsave_set_state;
    ic->ic_mhz2ieee = ol_ath_mhz2ieee;
    ic->ic_get_noisefloor = ol_ath_get_noisefloor;
    ic->ic_get_chainnoisefloor = ol_ath_get_chainnoisefloor;
    ic->ic_set_txPowerLimit = ol_ath_setTxPowerLimit;
    ic->ic_get_common_power = ol_ath_get_common_power;
    ic->ic_get_TSF32        = ol_ath_getTSF32;
    ic->ic_rmgetcounters = ol_ath_getrmcounters;
    ic->ic_get_wpsPushButton = ol_ath_wpsPushButton;
    ic->ic_clear_phystats = ol_ath_clear_phystats;
    ic->ic_set_macaddr = ol_ath_set_macaddr;
    ic->ic_log_text = ol_ath_log_text;
    ic->ic_set_chain_mask = ol_ath_set_chain_mask;
    ic->ic_get_mfpsupport = ol_ath_getmfpsupport;
    ic->ic_set_hwmfpQos   = ol_ath_setmfpQos;
    ic->ic_get_tx_hw_retries  = ol_ath_get_tx_hw_retries;
    ic->ic_get_tx_hw_success  = ol_ath_get_tx_hw_success;
#if ATH_SUPPORT_IQUE
    ic->ic_set_acparams = ol_ath_set_acparams;
    ic->ic_set_rtparams = ol_ath_set_rtparams;
    ic->ic_get_iqueconfig = ol_ath_get_iqueconfig;
    ic->ic_set_hbrparams = ol_ath_set_hbrparams;
#endif
    ic->ic_set_config = ol_ath_set_config;
    ic->ic_set_safemode = ol_ath_set_safemode;

#ifdef ATH_SUPPORT_TxBF // For TxBF RC
#ifdef TXBF_TODO
    ic->ic_get_pos2_data = ol_ath_net80211_get_pos2_data;
    ic->ic_txbfrcupdate = ol_ath_net80211_txbfrcupdate; 
    ic->ic_ieee80211_send_cal_qos_nulldata = ieee80211_send_cal_qos_nulldata;
    ic->ic_ap_save_join_mac = ol_ath_net80211_ap_save_join_mac;
    ic->ic_start_imbf_cal = ol_ath_net80211_start_imbf_cal;
    ic->ic_csi_report_send = ol_ath_net80211_CSI_Frame_send;
#endif
    
#ifdef IEEE80211_DEBUG_REFCNT
    ic->ic_ieee80211_find_node_debug = ieee80211_find_node_debug;
#else
    ic->ic_ieee80211_find_node = ieee80211_find_node;
#endif //IEEE80211_DEBUG_REFCNT
    ic->ic_v_cv_send = ol_ath_net80211_v_cv_send;
    ic->ic_txbf_alloc_key = ol_ath_net80211_txbf_alloc_key;
    ic->ic_txbf_set_key = ol_ath_net80211_txbf_set_key;
    ic->ic_init_sw_cv_timeout = ol_ath_net80211_init_sw_cv_timeout;
    ic->ic_set_txbf_caps = ol_ath_set_txbfcapability;
#ifdef TXBF_DEBUG
	ic->ic_txbf_check_cvcache = ol_ath_net80211_txbf_check_cvcache;
#endif
    ic->ic_txbf_stats_rpt_inc = ol_ath_net80211_txbf_stats_rpt_inc;
    ic->ic_txbf_set_rpt_received = ol_ath_net80211_txbf_set_rpt_received;
#endif
    ic->ic_mcast_group_update = ol_ath_mcast_group_update;
    ic->ic_get_cur_chan_nf = ol_ath_net80211_get_cur_chan_noisefloor; 
    /*
     * pktlog scn initialization
     */
    ol_pl_sethandle(&(scn->pl_dev), scn);
    ol_pktlog_attach(scn);

#if ATH_SUPPORT_SPECTRAL
    if (ol_if_spectral_setup(ic) == 0) {
        printk("SPECTRAL : Not supported\n");
    }
#endif

#if ATH_SUPPORT_GREEN_AP
    if (ol_if_green_ap_attach(ic) == 0) {
        printk("GREEN-AP : Not supported\n");
    }
#endif  /* ATH_SUPPORT_GREEN_AP */

    ol_ath_wow_attach(ic);
    ol_ath_stats_attach(ic);

#if ATH_OL_FAST_CHANNEL_RESET_WAR
    ol_ath_fast_chan_change(scn);
#endif
}

int
ol_asf_adf_attach(struct ol_ath_softc_net80211 *scn)
{
    static int first = 1;
    osdev_t osdev = scn->sc_osdev;

    /*
     * Configure the shared asf_amem and asf_print instances with ADF
     * function pointers for mem alloc/free, lock/unlock, and print.
     * (Do this initialization just once.)
     */
    if (first) {
        static adf_os_spinlock_t asf_amem_lock;
        static adf_os_spinlock_t asf_print_lock;

        first = 0;

        adf_os_spinlock_init(&asf_amem_lock);
        asf_amem_setup(
            (asf_amem_alloc_fp) adf_os_mem_alloc_outline,
            (asf_amem_free_fp) adf_os_mem_free_outline,
            (void *) osdev,
            (asf_amem_lock_fp) adf_os_spin_lock_bh_outline,
            (asf_amem_unlock_fp) adf_os_spin_unlock_bh_outline,
            (void *) &asf_amem_lock);
        adf_os_spinlock_init(&asf_print_lock);
        asf_print_setup(
            (asf_vprint_fp) adf_os_vprint,
            (asf_print_lock_fp) adf_os_spin_lock_bh_outline,
            (asf_print_unlock_fp) adf_os_spin_unlock_bh_outline,
            (void *) &asf_print_lock);
    }

    /*
     * Also allocate our own dedicated asf_amem instance.
     * For now, this dedicated amem instance will be used by the
     * HAL's ath_hal_malloc.
     * Later this dedicated amem instance will be used throughout
     * the driver, rather than using the shared asf_amem instance.
     *
     * The platform-specific code that calls this ath_attach function
     * may have already set up an amem instance, if it had to do
     * memory allocation before calling ath_attach.  So, check if
     * scn->amem.handle is initialized already - if not, set it up here.
     */
    if (!scn->amem.handle) {
        adf_os_spinlock_init(&scn->amem.lock);
        scn->amem.handle = asf_amem_create(
            NULL, /* name */
            0,  /* no limit on allocations */
            (asf_amem_alloc_fp) adf_os_mem_alloc_outline,
            (asf_amem_free_fp) adf_os_mem_free_outline,
            (void *) osdev,
            (asf_amem_lock_fp) adf_os_spin_lock_bh_outline,
            (asf_amem_unlock_fp) adf_os_spin_unlock_bh_outline,
            (void *) &scn->amem.lock,
            NULL /* use adf_os_mem_alloc + osdev to alloc this amem object */);
        if (!scn->amem.handle) {
            adf_os_spinlock_destroy(&scn->amem.lock);
            return -ENOMEM;
        }
    }

    return EOK;
}

int
ol_ath_attach(u_int16_t devid, struct ol_ath_softc_net80211 *scn, 
              IEEE80211_REG_PARAMETERS *ieee80211_conf_parm, 
              ol_ath_update_fw_config_cb cfg_cb)
{
    struct ieee80211com     *ic = &scn->sc_ic;
    HTC_INIT_INFO  htcInfo;
    int status = 0;
    osdev_t osdev = scn->sc_osdev;

    adf_os_spinlock_init(&scn->scn_lock);

    scn->cfg_cb = cfg_cb;
    /* initialize default target config */
    ol_ath_set_default_tgt_config(scn);

#ifdef AH_CAL_IN_FLASH_PCI
#define HOST_CALDATA_SIZE (16 * 1024)
    scn->cal_in_flash = 1;
#ifndef ATH_CAL_NAND_FLASH
    scn->cal_mem = A_IOREMAP(AH_CAL_LOCATIONS_PCI, HOST_CALDATA_SIZE);
    if (!scn->cal_mem) {
        printk("%s: A_IOREMAP failed\n", __func__);
        return -1;
    }
#endif
#endif

    /*
     * 1. Initialize BMI
     */
    BMIInit(scn);
    printk("%s() BMI inited.\n", __func__);

    if (!scn->is_sim) {
        unsigned int bmi_user_agent;
        struct bmi_target_info targ_info;

        /*
         * 2. Get target information
         */
        OS_MEMZERO(&targ_info, sizeof(targ_info));
        if (BMIGetTargetInfo(scn->hif_hdl, &targ_info, scn) != A_OK) {
            status = -1;
            goto attach_failed;
        }
        printk("%s() BMI Get Target Info.\n", __func__);

        scn->target_type = targ_info.target_type;
        scn->target_version = targ_info.target_ver;
        printk("%s() TARGET TYPE: %d Vers 0x%x\n", __func__, scn->target_type, scn->target_version);
        ol_ath_host_config_update(scn);

        bmi_user_agent = ol_ath_bmi_user_agent_init(scn);
        if (bmi_user_agent) {
            /* User agent handles BMI phase */
            int rv;

            rv = ol_ath_wait_for_bmi_user_agent(scn);
            if (rv) {
                status = -1;
                goto attach_failed;
            }
        } else {
            /* Driver handles BMI phase */

            /*
             * 3. Configure target
             */
            if (ol_ath_configure_target(scn) != EOK) {
                status = -1;
                goto attach_failed;
            }
            printk("%s() configure Target .\n", __func__);
    
            /*
             * 4. Download firmware image and data files
             */
            if (ol_ath_download_firmware(scn) != EOK)
            {
                status = -EIO;
                goto attach_failed;
            }
            printk("%s() Download FW. \n", __func__);
        }
    } 

    /*
     * 5. Create HTC
     */
    OS_MEMZERO(&htcInfo,sizeof(htcInfo));
    htcInfo.pContext = scn;
    htcInfo.TargetFailure = ol_target_failure;
    htcInfo.TargetSendSuspendComplete = ol_target_send_suspend_complete;

    if ((scn->htc_handle = HTCCreate(scn->hif_hdl, &htcInfo, scn->adf_dev)) == NULL)
    {
        status = -1;
        goto attach_failed;
    }
    printk("%s() HT Create .\n", __func__);

    HIFClaimDevice(scn->hif_hdl, scn);
    printk("%s() HIF Claim.\n", __func__);

    /*
     * 6. Complete BMI phase
     */
    if (BMIDone(scn->hif_hdl, scn) != A_OK)
    {
         status = -EIO;
         goto attach_failed;
    }
    printk("%s() BMI Done. \n", __func__);

    if (!bypasswmi) {
        /*
         * 7. Initialize WMI
         */
        if ((scn->wmi_handle = wmi_unified_attach(scn, scn->sc_osdev)) == NULL)
        {
            printk("%s() Failed to initialize WMI.\n", __func__);
            status = -1;
            goto attach_failed;
        }
        printk("%s() WMI attached. wmi_handle %p \n", __func__, scn->wmi_handle);
        wmi_unified_register_event_handler(scn->wmi_handle, WMI_DEBUG_PRINT_EVENTID,
                                                wmi_unified_debug_print_event_handler, NULL);
    }
    if (HTCWaitTarget(scn->htc_handle) != A_OK) {
        status = -EIO;
        goto attach_failed;
    }
    if (!bypasswmi) {
        dbglog_init(scn->wmi_handle);

        /* FIXME: casting sc_osdev to adf_os_device is not ok for all OS 
          (not an issue for linux as linux ignores the handle) */ 
        /*
         * ol_txrx_pdev_attach needs to be called after calling
         * HTCWaitTarget but before calling HTCStart, so HTT can
         * do its HTC service connection.
         */
        scn->pdev_txrx_handle = ol_txrx_pdev_attach((ol_pdev_handle)scn,
                                                    scn->htc_handle,
                                                    scn->adf_dev); 
        if (scn->pdev_txrx_handle == NULL) {
           printk("%s: pdev attach failed\n",__func__);
            status = -EIO;
            goto attach_failed;
        }
    }
    /*
     * 8. Connect Services to HTC
     */
    if ((status = ol_ath_connect_htc(scn)) != A_OK)
    {
        status = -EIO;
        printk("%s: connect_htc failed\n",__func__);
        goto attach_failed;
    }

    if (!bypasswmi) {
        /*
         * Invoke the host datapath initializations that involve messages
         * to the target.
         * (This can't be done until after the HTCStart call, which is in
         * ol_ath_connect_htc.)
         */
        if (scn->target_version != AR6004_VERSION_REV1_3) {
           if (ol_txrx_pdev_attach_target(scn->pdev_txrx_handle) != A_OK) {
                printk("%s: txrx pdev attach failed\n",__func__);
                status = -EIO;
                goto attach_failed;
            }
        }
    }

    printk("%s() connect HTC. \n", __func__);
    if (!bypasswmi) {
        // Use attach_failed1 for failures beyond this
        /*
         * 9. WLAN/UMAC initialization
         */
        ic->ic_is_mode_offload = ol_ath_net80211_is_mode_offload;
        ic->ic_osdev = osdev;
        ic->ic_adf_dev = scn->adf_dev;
        if (scn->target_version == AR6004_VERSION_REV1_3) {
        /*
           It's Hard code for HAL capability and We don't use this branch for McKinley.
           Because McKinley don't support WMI UNIFIED SERVICE READY,
        */
            scn->hal_reg_capabilities.eeprom_rd = 0;
            scn->hal_reg_capabilities.eeprom_rd_ext = 0x1f;
            scn->hal_reg_capabilities.high_2ghz_chan = 0xaac;
            scn->hal_reg_capabilities.high_5ghz_chan = 0x17d4;
            scn->hal_reg_capabilities.low_2ghz_chan = 0x908;
            scn->hal_reg_capabilities.low_5ghz_chan = 0x1338;
            scn->hal_reg_capabilities.regcap1 = 7;
            scn->hal_reg_capabilities.regcap2 = 0xbc0;
            scn->hal_reg_capabilities.wireless_modes = 0x1f9001;
            scn->phy_capability = 1;
            ol_regdmn_attach(scn);
            ol_regdmn_set_regdomain(scn->ol_regdmn_handle, scn->hal_reg_capabilities.eeprom_rd);
            ol_regdmn_set_regdomain_ext(scn->ol_regdmn_handle, scn->hal_reg_capabilities.eeprom_rd_ext);			            
        }
        ol_regdmn_start(scn->ol_regdmn_handle, ieee80211_conf_parm);

        /*
        To propagate the country settings to UMAC layer so that 
        tools like wlanmon is able to get the information they want
        */
        ic->ic_get_currentCountry(ic, &ic->ic_country);

        ol_ath_setup_rates(ic);
        ol_ath_phyerr_attach(ic);
        ieee80211com_set_cap_ext(ic, IEEE80211_CEXT_PERF_PWR_OFLD);
        ol_ath_dev_attach(scn, ieee80211_conf_parm);
        
#if ATH_SUPPORT_DFS
        ol_if_dfs_setup(ic);
#endif

        ol_ath_vap_attach(ic);
        ol_ath_node_attach(scn, ic);
        ol_ath_beacon_attach(ic);
        ol_ath_rtt_meas_report_attach(ic);
#ifdef QVIT
        ol_ath_qvit_attach(scn);
#endif
        ol_ath_utf_attach(scn);

        ol_ath_mgmt_attach(ic);

        ol_ath_chan_info_attach(ic);
        /* attach the dcs functionality */
        ol_ath_dcs_attach(ic);
        /* As of now setting ic with all ciphers assuming 
         * hardware will support, eventually to query 
         * the hardware to figure out h/w crypto support.
         */
        ieee80211com_set_cap(ic, IEEE80211_C_WEP);
        ieee80211com_set_cap(ic, IEEE80211_C_AES);
        ieee80211com_set_cap(ic, IEEE80211_C_AES_CCM);
        ieee80211com_set_cap(ic, IEEE80211_C_CKIP);
        ieee80211com_set_cap(ic, IEEE80211_C_TKIP);
        ieee80211com_set_cap(ic, IEEE80211_C_TKIPMIC);
        if (WMI_SERVICE_IS_ENABLED(scn->wmi_service_bitmap, WMI_SERVICE_11AC)) {
            ieee80211com_set_cap_ext(ic, IEEE80211_CEXT_11AC);
        }

        if (ieee80211_conf_parm->wmeEnabled) {
            ieee80211com_set_cap(ic, IEEE80211_C_WME);
        }
        
#if ATH_SUPPORT_WAPI
        ieee80211com_set_cap(ic, IEEE80211_C_WAPI);
#endif
#if UMAC_SCAN_OFFLOAD
        ol_scan_update_channel_list(ic->ic_scanner);
#endif
           
        wmi_unified_register_event_handler(scn->wmi_handle, WMI_WLAN_PROFILE_DATA_EVENTID,
                                                wmi_unified_wlan_profile_data_event_handler, NULL);

        wmi_unified_register_event_handler(scn->wmi_handle, WMI_PDEV_TPC_CONFIG_EVENTID,
                                                wmi_unified_pdev_tpc_config_event_handler, NULL);

        wmi_unified_register_event_handler(scn->wmi_handle, WMI_GPIO_INPUT_EVENTID,
                                                wmi_unified_gpio_input_event_handler, NULL);
    }
    printk("%s() UMAC attach . \n", __func__);
#if ATH_SUPPORT_DFS
    ol_if_dfs_configure(ic);
#endif
    return EOK;

attach_failed:
    if (scn->htc_handle) {
        HTCDestroy(scn->htc_handle);
        scn->htc_handle = NULL;
    }
    BMICleanup(scn);
    if (!bypasswmi) {
        if (scn->wmi_handle) {
            wmi_unified_detach(scn->wmi_handle);
            scn->wmi_handle = NULL;
        }
        if (scn->pdev_txrx_handle) {
            /* Force delete txrx pdev */
            ol_txrx_pdev_detach(scn->pdev_txrx_handle, 1);
            scn->pdev_txrx_handle = NULL;
        }
    }
#ifdef AH_CAL_IN_FLASH_PCI
#ifndef ATH_CAL_NAND_FLASH    
    if (scn->cal_mem) {
        A_IOUNMAP(scn->cal_mem);
        scn->cal_mem = 0;
        scn->cal_in_flash = 0;
    }
#endif
#endif
    return status;
}

int
ol_ath_detach(struct ol_ath_softc_net80211 *scn, int force)
{
    struct ieee80211com     *ic;
    int status = 0,idx;
    ic = &scn->sc_ic;

    ol_ath_stats_detach(ic);

    ol_ath_wow_detach(ic);

    if (!bypasswmi) {
        ieee80211_stop_running(ic);
#if ATH_SUPPORT_DFS
        ol_if_dfs_teardown(ic);
#endif
        ol_ath_phyerr_detach(ic);

        /*
         * NB: the order of these is important:
         * o call the 802.11 layer before detaching the hal to
         *   insure callbacks into the driver to delete global
         *   key cache entries can be handled
         * o reclaim the tx queue data structures after calling
         *   the 802.11 layer as we'll get called back to reclaim
         *   node state and potentially want to use them
         * o to cleanup the tx queues the hal is called, so detach
         *   it last
         * Other than that, it's straightforward...
         */
        ieee80211_ifdetach(ic);
    }
#if 0 /* TBD */
    ol_ath_vap_detach(ic);
    ol_ath_node_detach(scn, ic);
    ol_ath_beacon_detach(ic);
    ol_ath_scan_detach(ic);

    ol_ath_mgmt_detach(ic);
#endif


    
    if (scn->pl_dev){
        ol_pktlog_detach(scn);
    }

    ol_regdmn_detach(scn->ol_regdmn_handle);

    adf_os_spinlock_destroy(&scn->amem.lock);
    asf_amem_destroy(scn->amem.handle, NULL);
    scn->amem.handle = NULL;

    ol_ath_disconnect_htc(scn);
#ifdef QVIT
    ol_ath_qvit_detach(scn);
#endif
    if (!bypasswmi) {
        ol_ath_utf_detach(scn);
        /*
         * The call to ol_txrx_pdev_detach has to happen after the call to
         * ol_ath_disconnect_htc, so that if there are any outstanding
         * tx packets inside HTC, the cleanup callbacks into HTT and txrx
         * will still be valid.
         */
        if (scn->pdev_txrx_handle) {
           ol_txrx_pdev_detach(scn->pdev_txrx_handle, force);
            scn->pdev_txrx_handle = NULL;
        }
    }
    if (scn->htc_handle) {
        HTCDestroy(scn->htc_handle);
        scn->htc_handle = NULL;
    }
    if (!bypasswmi) {
        if (scn->wmi_handle)
            wmi_unified_detach(scn->wmi_handle);
    }
    /* Cleanup BMI */
    BMICleanup(scn);

    if (scn->hif_hdl != NULL) {
        /*
         * Release the device so we do not get called back on remove
         * incase we we're explicity destroyed by module unload
         */
        HIFReleaseDevice(scn->hif_hdl);
        HIFShutDownDevice(scn->hif_hdl);
        scn->hif_hdl = NULL;
    }

#if ATH_SUPPORT_SPECTRAL
    ol_if_spectral_detach(ic);
#endif  // ATH_SUPPORT_SPECTRAL

#if ATH_SUPPORT_GREEN_AP
    ol_if_green_ap_detach(ic);
#endif  /* ATH_SUPPORT_GREEN_AP */

#ifdef AH_CAL_IN_FLASH_PCI
#ifndef ATH_CAL_NAND_FLASH    
    if (scn->cal_mem) {
        A_IOUNMAP(scn->cal_mem);
        scn->cal_mem = 0;
        scn->cal_in_flash = 0;
    }
#endif
#endif
    adf_os_spinlock_destroy(&scn->scn_lock);
    
    for(idx=0;idx<scn->num_mem_chunks; ++idx) {  
        adf_os_mem_free_consistent(
            scn->adf_dev, 
            scn->mem_chunks[idx].len, 
            scn->mem_chunks[idx].vaddr,
            scn->mem_chunks[idx].paddr,
            adf_os_get_dma_mem_context((&(scn->mem_chunks[idx])), memctx));
    }
    /* No Target accesses of any kind after this point */
    return status;
}

int
ol_ath_resume(struct ol_ath_softc_net80211 *scn)
{
    return 0;
}

int
ol_ath_suspend(struct ol_ath_softc_net80211 *scn)
{
    return 0;
}

void
ol_ath_target_status_update(struct ol_ath_softc_net80211 *scn, ol_target_status status)
{
    /* target lost, host needs to recover/reattach */
    scn->target_status = status;
}

void
ol_ath_suspend_resume_attach(struct ol_ath_softc_net80211 *scn)
{
}


int
ol_ath_suspend_target(struct ol_ath_softc_net80211 *scn, int disable_target_intr)
{
    wmi_pdev_suspend_cmd* cmd;
    wmi_buf_t wmibuf;
    u_int32_t len = sizeof(wmi_pdev_suspend_cmd);
    /*send the comand to Target to ignore the 
    * PCIE reset so as to ensure that Host and target 
    * states are in sync*/
    wmibuf = wmi_buf_alloc(scn->wmi_handle, len);
    if (wmibuf == NULL) {
        return -1;
    }

    cmd = (wmi_pdev_suspend_cmd *)wmi_buf_data(wmibuf);
    if (disable_target_intr) {
        cmd->suspend_opt = WMI_PDEV_SUSPEND_AND_DISABLE_INTR;
    }
    else {
        cmd->suspend_opt = WMI_PDEV_SUSPEND;
    }

    return wmi_unified_cmd_send(scn->wmi_handle, wmibuf, len,
                                WMI_PDEV_SUSPEND_CMDID);
}

int
ol_ath_resume_target(struct ol_ath_softc_net80211 *scn)
{
    wmi_buf_t wmibuf;

    wmibuf = wmi_buf_alloc(scn->wmi_handle, 0);
    if (wmibuf == NULL) {
        return  -1;
    }
    return wmi_unified_cmd_send(scn->wmi_handle, wmibuf, 0,
                                WMI_PDEV_RESUME_CMDID);
}
#ifndef A_MIN
#define A_MIN(a,b)    ((a)<(b)?(a):(b))
#endif
/*
 * ol_ath_cw_interference_handler
 *
 * Functionality of this should be the same as
 * ath_net80211_cw_interference_handler() in lmac layer of the direct attach
 * drivers. Keep this same across both. 
 *
 * When the cw interference is sent from the target, kick start the scan
 * with auto channel. This is disruptive channel change. Non-discruptive
 * channel change is the responsibility of scan module.
 *
 */
static void
ol_ath_wlan_n_cw_interference_handler(struct ieee80211com *ic)
{
    struct ieee80211vap *vap;
    struct ol_ath_softc_net80211 *scn = OL_ATH_SOFTC_NET80211(ic);

    /* Check if CW Interference is already been found and being handled */
    if (ic->cw_inter_found)
        return;

	printk("DCS: inteference_handler - start");

    spin_lock(&ic->ic_lock);

    /* 
	 * mark this channel as cw_interference is found
     * Set the CW interference flag so that ACS does not bail out this flag
     * would be reset in ieee80211_beacon.c:ieee80211_beacon_update()
     */
    ic->cw_inter_found = 1;

    /* Before triggering the channel change, turn off the dcs until the
     * channel change completes, to avoid repeated reports.
     */
    (void)wmi_unified_pdev_set_param(scn->wmi_handle, WMI_PDEV_PARAM_DCS, 0);
    printk("DCS channel change triggered, disabling until channel change completes\n");
    OL_ATH_DCS_CLR_RUNSTATE(scn->scn_dcs.dcs_enable);

    TAILQ_FOREACH(vap, &ic->ic_vaps, iv_next) {
        if (vap->iv_opmode == IEEE80211_M_HOSTAP) {
            printk("De-authenticating all the nodes before channel change \n");
            wlan_deauth_all_stas(vap);
        }
    }
    /* Loop through and figure the first VAP on this radio */
    /* FIXME
     * There could be some issue in mbssid mode. It does look like if
     * wlan_set_channel fails on first vap, it tries on the second vap
     * again. Given that all vaps on same radio, we may need not do this.
     * Need a test case for this. Leaving the code as it is.
     */
    TAILQ_FOREACH(vap, &ic->ic_vaps, iv_next) {
        if (vap->iv_opmode == IEEE80211_M_HOSTAP) {
            if (!wlan_set_channel(vap, IEEE80211_CHAN_ANY)) {
                /* ACS is done on per radio, so calling it once is 
                * good enough 
                */    
                goto done;
            }
        }
    }
    /* Should not come here, something is not right, hope something better happens
     * next time the flag is set 
     */

done:
    spin_unlock(&ic->ic_lock);
	printk("DCS: %s interference_handling is complete...", __func__);
}

inline static void
wlan_dcs_im_copy_stats(wlan_dcs_im_tgt_stats_t *prev_stats, wlan_dcs_im_tgt_stats_t *curr_stats)
{
	/* right now no other actions are required beyond memcopy, if
	 * rquired the rest of the code would follow
	 */
	OS_MEMCPY(prev_stats, curr_stats, sizeof(wlan_dcs_im_tgt_stats_t));
}

inline static void
wlan_dcs_im_print_stats(wlan_dcs_im_tgt_stats_t *prev_stats, wlan_dcs_im_tgt_stats_t *curr_stats)
{
	/* debug, dump all received stats first */
	printk("tgt_curr/tsf,%u", curr_stats->reg_tsf32);
	printk(",tgt_curr/last_ack_rssi,%u", curr_stats->last_ack_rssi);
    printk(",tgt_curr/tx_waste_time,%u", curr_stats->tx_waste_time);
    printk(",tgt_curr/dcs_rx_time,%u", curr_stats->rx_time);
	printk(",tgt_curr/listen_time,%u", curr_stats->mib_stats.listen_time);
	printk(",tgt_curr/tx_frame_cnt,%u", curr_stats->mib_stats.reg_tx_frame_cnt);
	printk(",tgt_curr/rx_frame_cnt,%u", curr_stats->mib_stats.reg_rx_frame_cnt);
	printk(",tgt_curr/rxclr_cnt,%u", curr_stats->mib_stats.reg_rxclr_cnt);
	printk(",tgt_curr/reg_cycle_cnt,%u", curr_stats->mib_stats.reg_cycle_cnt);
	printk(",tgt_curr/rxclr_ext_cnt,%u", curr_stats->mib_stats.reg_rxclr_ext_cnt);
	printk(",tgt_curr/ofdm_phyerr_cnt,%u", curr_stats->mib_stats.reg_ofdm_phyerr_cnt);
	printk(",tgt_curr/cck_phyerr_cnt,%u", curr_stats->mib_stats.reg_cck_phyerr_cnt);

	printk("tgt_prev/tsf,%u", prev_stats->reg_tsf32);
	printk(",tgt_prev/last_ack_rssi,%u", prev_stats->last_ack_rssi);
    printk(",tgt_prev/tx_waste_time,%u", prev_stats->tx_waste_time);
    printk(",tgt_prev/rx_time,%u", prev_stats->rx_time);
	printk(",tgt_prev/listen_time,%u", prev_stats->mib_stats.listen_time);
	printk(",tgt_prev/tx_frame_cnt,%u", prev_stats->mib_stats.reg_tx_frame_cnt);
	printk(",tgt_prev/rx_frame_cnt,%u", prev_stats->mib_stats.reg_rx_frame_cnt);
	printk(",tgt_prev/rxclr_cnt,%u", prev_stats->mib_stats.reg_rxclr_cnt);
	printk(",tgt_prev/reg_cycle_cnt,%u", prev_stats->mib_stats.reg_cycle_cnt);
	printk(",tgt_prev/rxclr_ext_cnt,%u", prev_stats->mib_stats.reg_rxclr_ext_cnt);
	printk(",tgt_prev/ofdm_phyerr_cnt,%u", prev_stats->mib_stats.reg_ofdm_phyerr_cnt); 
	printk(",tgt_prev/cck_phyerr_cnt,%u", prev_stats->mib_stats.reg_cck_phyerr_cnt); 
}

/*
 * reg_xxx - All the variables are contents of the corresponding
 *  		register contents
 * xxx_delta - computed difference between previous cycle and c
 * 		    current cycle
 * reg_xxx_cu	- Computed channel utillization in %,
 *  		computed through register statistics 
 * 
 * FIXME ideally OL and non-OL layers can re-use the same code.  
 * But this is done differently between OL and non-OL paths. 
 * We may need to rework this completely for both to work with single
 * piece of code. The non-OL path also need lot of rework. All  
 * stats need to be taken to UMAC layer. That seems to be too
 * much of work to do at this point of time. Right now host code
 * is limited to one below function, and this function alone  
 * need to go UMAC. Given that function is small, tend to keep
 * only here here. If this gets any bigger we shall try doing it
 * in umac, and merge entire code ( ol and non-ol to umac ).
 */
static void
ol_ath_wlan_interference_handler(ol_scn_t scn, wlan_dcs_im_tgt_stats_t *curr_stats)
{
	wlan_dcs_im_tgt_stats_t *prev_stats;

	u_int32_t reg_tsf_delta = 0;                /* prev-tsf32  - curr-tsf32 */
	u_int32_t rxclr_delta = 0;                  /* prev-RXCLR - curr-RXCLR */
	u_int32_t rxclr_ext_delta = 0;              /* prev-RXEXTCLR - curreent RXEXTCLR, most of the time this is zero, chip issue ?? */
	u_int32_t cycle_count_delta = 0;            /* prev CCYCLE - curr CCYCLE */
	u_int32_t tx_frame_delta = 0;               /* prev TFCT - curr TFCNT */
	u_int32_t rx_frame_delta = 0;               /* prev RFCNT - curr RFCNT */
	u_int32_t reg_total_cu = 0; 				/* total channel utilization in %*/
	u_int32_t reg_tx_cu = 0;					/* transmit channel utilization in %*/ 
    u_int32_t reg_rx_cu = 0;					/* receive channel utilization in %*/
	u_int32_t reg_unused_cu = 0;                /* unused channel utillization */
	u_int32_t rx_time_cu=0;						/* computed rx time*/
	u_int32_t reg_ofdm_phyerr_delta = 0;		/* delta ofdm errors */
	u_int32_t reg_cck_phyerr_delta = 0;			/* delta cck errors*/
	u_int32_t reg_ofdm_phyerr_cu = 0;			/* amount utilization by ofdm errors*/
	u_int32_t ofdm_phy_err_rate = 0;			/* rate at which ofdm errors are seen*/
	u_int32_t cck_phy_err_rate=0;				/* rate at which cck errors are seen*/
	u_int32_t max_phy_err_rate = 0;
    u_int32_t max_phy_err_count = 0;
	u_int32_t total_wasted_cu = 0;
	u_int32_t wasted_tx_cu = 0;
	u_int32_t tx_err = 0;
	int too_many_phy_errors = 0;

	if (!scn || !curr_stats) {
		printk("\nDCS: scn is NULL\n");
		return;
	}

    prev_stats =  &scn->scn_dcs.scn_dcs_im_stats.prev_dcs_im_stats;

    if (unlikely(scn->scn_dcs.dcs_debug >= DCS_DEBUG_VERBOSE)) {
        wlan_dcs_im_print_stats(prev_stats, curr_stats);
    }

    /* counters would have wrapped. Ideally we should be able to figure this
     * out, but we never know how many times counters wrapped. just ignore 
     */
	if ((curr_stats->mib_stats.listen_time <= 0) ||
        (curr_stats->reg_tsf32 < prev_stats->reg_tsf32)) {

		if (unlikely(scn->scn_dcs.dcs_debug >= DCS_DEBUG_VERBOSE)) {
            printk("\nDCS: ignoring due to negative TSF value\n");
        }
        /* copy the current stats to previous stats for next run */
		wlan_dcs_im_copy_stats(prev_stats, curr_stats);
		return;
	}

	reg_tsf_delta = curr_stats->reg_tsf32 - prev_stats->reg_tsf32;

	/* do nothing if current stats are not seeming good, probably
	 * a reset happened on chip, force cleared
	 */
	if (prev_stats->mib_stats.reg_rxclr_cnt > curr_stats->mib_stats.reg_rxclr_cnt) {
		if (unlikely(scn->scn_dcs.dcs_debug >= DCS_DEBUG_VERBOSE)) {
            printk("\nDCS: ignoring due to negative rxclr count\n");
        }

        /* copy the current stats to previous stats for next run */
		wlan_dcs_im_copy_stats(prev_stats, curr_stats);
		return;
	}

	rxclr_delta = curr_stats->mib_stats.reg_rxclr_cnt - prev_stats->mib_stats.reg_rxclr_cnt;
	rxclr_ext_delta = curr_stats->mib_stats.reg_rxclr_ext_cnt -
								prev_stats->mib_stats.reg_rxclr_ext_cnt;
	tx_frame_delta = curr_stats->mib_stats.reg_tx_frame_cnt - 
								prev_stats->mib_stats.reg_tx_frame_cnt;

	rx_frame_delta = curr_stats->mib_stats.reg_rx_frame_cnt - 
								prev_stats->mib_stats.reg_rx_frame_cnt;

	cycle_count_delta = curr_stats->mib_stats.reg_cycle_cnt -
								prev_stats->mib_stats.reg_cycle_cnt;

    if (unlikely(scn->scn_dcs.dcs_debug >= DCS_DEBUG_VERBOSE)) {
        printk(",rxclr_delta,%u,rxclr_ext_delta,%u,tx_frame_delta,%u,rx_frame_delta,%u,cycle_count_delta,%u",
                rxclr_delta , rxclr_ext_delta , tx_frame_delta,
                rx_frame_delta , cycle_count_delta );
    }

	/* total channel utiliztaion is the amount of time RXCLR is
	 * counted. RXCLR is counted, when 'RX is NOT clear', please
	 * refer to mac documentation. It means either TX or RX is ON
     *
     * Why shift by 8 ? after multiplication it could overflow. At one
     * second rate, neither cycle_count_celta, nor the tsf_delta would be
     * zero after shift by 8 bits
	 */
	reg_total_cu = ((rxclr_delta >> 8) * 100) / (cycle_count_delta >>8);
	reg_tx_cu = ((tx_frame_delta >> 8 ) * 100) / (cycle_count_delta >> 8); 
	reg_rx_cu = ((rx_frame_delta >> 8 ) * 100) / (cycle_count_delta >> 8); 
	rx_time_cu = ((curr_stats->rx_time >> 8) * 100 ) / (reg_tsf_delta >> 8);

    /**
     * Amount of the time AP received cannot go higher than the receive
     * cycle count delta. If at all it is, there should have been a
     * compution error, ceil it to receive_cycle_count_diff 
     */
	if (rx_time_cu > reg_rx_cu) {
		rx_time_cu = reg_rx_cu;
	}

    if (unlikely(scn->scn_dcs.dcs_debug >= DCS_DEBUG_VERBOSE)) {
        printk(",reg_total_cu,%u,reg_tx_cu,%u,reg_rx_cu,%u,rx_time_cu,%u",
                    reg_total_cu, reg_tx_cu, reg_rx_cu, rx_time_cu);
    }
    
	/* Unusable channel utilization is amount of time that we
	 * spent in backoff or waiting for other transmit/receive to
	 * complete. If there is interference it is more likely that
	 * we overshoot the limit. In case of multiple stations, we
	 * still see increased channel utilization.  This assumption may
	 * not be true for the VOW scenario where either multicast or
	 * unicast-UDP is used ( mixed traffic would still cause high
	 * channel utilization).
     */
	wasted_tx_cu = ((curr_stats->tx_waste_time >> 8) * 100 ) / (reg_tsf_delta >> 8);

    /** 
     * transmit channel utilization cannot go higher than the amount of time
     * wasted, if so cap the wastage to transmit channel utillzation. This
     * could happen to compution error. 
     */
	if (reg_tx_cu < wasted_tx_cu) {
		wasted_tx_cu = reg_tx_cu;
	}

	tx_err = (reg_tx_cu  && wasted_tx_cu) ? (wasted_tx_cu * 100 )/reg_tx_cu : 0;

    /**
     * The below actually gives amount of time we are not using, or the
     * interferer is active. 
     * rx_time_cu is what computed receive time *NOT* rx_cycle_count
     * rx_cycle_count is our receive+interferer's transmit
     * un-used is really total_cycle_counts -
     *      (our_rx_time(rx_time_cu)+ our_receive_time)
     */
	reg_unused_cu = (reg_total_cu >= (reg_tx_cu + rx_time_cu)) ?
							(reg_total_cu - (reg_tx_cu + rx_time_cu)) : 0;

    /* if any retransmissions are there, count them as wastage
     */
	total_wasted_cu = reg_unused_cu + wasted_tx_cu;

	/* check ofdm and cck errors */
    if (unlikely(curr_stats->mib_stats.reg_ofdm_phyerr_cnt  <
            prev_stats->mib_stats.reg_ofdm_phyerr_cnt)) {
        reg_ofdm_phyerr_delta = curr_stats->mib_stats.reg_ofdm_phyerr_cnt ; 
    } else {
        reg_ofdm_phyerr_delta = curr_stats->mib_stats.reg_ofdm_phyerr_cnt - 
                                    prev_stats->mib_stats.reg_ofdm_phyerr_cnt;
    }

	if (unlikely(curr_stats->mib_stats.reg_cck_phyerr_cnt  <
            prev_stats->mib_stats.reg_cck_phyerr_cnt)) {
        reg_cck_phyerr_delta = curr_stats->mib_stats.reg_cck_phyerr_cnt; 
    } else {
        reg_cck_phyerr_delta = curr_stats->mib_stats.reg_cck_phyerr_cnt -
                                    prev_stats->mib_stats.reg_cck_phyerr_cnt;
    }

	/* add the influence of ofdm phy errors to the wasted channel
	 * utillization, this computed through time wasted in errors,
	 */
	reg_ofdm_phyerr_cu = reg_ofdm_phyerr_delta * scn->scn_dcs.phy_err_penalty ;
	total_wasted_cu += (reg_ofdm_phyerr_cu > 0) ?  (((reg_ofdm_phyerr_cu >> 8) * 100) / (reg_tsf_delta >> 8)) : 0;

	ofdm_phy_err_rate = (curr_stats->mib_stats.reg_ofdm_phyerr_cnt * 1000) /
                                curr_stats->mib_stats.listen_time;
	cck_phy_err_rate = (curr_stats->mib_stats.reg_cck_phyerr_cnt * 1000) /
                                curr_stats->mib_stats.listen_time;

    if (unlikely(scn->scn_dcs.dcs_debug >= DCS_DEBUG_VERBOSE)) {
        printk(",reg_unused_cu,%u,reg_ofdm_phyerr_delta,%u,reg_cck_phyerr_delta,%u,reg_ofdm_phyerr_cu,%u",
                    reg_unused_cu , reg_ofdm_phyerr_delta , reg_cck_phyerr_delta , reg_ofdm_phyerr_cu);
        printk(",total_wasted_cu,%u,ofdm_phy_err_rate,%u,cck_phy_err_rate,%u",
                    total_wasted_cu , ofdm_phy_err_rate , cck_phy_err_rate );
        printk(",new_unused_cu,%u,reg_ofdm_phy_error_cu,%u\n",
                reg_unused_cu, (curr_stats->mib_stats.reg_ofdm_phyerr_cnt*100)/
                                        curr_stats->mib_stats.listen_time);
    }

	/* check if the error rates are higher than the thresholds*/
	max_phy_err_rate = MAX(ofdm_phy_err_rate, cck_phy_err_rate);

	max_phy_err_count = MAX(curr_stats->mib_stats.reg_ofdm_phyerr_cnt,
                                curr_stats->mib_stats.reg_cck_phyerr_cnt);

    if (unlikely(scn->scn_dcs.dcs_debug >= DCS_DEBUG_VERBOSE)) {
        printk(",max_phy_err_rate,%u, max_phy_err_count,%u",max_phy_err_rate , max_phy_err_count);
    }

	if (((max_phy_err_rate >= scn->scn_dcs.phy_err_threshold) &&
				(max_phy_err_count > scn->scn_dcs.phy_err_threshold)) ||
         (curr_stats->phyerr_cnt > scn->scn_dcs.radar_err_threshold)) {
		too_many_phy_errors = 1;
	}
    
	if (reg_unused_cu >= scn->scn_dcs.coch_intr_thresh) {
		scn->scn_dcs.scn_dcs_im_stats.im_intr_cnt+=2; /* quickly reach to decision*/
	} else if (too_many_phy_errors &&
			   ((total_wasted_cu > scn->scn_dcs.coch_intr_thresh) &&
					(reg_tx_cu + reg_rx_cu) > scn->scn_dcs.user_max_cu)){ /* removed tx_err check here */
		scn->scn_dcs.scn_dcs_im_stats.im_intr_cnt++;
	}

	if (scn->scn_dcs.scn_dcs_im_stats.im_intr_cnt >= scn->scn_dcs.intr_detection_threshold) {

        if (unlikely(scn->scn_dcs.dcs_debug >= DCS_DEBUG_CRITICAL)) {
            printk("\n%s interference threshould exceeded\n", __func__);
            printk(",unused_cu,%u,too_any_phy_errors,%u,total_wasted_cu,%u,reg_tx_cu,%u,reg_rx_cu,%u\n",
                    reg_unused_cu, too_many_phy_errors, total_wasted_cu,reg_tx_cu, reg_rx_cu);
        }

		scn->scn_dcs.scn_dcs_im_stats.im_intr_cnt = 0;
		scn->scn_dcs.scn_dcs_im_stats.im_samp_cnt = 0;
        /* once the interference is detected, change the channel, as on
         * today this is common routine for wirelesslan and non-wirelesslan
         * interference. Name as such kept the same because of the DA code,
         * which is using the same function. 
         */
		ol_ath_wlan_n_cw_interference_handler(scn);
	} else if (!scn->scn_dcs.scn_dcs_im_stats.im_intr_cnt ||
				scn->scn_dcs.scn_dcs_im_stats.im_samp_cnt >= scn->scn_dcs.intr_detection_window) {
		scn->scn_dcs.scn_dcs_im_stats.im_intr_cnt = 0;
		scn->scn_dcs.scn_dcs_im_stats.im_samp_cnt = 0;
	} 

	/* count the current run too*/
	scn->scn_dcs.scn_dcs_im_stats.im_samp_cnt++;

    /* copy the stats for next cycle */
	wlan_dcs_im_copy_stats(prev_stats, curr_stats);

    if (unlikely(scn->scn_dcs.dcs_debug >= DCS_DEBUG_VERBOSE)) {
        printk(",intr_count,%u,sample_count,%d\n",scn->scn_dcs.scn_dcs_im_stats.im_intr_cnt,scn->scn_dcs.scn_dcs_im_stats.im_samp_cnt);
    }
}

/*
 * wmi_unified_dcs_interference_handler
 *  
 * There are two different interference types can be reported by the 
 * target firmware. Today either that is wireless interference or 
 * could be a non-wireless lan interference. All of these are reported 
 * WMI message. 
 * 
 * Message is of type wmi_dcs_interence_type_t
 *
 */
static int 
wmi_unified_dcs_interference_handler (ol_scn_t scn, u_int8_t *data, u_int16_t datalen, void *context)
{
	wmi_dcs_interference_event_t *int_event = (wmi_dcs_interference_event_t*)data; 
	
    /* do not handle any thing if host is in disabled state
     * This shall not happen, provide extra safty for against any delays
     * causing any kind of races.
     */
    if (!(OL_IS_DCS_RUNNING(scn->scn_dcs.dcs_enable))) {
        return 0;
    }
	switch (int_event->interference_type) {
	case ATH_CAP_DCS_CWIM: /* cw interferecne*/
		if (OL_IS_DCS_ENABLED(scn->scn_dcs.dcs_enable) & ATH_CAP_DCS_CWIM) {
			ol_ath_wlan_n_cw_interference_handler(scn);
		}  
		break;
	case ATH_CAP_DCS_WLANIM: /* wlan interference stats*/
		if (OL_IS_DCS_ENABLED(scn->scn_dcs.dcs_enable) & ATH_CAP_DCS_WLANIM) {
			ol_ath_wlan_interference_handler(scn, &int_event->int_event.wlan_stat);
		}  
		break;
	default:
		if (unlikely(scn->scn_dcs.dcs_debug >= DCS_DEBUG_CRITICAL)) {
            printk("DCS: unidentified interference type reported");
        }
		break;
	}
    return 0;
}


#define TPC_TABLE_TYPE_CDD  0
#define TPC_TABLE_TYPE_STBC 1
#define TPC_TABLE_TYPE_TXBF 2

u_int8_t 
tpc_config_get_rate_tpc(wmi_pdev_tpc_config_event *ev, u_int32_t rate_idx, u_int32_t num_chains, u_int8_t rate_code, u_int8_t type)
{
    u_int8_t tpc;
    u_int8_t num_streams;
    u_int8_t preamble;
    u_int8_t chain_idx;

    num_streams = 1 + AR600P_GET_HW_RATECODE_NSS(rate_code);
    preamble = AR600P_GET_HW_RATECODE_PREAM(rate_code);
    chain_idx = num_chains - 1;

    /*
     * find TPC based on the target power for that rate and the maximum
     * allowed regulatory power based on the number of tx chains.
     */
    tpc = A_MIN(ev->ratesArray[rate_idx], ev->maxRegAllowedPower[chain_idx]);

    if (ev->numTxChain > 1) { 
        /* 
         * Apply array gain factor for non-cck frames and when
         * num of chains used is more than the number of streams
         */ 
        if (preamble != AR600P_HW_RATECODE_PREAM_CCK) { 
            u_int8_t stream_idx;

            stream_idx = num_streams - 1;
            if (type == TPC_TABLE_TYPE_STBC) {
                if (num_chains > num_streams) {
                    tpc = A_MIN(tpc, ev->maxRegAllowedPowerAGSTBC[chain_idx - 1][stream_idx]);
                }
            } else if (type == TPC_TABLE_TYPE_TXBF) {
                if (num_chains > num_streams) {
                    tpc = A_MIN(tpc, ev->maxRegAllowedPowerAGTXBF[chain_idx - 1][stream_idx]);
                }
            } else {
                if (num_chains > num_streams) {
                    tpc = A_MIN(tpc, ev->maxRegAllowedPowerAGCDD[chain_idx - 1][stream_idx]);
                }
            }
        }
    }

    return tpc;
}


void
tpc_config_disp_tables(wmi_pdev_tpc_config_event *ev, u_int8_t *rate_code, u_int16_t *pream_table, u_int8_t type)
{
    u_int32_t i;
    u_char table_str[][5] =  {
        "CDD ",
        "STBC",
        "TXBF"
    };
    u_char pream_str[][6] = {
        "CCK  ",
        "OFDM ",
        "HT20 ",
        "HT40 ",
        "VHT20",
        "VHT40",
        "VHT80",
        "HTDUP"
    };
    u_int32_t pream_idx;
    u_int8_t tpc1 = 0;
    u_int8_t tpc2 = 0;
    u_int8_t tpc3 = 0;

    switch (type) {
        case TPC_TABLE_TYPE_CDD:
            if (!(ev->flags & WMI_TPC_CONFIG_EVENT_FLAG_TABLE_CDD)) {
                AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("%s : CDD not supported \n",__func__));
                return;
            }
            break;
        case TPC_TABLE_TYPE_STBC:
            if (!(ev->flags & WMI_TPC_CONFIG_EVENT_FLAG_TABLE_STBC)) {
                AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("%s : STBC not supported \n",__func__));
                return;
            }
            break;
        case TPC_TABLE_TYPE_TXBF:
            if (!(ev->flags & WMI_TPC_CONFIG_EVENT_FLAG_TABLE_TXBF)) {
                AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("%s : TXBF not supported \n",__func__));
                return;
            }
            break;
        default:
            AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("%s : Invalid type %d \n",__func__, type));
            return;
    }

    AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("**************** %s POWER TABLE ****************\n",table_str[type])) ;
    AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("**************************************************\n"));

    pream_idx = 0;
    for(i = 0;i < ev->rateMax; i++) {
        if (i == pream_table[pream_idx]) {
            pream_idx++;
        }
        tpc1 = tpc_config_get_rate_tpc(ev, i, 1, rate_code[i], type);
        if (ev->numTxChain > 1) {
            tpc2 = tpc_config_get_rate_tpc(ev, i, 2, rate_code[i], type);
        }
        if (ev->numTxChain > 2) {
            tpc3 = tpc_config_get_rate_tpc(ev, i, 3, rate_code[i], type);
        }
        if (ev->numTxChain == 1) {
            AR_DEBUG_PRINTF(ATH_DEBUG_ERR, ("%8d %s 0x%2x %8d \n", i, pream_str[pream_idx], rate_code[i], tpc1));
        } else if (ev->numTxChain == 2) {
            AR_DEBUG_PRINTF(ATH_DEBUG_ERR, ("%8d %s 0x%2x %8d %8d \n", i, pream_str[pream_idx], rate_code[i], tpc1, tpc2));
        } else if (ev->numTxChain == 3) {
            AR_DEBUG_PRINTF(ATH_DEBUG_ERR, ("%8d %s 0x%2x %8d %8d %8d \n",i, pream_str[pream_idx], rate_code[i], tpc1, tpc2, tpc3));
        }
    }
    AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("**************************************************\n"));
    
    return;
}

int
wmi_unified_pdev_tpc_config_event_handler (ol_scn_t scn, u_int8_t *data, u_int16_t datalen, void *context)
{
    wmi_pdev_tpc_config_event *ev;
    u_int32_t i, j;
    u_int8_t rate_code[200];
    u_int16_t pream_table[10];
    u_int8_t rate_idx;
    u_int32_t pream_idx;

    ev = (wmi_pdev_tpc_config_event *)data;

#ifdef BIG_ENDIAN_HOST 
    {   
        /*
         * Target is in little endian, copy engine interface will 
         * swap at the dword boundary. Re-swap the byte stream
         * arrays
         */
        u_int32_t *destp, *srcp;
        u_int32_t len;

        srcp = &ev->maxRegAllowedPower[0];
        destp = &ev->maxRegAllowedPower[0];
        len = sizeof(wmi_pdev_tpc_config_event) - offsetof(wmi_pdev_tpc_config_event, maxRegAllowedPower);
        for(i=0; i < (roundup(len, sizeof(u_int32_t))/4); i++) {
            *destp = le32_to_cpu(*srcp);
            destp++; srcp++;
        }
    }
#endif

    /* Create the rate code table based on the chains supported */
    rate_idx = 0;
    pream_idx = 0;

    /* Fill CCK rate code */
    for (i=0;i<4;i++) {
        rate_code[rate_idx] = AR600P_ASSEMBLE_HW_RATECODE(i, 0, AR600P_HW_RATECODE_PREAM_CCK);
        rate_idx++;
    } 
    pream_table[pream_idx] = rate_idx;
    pream_idx++;

    /* Fill OFDM rate code */
    for (i=0;i<8;i++) {
        rate_code[rate_idx] = AR600P_ASSEMBLE_HW_RATECODE(i, 0, AR600P_HW_RATECODE_PREAM_OFDM);
        rate_idx++;
    } 
    pream_table[pream_idx] = rate_idx;
    pream_idx++;

    /* Fill HT20 rate code */
    for (i=0;i<ev->numTxChain;i++) {
        for (j=0;j<8;j++) {
            rate_code[rate_idx] = AR600P_ASSEMBLE_HW_RATECODE(j, i, AR600P_HW_RATECODE_PREAM_HT);
            rate_idx++;
        }
    } 
    pream_table[pream_idx] = rate_idx;
    pream_idx++;

    /* Fill HT40 rate code */
    for (i=0;i<ev->numTxChain;i++) {
        for (j=0;j<8;j++) {
            rate_code[rate_idx] = AR600P_ASSEMBLE_HW_RATECODE(j, i, AR600P_HW_RATECODE_PREAM_HT);
            rate_idx++;
        }
    } 
    pream_table[pream_idx] = rate_idx;
    pream_idx++;

    /* Fill VHT20 rate code */
    for (i=0;i<ev->numTxChain;i++) {
        for (j=0;j<10;j++) {
            rate_code[rate_idx] = AR600P_ASSEMBLE_HW_RATECODE(j, i, AR600P_HW_RATECODE_PREAM_VHT);
            rate_idx++;
        }
    } 
    pream_table[pream_idx] = rate_idx;
    pream_idx++;

    /* Fill VHT40 rate code */
    for (i=0;i<ev->numTxChain;i++) {
        for (j=0;j<10;j++) {
            rate_code[rate_idx] = AR600P_ASSEMBLE_HW_RATECODE(j, i, AR600P_HW_RATECODE_PREAM_VHT);
            rate_idx++;
        }
    } 
    pream_table[pream_idx] = rate_idx;
    pream_idx++;

    /* Fill VHT80 rate code */
    for (i=0;i<ev->numTxChain;i++) {
        for (j=0;j<10;j++) {
            rate_code[rate_idx] = AR600P_ASSEMBLE_HW_RATECODE(j, i, AR600P_HW_RATECODE_PREAM_VHT);
            rate_idx++;
        }
    } 
    pream_table[pream_idx] = rate_idx;
    pream_idx++;

    rate_code[rate_idx++] = AR600P_ASSEMBLE_HW_RATECODE(0, 0, AR600P_HW_RATECODE_PREAM_CCK);
    rate_code[rate_idx++] = AR600P_ASSEMBLE_HW_RATECODE(0, 0, AR600P_HW_RATECODE_PREAM_OFDM);
    rate_code[rate_idx++] = AR600P_ASSEMBLE_HW_RATECODE(0, 0, AR600P_HW_RATECODE_PREAM_CCK);
    rate_code[rate_idx++] = AR600P_ASSEMBLE_HW_RATECODE(0, 0, AR600P_HW_RATECODE_PREAM_OFDM);
    rate_code[rate_idx++] = AR600P_ASSEMBLE_HW_RATECODE(0, 0, AR600P_HW_RATECODE_PREAM_OFDM);

    /* use 0xFFFF to indicate end of table */
    pream_table[pream_idx] = 0xFFFF;

    AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("**************************************************\n"));
    AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("TPC Config for channel %4d mode %2d \n", ev->chanFreq, ev->phyMode));
    AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("**************************************************\n"));

    AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("CTL           = 0x%2x   Reg. Domain           = %2d \n", ev->ctl, ev->regDomain));
    AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("Antenna Gain  = %2d     Reg. Max Antenna Gain = %2d \n", ev->twiceAntennaGain, ev->twiceAntennaReduction));
    AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("Power Limit   = %2d     Reg. Max Power        = %2d \n", ev->powerLimit, ev->twiceMaxRDPower));
    AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("Num tx chains = %2d    Num  Supported Rates  = %2d \n", ev->numTxChain, ev->rateMax));
    AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("**************************************************\n"));

    tpc_config_disp_tables(ev, rate_code, pream_table, TPC_TABLE_TYPE_CDD);
    tpc_config_disp_tables(ev, rate_code, pream_table, TPC_TABLE_TYPE_STBC);
    tpc_config_disp_tables(ev, rate_code, pream_table, TPC_TABLE_TYPE_TXBF);


    return 0;
}

int
wmi_unified_gpio_config(wmi_unified_t wmi_handle, u_int32_t gpio_num, u_int32_t input,
                        u_int32_t pull_type, u_int32_t intr_mode)
{
    wmi_gpio_config_cmd *cmd;
    wmi_buf_t wmibuf;
    u_int32_t len = sizeof(wmi_gpio_config_cmd);

    /* Sanity Checks */
    if (pull_type > WMI_GPIO_PULL_DOWN || intr_mode > WMI_GPIO_INTTYPE_LEVEL_HIGH) {
        return -1;
    }

    wmibuf = wmi_buf_alloc(wmi_handle, len);
    if (wmibuf == NULL) {
        return -1;
    }
 
    cmd = (wmi_gpio_config_cmd *)wmi_buf_data(wmibuf);
    cmd->gpio_num = gpio_num;
    cmd->input = input;
    cmd->pull_type = pull_type;
    cmd->intr_mode = intr_mode;
    return wmi_unified_cmd_send(wmi_handle, wmibuf, len, WMI_GPIO_CONFIG_CMDID);
}

int
wmi_unified_gpio_output(wmi_unified_t wmi_handle, u_int32_t gpio_num, u_int32_t set)
{
    wmi_gpio_output_cmd *cmd;
    wmi_buf_t wmibuf;
    u_int32_t len = sizeof(wmi_gpio_output_cmd);

    wmibuf = wmi_buf_alloc(wmi_handle, len);
    if (wmibuf == NULL) {
        return -1;
    }
 
    cmd = (wmi_gpio_output_cmd *)wmi_buf_data(wmibuf);
    cmd->gpio_num = gpio_num;
    cmd->set = set;
    return wmi_unified_cmd_send(wmi_handle, wmibuf, len, WMI_GPIO_OUTPUT_CMDID);
}

int
wmi_unified_gpio_input_event_handler (ol_scn_t scn, u_int8_t *data, u_int16_t datalen, void *context)
{
    wmi_gpio_input_event *ev;
    ev = (wmi_gpio_input_event*) data;
    printk("\n%s: GPIO Input Event on Num %d\n", __func__, ev->gpio_num);
    return 0;
}
#endif
